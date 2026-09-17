#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Memory/LinearAllocator.h>
#include <Foundation/Memory/Policies/AllocPolicyStack.h>

struct alignas(W_ALIGNMENT_MINIMUM) NonAlignedVector
{
  W_DECLARE_POD_TYPE();

  NonAlignedVector()
  {
    x = 5.0f;
    y = 6.0f;
    z = 8.0f;
  }

  float x;
  float y;
  float z;
};

struct alignas(16) AlignedVector
{
  W_DECLARE_POD_TYPE();

  AlignedVector()
  {
    x = 5.0f;
    y = 6.0f;
    z = 8.0f;
  }

  float x;
  float y;
  float z;
  float w;
};

template <typename T>
void TestAlignmentHelper(size_t uiExpectedAlignment)
{
  WAllocator* pAllocator = WFoundation::GetAlignedAllocator();
  W_TEST_BOOL(pAllocator != nullptr);

  size_t uiAlignment = alignof(T);
  W_TEST_INT(uiAlignment, uiExpectedAlignment);

  T testOnStack = T();
  W_TEST_BOOL(WMemoryUtils::IsAligned(&testOnStack, uiExpectedAlignment));

  T* pTestBuffer = W_NEW_RAW_BUFFER(pAllocator, T, 32);
  WArrayPtr<T> TestArray = W_NEW_ARRAY(pAllocator, T, 32);

  // default constructor should be called even if we declare as a pod type
  W_TEST_FLOAT(TestArray[0].x, 5.0f, 0.0f);
  W_TEST_FLOAT(TestArray[0].y, 6.0f, 0.0f);
  W_TEST_FLOAT(TestArray[0].z, 8.0f, 0.0f);

  W_TEST_BOOL(WMemoryUtils::IsAligned(pTestBuffer, uiExpectedAlignment));
  W_TEST_BOOL(WMemoryUtils::IsAligned(TestArray.GetPtr(), uiExpectedAlignment));

  size_t uiExpectedSize = sizeof(T) * 32;

  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::AllocationStats)
  {
    W_TEST_INT(pAllocator->AllocatedSize(pTestBuffer), uiExpectedSize);

    WAllocator::Stats stats = pAllocator->GetStats();
    W_TEST_INT(stats.m_uiAllocationSize, uiExpectedSize * 2);
    W_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 2);
  }

  W_DELETE_ARRAY(pAllocator, TestArray);
  W_DELETE_RAW_BUFFER(pAllocator, pTestBuffer);

  if constexpr (WAllocatorTrackingMode::Default >= WAllocatorTrackingMode::Basics)
  {
    WAllocator::Stats stats = pAllocator->GetStats();
    W_TEST_INT(stats.m_uiAllocationSize, 0);
    W_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 0);
  }
}

W_CREATE_SIMPLE_TEST_GROUP(Memory);

W_CREATE_SIMPLE_TEST(Memory, Allocator)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Alignment")
  {
    TestAlignmentHelper<NonAlignedVector>(W_ALIGNMENT_MINIMUM);
    TestAlignmentHelper<AlignedVector>(16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LargeBlockAllocator")
  {
    enum
    {
      BLOCK_SIZE_IN_BYTES = 4096 * 4
    };
    const WUInt32 uiPageSize = WSystemInformation::Get().GetMemoryPageSize();

    WLargeBlockAllocator<BLOCK_SIZE_IN_BYTES> allocator("Test", WFoundation::GetDefaultAllocator(), WAllocatorTrackingMode::AllocationStats);

    WDynamicArray<WDataBlock<int, BLOCK_SIZE_IN_BYTES>> blocks;
    blocks.Reserve(1000);

    for (WUInt32 i = 0; i < 17; ++i)
    {
      auto block = allocator.AllocateBlock<int>();
      W_TEST_BOOL(WMemoryUtils::IsAligned(block.m_pData, uiPageSize)); // test page alignment
      W_TEST_INT(block.m_uiCount, 0);

      blocks.PushBack(block);
    }

    WAllocator::Stats stats = allocator.GetStats();

    W_TEST_BOOL(stats.m_uiNumAllocations == 17);
    W_TEST_BOOL(stats.m_uiNumDeallocations == 0);
    W_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (WUInt32 i = 0; i < 200; ++i)
    {
      auto block = allocator.AllocateBlock<int>();
      blocks.PushBack(block);
    }

    for (WUInt32 i = 0; i < 200; ++i)
    {
      allocator.DeallocateBlock(blocks.PeekBack());
      blocks.PopBack();
    }

    stats = allocator.GetStats();

    W_TEST_BOOL(stats.m_uiNumAllocations == 217);
    W_TEST_BOOL(stats.m_uiNumDeallocations == 200);
    W_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (WUInt32 i = 0; i < 2000; ++i)
    {
      WUInt32 uiAction = rand() % 2;
      if (uiAction == 0)
      {
        blocks.PushBack(allocator.AllocateBlock<int>());
      }
      else if (blocks.GetCount() > 0)
      {
        WUInt32 uiIndex = rand() % blocks.GetCount();
        auto block = blocks[uiIndex];

        allocator.DeallocateBlock(block);

        blocks.RemoveAtAndSwap(uiIndex);
      }
    }

    for (WUInt32 i = 0; i < blocks.GetCount(); ++i)
    {
      allocator.DeallocateBlock(blocks[i]);
    }

    stats = allocator.GetStats();

    W_TEST_BOOL(stats.m_uiNumAllocations - stats.m_uiNumDeallocations == 0);
    W_TEST_BOOL(stats.m_uiAllocationSize == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LinearAllocator")
  {
    WLinearAllocator<> allocator("TestLinearAllocator", WFoundation::GetAlignedAllocator(), 4096);

    void* blocks[8];
    for (size_t i = 0; i < W_ARRAY_SIZE(blocks); i++)
    {
      size_t size = i + 1;
      blocks[i] = allocator.Allocate(size, sizeof(void*), nullptr);
      W_TEST_BOOL(blocks[i] != nullptr);
      if (i > 0)
      {
        W_TEST_BOOL((WUInt8*)blocks[i - 1] + (size - 1) <= blocks[i]);
      }
    }

    for (size_t i = W_ARRAY_SIZE(blocks); i--;)
    {
      allocator.Deallocate(blocks[i]);
    }

    size_t sizes[] = {128, 128, 4096, 1024, 1024, 16000, 512, 512, 768, 768, 16000, 16000, 16000, 16000};
    void* allocs[W_ARRAY_SIZE(sizes)];
    for (size_t i = 0; i < W_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      W_TEST_BOOL(allocs[i] != nullptr);
    }
    for (size_t i = W_ARRAY_SIZE(sizes); i--;)
    {
      allocator.Deallocate(allocs[i]);
    }
    allocator.Reset();

    for (size_t i = 0; i < W_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      W_TEST_BOOL(allocs[i] != nullptr);
    }
    allocator.Reset();
    allocs[0] = allocator.Allocate(8, sizeof(void*), nullptr);
    W_TEST_BOOL(allocs[0] < allocs[1]);
    allocator.Deallocate(allocs[0]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LinearAllocator with non-PODs")
  {
    WLinearAllocator<> allocator("TestLinearAllocator", WFoundation::GetAlignedAllocator(), 4096);

    WDynamicArray<WConstructionCounter*> counters;
    counters.Reserve(100);

    for (WUInt32 i = 0; i < 100; ++i)
    {
      counters.PushBack(W_NEW(&allocator, WConstructionCounter));
    }

    for (WUInt32 i = 0; i < 100; ++i)
    {
      W_NEW(&allocator, NonAlignedVector);
    }

    W_TEST_BOOL(WConstructionCounter::HasConstructed(100));

    for (WUInt32 i = 0; i < 50; ++i)
    {
      W_DELETE(&allocator, counters[i * 2]);
    }

    W_TEST_BOOL(WConstructionCounter::HasDestructed(50));

    allocator.Reset();

    W_TEST_BOOL(WConstructionCounter::HasDestructed(50));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TempAllocator")
  {
    using TempAllocatorType = WAllocatorWithPolicy<WAllocPolicyStack<true>>;

    // Basic LIFO allocate/deallocate
    {
      TempAllocatorType allocator("TestTempAllocator", WFoundation::GetAlignedAllocator());

      WUInt32* pA = W_NEW_RAW_BUFFER(&allocator, WUInt32, 64);
      WUInt32* pB = W_NEW_RAW_BUFFER(&allocator, WUInt32, 128);
      WUInt32* pC = W_NEW_RAW_BUFFER(&allocator, WUInt32, 256);

      W_TEST_BOOL(pA != nullptr);
      W_TEST_BOOL(pB != nullptr);
      W_TEST_BOOL(pC != nullptr);

      // All within the same bucket, so addresses should be ascending
      W_TEST_BOOL(pA < pB);
      W_TEST_BOOL(pB < pC);

      // make copy of pA to check if it gets reused after deallocation
      WUInt32* pExpectedAlloc = pA;

      // Deallocate in LIFO order
      W_DELETE_RAW_BUFFER(&allocator, pC);
      W_DELETE_RAW_BUFFER(&allocator, pB);
      W_DELETE_RAW_BUFFER(&allocator, pA);

      WUInt32* pD = W_NEW_RAW_BUFFER(&allocator, WUInt32, 64);
      W_TEST_BOOL(pD != nullptr);
      W_TEST_BOOL(pD == pExpectedAlloc); // should reuse the same memory

      W_DELETE_RAW_BUFFER(&allocator, pD);
    }

    // Out-of-order deallocation
    {
      TempAllocatorType allocator("TestTempAllocator", WFoundation::GetAlignedAllocator());

      WUInt32* ptrs[5];
      for (int i = 0; i < 5; ++i)
      {
        ptrs[i] = W_NEW_RAW_BUFFER(&allocator, WUInt32, 32);
        W_TEST_BOOL(ptrs[i] != nullptr);
      }

      WUInt32* pExpectedAlloc = ptrs[1]; // should be reused after free

      // Free indices 1, 2, 3 out of order (all become nullptr entries)
      W_DELETE_RAW_BUFFER(&allocator, ptrs[1]);
      W_DELETE_RAW_BUFFER(&allocator, ptrs[2]);
      W_DELETE_RAW_BUFFER(&allocator, ptrs[3]);

      // Free index 4 (last) - should cascade and also pop the nullptr entries for 3, 2, 1
      W_DELETE_RAW_BUFFER(&allocator, ptrs[4]);

      WUInt32* pD = W_NEW_RAW_BUFFER(&allocator, WUInt32, 64);
      W_TEST_BOOL(pD != nullptr);
      W_TEST_BOOL(pD == pExpectedAlloc); // should reuse the same memory

      W_DELETE_RAW_BUFFER(&allocator, ptrs[0]);
      W_DELETE_RAW_BUFFER(&allocator, pD);
    }

    // Multiple buckets (allocations that span across bucket boundaries)
    {
      TempAllocatorType allocator("TestTempAllocator", WFoundation::GetAlignedAllocator());

      // Fill first bucket (1024*1024 bytes max)
      WUInt32* pA = W_NEW_RAW_BUFFER(&allocator, WUInt32, 128 * 1024);
      WUInt32* pB = W_NEW_RAW_BUFFER(&allocator, WUInt32, 128 * 1024);
      W_TEST_BOOL(pA != nullptr);
      W_TEST_BOOL(pB != nullptr);

      // This should trigger a new bucket
      WUInt32* pC = W_NEW_RAW_BUFFER(&allocator, WUInt32, 64);
      W_TEST_BOOL(pC != nullptr);

      // Deallocate in LIFO order - should roll back across bucket boundary
      W_DELETE_RAW_BUFFER(&allocator, pC);
      W_DELETE_RAW_BUFFER(&allocator, pB);
      W_DELETE_RAW_BUFFER(&allocator, pA);
    }

    // Large allocations that exceed max allocation size (parent allocator fallback)
    {
      TempAllocatorType allocator("TestTempAllocator", WFoundation::GetAlignedAllocator());

      // This allocation exceeds the max allocation size, so it goes to the parent allocator
      WUInt32* pLarge = W_NEW_RAW_BUFFER(&allocator, WUInt32, 1024 * 1024);
      W_TEST_BOOL(pLarge != nullptr);

      // Mix a normal allocation in between
      WUInt32* pSmall = W_NEW_RAW_BUFFER(&allocator, WUInt32, 64);
      W_TEST_BOOL(pSmall != nullptr);

      // Deallocating the large one should go to parent (not found in m_Allocations search, falls through)
      W_DELETE_RAW_BUFFER(&allocator, pLarge);

      W_DELETE_RAW_BUFFER(&allocator, pSmall);
    }
  }
}
