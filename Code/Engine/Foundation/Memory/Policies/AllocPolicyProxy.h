#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>

/// This Allocation policy redirects all operations to its parent.
///
/// \note Note that the stats are taken on the proxy as well as on the parent.
///
/// \see WAllocatorWithPolicy
class WAllocPolicyProxy
{
public:
  W_FORCE_INLINE WAllocPolicyProxy(WAllocator* pParent)
    : m_pParent(pParent)
  {
    W_ASSERT_ALWAYS(m_pParent != nullptr, "Parent allocator must not be nullptr");
  }

  W_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign) { return m_pParent->Allocate(uiSize, uiAlign); }

  W_FORCE_INLINE void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
  {
    return m_pParent->Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);
  }

  W_FORCE_INLINE void Deallocate(void* pPtr) { m_pParent->Deallocate(pPtr); }

  W_FORCE_INLINE size_t AllocatedSize(const void* pPtr) { return m_pParent->AllocatedSize(pPtr); }

  W_ALWAYS_INLINE WAllocator* GetParent() const { return m_pParent; }

private:
  WAllocator* m_pParent;
};
