#pragma once

#include <Foundation/Basics.h>

/// Default heap memory allocation policy.
///
/// \see WAllocatorWithPolicy
class WAllocPolicyHeap
{
public:
  W_ALWAYS_INLINE WAllocPolicyHeap(WAllocator* pParent) { W_IGNORE_UNUSED(pParent); }
  W_ALWAYS_INLINE ~WAllocPolicyHeap() = default;

  W_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    W_IGNORE_UNUSED(uiAlign);

    // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
    // if these asserts fail, you need to check what container made the allocation and change it
    // to use an aligned allocator, e.g. WAlignedAllocatorWrapper

    // unfortunately using W_ALIGNMENT_MINIMUM doesn't work, because even on 32 Bit systems we try to do allocations with 8 Byte
    // alignment interestingly, the code that does that, seems to work fine anyway
    W_ASSERT_DEBUG(uiAlign <= 8, "This allocator does not guarantee alignments larger than 8. Use an aligned allocator to allocate the desired data type.");

    void* ptr = malloc(uiSize);
    W_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  W_FORCE_INLINE void* Reallocate(void* pCurrentPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
  {
    W_IGNORE_UNUSED(uiCurrentSize);
    W_IGNORE_UNUSED(uiAlign);

    void* ptr = realloc(pCurrentPtr, uiNewSize);
    W_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  W_ALWAYS_INLINE void Deallocate(void* pPtr)
  {
    free(pPtr);
  }

  W_ALWAYS_INLINE WAllocator* GetParent() const { return nullptr; }
};
