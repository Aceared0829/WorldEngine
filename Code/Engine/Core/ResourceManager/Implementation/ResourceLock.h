#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>

/// Helper class to acquire and release a resource safely.
///
/// The constructor calls WResourceManager::BeginAcquireResource, the destructor makes sure to call WResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
///
/// Whether the acquisition succeeded or returned a loading fallback, missing fallback or even no result, at all,
/// can be retrieved through GetAcquireResult().
/// \note If a resource is missing, but no missing fallback is specified for the resource type, the code will fail with an assertion,
/// unless you used WResourceAcquireMode::BlockTillLoaded_NeverFail. Only then will the error be silently ignored and the acquire result
/// will be WResourceAcquireResult::None.
///
/// \sa WResourceManager::BeginAcquireResource()
/// \sa WResourceAcquireMode
/// \sa WResourceAcquireResult
template <class RESOURCE_TYPE>
class WResourceLock
{
public:
  W_ALWAYS_INLINE WResourceLock(const WTypedResourceHandle<RESOURCE_TYPE>& hResource, WResourceAcquireMode mode,
    const WTypedResourceHandle<RESOURCE_TYPE>& hFallbackResource = WTypedResourceHandle<RESOURCE_TYPE>())
  {
    m_pResource = WResourceManager::BeginAcquireResource(hResource, mode, hFallbackResource, &m_AcquireResult);
  }

  WResourceLock(const WResourceLock&) = delete;

  WResourceLock(WResourceLock&& other)
    : m_AcquireResult(other.m_AcquireResult)
    , m_pResource(other.m_pResource)
  {
    other.m_pResource = nullptr;
    other.m_AcquireResult = WResourceAcquireResult::None;
  }

  W_ALWAYS_INLINE ~WResourceLock()
  {
    if (m_pResource)
    {
      WResourceManager::EndAcquireResource(m_pResource);
    }
  }

  W_ALWAYS_INLINE RESOURCE_TYPE* operator->() { return m_pResource; }
  W_ALWAYS_INLINE const RESOURCE_TYPE* operator->() const { return m_pResource; }

  W_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }
  W_ALWAYS_INLINE explicit operator bool() const { return m_pResource != nullptr; }

  W_ALWAYS_INLINE WResourceAcquireResult GetAcquireResult() const { return m_AcquireResult; }

  W_ALWAYS_INLINE const RESOURCE_TYPE* GetPointer() const { return m_pResource; }
  W_ALWAYS_INLINE RESOURCE_TYPE* GetPointerNonConst() const { return m_pResource; }

private:
  WResourceAcquireResult m_AcquireResult;
  RESOURCE_TYPE* m_pResource;
};
