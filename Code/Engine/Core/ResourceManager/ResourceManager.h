#pragma once

#include <Core/ResourceManager/Implementation/WorkerTasks.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Types/UniquePtr.h>

class WResourceManagerState;

/// The central class for managing all types derived from WResource
class W_CORE_DLL WResourceManager
{
  friend class WResourceManagerState;
  static WUniquePtr<WResourceManagerState> s_pState;

  /// \name Events
  ///@{

public:
  /// Events on individual resources. Subscribe to this to get a notification for events happening on any resource.
  /// If you are only interested in events for a specific resource, subscribe on directly on that instance.
  static const WEvent<const WResourceEvent&, WMutex>& GetResourceEvents();

  /// Events for the resource manager that affect broader things.
  static const WEvent<const WResourceManagerEvent&, WMutex>& GetManagerEvents();

  /// Goes through all existing resources and broadcasts the 'Exists' event.
  ///
  /// Used to announce all currently existing resources to interested event listeners (ie tools).
  static void BroadcastExistsEvent();

  ///@}
  /// \name Loading and creating resources
  ///@{

public:
  /// Returns a handle to the requested resource. szResourceID must uniquely identify the resource, different spellings / casing will result in different resources.
  ///
  /// After the call to this function the resource definitely exists in memory. Upon access through BeginAcquireResource / WResourceLock
  /// the resource will be loaded. If it is not possible to load the resource it will change to a 'missing' state. If the code accessing the
  /// resource cannot handle that case, the application will 'terminate' (that means crash).
  template <typename ResourceType>
  static WTypedResourceHandle<ResourceType> LoadResource(WStringView sResourceID);

  /// Same as LoadResource(), but additionally allows to set a custom fallback resource for this
  /// instance.
  ///
  /// If a valid fallback resource is specified, the resource will store that as its instance specific fallback resource. This will be used when trying to acquire the resource later.
  template <typename ResourceType>
  static WTypedResourceHandle<ResourceType> LoadResource(WStringView sResourceID, WTypedResourceHandle<ResourceType> hLoadingFallback);


  /// Same as LoadResource(), but instead of a template argument, the resource type to use is given as WRTTI info. Returns a
  /// typeless handle due to the missing template argument.
  static WTypelessResourceHandle LoadResourceByType(const WRTTI* pResourceType, WStringView sResourceID);

  /// Checks whether any resource loading is in progress
  static bool IsAnyLoadingInProgress();

  /// Generates a unique resource ID with the given prefix.
  ///
  /// Provide a prefix that is preferably not used anywhere else (i.e., closely related to your code).
  /// If the prefix is not also used to manually generate resource IDs, this function is guaranteed to return a unique resource ID.
  static WString GenerateUniqueResourceID(WStringView sResourceIDPrefix);

  /// Creates a resource from a descriptor.
  ///
  /// \param szResourceID The unique ID by which the resource is identified. E.g. in GetExistingResource()
  /// \param descriptor A type specific descriptor that holds all the information to create the resource.
  /// \param szResourceDescription An optional description that might help during debugging. Often a human readable name or path is stored
  /// here, to make it easier to identify this resource.
  template <typename ResourceType, typename DescriptorType>
  static WTypedResourceHandle<ResourceType> CreateResource(WStringView sResourceID, DescriptorType&& descriptor, WStringView sResourceDescription = nullptr);

  /// Returns a handle to the resource with the given ID if it exists or creates it from a descriptor.
  ///
  /// \param szResourceID The unique ID by which the resource is identified. E.g. in GetExistingResource()
  /// \param descriptor A type specific descriptor that holds all the information to create the resource.
  /// \param szResourceDescription An optional description that might help during debugging. Often a human readable name or path is stored here, to make it easier to identify this resource.
  template <typename ResourceType, typename DescriptorType>
  static WTypedResourceHandle<ResourceType> GetOrCreateResource(WStringView sResourceID, DescriptorType&& descriptor, WStringView sResourceDescription = nullptr);

  /// Returns a handle to the resource with the given ID. If the resource does not exist, the handle is invalid.
  ///
  /// Use this if a resource needs to be created procedurally (with CreateResource()), but might already have been created.
  /// If the returned handle is invalid, then just go through the resource creation step.
  template <typename ResourceType>
  static WTypedResourceHandle<ResourceType> GetExistingResource(WStringView sResourceID);

  /// Same as GetExistingResourceByType() but allows to specify the resource type as an WRTTI.
  static WTypelessResourceHandle GetExistingResourceByType(const WRTTI* pResourceType, WStringView sResourceID);

  template <typename ResourceType>
  static WTypedResourceHandle<ResourceType> GetExistingResourceOrCreateAsync(WStringView sResourceID, WUniquePtr<WResourceTypeLoader>&& pLoader, WTypedResourceHandle<ResourceType> hLoadingFallback = {})
  {
    WTypelessResourceHandle hTypeless = GetExistingResourceOrCreateAsync(WGetStaticRTTI<ResourceType>(), sResourceID, std::move(pLoader));

    auto hTyped = WTypedResourceHandle<ResourceType>((ResourceType*)hTypeless.m_pResource);

    if (hLoadingFallback.IsValid())
    {
      ((ResourceType*)hTypeless.m_pResource)->SetLoadingFallbackResource(hLoadingFallback);
    }

    return hTyped;
  }

  static WTypelessResourceHandle GetExistingResourceOrCreateAsync(const WRTTI* pResourceType, WStringView sResourceID, WUniquePtr<WResourceTypeLoader>&& pLoader);

  /// Triggers loading of the given resource.
  static void PreloadResource(const WTypelessResourceHandle& hResource);

  /// Similar to locking a resource with 'BlockTillLoaded' acquire mode, but can be done with a typeless handle and does not return a result.
  static void ForceLoadResourceNow(const WTypelessResourceHandle& hResource);

  /// Returns the current loading state of the given resource.
  static WResourceState GetLoadingState(const WTypelessResourceHandle& hResource);

  ///@}
  /// \name Reloading resources
  ///@{

public:
  /// Goes through all resources and makes sure they are reloaded, if they have changed. If bForce is true, all resources
  /// are updated, even if there is no indication that they have changed.
  static WUInt32 ReloadAllResources(bool bForce);

  /// Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  template <typename ResourceType>
  static WUInt32 ReloadResourcesOfType(bool bForce);

  /// Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  static WUInt32 ReloadResourcesOfType(const WRTTI* pType, bool bForce);

  /// Reloads only the one specific resource. If bForce is true, it is updated, even if there is no indication that it has changed.
  template <typename ResourceType>
  static bool ReloadResource(const WTypedResourceHandle<ResourceType>& hResource, bool bForce);

  /// Reloads only the one specific resource. If bForce is true, it is updated, even if there is no indication that it has changed.
  static bool ReloadResource(const WRTTI* pType, const WTypelessResourceHandle& hResource, bool bForce);


  /// Calls ReloadResource() on the given resource, but makes sure that the reload happens with the given custom loader.
  ///
  /// Use this e.g. with a WResourceLoaderFromMemory to replace an existing resource with new data that was created on-the-fly.
  /// Using this function will set the 'PreventFileReload' flag on the resource and thus prevent further reload actions.
  ///
  /// \sa RestoreResource()
  static void UpdateResourceWithCustomLoader(const WTypelessResourceHandle& hResource, WUniquePtr<WResourceTypeLoader>&& pLoader);

  /// Removes the 'PreventFileReload' flag and forces a reload on the resource.
  ///
  /// \sa UpdateResourceWithCustomLoader()
  static void RestoreResource(const WTypelessResourceHandle& hResource);

  ///@}
  /// \name Acquiring resources
  ///@{

public:
  /// Acquires a resource pointer from a handle. Prefer to use WResourceLock, which wraps BeginAcquireResource / EndAcquireResource
  ///
  /// \param hResource The resource to acquire
  /// \param mode The desired way to acquire the resource. See WResourceAcquireMode for details.
  /// \param hLoadingFallback A custom fallback resource that should be returned if hResource is not yet available. Allows to use domain
  /// specific knowledge to get a better fallback.
  /// \param Priority Allows to adjust the priority of the resource. This will affect how fast
  /// the resource is loaded, in case it is not yet available.
  /// \param out_AcquireResult Returns how successful the acquisition was. See WResourceAcquireResult for details.
  template <typename ResourceType>
  static ResourceType* BeginAcquireResource(const WTypedResourceHandle<ResourceType>& hResource, WResourceAcquireMode mode,
    const WTypedResourceHandle<ResourceType>& hLoadingFallback = WTypedResourceHandle<ResourceType>(),
    WResourceAcquireResult* out_pAcquireResult = nullptr);

  /// Same as BeginAcquireResource but only for the base resource pointer.
  static WResource* BeginAcquireResourcePointer(const WRTTI* pType, const WTypelessResourceHandle& hResource);

  /// Needs to be called in concert with BeginAcquireResource() after accessing a resource has been finished. Prefer to use
  /// WResourceLock instead.
  template <typename ResourceType>
  static void EndAcquireResource(ResourceType* pResource);

  /// Same as EndAcquireResource but without the template parameter. See also BeginAcquireResourcePointer.
  static void EndAcquireResourcePointer(WResource* pResource);

  /// Forces the resource manager to treat WResourceAcquireMode::AllowLoadingFallback as WResourceAcquireMode::BlockTillLoaded on
  /// BeginAcquireResource.
  static void ForceNoFallbackAcquisition(WUInt32 uiNumFrames = 0xFFFFFFFF);

  /// If the returned number is greater 0 the resource manager treats WResourceAcquireMode::AllowLoadingFallback as
  /// WResourceAcquireMode::BlockTillLoaded on BeginAcquireResource.
  static WUInt32 GetForceNoFallbackAcquisition();

  /// Retrieves an array of pointers to resources of the indicated type which
  /// are loaded at the moment. Destroy the returned object as soon as possible as it
  /// holds the entire resource manager locked.
  template <typename ResourceType>
  static WLockedObject<WMutex, WDynamicArray<WResource*>> GetAllResourcesOfType();

  ///@}
  /// \name Unloading resources
  ///@{

public:
  /// Deallocates all resources whose refcount has reached 0. Returns the number of deleted resources.
  static WUInt32 FreeAllUnusedResources();

  /// Deallocates resources whose refcount has reached 0. Returns the number of deleted resources.
  static WUInt32 FreeUnusedResources(WTime timeout, WTime lastAcquireThreshold);

  /// If timeout is not zero, FreeUnusedResources() is called once every frame with the given parameters.
  static void SetAutoFreeUnused(WTime timeout, WTime lastAcquireThreshold);

  /// If set to 'false' resources of the given type will not be incrementally unloaded in the background, when they are not referenced anymore.
  template <typename ResourceType>
  static void SetIncrementalUnloadForResourceType(bool bActive);

  template <typename TypeBeingUpdated, typename TypeItWantsToAcquire>
  static void AllowResourceTypeAcquireDuringUpdateContent()
  {
    AllowResourceTypeAcquireDuringUpdateContent(WGetStaticRTTI<TypeBeingUpdated>(), WGetStaticRTTI<TypeItWantsToAcquire>());
  }

  static void AllowResourceTypeAcquireDuringUpdateContent(const WRTTI* pTypeBeingUpdated, const WRTTI* pTypeItWantsToAcquire);

  static bool IsResourceTypeAcquireDuringUpdateContentAllowed(const WRTTI* pTypeBeingUpdated, const WRTTI* pTypeItWantsToAcquire);

private:
  static WResult DeallocateResource(WResource* pResource);

  ///@}
  /// \name Miscellaneous
  ///@{

public:
  /// Returns the resource manager mutex. Allows to lock the manager on a thread when multiple operations need to be done in
  /// sequence.
  static WMutex& GetMutex() { return s_ResourceMutex; }

  /// Must be called once per frame for some bookkeeping.
  static void PerFrameUpdate();

  /// Makes sure that no further resource loading will take place.
  static void EngineAboutToShutdown();

  /// Calls WResource::ResetResource() on all resources.
  ///
  /// This is mostly for usage in tools to reset resource whose state can be modified at runtime, to reset them to their original state.
  static void ResetAllResources();

  /// Calls WResource::UpdateContent() to fill the resource with 'low resolution' data
  ///
  /// This will early out, if the resource has gotten low-res data before.
  /// The resource itself may ignore the data, if it has already gotten low/high res data before.
  ///
  /// The typical use case is, that some other piece of code stores a low-res version of a resource to be able to get
  /// a resource into a usable state. For instance, a material may store low resolution texture data for every texture that it references.
  /// Then when 'loading' the textures, it can pass this low-res data to the textures, such that rendering can give decent results right
  /// away. If the textures have already been loaded before, or some other material already had low-res data, the call exits quickly.
  static void SetResourceLowResData(const WTypelessResourceHandle& hResource, WStreamReader* pStream);

  ///@}
  /// \name Type specific loaders
  ///@{

public:
  /// Sets the resource loader to use when no type specific resource loader is available.
  static void SetDefaultResourceLoader(WResourceTypeLoader* pDefaultLoader);

  /// Returns the resource loader to use when no type specific resource loader is available.
  static WResourceTypeLoader* GetDefaultResourceLoader();

  /// Sets the resource loader to use for the given resource type.
  ///
  /// \note This is bound to one specific type. Derived types do not inherit the type loader.
  template <typename ResourceType>
  static void SetResourceTypeLoader(WResourceTypeLoader* pCreator);

  ///@}
  /// \name Named resources
  ///@{

public:
  /// Registers a 'named' resource. When a resource is looked up using \a szLookupName, the lookup will be redirected to \a
  /// szRedirectionResource.
  ///
  /// This can be used to register a resource under an easier to use name. For example one can register "MenuBackground" as the name for "{
  /// E50DCC85-D375-4999-9CFE-42F1377FAC85 }". If the lookup name already exists, it will be overwritten.
  static void RegisterNamedResource(WStringView sLookupName, WStringView sRedirectionResource);

  /// Removes a previously registered name from the redirection table.
  static void UnregisterNamedResource(WStringView sLookupName);


  ///@}
  /// \name Asset system interaction
  ///@{

public:
  /// Registers which resource type to use to load an asset with the given type name
  static void RegisterResourceForAssetType(WStringView sAssetTypeName, const WRTTI* pResourceType);

  /// Returns the resource type that was registered to handle the given asset type for loading. nullptr if no resource type was
  /// registered for this asset type.
  static const WRTTI* FindResourceForAssetType(WStringView sAssetTypeName);

  ///@}
  /// \name Export mode
  ///@{

public:
  /// Enables export mode. In this mode the resource manager will assert when it actually tries to load a resource.
  /// This can be useful when exporting resource handles but the actual resource content is not needed.
  static void EnableExportMode(bool bEnable);

  /// Returns whether export mode is active.
  static bool IsExportModeEnabled();

  /// Creates a resource handle for the given resource ID. This method can only be used if export mode is enabled.
  /// Internally it will create a resource but does not load the content. This way it can be ensured that the resource handle is always only
  /// the size of a pointer.
  template <typename ResourceType>
  static WTypedResourceHandle<ResourceType> GetResourceHandleForExport(WStringView sResourceID);


  ///@}
  /// \name Resource Type Overrides
  ///@{

public:
  /// Registers a resource type to be used instead of any of it's base classes, when loading specific data
  ///
  /// When resource B is derived from A it can be registered to be instantiated when loading data, even if the code specifies to use a
  /// resource of type A.
  /// Whenever LoadResource<A>() is executed, the registered callback \a OverrideDecider is run to figure out whether B should be
  /// instantiated instead. If OverrideDecider returns true, B is used.
  ///
  /// OverrideDecider is given the resource ID after it has been resolved by the WFileSystem. So it has to be able to make its decision
  /// from the file path, name or extension.
  /// The override is registered for all base classes of \a pDerivedTypeToUse, in case the derivation hierarchy is longer.
  ///
  /// Without calling this at startup, a derived resource type has to be manually requested in code.
  static void RegisterResourceOverrideType(const WRTTI* pDerivedTypeToUse, WDelegate<bool(const WStringBuilder&)> overrideDecider);

  /// Unregisters \a pDerivedTypeToUse as an override resource
  ///
  /// \sa RegisterResourceOverrideType()
  static void UnregisterResourceOverrideType(const WRTTI* pDerivedTypeToUse);

  ///@}
  /// \name Resource Fallbacks
  ///@{

public:
  /// Specifies which resource to use as a loading fallback for the given type, while a resource is not yet loaded.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeLoadingFallback(const WTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeLoadingFallback(hResource);
  }

  /// \sa SetResourceTypeLoadingFallback()
  template <typename RESOURCE_TYPE>
  static inline const WTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeLoadingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeLoadingFallback();
  }

  /// Specifies which resource to use as a missing fallback for the given type, when a resource cannot be loaded.
  ///
  /// \note If no missing fallback is specified, trying to load a resource that does not exist will assert at runtime.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeMissingFallback(const WTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeMissingFallback(hResource);
  }

  /// \sa SetResourceTypeMissingFallback()
  template <typename RESOURCE_TYPE>
  static inline const WTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeMissingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeMissingFallback();
  }

  using ResourceCleanupCB = WDelegate<void()>;

  /// [internal] Used by WResource to register a cleanup function to be called at resource manager shutdown.
  static void AddResourceCleanupCallback(ResourceCleanupCB cb);

  /// \sa AddResourceCleanupCallback()
  static void ClearResourceCleanupCallback(ResourceCleanupCB cb);

  /// This will clear ALL resources that were registered as 'missing' or 'loading' fallback resources. This is called early during
  /// system shutdown to clean up resources.
  static void ExecuteAllResourceCleanupCallbacks();

  ///@}
  /// \name Resource Priorities
  ///@{

public:
  /// Specifies which resource to use as a loading fallback for the given type, while a resource is not yet loaded.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeDefaultPriority(WResourcePriority priority)
  {
    GetResourceTypePriorities()[WGetStaticRTTI<RESOURCE_TYPE>()] = priority;
  }

private:
  static WMap<const WRTTI*, WResourcePriority>& GetResourceTypePriorities();
  ///@}

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

private:
  friend class WResource;
  friend class WResourceManagerWorkerDataLoad;
  friend class WResourceManagerWorkerUpdateContent;
  friend class WResourceHandleReadContext;

  // Events
private:
  static void BroadcastResourceEvent(const WResourceEvent& e);

  // Miscellaneous
private:
  static WMutex s_ResourceMutex;

  // Startup / shutdown
private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ResourceManager);
  static void OnEngineShutdown();
  static void OnCoreShutdown();
  static void OnCoreStartup();
  static void PluginEventHandler(const WPluginEvent& e);

  // Loading / reloading / creating resources
private:
  struct LoadedResources
  {
    WHashTable<WTempHashedString, WResource*> m_Resources;
  };

  struct LoadingInfo
  {
    float m_fPriority = 0;
    WResource* m_pResource = nullptr;

    W_ALWAYS_INLINE bool operator==(const LoadingInfo& rhs) const { return m_pResource == rhs.m_pResource; }
    W_ALWAYS_INLINE bool operator<(const LoadingInfo& rhs) const { return m_fPriority < rhs.m_fPriority; }
  };
  static void EnsureResourceLoadingState(WResource* pResource, const WResourceState RequestedState);
  static void EnsureResourceCondition(WResource* pResource, const WDelegate<bool()>& condition);
  static void PreloadResource(WResource* pResource);
  static void InternalPreloadResource(WResource* pResource, bool bHighestPriority);

  template <typename ResourceType, typename DescriptorType>
  static WTypedResourceHandle<ResourceType> CreateResourceInternal(WStringView sResourceID, DescriptorType&& descriptor, WStringView sResourceDescription, bool bAllowGetFallback);

  template <typename ResourceType>
  static ResourceType* GetResource(WStringView sResourceID, bool bIsReloadable);
  static WResource* GetResource(const WRTTI* pRtti, WStringView sResourceID, bool bIsReloadable);
  static void RunWorkerTask();
  static void UpdateLoadingDeadlines();
  static void ReverseBubbleSortStep(WDeque<LoadingInfo>& data);
  static bool ReloadResource(WResource* pResource, bool bForce);

  static void SetupWorkerTasks();
  static WTime GetLastFrameUpdate();
  static WHashTable<const WRTTI*, LoadedResources>& GetLoadedResources();
  static WDynamicArray<WResource*>& GetLoadedResourceOfTypeTempContainer();

  W_ALWAYS_INLINE static bool IsQueuedForLoading(WResource* pResource) { return pResource->m_Flags.IsSet(WResourceFlags::IsQueuedForLoading); }
  [[nodiscard]] static WResult RemoveFromLoadingQueue(WResource* pResource);
  static void AddToLoadingQueue(WResource* pResource, bool bHighPriority);

  struct ResourceTypeInfo
  {
    bool m_bIncrementalUnload = true;
    bool m_bAllowNestedAcquireCached = false;

    WHybridArray<const WRTTI*, 8> m_NestedTypes;
  };

  static ResourceTypeInfo& GetResourceTypeInfo(const WRTTI* pRtti);

  // Type loaders
private:
  static WResourceTypeLoader* GetResourceTypeLoader(const WRTTI* pRTTI);

  static WMap<const WRTTI*, WResourceTypeLoader*>& GetResourceTypeLoaders();

  // Override / derived resources
private:
  struct DerivedTypeInfo
  {
    const WRTTI* m_pDerivedType = nullptr;
    WDelegate<bool(const WStringBuilder&)> m_Decider;
  };

  /// Checks whether there is a type override for pRtti given szResourceID and returns that
  static const WRTTI* FindResourceTypeOverride(const WRTTI* pRtti, WStringView sResourceID);
};

#include <Core/ResourceManager/Implementation/ResourceHandleReflection.h>
#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/Implementation/ResourceManager_inl.h>
