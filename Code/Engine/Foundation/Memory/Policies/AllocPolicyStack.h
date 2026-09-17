#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// This policy implements a stack allocator. It is designed for scenarios where you have a lot of short-lived allocations that are freed in a LIFO order,
/// but it also supports freeing in arbitrary order (at the cost of some fragmentation).
///
/// For debugging purposes, the policy can also overwrite all freed memory with 0xCDCDCDCD to make it easier to find use-after-free situations.
///
/// \see WAllocatorWithPolicy
template <bool OverwriteMemoryOnFree = false>
class WAllocPolicyStack
{
public:
  enum
  {
    Alignment = 16
  };

  W_FORCE_INLINE WAllocPolicyStack(WAllocator* pParent)
    : m_pParent(pParent)
    , m_uiMaxAllocSizeLog2(20) // 1024 * 1024 = 2^20
  {
  }

  W_FORCE_INLINE ~WAllocPolicyStack()
  {
    W_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && m_uiCurrentBucketOffset == 0 && m_Allocations.IsEmpty(), "There is still something allocated!");
    for (auto& bucket : m_Buckets)
    {
      m_pParent->Deallocate(bucket.GetPtr());
    }
  }

  /// Sets the maximum allocation size that can be handled by this stack allocator. Allocations larger than this size will be directly allocated from the parent allocator.
  W_FORCE_INLINE void SetMaxAllocationSize(WUInt32 uiSize)
  {
    W_ASSERT_DEV(m_Allocations.IsEmpty(), "Cannot change max allocation size while there are active allocations!");

    m_uiMaxAllocSizeLog2 = WMath::Log2i(uiSize);
  }

  W_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    // For allocations larger than the max allocation size, we directly allocate from the parent allocator.
    if (uiSize > GetMaxAllocationSize())
    {
      return m_pParent->Allocate(uiSize, uiAlign);
    }

    W_LOCK(m_Mutex);

    W_IGNORE_UNUSED(uiAlign);
    W_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment {0}", ((WUInt32)uiAlign));
    const WUInt32 uiSize32 = static_cast<WUInt32>(WMemoryUtils::AlignSize(uiSize, (size_t)Alignment));

    auto bucket = GetOrCreateBucket(uiSize32);
    W_ASSERT_DEBUG(m_uiCurrentBucketOffset + uiSize <= bucket.GetCount(), "");

    WUInt8* ptr = bucket.GetPtr() + m_uiCurrentBucketOffset;

    const WUInt32 uiGlobalOffset = (m_uiCurrentBucketIndex << m_uiMaxAllocSizeLog2) + m_uiCurrentBucketOffset;
    m_Allocations.PushBack({ptr, uiGlobalOffset, uiSize32});

    m_uiCurrentBucketOffset += uiSize32;

    return ptr;
  }

  W_FORCE_INLINE void Deallocate(void* pPtr)
  {
    W_LOCK(m_Mutex);

    if (m_Allocations.IsEmpty())
    {
      m_pParent->Deallocate(pPtr);
      return;
    }

    auto& lastAlloc = m_Allocations.PeekBack();
    if (lastAlloc.m_Ptr == pPtr)
    {
      const WUInt32 uiOffsetMask = WMath::Bitmask_LowN<WUInt32>(m_uiMaxAllocSizeLog2);

      m_uiCurrentBucketIndex = static_cast<WUInt16>(lastAlloc.m_uiGlobalOffset >> m_uiMaxAllocSizeLog2);
      m_uiCurrentBucketOffset = lastAlloc.m_uiGlobalOffset & uiOffsetMask;

      if constexpr (OverwriteMemoryOnFree)
      {
        WMemoryUtils::PatternFill(static_cast<WUInt8*>(pPtr), 0xCD, lastAlloc.m_uiSize);
      }

      m_Allocations.PopBack();

      while (m_Allocations.IsEmpty() == false)
      {
        auto& alloc = m_Allocations.PeekBack();
        if (alloc.m_Ptr != nullptr)
          return;

        m_uiCurrentBucketIndex = static_cast<WUInt16>(alloc.m_uiGlobalOffset >> m_uiMaxAllocSizeLog2);
        m_uiCurrentBucketOffset = alloc.m_uiGlobalOffset & uiOffsetMask;

        m_Allocations.PopBack();
      }

      return;
    }
    else
    {
      for (WUInt32 i = m_Allocations.GetCount() - 1; i > 0; --i)
      {
        auto& alloc = m_Allocations[i - 1];
        if (alloc.m_Ptr == pPtr)
        {
          if constexpr (OverwriteMemoryOnFree)
          {
            WMemoryUtils::PatternFill(static_cast<WUInt8*>(pPtr), 0xCD, alloc.m_uiSize);
          }

          alloc.m_Ptr = nullptr;
          return;
        }
      }
    }

    // Allocation not found, this means it was a large allocation that was directly allocated from the parent allocator.
    m_pParent->Deallocate(pPtr);
  }

  W_ALWAYS_INLINE WAllocator* GetParent() const { return m_pParent; }

private:
  WUInt32 GetMaxAllocationSize() const { return 1u << m_uiMaxAllocSizeLog2; }

  WArrayPtr<WUInt8> GetOrCreateBucket(WUInt32 uiRequestedSize)
  {
    const WUInt32 uiMaxAllocSize = GetMaxAllocationSize();
    const bool bFitsIntoCurrentBucket = !m_Buckets.IsEmpty() && m_uiCurrentBucketOffset + uiRequestedSize <= uiMaxAllocSize;
    if (!bFitsIntoCurrentBucket)
    {
      m_uiCurrentBucketIndex = static_cast<WUInt16>(m_Buckets.GetCount());

      auto newBucket = WMakeArrayPtr(static_cast<WUInt8*>(m_pParent->Allocate(uiMaxAllocSize, Alignment)), uiMaxAllocSize);
      m_Buckets.PushBack(newBucket);

      m_uiCurrentBucketOffset = 0;
    }

    return m_Buckets[m_uiCurrentBucketIndex];
  }

  WAllocator* m_pParent = nullptr;

  WMutex m_Mutex;

  WUInt16 m_uiMaxAllocSizeLog2 = 0;
  WUInt16 m_uiCurrentBucketIndex = 0;
  WUInt32 m_uiCurrentBucketOffset = 0;

  WSmallArray<WArrayPtr<WUInt8>, 4> m_Buckets;

  struct AllocationInfo
  {
    W_DECLARE_POD_TYPE();

    void* m_Ptr;
    WUInt32 m_uiGlobalOffset;
    WUInt32 m_uiSize;
  };

  WSmallArray<AllocationInfo, 16> m_Allocations;
};
