#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

WResourceManagerWorkerDataLoad::WResourceManagerWorkerDataLoad() = default;
WResourceManagerWorkerDataLoad::~WResourceManagerWorkerDataLoad() = default;

void WResourceManagerWorkerDataLoad::Execute()
{
  W_PROFILE_SCOPE("LoadResourceFromDisk");

  WResource* pResourceToLoad = nullptr;
  WResourceTypeLoader* pLoader = nullptr;
  WUniquePtr<WResourceTypeLoader> pCustomLoader;

  {
    W_LOCK(WResourceManager::s_ResourceMutex);

    if (WResourceManager::s_pState->m_LoadingQueue.IsEmpty())
    {
      WResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
      return;
    }

    WResourceManager::UpdateLoadingDeadlines();

    auto it = WResourceManager::s_pState->m_LoadingQueue.PeekFront();
    pResourceToLoad = it.m_pResource;
    WResourceManager::s_pState->m_LoadingQueue.PopFront();

    if (pResourceToLoad->m_Flags.IsSet(WResourceFlags::HasCustomDataLoader))
    {
      pCustomLoader = std::move(WResourceManager::s_pState->m_CustomLoaders[pResourceToLoad]);
      pLoader = pCustomLoader.Borrow();
      pResourceToLoad->m_Flags.Remove(WResourceFlags::HasCustomDataLoader);
      pResourceToLoad->m_Flags.Add(WResourceFlags::PreventFileReload);
    }
  }

  if (pLoader == nullptr)
    pLoader = WResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  W_ASSERT_DEV(pLoader != nullptr, "No Loader function available for Resource Type '{0}'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  WResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // we need this info later to do some work in a lock, all the directly following code is outside the lock
  const bool bResourceIsLoadedOnMainThread = pResourceToLoad->GetBaseResourceFlags().IsAnySet(WResourceFlags::UpdateOnMainThread);

  WSharedPtr<WResourceManagerWorkerUpdateContent> pUpdateContentTask;
  WTaskGroupID* pUpdateContentGroup = nullptr;

  W_LOCK(WResourceManager::s_ResourceMutex);

  // try to find an update content task that has finished and can be reused
  for (WUInt32 i = 0; i < WResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    auto& td = WResourceManager::s_pState->m_WorkerTasksUpdateContent[i];

    if (WTaskSystem::IsTaskGroupFinished(td.m_GroupId))
    {
      pUpdateContentTask = td.m_pTask;
      pUpdateContentGroup = &td.m_GroupId;
      break;
    }
  }

  // if no such task could be found, we must allocate a new one
  if (pUpdateContentTask == nullptr)
  {
    WStringBuilder s;
    s.SetFormat("Resource Content Updater {0}", WResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount());

    auto& td = WResourceManager::s_pState->m_WorkerTasksUpdateContent.ExpandAndGetRef();
    td.m_pTask = W_DEFAULT_NEW(WResourceManagerWorkerUpdateContent);
    td.m_pTask->ConfigureTask(s, WTaskNesting::Maybe);

    pUpdateContentTask = td.m_pTask;
    pUpdateContentGroup = &td.m_GroupId;
  }

  // always updated together with pUpdateContentTask
  W_MSVC_ANALYSIS_ASSUME(pUpdateContentGroup != nullptr);

  // set up the data load task and launch it
  {
    pUpdateContentTask->m_LoaderData = LoaderData;
    pUpdateContentTask->m_pLoader = pLoader;
    pUpdateContentTask->m_pCustomLoader = std::move(pCustomLoader);
    pUpdateContentTask->m_pResourceToLoad = pResourceToLoad;
    pUpdateContentTask->m_pResourceToLoad->m_iReferenceCount.Increment();

    // schedule the task to run, either on the main thread or on some other thread
    *pUpdateContentGroup = WTaskSystem::StartSingleTask(
      pUpdateContentTask, bResourceIsLoadedOnMainThread ? WTaskPriority::SomeFrameMainThread : WTaskPriority::LateNextFrame);

    // restart the next loading task (this one is about to finish)
    WResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
    WResourceManager::RunWorkerTask();

    pCustomLoader.Clear();
  }
}


//////////////////////////////////////////////////////////////////////////

WResourceManagerWorkerUpdateContent::WResourceManagerWorkerUpdateContent() = default;
WResourceManagerWorkerUpdateContent::~WResourceManagerWorkerUpdateContent() = default;

void WResourceManagerWorkerUpdateContent::Execute()
{
  if (!m_LoaderData.m_sResourceDescription.IsEmpty())
    m_pResourceToLoad->SetResourceDescription(m_LoaderData.m_sResourceDescription);

  m_pResourceToLoad->CallUpdateContent(m_LoaderData.m_pDataStream);

  if (m_pResourceToLoad->m_uiQualityLevelsLoadable > 0)
  {
    // if the resource can have more details loaded, put it into the preload queue right away again
    WResourceManager::PreloadResource(m_pResourceToLoad);
  }

  // update the file modification date, if available
  if (m_LoaderData.m_LoadedFileModificationDate.IsValid())
    m_pResourceToLoad->m_LoadedFileModificationTime = m_LoaderData.m_LoadedFileModificationDate;

  W_ASSERT_DEV(m_pResourceToLoad->GetLoadingState() != WResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    WResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    m_pResourceToLoad->UpdateMemoryUsage(MemUsage);

    W_ASSERT_DEV(
      MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", m_pResourceToLoad->GetResourceID());
    W_ASSERT_DEV(
      MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", m_pResourceToLoad->GetResourceID());

    m_pResourceToLoad->m_MemoryUsage = MemUsage;
  }

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    W_LOCK(WResourceManager::s_ResourceMutex);
    W_ASSERT_DEV(WResourceManager::IsQueuedForLoading(m_pResourceToLoad), "Multi-threaded access detected");
    m_pResourceToLoad->m_Flags.Remove(WResourceFlags::IsQueuedForLoading);
    m_pResourceToLoad->m_LastAcquire = WResourceManager::GetLastFrameUpdate();
  }

  m_pLoader = nullptr;
  m_pResourceToLoad->m_iReferenceCount.Decrement();
  m_pResourceToLoad = nullptr;
}
