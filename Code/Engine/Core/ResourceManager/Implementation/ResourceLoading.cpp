#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>

WTypelessResourceHandle WResourceManager::LoadResourceByType(const WRTTI* pResourceType, WStringView sResourceID)
{
  // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
  W_LOCK(s_ResourceMutex);
  return WTypelessResourceHandle(GetResource(pResourceType, sResourceID, true));
}

void WResourceManager::InternalPreloadResource(WResource* pResource, bool bHighestPriority)
{
  if (s_pState->m_bShutdown)
    return;

  // Runtime created resources without loaders are not loaded via tasks but created on the stack directly.
  if (pResource->GetBaseResourceFlags().IsSet(WResourceFlags::IsCreatedResource) && !pResource->GetBaseResourceFlags().IsSet(WResourceFlags::HasCustomDataLoader))
    return;

  W_LOCK(s_ResourceMutex);

  // if there is nothing else that could be loaded, just return right away
  if (pResource->GetLoadingState() == WResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0)
  {
    // due to the threading this can happen for all resource types and is valid
    // W_ASSERT_DEV(!IsQueuedForLoading(pResource), "Invalid flag on resource type '{0}'",
    // pResource->GetDynamicRTTI()->GetTypeName());
    return;
  }

  W_PROFILE_SCOPE("InternalPreloadResource");

  W_ASSERT_DEV(!s_pState->m_bExportMode, "Resources should not be loaded in export mode");

  // if we are already loading this resource, early out
  if (IsQueuedForLoading(pResource))
  {
    // however, if it now has highest priority and is still in the loading queue (so not yet started)
    // move it to the front of the queue
    if (bHighestPriority)
    {
      // if it is not in the queue anymore, it has already been started by some thread
      if (RemoveFromLoadingQueue(pResource).Succeeded())
      {
        AddToLoadingQueue(pResource, bHighestPriority);
      }
    }

    return;
  }
  else
  {
    AddToLoadingQueue(pResource, bHighestPriority);

    if (bHighestPriority && WTaskSystem::GetCurrentThreadWorkerType() == WWorkerThreadType::FileAccess)
    {
      WResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
    }

    RunWorkerTask();
  }
}

void WResourceManager::SetupWorkerTasks()
{
  if (!s_pState->m_bTaskNamesInitialized)
  {
    s_pState->m_bTaskNamesInitialized = true;
    WStringBuilder s;

    {
      static constexpr WUInt32 InitialDataLoadTasks = 4;

      for (WUInt32 i = 0; i < InitialDataLoadTasks; ++i)
      {
        s.SetFormat("Resource Data Loader {0}", i);
        auto& data = s_pState->m_WorkerTasksDataLoad.ExpandAndGetRef();
        data.m_pTask = W_DEFAULT_NEW(WResourceManagerWorkerDataLoad);
        data.m_pTask->ConfigureTask(s, WTaskNesting::Maybe);
      }
    }

    {
      static constexpr WUInt32 InitialUpdateContentTasks = 16;

      for (WUInt32 i = 0; i < InitialUpdateContentTasks; ++i)
      {
        s.SetFormat("Resource Content Updater {0}", i);
        auto& data = s_pState->m_WorkerTasksUpdateContent.ExpandAndGetRef();
        data.m_pTask = W_DEFAULT_NEW(WResourceManagerWorkerUpdateContent);
        data.m_pTask->ConfigureTask(s, WTaskNesting::Maybe);
      }
    }
  }
}

void WResourceManager::RunWorkerTask()
{
  if (s_pState->m_bShutdown)
    return;

  W_ASSERT_DEV(s_ResourceMutex.IsLocked(), "");

  SetupWorkerTasks();

  if (s_pState->m_bAllowLaunchDataLoadTask && !s_pState->m_LoadingQueue.IsEmpty())
  {
    s_pState->m_bAllowLaunchDataLoadTask = false;

    for (WUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
    {
      if (s_pState->m_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
      {
        s_pState->m_WorkerTasksDataLoad[i].m_GroupId =
          WTaskSystem::StartSingleTask(s_pState->m_WorkerTasksDataLoad[i].m_pTask, WTaskPriority::FileAccess);
        return;
      }
    }

    // could not find any unused task -> need to create a new one
    {
      WStringBuilder s;
      s.SetFormat("Resource Data Loader {0}", s_pState->m_WorkerTasksDataLoad.GetCount());
      auto& data = s_pState->m_WorkerTasksDataLoad.ExpandAndGetRef();
      data.m_pTask = W_DEFAULT_NEW(WResourceManagerWorkerDataLoad);
      data.m_pTask->ConfigureTask(s, WTaskNesting::Maybe);
      data.m_GroupId = WTaskSystem::StartSingleTask(data.m_pTask, WTaskPriority::FileAccess);
    }
  }
}

void WResourceManager::ReverseBubbleSortStep(WDeque<LoadingInfo>& data)
{
  // Yep, it's really bubble sort!
  // This will move the entry with the smallest value to the front and move all other values closer to their correct position,
  // which is exactly what we need for the priority queue.
  // We do this once a frame, which gives us nice iterative sorting, with relatively deterministic performance characteristics.

  W_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  const WUInt32 uiCount = data.GetCount();

  for (WUInt32 i = uiCount; i > 1; --i)
  {
    const WUInt32 idx2 = i - 1;
    const WUInt32 idx1 = i - 2;

    if (data[idx1].m_fPriority > data[idx2].m_fPriority)
    {
      WMath::Swap(data[idx1], data[idx2]);
    }
  }
}

void WResourceManager::UpdateLoadingDeadlines()
{
  if (s_pState->m_LoadingQueue.IsEmpty())
    return;

  W_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  W_PROFILE_SCOPE("UpdateLoadingDeadlines");

  const WUInt32 uiCount = s_pState->m_LoadingQueue.GetCount();
  s_pState->m_uiLastResourcePriorityUpdateIdx = WMath::Min(s_pState->m_uiLastResourcePriorityUpdateIdx, uiCount);

  WUInt32 uiUpdateCount = WMath::Min(50u, uiCount - s_pState->m_uiLastResourcePriorityUpdateIdx);

  if (uiUpdateCount == 0)
  {
    s_pState->m_uiLastResourcePriorityUpdateIdx = 0;
    uiUpdateCount = WMath::Min(50u, uiCount - s_pState->m_uiLastResourcePriorityUpdateIdx);
  }

  if (uiUpdateCount > 0)
  {
    {
      W_PROFILE_SCOPE("EvalLoadingDeadlines");

      const WTime tNow = WTime::Now();

      for (WUInt32 i = 0; i < uiUpdateCount; ++i)
      {
        auto& element = s_pState->m_LoadingQueue[s_pState->m_uiLastResourcePriorityUpdateIdx];
        element.m_fPriority = element.m_pResource->GetLoadingPriority(tNow);
        ++s_pState->m_uiLastResourcePriorityUpdateIdx;
      }
    }

    {
      W_PROFILE_SCOPE("SortLoadingDeadlines");
      ReverseBubbleSortStep(s_pState->m_LoadingQueue);
    }
  }
}

void WResourceManager::PreloadResource(WResource* pResource)
{
  InternalPreloadResource(pResource, false);
}

void WResourceManager::PreloadResource(const WTypelessResourceHandle& hResource)
{
  W_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  WResource* pResource = hResource.m_pResource;
  PreloadResource(pResource);
}

WResourceState WResourceManager::GetLoadingState(const WTypelessResourceHandle& hResource)
{
  if (hResource.m_pResource == nullptr)
    return WResourceState::Invalid;

  return hResource.m_pResource->GetLoadingState();
}

WResult WResourceManager::RemoveFromLoadingQueue(WResource* pResource)
{
  W_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");

  if (!IsQueuedForLoading(pResource))
    return W_SUCCESS;

  LoadingInfo li;
  li.m_pResource = pResource;

  if (s_pState->m_LoadingQueue.RemoveAndSwap(li))
  {
    pResource->m_Flags.Remove(WResourceFlags::IsQueuedForLoading);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WResourceManager::AddToLoadingQueue(WResource* pResource, bool bHighestPriority)
{
  W_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");
  W_ASSERT_DEV(IsQueuedForLoading(pResource) == false, "Resource is already in the loading queue");

  pResource->m_Flags.Add(WResourceFlags::IsQueuedForLoading);

  LoadingInfo li;
  li.m_pResource = pResource;

  if (bHighestPriority)
  {
    pResource->SetPriority(WResourcePriority::Critical);
    li.m_fPriority = 0.0f;
    s_pState->m_LoadingQueue.PushFront(li);
  }
  else
  {
    li.m_fPriority = pResource->GetLoadingPriority(s_pState->m_LastFrameUpdate);
    s_pState->m_LoadingQueue.PushBack(li);
  }
}

bool WResourceManager::ReloadResource(WResource* pResource, bool bForce)
{
  W_LOCK(s_ResourceMutex);

  if (!pResource->m_Flags.IsAnySet(WResourceFlags::IsReloadable))
    return false;

  if (!bForce && pResource->m_Flags.IsAnySet(WResourceFlags::PreventFileReload))
    return false;

  WResourceTypeLoader* pLoader = WResourceManager::GetResourceTypeLoader(pResource->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResource->GetDefaultResourceTypeLoader();

  if (pLoader == nullptr)
    return false;

  // no need to reload resources that are not loaded so far
  if (pResource->GetLoadingState() == WResourceState::Unloaded)
    return false;

  bool bAllowPreloading = true;

  // if the resource is already in the loading queue we can just keep it there
  if (IsQueuedForLoading(pResource))
  {
    bAllowPreloading = false;

    LoadingInfo li;
    li.m_pResource = pResource;

    if (s_pState->m_LoadingQueue.IndexOf(li) == WInvalidIndex)
    {
      // the resource is marked as 'loading' but it is not in the queue anymore
      // that means some task is already working on loading it
      // therefore we should not touch it (especially unload it), it might end up in an inconsistent state

      WLog::Dev(
        "Resource '{0}' is not being reloaded, because it is currently being loaded", WArgSensitive(pResource->GetResourceID(), "ResourceID"));
      return false;
    }
  }

  // if bForce, skip the outdated check
  if (!bForce)
  {
    if (!pLoader->IsResourceOutdated(pResource))
      return false;

    if (pResource->GetLoadingState() == WResourceState::LoadedResourceMissing)
    {
      WLog::Dev("Resource '{0}' is missing and will be tried to be reloaded ('{1}')", WArgSensitive(pResource->GetResourceID(), "ResourceID"),
        WArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
    else
    {
      WLog::Dev("Resource '{0}' is outdated and will be reloaded ('{1}')", WArgSensitive(pResource->GetResourceID(), "ResourceID"),
        WArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
  }

  if (pResource->GetBaseResourceFlags().IsSet(WResourceFlags::UpdateOnMainThread) == false || WThreadUtils::IsMainThread())
  {
    // make sure existing data is purged
    pResource->CallUnloadData(WResource::Unload::AllQualityLevels);

    W_ASSERT_DEV(pResource->GetLoadingState() <= WResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.",
      pResource->GetResourceID());
  }
  else
  {
    s_pState->m_ResourcesToUnloadOnMainThread.Insert(WTempHashedString(pResource->GetResourceID()), pResource->GetDynamicRTTI());
  }

  if (bAllowPreloading)
  {
    const WTime tNow = s_pState->m_LastFrameUpdate;

    // resources that have been in use recently will be put into the preload queue immediately
    // everything else will be loaded on demand
    if (pResource->GetLastAcquireTime() >= tNow - WTime::MakeFromSeconds(30.0))
    {
      PreloadResource(pResource);
    }
  }

  return true;
}

WUInt32 WResourceManager::ReloadResourcesOfType(const WRTTI* pType, bool bForce)
{
  W_LOCK(s_ResourceMutex);
  W_LOG_BLOCK("WResourceManager::ReloadResourcesOfType", pType->GetTypeName());

  WUInt32 count = 0;

  LoadedResources& lr = s_pState->m_LoadedResources[pType];

  for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    if (ReloadResource(it.Value(), bForce))
      ++count;
  }

  return count;
}

WUInt32 WResourceManager::ReloadAllResources(bool bForce)
{
  W_PROFILE_SCOPE("ReloadAllResources");

  W_LOCK(s_ResourceMutex);
  W_LOG_BLOCK("WResourceManager::ReloadAllResources");

  WUInt32 count = 0;

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      if (ReloadResource(it.Value(), bForce))
        ++count;
    }
  }

  if (count > 0)
  {
    WResourceManagerEvent e;
    e.m_Type = WResourceManagerEvent::Type::ReloadAllResources;

    s_pState->m_ManagerEvents.Broadcast(e);
  }

  return count;
}

void WResourceManager::UpdateResourceWithCustomLoader(const WTypelessResourceHandle& hResource, WUniquePtr<WResourceTypeLoader>&& pLoader)
{
  W_LOCK(s_ResourceMutex);

  hResource.m_pResource->m_Flags.Add(WResourceFlags::HasCustomDataLoader);
  s_pState->m_CustomLoaders[hResource.m_pResource] = std::move(pLoader);
  // if there was already a custom loader set, but it got no action yet, it is deleted here and replaced with the newer loader

  ReloadResource(hResource.m_pResource, true);
};

void WResourceManager::EnsureResourceLoadingState(WResource* pResource, const WResourceState RequestedState)
{
  return EnsureResourceCondition(pResource, [=]() -> bool
    { return (WInt32)pResource->GetLoadingState() >= (WInt32)RequestedState ||
             (pResource->GetLoadingState() == WResourceState::LoadedResourceMissing); });
}

void WResourceManager::EnsureResourceCondition(WResource* pResourceToLoad, const WDelegate<bool()>& condition)
{
  const WRTTI* pOwnRtti = pResourceToLoad->GetDynamicRTTI();

  // help loading until the requested resource is available
  while (!condition())
  {
    WTaskGroupID tgid;

    {
      W_LOCK(s_ResourceMutex);

      for (WUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
      {
        const WResource* pQueuedResource = s_pState->m_WorkerTasksUpdateContent[i].m_pTask->m_pResourceToLoad;

        if (pQueuedResource != nullptr && pQueuedResource != pResourceToLoad && !s_pState->m_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
        {
          if (!IsResourceTypeAcquireDuringUpdateContentAllowed(pQueuedResource->GetDynamicRTTI(), pOwnRtti))
          {
            tgid = s_pState->m_WorkerTasksUpdateContent[i].m_GroupId;
            break;
          }
        }
      }
    }

    if (tgid.IsValid())
    {
      WTaskSystem::WaitForGroup(tgid);
    }
    else
    {
      // do not use WThreadUtils::YieldTimeSlice here, otherwise the thread is not tagged as 'blocked' in the TaskSystem
      WTaskSystem::WaitForCondition(condition);
    }
  }
}
