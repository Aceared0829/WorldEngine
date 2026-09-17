#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>

/// Aligned Heap memory allocation policy.
///
/// \see WAllocatorWithPolicy
class WAllocPolicyAlignedHeap
{
public:
  W_ALWAYS_INLINE WAllocPolicyAlignedHeap(WAllocator* pParent) { W_IGNORE_UNUSED(pParent); }
  W_ALWAYS_INLINE ~WAllocPolicyAlignedHeap() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void Deallocate(void* pPtr);

  W_ALWAYS_INLINE WAllocator* GetParent() const { return nullptr; }
};

// include the platform specific implementation
#include <AllocPolicyAlignedHeap_Platform.h>
