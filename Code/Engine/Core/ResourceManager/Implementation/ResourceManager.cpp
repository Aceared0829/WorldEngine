#include <Core/CorePCH.h>
#include <Foundation/Time/Clock.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

/// \todo Do not unload resources while they are acquired
/// \todo Resource Type Memory Thresholds
/// \todo Preload does not load all quality levels

/// Infos to Display:
///   Ref Count (max)
///   Fallback: Type / Instance
///   Loading Time

/// Resource Flags:
/// Category / Group (Texture Sets)

/// Resource Loader
///   Requires No File Access -> on non-File Thread

WUniquePtr<WResourceManagerState> WResourceManager::s_pState;
WMutex WResourceManager::s_ResourceMutex;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Core, ResourceManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::OnCoreStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::OnCoreShutdown();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WResourceManager::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


WResourceTypeLoader* WResourceManager::GetResourceTypeLoader(const WRTTI* pRTTI)
{
  return s_pState->m_ResourceTypeLoader[pRTTI];
}

WMap<const WRTTI*, WResourceTypeLoader*>& WResourceManager::GetResourceTypeLoaders()
{
  return s_pState->m_ResourceTypeLoader;
}

void WResourceManager::AddResourceCleanupCallback(ResourceCleanupCB cb)
{
  W_ASSERT_DEV(cb.IsComparable(), "Delegates with captures are not allowed");

  for (WUInt32 i = 0; i < s_pState->m_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_pState->m_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
      return;
  }

  s_pState->m_ResourceCleanupCallbacks.PushBack(cb);
}

void WResourceManager::ClearResourceCleanupCallback(ResourceCleanupCB cb)
{
  for (WUInt32 i = 0; i < s_pState->m_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_pState->m_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
    {
      s_pState->m_ResourceCleanupCallbacks.RemoveAtAndSwap(i);
      return;
    }
  }
}

void WResourceManager::ExecuteAllResourceCleanupCallbacks()
{
  if (s_pState == nullptr)
  {
    // In case resource manager wasn't initialized, nothing to do
    return;
  }

  WDynamicArray<ResourceCleanupCB> callbacks = s_pState->m_ResourceCleanupCallbacks;
  s_pState->m_ResourceCleanupCallbacks.Clear();

  for (auto& cb : callbacks)
  {
    cb();
  }

  W_ASSERT_DEV(s_pState->m_ResourceCleanupCallbacks.IsEmpty(), "During resource cleanup, new resource cleanup callbacks were registered.");
}

WMap<const WRTTI*, WResourcePriority>& WResourceManager::GetResourceTypePriorities()
{
  return s_pState->m_ResourceTypePriorities;
}

void WResourceManager::BroadcastResourceEvent(const WResourceEvent& e)
{
  W_LOCK(s_ResourceMutex);

  // broadcast it through the resource to everyone directly interested in that specific resource
  e.m_pResource->m_ResourceEvents.Broadcast(e);

  // and then broadcast it to everyone else through the general event
  s_pState->m_ResourceEvents.Broadcast(e);
}

void WResourceManager::RegisterResourceForAssetType(WStringView sAssetTypeName, const WRTTI* pResourceType)
{
  WStringBuilder s = sAssetTypeName;
  s.ToLower();

  s_pState->m_AssetToResourceType[s] = pResourceType;
}

const WRTTI* WResourceManager::FindResourceForAssetType(WStringView sAssetTypeName)
{
  WStringBuilder s = sAssetTypeName;
  s.ToLower();

  return s_pState->m_AssetToResourceType.GetValueOrDefault(s, nullptr);
}

void WResourceManager::ForceNoFallbackAcquisition(WUInt32 uiNumFrames /*= 0xFFFFFFFF*/)
{
  s_pState->m_uiForceNoFallbackAcquisition = WMath::Max(s_pState->m_uiForceNoFallbackAcquisition, uiNumFrames);
}

WUInt32 WResourceManager::FreeAllUnusedResources()
{
  W_LOG_BLOCK("WResourceManager::FreeAllUnusedResources");

  W_PROFILE_SCOPE("FreeAllUnusedResources");

  if (s_pState == nullptr)
  {
    // In case resource manager wasn't initialized, no resources to unload
    return 0;
  }

  const bool bFreeAllUnused = true;

  WUInt32 uiUnloaded = 0;
  bool bUnloadedAny = false;
  bool bAnyFailed = false;

  do
  {
    {
      W_LOCK(s_ResourceMutex);

      bUnloadedAny = false;

      for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
      {
        LoadedResources& lr = itType.Value();

        for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); /* empty */)
        {
          WResource* pReference = it.Value();

          if (pReference->m_iReferenceCount == 0)
          {
            bUnloadedAny = true; // make sure to try again, even if DeallocateResource() fails; need to release our lock for that to prevent dead-locks

            if (DeallocateResource(pReference).Succeeded())
            {
              ++uiUnloaded;

              it = lr.m_Resources.Remove(it);
              continue;
            }
            else
            {
              bAnyFailed = true;
            }
          }

          ++it;
        }
      }
    }

    if (bAnyFailed)
    {
      // When this happens, it is possible that the resource that failed to be deleted
      // is dependent on a task that needs to be executed on THIS thread (main thread).
      // Therefore, help executing some tasks here, to unblock the task system.

      bAnyFailed = false;

      WInt32 iHelpExecTasksRounds = 1;
      WTaskSystem::WaitForCondition([&iHelpExecTasksRounds]()
        { return iHelpExecTasksRounds-- <= 0; });
    }

  } while (bFreeAllUnused && bUnloadedAny);

  return uiUnloaded;
}

WUInt32 WResourceManager::FreeUnusedResources(WTime timeout, WTime lastAcquireThreshold)
{
  if (timeout.IsZeroOrNegative())
    return 0;

  W_LOCK(s_ResourceMutex);
  W_LOG_BLOCK("WResourceManager::FreeUnusedResources");
  W_PROFILE_SCOPE("FreeUnusedResources");

  auto itResourceType = s_pState->m_LoadedResources.Find(s_pState->m_pFreeUnusedLastType);
  if (!itResourceType.IsValid())
  {
    itResourceType = s_pState->m_LoadedResources.GetIterator();
  }

  if (!itResourceType.IsValid())
    return 0;

  auto itResourceID = itResourceType.Value().m_Resources.Find(s_pState->m_sFreeUnusedLastResourceID);
  if (!itResourceID.IsValid())
  {
    itResourceID = itResourceType.Value().m_Resources.GetIterator();
  }

  const WTime tStart = WTime::Now();

  WUInt32 uiDeallocatedCount = 0;

  WStringBuilder sResourceName;

  const WRTTI* pLastTypeCheck = nullptr;

  // stop once we wasted enough time
  while (WTime::Now() - tStart < timeout)
  {
    if (!itResourceID.IsValid())
    {
      // reached the end of this resource type
      // advance to the next resource type
      ++itResourceType;

      if (!itResourceType.IsValid())
      {
        // if we reached the end, reset everything and stop

        s_pState->m_pFreeUnusedLastType = nullptr;
        s_pState->m_sFreeUnusedLastResourceID = WTempHashedString();
        return uiDeallocatedCount;
      }


      // reset resource ID to the beginning of this type and start over
      itResourceID = itResourceType.Value().m_Resources.GetIterator();
      continue;
    }

    s_pState->m_pFreeUnusedLastType = itResourceType.Key();
    s_pState->m_sFreeUnusedLastResourceID = itResourceID.Key();

    if (pLastTypeCheck != itResourceType.Key())
    {
      pLastTypeCheck = itResourceType.Key();

      if (GetResourceTypeInfo(pLastTypeCheck).m_bIncrementalUnload == false)
      {
        itResourceID = itResourceType.Value().m_Resources.GetEndIterator();
        continue;
      }
    }

    WResource* pResource = itResourceID.Value();

    if ((pResource->GetReferenceCount() == 0) && (tStart - pResource->GetLastAcquireTime() > lastAcquireThreshold))
    {
      sResourceName = pResource->GetResourceID();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      // A zero acquire timestamp means the resource was created (a handle was made for it) but never
      // acquired even once, so 'lastAcquireThreshold' could not protect it - it becomes eligible for
      // freeing on the very next sweep. Harmless once, but a caller that does this in an update loop
      // (typically: WResourceManager::LoadResource into a *local* handle that is dropped again, on a
      // code path that then doesn't use it) recreates and frees the resource forever. Warn about that,
      // but only from the second time on, so that a one-off create-and-discard stays quiet.
      const bool bNeverAcquired = pResource->GetLastAcquireTime().IsZero();
#endif

      if (DeallocateResource(pResource).Succeeded())
      {
        WLog::Debug("Freed '{}'", WArgSensitive(sResourceName, "ResourceID"));

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
        if (bNeverAcquired)
        {
          WUInt8& uiState = s_pState->m_NeverAcquiredResources[WTempHashedString(sResourceName.GetView())];
          uiState = uiState + 1; // allow wrap around -> repeated warnings

          if (uiState == 5)
          {
            WLog::Warning("Resource '{}' is repeatedly created and freed without ever being acquired. Keep the resource handle around instead of recreating it, otherwise it is reloaded from scratch every time.", WArgSensitive(sResourceName, "ResourceID"));
          }
        }
#endif

        ++uiDeallocatedCount;
        itResourceID = itResourceType.Value().m_Resources.Remove(itResourceID);
        continue;
      }
    }

    ++itResourceID;
  }

  return uiDeallocatedCount;
}

void WResourceManager::SetAutoFreeUnused(WTime timeout, WTime lastAcquireThreshold)
{
  s_pState->m_AutoFreeUnusedTimeout = timeout;
  s_pState->m_AutoFreeUnusedThreshold = lastAcquireThreshold;
}

void WResourceManager::AllowResourceTypeAcquireDuringUpdateContent(const WRTTI* pTypeBeingUpdated, const WRTTI* pTypeItWantsToAcquire)
{
  auto& info = s_pState->m_TypeInfo[pTypeBeingUpdated];

  W_ASSERT_DEV(info.m_bAllowNestedAcquireCached == false, "AllowResourceTypeAcquireDuringUpdateContent for type '{}' must be called before the resource info has been requested.", pTypeBeingUpdated->GetTypeName());

  if (info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) == WInvalidIndex)
  {
    info.m_NestedTypes.PushBack(pTypeItWantsToAcquire);
  }
}

bool WResourceManager::IsResourceTypeAcquireDuringUpdateContentAllowed(const WRTTI* pTypeBeingUpdated, const WRTTI* pTypeItWantsToAcquire)
{
  W_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "");

  auto& info = s_pState->m_TypeInfo[pTypeBeingUpdated];

  if (!info.m_bAllowNestedAcquireCached)
  {
    info.m_bAllowNestedAcquireCached = true;

    WSet<const WRTTI*> visited;
    WSet<const WRTTI*> todo;
    WSet<const WRTTI*> deps;

    for (const WRTTI* pRtti : info.m_NestedTypes)
    {
      WRTTI::ForEachDerivedType(pRtti, [&](const WRTTI* pDerived)
        { todo.Insert(pDerived); });
    }

    while (!todo.IsEmpty())
    {
      auto it = todo.GetIterator();
      const WRTTI* pRtti = it.Key();
      todo.Remove(it);

      if (visited.Contains(pRtti))
        continue;

      visited.Insert(pRtti);
      deps.Insert(pRtti);

      for (const WRTTI* pNestedRtti : s_pState->m_TypeInfo[pRtti].m_NestedTypes)
      {
        if (!visited.Contains(pNestedRtti))
        {
          WRTTI::ForEachDerivedType(pNestedRtti, [&](const WRTTI* pDerived)
            { todo.Insert(pDerived); });
        }
      }
    }

    info.m_NestedTypes.Clear();
    for (const WRTTI* pRtti : deps)
    {
      info.m_NestedTypes.PushBack(pRtti);
    }
    info.m_NestedTypes.Sort();
  }

  return info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) != WInvalidIndex;
}

WResult WResourceManager::DeallocateResource(WResource* pResource)
{
  // W_ASSERT_DEBUG(pResource->m_iLockCount == 0, "Resource '{0}' has a refcount of zero, but is still in an acquired state.", pResource->GetResourceID());

  if (RemoveFromLoadingQueue(pResource).Failed())
  {
    // cannot deallocate resources that are currently queued for loading,
    // especially when they are already picked up by a task
    return W_FAILURE;
  }

  pResource->CallUnloadData(WResource::Unload::AllQualityLevels);

  W_ASSERT_DEBUG(pResource->GetLoadingState() <= WResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.", pResource->GetResourceID());

  // broadcast that we are going to delete the resource
  {
    WResourceEvent e;
    e.m_pResource = pResource;
    e.m_Type = WResourceEvent::Type::ResourceDeleted;
    WResourceManager::BroadcastResourceEvent(e);
  }

  W_ASSERT_DEV(pResource->GetReferenceCount() == 0, "The resource '{}' ({}) is being deallocated, you just stored a handle to it, which won't work! If you are listening to WResourceEvent::Type::ResourceContentUnloading then additionally listen to WResourceEvent::Type::ResourceDeleted to clean up handles to dead resources.", pResource->GetResourceID(), pResource->GetResourceDescription());

  // delete the resource via the RTTI provided allocator
  pResource->GetDynamicRTTI()->GetAllocator()->Deallocate(pResource);

  return W_SUCCESS;
}

// To allow triggering this event without a link dependency
// Used by Fileserve, to trigger this event, even though Fileserve should not have a link dependency on Core
W_ON_GLOBAL_EVENT(WResourceManager_ReloadAllResources)
{
  W_IGNORE_UNUSED(param0);
  W_IGNORE_UNUSED(param1);
  W_IGNORE_UNUSED(param2);
  W_IGNORE_UNUSED(param3);

  WResourceManager::ReloadAllResources(false);
}
void WResourceManager::ResetAllResources()
{
  W_LOCK(s_ResourceMutex);
  W_LOG_BLOCK("WResourceManager::ReloadAllResources");

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      WResource* pResource = it.Value();
      pResource->ResetResource();
    }
  }
}

void WResourceManager::PerFrameUpdate()
{
  W_PROFILE_SCOPE("WResourceManagerUpdate");

  s_pState->m_LastFrameUpdate = WClock::GetGlobalClock()->GetLastUpdateTime();

  if (s_pState->m_bBroadcastExistsEvent)
  {
    W_LOCK(s_ResourceMutex);

    s_pState->m_bBroadcastExistsEvent = false;

    for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
    {
      for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        WResourceEvent e;
        e.m_Type = WResourceEvent::Type::ResourceExists;
        e.m_pResource = it.Value();

        WResourceManager::BroadcastResourceEvent(e);
      }
    }
  }

  {
    W_LOCK(s_ResourceMutex);

    for (auto it = s_pState->m_ResourcesToUnloadOnMainThread.GetIterator(); it.IsValid(); it.Next())
    {
      // Identify the container of loaded resource for the type of resource we want to unload.
      LoadedResources loadedResourcesForType;
      if (s_pState->m_LoadedResources.TryGetValue(it.Value(), loadedResourcesForType) == false)
      {
        continue;
      }

      // See, if the resource we want to unload still exists.
      WResource* resourceToUnload = nullptr;

      if (loadedResourcesForType.m_Resources.TryGetValue(it.Key(), resourceToUnload) == false)
      {
        continue;
      }

      W_ASSERT_DEV(resourceToUnload != nullptr, "Found a resource above, should not be nullptr.");

      // If the resource was still loaded, we are going to unload it now.
      resourceToUnload->CallUnloadData(WResource::Unload::AllQualityLevels);

      W_ASSERT_DEV(resourceToUnload->GetLoadingState() <= WResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.", resourceToUnload->GetResourceID());
    }

    s_pState->m_ResourcesToUnloadOnMainThread.Clear();
  }

  if (s_pState->m_AutoFreeUnusedTimeout.IsPositive())
  {
    FreeUnusedResources(s_pState->m_AutoFreeUnusedTimeout, s_pState->m_AutoFreeUnusedThreshold);
  }

  if (s_pState->m_uiForceNoFallbackAcquisition > 0)
  {
    s_pState->m_uiForceNoFallbackAcquisition--;
  }
}

const WEvent<const WResourceEvent&, WMutex>& WResourceManager::GetResourceEvents()
{
  return s_pState->m_ResourceEvents;
}

const WEvent<const WResourceManagerEvent&, WMutex>& WResourceManager::GetManagerEvents()
{
  return s_pState->m_ManagerEvents;
}

void WResourceManager::BroadcastExistsEvent()
{
  s_pState->m_bBroadcastExistsEvent = true;
}

void WResourceManager::PluginEventHandler(const WPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case WPluginEvent::AfterStartupShutdown:
    {
      // unload all resources until there are no more that can be unloaded
      // this is to prevent having resources allocated that came from a dynamic plugin
      FreeAllUnusedResources();
    }
    break;

    default:
      break;
  }
}

void WResourceManager::OnCoreStartup()
{
  s_pState = W_DEFAULT_NEW(WResourceManagerState);

  W_LOCK(s_ResourceMutex);
  s_pState->m_bAllowLaunchDataLoadTask = true;
  s_pState->m_bShutdown = false;

  WPlugin::Events().AddEventHandler(PluginEventHandler);
}

void WResourceManager::EngineAboutToShutdown()
{
  {
    W_LOCK(s_ResourceMutex);

    if (s_pState == nullptr)
    {
      // In case resource manager wasn't initialized, nothing to do
      return;
    }

    s_pState->m_bAllowLaunchDataLoadTask = false; // prevent a new one from starting
    s_pState->m_bShutdown = true;
  }

  for (WUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
  {
    WTaskSystem::CancelTask(s_pState->m_WorkerTasksDataLoad[i].m_pTask).IgnoreResult();
  }

  for (WUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    WTaskSystem::CancelTask(s_pState->m_WorkerTasksUpdateContent[i].m_pTask).IgnoreResult();
  }

  {
    W_LOCK(s_ResourceMutex);

    for (auto entry : s_pState->m_LoadingQueue)
    {
      entry.m_pResource->m_Flags.Remove(WResourceFlags::IsQueuedForLoading);
    }

    s_pState->m_LoadingQueue.Clear();

    // Since we just canceled all loading tasks above and cleared the loading queue,
    // some resources may still be flagged as 'loading', but can never get loaded.
    // That can deadlock the 'FreeAllUnused' function, because it won't delete 'loading' resources.
    // Therefore we need to make sure no resource has the IsQueuedForLoading flag set anymore.
    for (auto itTypes : s_pState->m_LoadedResources)
    {
      for (auto itRes : itTypes.Value().m_Resources)
      {
        WResource* pRes = itRes.Value();

        if (pRes->GetBaseResourceFlags().IsSet(WResourceFlags::IsQueuedForLoading))
        {
          pRes->m_Flags.Remove(WResourceFlags::IsQueuedForLoading);
        }
      }
    }
  }
}

bool WResourceManager::IsAnyLoadingInProgress()
{
  W_LOCK(s_ResourceMutex);

  if (s_pState->m_LoadingQueue.GetCount() > 0)
  {
    return true;
  }

  for (WUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
  {
    if (!s_pState->m_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }

  for (WUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    if (!s_pState->m_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }
  return false;
}

void WResourceManager::OnEngineShutdown()
{
  WResourceManagerEvent e;
  e.m_Type = WResourceManagerEvent::Type::ManagerShuttingDown;

  // in case of a crash inside the event broadcast or ExecuteAllResourceCleanupCallbacks():
  // you might have a resource type added through a dynamic plugin that has already been unloaded,
  // but the event handler is still referenced
  // to fix this, call WResource::CleanupDynamicPluginReferences() on that resource type during engine shutdown (see WStartup)
  s_pState->m_ManagerEvents.Broadcast(e);

  ExecuteAllResourceCleanupCallbacks();

  EngineAboutToShutdown();

  // unload all resources until there are no more that can be unloaded
  FreeAllUnusedResources();
}

void WResourceManager::OnCoreShutdown()
{
  OnEngineShutdown();

  W_LOG_BLOCK("Referenced Resources");

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    const WRTTI* pRtti = itType.Key();
    LoadedResources& lr = itType.Value();

    if (!lr.m_Resources.IsEmpty())
    {
      W_LOG_BLOCK("Type", pRtti->GetTypeName());

      WLog::Error("{0} resource of type '{1}' are still referenced.", lr.m_Resources.GetCount(), pRtti->GetTypeName());

      for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        WResource* pReference = it.Value();

        WLog::Info("RC = {0}, ID = '{1}'", pReference->GetReferenceCount(), WArgSensitive(pReference->GetResourceID(), "ResourceID"));

#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)
        pReference->PrintHandleStackTraces();
#endif
      }
    }
  }

  WPlugin::Events().RemoveEventHandler(PluginEventHandler);

  s_pState.Clear();
}

WResource* WResourceManager::GetResource(const WRTTI* pRtti, WStringView sResourceID, bool bIsReloadable)
{
  if (sResourceID.IsEmpty())
    return nullptr;

  W_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Calling code must lock the mutex until the resource pointer is stored in a handle");

  // redirect requested type to override type, if available
  pRtti = FindResourceTypeOverride(pRtti, sResourceID);

  W_ASSERT_DEBUG(pRtti != nullptr, "There is no RTTI information available for the given resource type '{0}'", W_PP_STRINGIFY(ResourceType));

  if (pRtti->GetTypeFlags().IsSet(WTypeFlags::Abstract))
  {
    // this can happen for assets that use a resource type override (such as scripts) shortly after they have been created
    return nullptr;
  }

  W_ASSERT_DEBUG(pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate(), "There is no RTTI allocator available for the given resource type '{0}'", W_PP_STRINGIFY(ResourceType));

  WTempHashedString sHashedResourceID(sResourceID);

  WHashedString* redirection;
  if (s_pState->m_NamedResources.TryGetValue(sHashedResourceID, redirection))
  {
    sHashedResourceID = *redirection;
    sResourceID = redirection->GetView();
  }

  LoadedResources& lr = s_pState->m_LoadedResources[pRtti];

  WResource*& pResource = lr.m_Resources[sHashedResourceID];
  if (pResource != nullptr)
    return pResource;

  pResource = pRtti->GetAllocator()->Allocate<WResource>();
  pResource->m_Priority = s_pState->m_ResourceTypePriorities.GetValueOrDefault(pRtti, WResourcePriority::Medium);
  pResource->SetUniqueID(sResourceID, bIsReloadable);
  pResource->m_Flags.AddOrRemove(WResourceFlags::ResourceHasTypeFallback, pResource->HasResourceTypeLoadingFallback());

  return pResource;
}

void WResourceManager::RegisterResourceOverrideType(const WRTTI* pDerivedTypeToUse, WDelegate<bool(const WStringBuilder&)> overrideDecider)
{
  const WRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != WGetStaticRTTI<WResource>())
  {
    auto& info = s_pState->m_DerivedTypeInfos[pParentType].ExpandAndGetRef();
    info.m_pDerivedType = pDerivedTypeToUse;
    info.m_Decider = overrideDecider;

    pParentType = pParentType->GetParentType();
  }
}

void WResourceManager::UnregisterResourceOverrideType(const WRTTI* pDerivedTypeToUse)
{
  const WRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != WGetStaticRTTI<WResource>())
  {
    auto it = s_pState->m_DerivedTypeInfos.Find(pParentType);
    pParentType = pParentType->GetParentType();

    if (!it.IsValid())
      break;

    auto& infos = it.Value();

    for (WUInt32 i = infos.GetCount(); i > 0; --i)
    {
      if (infos[i - 1].m_pDerivedType == pDerivedTypeToUse)
        infos.RemoveAtAndSwap(i - 1);
    }
  }
}

const WRTTI* WResourceManager::FindResourceTypeOverride(const WRTTI* pRtti, WStringView sResourceID)
{
  auto it = s_pState->m_DerivedTypeInfos.Find(pRtti);

  if (!it.IsValid())
    return pRtti;

  WStringBuilder sRedirectedPath;
  WFileSystem::ResolveAssetRedirection(sResourceID, sRedirectedPath);

  while (it.IsValid())
  {
    for (const auto& info : it.Value())
    {
      if (info.m_Decider(sRedirectedPath))
      {
        pRtti = info.m_pDerivedType;
        it = s_pState->m_DerivedTypeInfos.Find(pRtti);
        continue;
      }
    }

    break;
  }

  return pRtti;
}

WString WResourceManager::GenerateUniqueResourceID(WStringView sResourceIDPrefix)
{
  WStringBuilder resourceID;
  resourceID.SetFormat("{}-{}", sResourceIDPrefix, s_pState->m_uiNextResourceID++);
  return resourceID;
}

WTypelessResourceHandle WResourceManager::GetExistingResourceByType(const WRTTI* pResourceType, WStringView sResourceID)
{
  WResource* pResource = nullptr;

  const WTempHashedString sResourceHash(sResourceID);

  W_LOCK(s_ResourceMutex);

  const WRTTI* pRtti = FindResourceTypeOverride(pResourceType, sResourceID);

  if (s_pState->m_LoadedResources[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return WTypelessResourceHandle(pResource);

  return WTypelessResourceHandle();
}

WTypelessResourceHandle WResourceManager::GetExistingResourceOrCreateAsync(const WRTTI* pResourceType, WStringView sResourceID, WUniquePtr<WResourceTypeLoader>&& pLoader)
{
  W_LOCK(s_ResourceMutex);

  WTypelessResourceHandle hResource = GetExistingResourceByType(pResourceType, sResourceID);

  if (hResource.IsValid())
    return hResource;

  hResource = GetResource(pResourceType, sResourceID, false);
  WResource* pResource = hResource.m_pResource;

  pResource->m_Flags.Add(WResourceFlags::HasCustomDataLoader | WResourceFlags::IsCreatedResource);
  s_pState->m_CustomLoaders[pResource] = std::move(pLoader);

  return hResource;
}

void WResourceManager::ForceLoadResourceNow(const WTypelessResourceHandle& hResource)
{
  W_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  WResource* pResource = hResource.m_pResource;

  if (pResource->GetLoadingState() != WResourceState::LoadedResourceMissing && pResource->GetLoadingState() != WResourceState::Loaded)
  {
    InternalPreloadResource(pResource, true);

    EnsureResourceLoadingState(hResource.m_pResource, WResourceState::Loaded);
  }
}

void WResourceManager::RegisterNamedResource(WStringView sLookupName, WStringView sRedirectionResource)
{
  W_LOCK(s_ResourceMutex);

  WTempHashedString lookup(sLookupName);

  WHashedString redirection;
  redirection.Assign(sRedirectionResource);

  s_pState->m_NamedResources[lookup] = redirection;
}

void WResourceManager::UnregisterNamedResource(WStringView sLookupName)
{
  W_LOCK(s_ResourceMutex);

  WTempHashedString hash(sLookupName);
  s_pState->m_NamedResources.Remove(hash);
}

void WResourceManager::SetResourceLowResData(const WTypelessResourceHandle& hResource, WStreamReader* pStream)
{
  WResource* pResource = hResource.m_pResource;

  if (pResource->GetBaseResourceFlags().IsSet(WResourceFlags::HasLowResData))
    return;

  if (!pResource->GetBaseResourceFlags().IsSet(WResourceFlags::IsReloadable))
    return;

  W_LOCK(s_ResourceMutex);

  // set this, even if we don't end up using the data (because some thread is already loading the full thing)
  pResource->m_Flags.Add(WResourceFlags::HasLowResData);

  if (IsQueuedForLoading(pResource))
  {
    // if we cannot find it in the queue anymore, some thread already started loading it
    // in this case, do not try to modify it
    if (RemoveFromLoadingQueue(pResource).Failed())
      return;
  }

  pResource->CallUpdateContent(pStream);

  W_ASSERT_DEV(pResource->GetLoadingState() != WResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    WResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    pResource->UpdateMemoryUsage(MemUsage);

    W_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", pResource->GetResourceID());
    W_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", pResource->GetResourceID());

    pResource->m_MemoryUsage = MemUsage;
  }
}

WResourceTypeLoader* WResourceManager::GetDefaultResourceLoader()
{
  return s_pState->m_pDefaultResourceLoader;
}

void WResourceManager::EnableExportMode(bool bEnable)
{
  W_ASSERT_DEV(s_pState != nullptr, "WStartup::StartupCoreSystems() must be called before using the WResourceManager.");

  s_pState->m_bExportMode = bEnable;
}

bool WResourceManager::IsExportModeEnabled()
{
  W_ASSERT_DEV(s_pState != nullptr, "WStartup::StartupCoreSystems() must be called before using the WResourceManager.");

  return s_pState->m_bExportMode;
}

void WResourceManager::RestoreResource(const WTypelessResourceHandle& hResource)
{
  W_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  WResource* pResource = hResource.m_pResource;
  pResource->m_Flags.Remove(WResourceFlags::PreventFileReload);

  ReloadResource(pResource, true);
}

WUInt32 WResourceManager::GetForceNoFallbackAcquisition()
{
  return s_pState->m_uiForceNoFallbackAcquisition;
}

WTime WResourceManager::GetLastFrameUpdate()
{
  return s_pState->m_LastFrameUpdate;
}

WHashTable<const WRTTI*, WResourceManager::LoadedResources>& WResourceManager::GetLoadedResources()
{
  return s_pState->m_LoadedResources;
}

WDynamicArray<WResource*>& WResourceManager::GetLoadedResourceOfTypeTempContainer()
{
  return s_pState->m_LoadedResourceOfTypeTempContainer;
}

void WResourceManager::SetDefaultResourceLoader(WResourceTypeLoader* pDefaultLoader)
{
  W_LOCK(s_ResourceMutex);

  s_pState->m_pDefaultResourceLoader = pDefaultLoader;
}

WResourceManager::ResourceTypeInfo& WResourceManager::GetResourceTypeInfo(const WRTTI* pRtti)
{
  return s_pState->m_TypeInfo[pRtti];
}

W_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceManager);
