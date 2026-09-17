#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Timestamp.h>

/// The base class for all resources.
class W_CORE_DLL WResource : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WResource, WReflectedClass);

public:
  enum class DoUpdate
  {
    OnMainThread,
    OnAnyThread,
    OnGraphicsResourceThreads ///< If set, the setting from UpdateGraphicsResource is used. This must be configured by the active renderer.
  };

  static DoUpdate UpdateGraphicsResource /*= DoUpdate::OnAnyThread*/;

protected:
  enum class Unload
  {
    AllQualityLevels,
    OneQualityLevel
  };

  /// Default constructor.
  WResource(DoUpdate ResourceUpdateThread, WUInt8 uiQualityLevelsLoadable);

  /// virtual destructor.
  virtual ~WResource();

public:
  struct MemoryUsage
  {
    MemoryUsage()
    {
      m_uiMemoryCPU = 0;
      m_uiMemoryGPU = 0;
    }

    WUInt64 m_uiMemoryCPU;
    WUInt64 m_uiMemoryGPU;
  };

  /// Returns the unique ID that identifies this resource. On a file resource this might be a path. Can also be a GUID or any other
  /// scheme that uniquely identifies the resource.
  W_ALWAYS_INLINE WStringView GetResourceID() const { return m_sUniqueID; }

  /// Returns the hash of the unique ID.
  W_ALWAYS_INLINE WUInt64 GetResourceIDHash() const { return m_uiUniqueIDHash; }

  /// The resource description allows to store an additional string that might be more descriptive during debugging, than the unique
  /// ID.
  void SetResourceDescription(WStringView sDescription);

  /// The resource description allows to store an additional string that might be more descriptive during debugging, than the unique
  /// ID.
  const WString& GetResourceDescription() const { return m_sResourceDescription; }

  /// The returns the resource description, if available, otherwise the resource ID.
  ///
  /// This is mainly for logging, where you want the more user friendly description, but the ID, if no description is available.
  const WString& GetResourceIdOrDescription() const { return m_sResourceDescription.IsEmpty() ? m_sUniqueID : m_sResourceDescription; }

  /// Returns the current state in which this resource is in.
  W_ALWAYS_INLINE WResourceState GetLoadingState() const { return m_LoadingState; }

  /// Returns the current maximum quality level that the resource could have.
  ///
  /// This is used to scale the amount data used. Once a resource is in the 'Loaded' state, it can still have different
  /// quality levels. E.g. a texture can be fully used with n mipmap levels, but there might be more that could be loaded.
  /// On the other hand a resource could have a higher 'loaded quality level' then the 'max quality level', if the user
  /// just changed settings and reduced the maximum quality level that should be used. In this case the resource manager
  /// will instruct the resource to unload some of its data soon.
  ///
  /// The quality level is a purely logical concept that can be handled very different by different resource types.
  /// E.g. a texture resource could theoretically use one quality level per available mipmap level. However, since
  /// the resource should generally be able to load and unload each quality level separately, it might make more sense
  /// for a texture resource, to use one quality level for everything up to 64*64, and then one quality level for each
  /// mipmap above that, which would result in 5 quality levels for a 1024*1024 texture.
  ///
  /// Most resource will have zero or one quality levels (which is the same) as they are either loaded or not.
  W_ALWAYS_INLINE WUInt8 GetNumQualityLevelsDiscardable() const { return m_uiQualityLevelsDiscardable; }

  /// Returns how many quality levels the resource may additionally load.
  W_ALWAYS_INLINE WUInt8 GetNumQualityLevelsLoadable() const { return m_uiQualityLevelsLoadable; }

  /// Returns the priority that is used by the resource manager to determine which resource to load next.
  float GetLoadingPriority(WTime now) const;

  /// Returns the current resource priority.
  WResourcePriority GetPriority() const { return m_Priority; }

  /// Changes the current resource priority.
  void SetPriority(WResourcePriority priority);

  /// Returns the basic flags for the resource type. Mostly used the resource manager.
  W_ALWAYS_INLINE const WBitflags<WResourceFlags>& GetBaseResourceFlags() const { return m_Flags; }

  /// Returns the information about the current memory usage of the resource.
  W_ALWAYS_INLINE const MemoryUsage& GetMemoryUsage() const { return m_MemoryUsage; }

  /// Returns the time at which the resource was (tried to be) acquired last.
  /// If a resource is acquired using WResourceAcquireMode::PointerOnly, this does not update the last acquired time, since the resource is
  /// not acquired for full use.
  W_ALWAYS_INLINE WTime GetLastAcquireTime() const { return m_LastAcquire; }

  /// Returns the reference count of this resource.
  W_ALWAYS_INLINE WInt32 GetReferenceCount() const { return m_iReferenceCount; }

  /// Returns the modification date of the file from which this resource was loaded.
  ///
  /// The date may be invalid, if it cannot be retrieved or the resource was created and not loaded.
  W_ALWAYS_INLINE const WTimestamp& GetLoadedFileModificationTime() const { return m_LoadedFileModificationTime; }

  /// Returns the current value of the resource change counter.
  /// Can be used to detect whether the resource has changed since using it last time.
  ///
  /// The resource change counter is increased by calling IncResourceChangeCounter() or
  /// whenever the resource content is updated.
  W_ALWAYS_INLINE WUInt32 GetCurrentResourceChangeCounter() const { return m_uiResourceChangeCounter; }

  /// Allows to manually increase the resource change counter to signal that dependent code might need to update.
  W_ALWAYS_INLINE void IncResourceChangeCounter() { ++m_uiResourceChangeCounter; }

  /// If the resource has modifications from the original state, it should reset itself to that state now (or force a reload on
  /// itself).
  virtual void ResetResource() {}

  /// Prints the stack-traces for all handles that currently reference this resource.
  ///
  /// Only implemented if W_RESOURCEHANDLE_STACK_TRACES is W_ON.
  /// Otherwise the function does nothing.
  void PrintHandleStackTraces();


  mutable WEvent<const WResourceEvent&, WMutex> m_ResourceEvents;

private:
  friend class WResourceManager;
  friend class WResourceManagerWorkerDataLoad;
  friend class WResourceManagerWorkerUpdateContent;

  /// Called by WResourceManager shortly after resource creation.
  void SetUniqueID(WStringView sUniqueID, bool bIsReloadable);

  void CallUnloadData(Unload WhatToUnload);

  /// Requests the resource to unload another quality level. If bFullUnload is true, the resource should unload all data, because it
  /// is going to be deleted afterwards.
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) = 0;

  void CallUpdateContent(WStreamReader* Stream);

  /// Called whenever more data for the resource is available. The resource must read the stream to update it's data.
  ///
  /// pStream may be nullptr in case the resource data could not be found.
  virtual WResourceLoadDesc UpdateContent(WStreamReader* pStream) = 0;

  /// Returns the resource type loader that should be used for this type of resource, unless it has been overridden on the
  /// WResourceManager.
  ///
  /// By default, this redirects to WResourceManager::GetDefaultResourceLoader. So there is one global default loader, that can be set
  /// on the resource manager. Overriding this function will then allow to use a different resource loader on a specific type.
  /// Additionally, one can override the resource loader from the outside, by setting it via WResourceManager::SetResourceTypeLoader.
  /// That last method always takes precedence and allows to modify the behavior without modifying the code for the resource.
  /// But in the default case, the resource defines which loader is used.
  virtual WResourceTypeLoader* GetDefaultResourceTypeLoader() const;

private:
  WAtomicInteger<WResourceState> m_LoadingState = WResourceState::Unloaded;
  WAtomicInteger<WUInt8> m_uiQualityLevelsDiscardable = 0;
  WAtomicInteger<WUInt8> m_uiQualityLevelsLoadable = 0;


protected:
  /// Non-const version for resources that want to write this variable directly.
  MemoryUsage& ModifyMemoryUsage() { return m_MemoryUsage; }

  /// Call this to specify whether a resource is reloadable.
  ///
  /// By default all created resources are flagged as not reloadable.
  /// All resources loaded from file are automatically flagged as reloadable.
  void SetIsReloadable(bool bIsReloadable) { m_Flags.AddOrRemove(WResourceFlags::IsReloadable, bIsReloadable); }

  /// Used internally by the code injection macros
  void SetHasLoadingFallback(bool bHasLoadingFallback) { m_Flags.AddOrRemove(WResourceFlags::ResourceHasFallback, bHasLoadingFallback); }

private:
  template <typename ResourceType>
  friend class WTypedResourceHandle;

  friend W_CORE_DLL_FRIEND void IncreaseResourceRefCount(WResource* pResource, const void* pOwner);
  friend W_CORE_DLL_FRIEND void DecreaseResourceRefCount(WResource* pResource, const void* pOwner);

#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)
  friend W_CORE_DLL_FRIEND void MigrateResourceRefCount(WResource* pResource, const void* pOldOwner, const void* pNewOwner);

  struct HandleStackTrace
  {
    WUInt32 m_uiNumPtrs = 0;
    void* m_Ptrs[64];
  };

  WMutex m_HandleStackTraceMutex;
  WHashTable<const void*, HandleStackTrace> m_HandleStackTraces;
#endif


  /// This function must be overridden by all resource types.
  ///
  /// It has to compute the memory used by this resource.
  /// It is called by the resource manager whenever the resource's data has been loaded or unloaded.
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) = 0;

  virtual void ReportResourceIsMissing();

  virtual bool HasResourceTypeLoadingFallback() const = 0;

  /// Called by WResourceMananger::CreateResource
  void VerifyAfterCreateResource(const WResourceLoadDesc& ld);

  WUInt64 m_uiUniqueIDHash = 0;
  WUInt32 m_uiResourceChangeCounter = 0;
  WAtomicInteger32 m_iReferenceCount = 0;
  // WAtomicInteger32 m_iLockCount = 0; // currently not used
  WString m_sUniqueID;
  WString m_sResourceDescription;
  MemoryUsage m_MemoryUsage;
  WBitflags<WResourceFlags> m_Flags;

  WTime m_LastAcquire;
  WResourcePriority m_Priority = WResourcePriority::Medium;
  WTimestamp m_LoadedFileModificationTime;

private:
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  static const WResource* GetCurrentlyUpdatingContent();
#endif
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GLORIOUS MACROS FOR RESOURCE CLASS CODE GENERATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Core/ResourceManager/ResourceManager.h>

#define W_RESOURCE_DECLARE_COMMON_CODE(SELF)                                                                                                \
  friend class ::WResourceManager;                                                                                                          \
                                                                                                                                             \
public:                                                                                                                                      \
  /*                                                                                                                                     \ \ \
  /// Unfortunately this has to be called manually from within dynamic plugins during core engine shutdown.                       \ \ \
  ///                                                                                                                                    \ \ \
  /// Without this, the dynamic plugin might still be referenced by the core engine during later shutdown phases and will crash, because \ \ \
  /// memory and code is still referenced, that is already unloaded.                                                                     \ \ \
  */                                                                                                                                         \
  static void CleanupDynamicPluginReferences();                                                                                              \
                                                                                                                                             \
  /*                                                                                                                                     \ \ \
  /// Returns a typed resource handle to this resource                                                                            \ \ \
  */                                                                                                                                         \
  WTypedResourceHandle<SELF> GetResourceHandle() const;                                                                                     \
                                                                                                                                             \
  /*                                                                                                                                     \ \ \
  /// Sets the fallback resource that can be used while this resource is not yet loaded.                                          \ \ \
  ///                                                                                                                                    \ \ \
  /// By default there is no fallback resource, so all resource will block the application when requested for the first time.            \ \ \
  */                                                                                                                                         \
  void SetLoadingFallbackResource(const WTypedResourceHandle<SELF>& hResource);                                                             \
                                                                                                                                             \
private:                                                                                                                                     \
  /* These functions are needed to access the static members, such that they get DLL exported, otherwise you get unresolved symbols */       \
  static void SetResourceTypeLoadingFallback(const WTypedResourceHandle<SELF>& hResource);                                                  \
  static void SetResourceTypeMissingFallback(const WTypedResourceHandle<SELF>& hResource);                                                  \
  static const WTypedResourceHandle<SELF>& GetResourceTypeLoadingFallback()                                                                 \
  {                                                                                                                                          \
    return s_TypeLoadingFallback;                                                                                                            \
  }                                                                                                                                          \
  static const WTypedResourceHandle<SELF>& GetResourceTypeMissingFallback()                                                                 \
  {                                                                                                                                          \
    return s_TypeMissingFallback;                                                                                                            \
  }                                                                                                                                          \
  virtual bool HasResourceTypeLoadingFallback() const override                                                                               \
  {                                                                                                                                          \
    return s_TypeLoadingFallback.IsValid();                                                                                                  \
  }                                                                                                                                          \
                                                                                                                                             \
  static WTypedResourceHandle<SELF> s_TypeLoadingFallback;                                                                                  \
  static WTypedResourceHandle<SELF> s_TypeMissingFallback;                                                                                  \
                                                                                                                                             \
  WTypedResourceHandle<SELF> m_hLoadingFallback;



#define W_RESOURCE_IMPLEMENT_COMMON_CODE(SELF)                                                                         \
  WTypedResourceHandle<SELF> SELF::s_TypeLoadingFallback;                                                              \
  WTypedResourceHandle<SELF> SELF::s_TypeMissingFallback;                                                              \
                                                                                                                        \
  void SELF::CleanupDynamicPluginReferences()                                                                           \
  {                                                                                                                     \
    s_TypeLoadingFallback.Invalidate();                                                                                 \
    s_TypeMissingFallback.Invalidate();                                                                                 \
    WResourceManager::ClearResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);                             \
  }                                                                                                                     \
                                                                                                                        \
  WTypedResourceHandle<SELF> SELF::GetResourceHandle() const                                                           \
  {                                                                                                                     \
    W_ASSERT_DEV(GetReferenceCount() > 0, "This resource is being deallocated, do not store a handle to it anymore!"); \
    WTypedResourceHandle<SELF> handle((SELF*)this);                                                                    \
    return handle;                                                                                                      \
  }                                                                                                                     \
                                                                                                                        \
  void SELF::SetLoadingFallbackResource(const WTypedResourceHandle<SELF>& hResource)                                   \
  {                                                                                                                     \
    m_hLoadingFallback = hResource;                                                                                     \
    SetHasLoadingFallback(m_hLoadingFallback.IsValid());                                                                \
  }                                                                                                                     \
                                                                                                                        \
  void SELF::SetResourceTypeLoadingFallback(const WTypedResourceHandle<SELF>& hResource)                               \
  {                                                                                                                     \
    s_TypeLoadingFallback = hResource;                                                                                  \
    W_RESOURCE_VALIDATE_FALLBACK(SELF);                                                                                \
    WResourceManager::AddResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);                               \
  }                                                                                                                     \
  void SELF::SetResourceTypeMissingFallback(const WTypedResourceHandle<SELF>& hResource)                               \
  {                                                                                                                     \
    s_TypeMissingFallback = hResource;                                                                                  \
    W_RESOURCE_VALIDATE_FALLBACK(SELF);                                                                                \
    WResourceManager::AddResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);                               \
  }


#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
#  define W_RESOURCE_VALIDATE_FALLBACK(SELF)                                       \
    if (hResource.IsValid())                                                        \
    {                                                                               \
      WResourceLock<SELF> lock(hResource, WResourceAcquireMode::BlockTillLoaded); \
      /* if this fails, the 'fallback resource' is missing itself*/                 \
    }
#else
#  define W_RESOURCE_VALIDATE_FALLBACK(SELF)
#endif

#define W_RESOURCE_DECLARE_CREATEABLE(SELF, SELF_DESCRIPTOR)      \
protected:                                                         \
  WResourceLoadDesc CreateResource(SELF_DESCRIPTOR&& descriptor); \
                                                                   \
private:

#define W_RESOURCE_IMPLEMENT_CREATEABLE(SELF, SELF_DESCRIPTOR) WResourceLoadDesc SELF::CreateResource(SELF_DESCRIPTOR&& descriptor)
