#pragma once

#include <Foundation/Containers/SmallArray.h>

/// This policy implements a linear allocator that can only grow and at some point all allocations gets reset at once.
///
/// For debugging purposes, the policy can also overwrite all freed memory with 0xCDCDCDCD to make it easier to find use-after-free situations.
///
/// \see WAllocatorWithPolicy
template <bool OverwriteMemoryOnReset = false>
class WAllocPolicyLinear
{
public:
  enum
  {
    Alignment = 16
  };

  W_FORCE_INLINE WAllocPolicyLinear(WAllocator* pParent)
    : m_pParent(pParent)
    , m_uiNextBucketSize(4096)
  {
  }

  W_FORCE_INLINE ~WAllocPolicyLinear()
  {
    W_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && (m_Buckets.IsEmpty() || m_Buckets[m_uiCurrentBucketIndex].GetPtr() == m_pNextAllocation),
      "There is still something allocated!");
    for (auto& bucket : m_Buckets)
    {
      m_pParent->Deallocate(bucket.GetPtr());
    }
  }

  /// Sets the size of the next bucket to allocate. This can be used to prevent an excessive number of buckets if the required total allocation size is known in advance.
  W_FORCE_INLINE void SetNextBucketSize(WUInt32 uiSize)
  {
    m_uiNextBucketSize = uiSize;
  }

  W_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    W_IGNORE_UNUSED(uiAlign);
    W_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment {0}", ((WUInt32)uiAlign));
    uiSize = WMemoryUtils::AlignSize(uiSize, (size_t)Alignment);

    bool bFoundBucket = !m_Buckets.IsEmpty() && m_pNextAllocation + uiSize <= m_Buckets[m_uiCurrentBucketIndex].GetEndPtr();

    if (!bFoundBucket)
    {
      // Check if there is an empty bucket that fits the allocation
      for (WUInt32 i = m_uiCurrentBucketIndex + 1; i < m_Buckets.GetCount(); ++i)
      {
        auto& testBucket = m_Buckets[i];
        if (uiSize <= testBucket.GetCount())
        {
          m_uiCurrentBucketIndex = i;
          m_pNextAllocation = testBucket.GetPtr();
          bFoundBucket = true;
          break;
        }
      }
    }

    if (!bFoundBucket)
    {
      while (uiSize > m_uiNextBucketSize)
      {
        W_ASSERT_DEBUG(m_uiNextBucketSize > 0, "");

        m_uiNextBucketSize *= 2;
      }

      m_uiCurrentBucketIndex = m_Buckets.GetCount();

      auto newBucket = WArrayPtr<WUInt8>(static_cast<WUInt8*>(m_pParent->Allocate(m_uiNextBucketSize, Alignment)), m_uiNextBucketSize);
      m_Buckets.PushBack(newBucket);

      m_pNextAllocation = newBucket.GetPtr();

      m_uiNextBucketSize *= 2;
    }

    W_ASSERT_DEBUG(m_pNextAllocation + uiSize <= m_Buckets[m_uiCurrentBucketIndex].GetEndPtr(), "");

    WUInt8* ptr = m_pNextAllocation;
    m_pNextAllocation += uiSize;
    return ptr;
  }

  W_FORCE_INLINE void Deallocate(void* pPtr)
  {
    W_IGNORE_UNUSED(pPtr);
    // Individual deallocation is not supported by this allocator
  }

  W_FORCE_INLINE void Reset()
  {
    m_uiCurrentBucketIndex = 0;
    m_pNextAllocation = !m_Buckets.IsEmpty() ? m_Buckets[0].GetPtr() : nullptr;

    if constexpr (OverwriteMemoryOnReset)
    {
      for (auto& bucket : m_Buckets)
      {
        WMemoryUtils::PatternFill(bucket.GetPtr(), 0xCD, bucket.GetCount());
      }
    }
  }

  W_FORCE_INLINE void FillStats(WAllocator::Stats& ref_stats)
  {
    ref_stats.m_uiNumAllocations = m_Buckets.GetCount();
    for (auto& bucket : m_Buckets)
    {
      ref_stats.m_uiAllocationSize += bucket.GetCount();
    }
  }

  W_ALWAYS_INLINE WAllocator* GetParent() const { return m_pParent; }

private:
  WAllocator* m_pParent = nullptr;

  WUInt32 m_uiCurrentBucketIndex = 0;
  WUInt32 m_uiNextBucketSize = 0;

  WUInt8* m_pNextAllocation = nullptr;

  WSmallArray<WArrayPtr<WUInt8>, 4> m_Buckets;
};
