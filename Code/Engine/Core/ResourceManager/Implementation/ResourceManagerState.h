#pragma once

#include <Core/CoreInternal.h>
W_CORE_INTERNAL_HEADER

#include <Core/ResourceManager/ResourceManager.h>

class WResourceManagerState
{
private:
  friend class WResource;
  friend class WResourceManager;
  friend class WResourceManagerWorkerDataLoad;
  friend class WResourceManagerWorkerUpdateContent;
  friend class WResourceHandleReadContext;

  /// \name Events
  ///@{

  WEvent<const WResourceEvent&, WMutex> m_ResourceEvents;
  WEvent<const WResourceManagerEvent&, WMutex> m_ManagerEvents;

  ///@}
  /// \name Resource Fallbacks
  ///@{

  WDynamicArray<WResourceManager::ResourceCleanupCB> m_ResourceCleanupCallbacks;

  ///@}
  /// \name Resource Priorities
  ///@{

  WMap<const WRTTI*, WResourcePriority> m_ResourceTypePriorities;

  ///@}

  struct TaskDataUpdateContent
  {
    WSharedPtr<WResourceManagerWorkerUpdateContent> m_pTask;
    WTaskGroupID m_GroupId;
  };

  struct TaskDataDataLoad
  {
    WSharedPtr<WResourceManagerWorkerDataLoad> m_pTask;
    WTaskGroupID m_GroupId;
  };

  bool m_bTaskNamesInitialized = false;
  bool m_bBroadcastExistsEvent = false;
  WUInt32 m_uiForceNoFallbackAcquisition = 0;

  // resources in this queue are waiting for a task to load them
  WDeque<WResourceManager::LoadingInfo> m_LoadingQueue;

  WHashTable<const WRTTI*, WResourceManager::LoadedResources> m_LoadedResources;

  bool m_bAllowLaunchDataLoadTask = true;
  bool m_bShutdown = false;

  WHybridArray<TaskDataUpdateContent, 24> m_WorkerTasksUpdateContent;
  WHybridArray<TaskDataDataLoad, 8> m_WorkerTasksDataLoad;

  WTime m_LastFrameUpdate;
  WUInt32 m_uiLastResourcePriorityUpdateIdx = 0;

  WDynamicArray<WResource*> m_LoadedResourceOfTypeTempContainer;
  WHashTable<WTempHashedString, const WRTTI*> m_ResourcesToUnloadOnMainThread;

  const WRTTI* m_pFreeUnusedLastType = nullptr;
  WTempHashedString m_sFreeUnusedLastResourceID;

  // Type Loaders

  WMap<const WRTTI*, WResourceTypeLoader*> m_ResourceTypeLoader;
  WResourceLoaderFromFile m_FileResourceLoader;
  WResourceTypeLoader* m_pDefaultResourceLoader = &m_FileResourceLoader;
  WMap<WResource*, WUniquePtr<WResourceTypeLoader>> m_CustomLoaders;


  // Override / derived resources

  WMap<const WRTTI*, WHybridArray<WResourceManager::DerivedTypeInfo, 4>> m_DerivedTypeInfos;


  // Named resources

  WHashTable<WTempHashedString, WHashedString> m_NamedResources;

  // Asset system interaction

  WMap<WString, const WRTTI*> m_AssetToResourceType;


  // Export mode

  bool m_bExportMode = false;
  WUInt32 m_uiNextResourceID = 0;

  // Resource Unloading
  WTime m_AutoFreeUnusedTimeout = WTime::MakeZero();
  WTime m_AutoFreeUnusedThreshold = WTime::MakeZero();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // How often a resource was freed again without ever having been acquired. Used to warn about inefficient resource usage.
  // See WResourceManager::FreeUnusedResources.
  WHashTable<WTempHashedString, WUInt8> m_NeverAcquiredResources;
#endif

  WMap<const WRTTI*, WResourceManager::ResourceTypeInfo> m_TypeInfo;
};
