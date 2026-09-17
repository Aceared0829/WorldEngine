#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// If this is set to W_ON, stack traces are recorded for every resource handle.
///
/// This can be used to find the places that create resource handles but do not properly clean them up.
#define W_RESOURCEHANDLE_STACK_TRACES W_OFF

class WResource;

template <typename T>
class WResourceLock;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
W_CORE_DLL void IncreaseResourceRefCount(WResource* pResource, const void* pOwner);
W_CORE_DLL void DecreaseResourceRefCount(WResource* pResource, const void* pOwner);

#if W_ENABLED(W_RESOURCEHANDLE_STACK_TRACES)
W_CORE_DLL void MigrateResourceRefCount(WResource* pResource, const void* pOldOwner, const void* pNewOwner);
#else
W_ALWAYS_INLINE void MigrateResourceRefCount(WResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
  W_IGNORE_UNUSED(pResource);
  W_IGNORE_UNUSED(pOldOwner);
  W_IGNORE_UNUSED(pNewOwner);
}
#endif

/// The typeless implementation of resource handles. A typed interface is provided by WTypedResourceHandle.
class W_CORE_DLL WTypelessResourceHandle
{
public:
  W_ALWAYS_INLINE WTypelessResourceHandle() = default;

  /// [internal] Increases the refcount of the given resource.
  WTypelessResourceHandle(WResource* pResource);

  /// Increases the refcount of the given resource
  W_ALWAYS_INLINE WTypelessResourceHandle(const WTypelessResourceHandle& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
    {
      IncreaseResourceRefCount(m_pResource, this);
    }
  }

  /// Move constructor, no refcount change is necessary.
  W_ALWAYS_INLINE WTypelessResourceHandle(WTypelessResourceHandle&& rhs)
  {
    m_pResource = rhs.m_pResource;
    rhs.m_pResource = nullptr;

    if (m_pResource)
    {
      MigrateResourceRefCount(m_pResource, &rhs, this);
    }
  }

  /// Releases any referenced resource.
  W_ALWAYS_INLINE ~WTypelessResourceHandle() { Invalidate(); }

  /// Returns whether the handle stores a valid pointer to a resource.
  W_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }

  /// Clears any reference to a resource and reduces its refcount.
  void Invalidate();

  /// Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  WUInt64 GetResourceIDHash() const;

  /// Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// If the handle is not valid, an empty string is returned.
  WStringView GetResourceID() const;

  /// The returns the resource description, if available, otherwise the resource ID.
  /// This is mainly for logging, where you want the more user friendly description, but the ID, if no description is available.
  /// If the handle is not valid, an empty string is returned.
  WStringView GetResourceIdOrDescription() const;

  /// Releases the current reference and increases the refcount of the given resource.
  void operator=(const WTypelessResourceHandle& rhs);

  /// Move operator, no refcount change is necessary.
  void operator=(WTypelessResourceHandle&& rhs);

  /// Checks whether the two handles point to the same resource.
  W_ALWAYS_INLINE bool operator==(const WTypelessResourceHandle& rhs) const { return m_pResource == rhs.m_pResource; }

  /// Checks whether the two handles point to the same resource.
  W_ALWAYS_INLINE bool operator!=(const WTypelessResourceHandle& rhs) const { return m_pResource != rhs.m_pResource; }

  /// For storing handles as keys in maps
  W_ALWAYS_INLINE bool operator<(const WTypelessResourceHandle& rhs) const { return m_pResource < rhs.m_pResource; }

  /// Checks whether the handle points to the given resource.
  W_ALWAYS_INLINE bool operator==(const WResource* rhs) const { return m_pResource == rhs; }

  /// Checks whether the handle points to the given resource.
  W_ALWAYS_INLINE bool operator!=(const WResource* rhs) const { return m_pResource != rhs; }

  /// Returns the type information of the resource or nullptr if the handle is invalid.
  const WRTTI* GetResourceType() const;

protected:
  WResource* m_pResource = nullptr;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class WResourceManager;
  friend class WResourceHandleWriteContext;
  friend class WResourceHandleReadContext;
  friend class WResourceHandleStreamOperations;
};

template <>
struct WHashHelper<WTypelessResourceHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WTypelessResourceHandle& value) { return WHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  W_ALWAYS_INLINE static bool Equal(const WTypelessResourceHandle& a, const WTypelessResourceHandle& b) { return a == b; }
};

/// The WTypedResourceHandle controls access to an WResource.
///
/// All resources must be referenced using WTypedResourceHandle instances (instantiated with the proper resource type as the template
/// argument). You must not store a direct pointer to a resource anywhere. Instead always store resource handles. To actually access a
/// resource, use WResourceManager::BeginAcquireResource and WResourceManager::EndAcquireResource after you have finished using it.
///
/// WTypedResourceHandle implements reference counting on resources. It also allows to redirect resources to fallback resources when they
/// are not yet loaded (if possible).
///
/// As long as there is one resource handle that references a resource, it is considered 'in use' and thus might not get unloaded.
/// So be careful where you store resource handles.
/// If necessary you can call Invalidate() to clear a resource handle and thus also remove the reference to the resource.
template <typename RESOURCE_TYPE>
class WTypedResourceHandle
{
public:
  using ResourceType = RESOURCE_TYPE;

  /// A default constructed handle is invalid and does not reference any resource.
  WTypedResourceHandle() = default;

  /// Increases the refcount of the given resource.
  explicit WTypedResourceHandle(ResourceType* pResource)
    : m_hTypeless(pResource)
  {
  }

  /// Increases the refcount of the given resource.
  WTypedResourceHandle(const WTypedResourceHandle<ResourceType>& rhs)
    : m_hTypeless(rhs.m_hTypeless)
  {
  }

  /// Move constructor, no refcount change is necessary.
  WTypedResourceHandle(WTypedResourceHandle<ResourceType>&& rhs)
    : m_hTypeless(std::move(rhs.m_hTypeless))
  {
  }

  template <typename BaseOrDerivedType>
  WTypedResourceHandle(const WTypedResourceHandle<BaseOrDerivedType>& rhs)
    : m_hTypeless(rhs.m_hTypeless)
  {
    static_assert(std::is_base_of<ResourceType, BaseOrDerivedType>::value || std::is_base_of<BaseOrDerivedType, ResourceType>::value, "Only related types can be assigned to handles of this type");

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (std::is_base_of<BaseOrDerivedType, ResourceType>::value)
    {
      W_ASSERT_DEBUG(rhs.IsValid(), "Cannot cast invalid base handle to derived type!");
      WResourceLock<BaseOrDerivedType> lock(rhs, WResourceAcquireMode::PointerOnly);
      W_ASSERT_DEBUG(WDynamicCast<const ResourceType*>(lock.GetPointer()) != nullptr, "Types are not related!");
    }
#endif
  }

  /// Releases the current reference and increases the refcount of the given resource.
  void operator=(const WTypedResourceHandle<ResourceType>& rhs) { m_hTypeless = rhs.m_hTypeless; }

  /// Move operator, no refcount change is necessary.
  void operator=(WTypedResourceHandle<ResourceType>&& rhs) { m_hTypeless = std::move(rhs.m_hTypeless); }

  /// Checks whether the two handles point to the same resource.
  W_ALWAYS_INLINE bool operator==(const WTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless == rhs.m_hTypeless; }

  /// Checks whether the two handles point to the same resource.
  W_ALWAYS_INLINE bool operator!=(const WTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless != rhs.m_hTypeless; }

  /// For storing handles as keys in maps
  W_ALWAYS_INLINE bool operator<(const WTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless < rhs.m_hTypeless; }

  /// Checks whether the handle points to the given resource.
  W_ALWAYS_INLINE bool operator==(const WResource* rhs) const { return m_hTypeless == rhs; }

  /// Checks whether the handle points to the given resource.
  W_ALWAYS_INLINE bool operator!=(const WResource* rhs) const { return m_hTypeless != rhs; }


  /// Returns the corresponding typeless resource handle.
  W_ALWAYS_INLINE operator const WTypelessResourceHandle() const { return m_hTypeless; }

  /// Returns the corresponding typeless resource handle.
  W_ALWAYS_INLINE operator WTypelessResourceHandle() { return m_hTypeless; }

  /// Returns whether the handle stores a valid pointer to a resource.
  W_ALWAYS_INLINE bool IsValid() const { return m_hTypeless.IsValid(); }

  /// Returns whether the handle stores a valid pointer to a resource.
  W_ALWAYS_INLINE explicit operator bool() const { return m_hTypeless.IsValid(); }

  /// Clears any reference to a resource and reduces its refcount.
  W_ALWAYS_INLINE void Invalidate() { m_hTypeless.Invalidate(); }

  /// Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  W_ALWAYS_INLINE WUInt64 GetResourceIDHash() const { return m_hTypeless.GetResourceIDHash(); }

  /// Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  W_ALWAYS_INLINE WStringView GetResourceID() const { return m_hTypeless.GetResourceID(); }

  /// The returns the resource description, if available, otherwise the resource ID.
  /// This is mainly for logging, where you want the more user friendly description, but the ID, if no description is available.
  /// If the handle is not valid, an empty string is returned.
  W_ALWAYS_INLINE WStringView GetResourceIdOrDescription() const { return m_hTypeless.GetResourceIdOrDescription(); }

  /// Attempts to copy the given typeless handle to this handle.
  ///
  /// It is an error to assign a typeless handle that references a resource with a mismatching type.
  void AssignFromTypelessHandle(const WTypelessResourceHandle& hHandle)
  {
    if (!hHandle.IsValid())
      return;

    W_ASSERT_DEV(hHandle.GetResourceType()->IsDerivedFrom<RESOURCE_TYPE>(), "Type '{}' does not match resource type '{}' in typeless handle.", WGetStaticRTTI<RESOURCE_TYPE>()->GetTypeName(), hHandle.GetResourceType()->GetTypeName());

    m_hTypeless = hHandle;
  }

private:
  template <typename T>
  friend class WTypedResourceHandle;

  // you must go through the resource manager to get access to the resource pointer
  friend class WResourceManager;
  friend class WResourceHandleWriteContext;
  friend class WResourceHandleReadContext;
  friend class WResourceHandleStreamOperations;

  WTypelessResourceHandle m_hTypeless;
};

template <typename T>
struct WHashHelper<WTypedResourceHandle<T>>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WTypedResourceHandle<T>& value) { return WHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  W_ALWAYS_INLINE static bool Equal(const WTypedResourceHandle<T>& a, const WTypedResourceHandle<T>& b) { return a == b; }
};

// Stream operations
class WResource;

class W_CORE_DLL WResourceHandleStreamOperations
{
public:
  template <typename ResourceType>
  static void WriteHandle(WStreamWriter& inout_stream, const WTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(inout_stream, hResource.m_hTypeless.m_pResource);
  }

  template <typename ResourceType>
  static void ReadHandle(WStreamReader& inout_stream, WTypedResourceHandle<ResourceType>& ref_hResourceHandle)
  {
    ReadHandle(inout_stream, ref_hResourceHandle.m_hTypeless);
  }

private:
  static void WriteHandle(WStreamWriter& Stream, const WResource* pResource);
  static void ReadHandle(WStreamReader& Stream, WTypelessResourceHandle& ResourceHandle);
};

/// Operator to serialize resource handles
template <typename ResourceType>
void operator<<(WStreamWriter& inout_stream, const WTypedResourceHandle<ResourceType>& hValue)
{
  WResourceHandleStreamOperations::WriteHandle(inout_stream, hValue);
}

/// Operator to deserialize resource handles
template <typename ResourceType>
void operator>>(WStreamReader& inout_stream, WTypedResourceHandle<ResourceType>& ref_hValue)
{
  WResourceHandleStreamOperations::ReadHandle(inout_stream, ref_hValue);
}
