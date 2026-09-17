#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>

namespace
{
  struct CustomComparer
  {
    W_ALWAYS_INLINE bool Less(WInt32 a, WInt32 b) const { return a > b; }

    // Comparision via operator. Sorting algorithm should prefer Less operator
    bool operator()(WInt32 a, WInt32 b) const { return a < b; }
  };
} // namespace

W_CREATE_SIMPLE_TEST(Algorithm, Sorting)
{
  WDynamicArray<WInt32> a1;

  for (WUInt32 i = 0; i < 2000; ++i)
  {
    a1.PushBack(rand() % 100000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "QuickSort")
  {
    WDynamicArray<WInt32> a2 = a1;

    WSorting::QuickSort(a1, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

    for (WUInt32 i = 1; i < a1.GetCount(); ++i)
    {
      W_TEST_BOOL(a1[i - 1] >= a1[i]);
    }

    WArrayPtr<WInt32> arrayPtr = a2;
    WSorting::QuickSort(arrayPtr, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

    for (WUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
    {
      W_TEST_BOOL(arrayPtr[i - 1] >= arrayPtr[i]);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "QuickSort - Lambda")
  {
    WDynamicArray<WInt32> a2 = a1;
    WSorting::QuickSort(a2, [](const auto& a, const auto& b)
      { return a > b; });

    for (WUInt32 i = 1; i < a2.GetCount(); ++i)
    {
      W_TEST_BOOL(a2[i - 1] >= a2[i]);
    }
  }
}
