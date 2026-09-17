#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Types/UniquePtr.h>

static WInt32 iCallPodConstructor = 0;
static WInt32 iCallPodDestructor = 0;
static WInt32 iCallNonPodConstructor = 0;
static WInt32 iCallNonPodDestructor = 0;

namespace DynamicArrayTestDetail
{
  using st = WConstructionCounter;

  static int g_iDummyCounter = 0;

  class Dummy
  {
  public:
    int a;
    int b;
    std::string s;

    Dummy()
      : a(0)
      , b(g_iDummyCounter++)
      , s("Test")
    {
    }
    Dummy(int a)
      : a(a)
      , b(g_iDummyCounter++)
      , s("Test")
    {
    }

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };

  WAllocator* g_pTestAllocator;

  struct WTestAllocatorWrapper
  {
    static WAllocator* GetAllocator() { return g_pTestAllocator; }
  };

  template <typename T = st, typename AllocatorWrapper = WTestAllocatorWrapper>
  static WDynamicArray<T, AllocatorWrapper> CreateArray(WUInt32 uiSize, WUInt32 uiOffset)
  {
    WDynamicArray<T, AllocatorWrapper> a;
    a.SetCount(uiSize);

    for (WUInt32 i = 0; i < uiSize; ++i)
      a[i] = T(uiOffset + i);

    return a;
  }
} // namespace DynamicArrayTestDetail

#if W_ENABLED(W_PLATFORM_64BIT)
static_assert(sizeof(WDynamicArray<WInt32>) == 24);
#else
static_assert(sizeof(WDynamicArray<WInt32>) == 16);
#endif

W_CREATE_SIMPLE_TEST_GROUP(Containers);

W_CREATE_SIMPLE_TEST(Containers, DynamicArray)
{
  iCallPodConstructor = 0;
  iCallPodDestructor = 0;
  iCallNonPodConstructor = 0;
  iCallNonPodDestructor = 0;

  WProxyAllocator proxy("DynamicArrayTestAllocator", WFoundation::GetDefaultAllocator());
  DynamicArrayTestDetail::g_pTestAllocator = &proxy;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WDynamicArray<WInt32> a1;
    WDynamicArray<DynamicArrayTestDetail::st> a2;

    W_TEST_BOOL(a1.GetCount() == 0);
    W_TEST_BOOL(a2.GetCount() == 0);
    W_TEST_BOOL(a1.IsEmpty());
    W_TEST_BOOL(a2.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor")
  {
    WDynamicArray<WInt32, DynamicArrayTestDetail::WTestAllocatorWrapper> a1;

    W_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (WInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    W_TEST_BOOL(a1.GetHeapMemoryUsage() >= 32 * sizeof(WInt32));

    WDynamicArray<WInt32> a2 = a1;
    WDynamicArray<WInt32> a3(a1);

    W_TEST_BOOL(a1 == a2);
    W_TEST_BOOL(a1 == a3);
    W_TEST_BOOL(a2 == a3);

    WInt32 test[] = {1, 2, 3, 4};
    WArrayPtr<WInt32> aptr(test);

    WDynamicArray<WInt32> a4(aptr);

    W_TEST_BOOL(a4 == aptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move Constructor / Operator")
  {
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move constructor
      WDynamicArray<DynamicArrayTestDetail::st, DynamicArrayTestDetail::WTestAllocatorWrapper> a1(DynamicArrayTestDetail::CreateArray(100, 20));

      W_TEST_INT(a1.GetCount(), 100);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator
      a1 = DynamicArrayTestDetail::CreateArray(200, 50);

      W_TEST_INT(a1.GetCount(), 200);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 50 + i);
    }

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move assignment with different allocators
      WConstructionCounterRelocatable::Reset();
      WProxyAllocator proxyAllocator("test allocator", WFoundation::GetDefaultAllocator());
      {
        WDynamicArray<WConstructionCounterRelocatable> a1(&proxyAllocator);

        a1 = DynamicArrayTestDetail::CreateArray<WConstructionCounterRelocatable, WDefaultAllocatorWrapper>(8, 70);
        W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(8, 0));
        W_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        W_TEST_INT(a1.GetCount(), 8);
        for (WUInt32 i = 0; i < a1.GetCount(); ++i)
          W_TEST_INT(a1[i].m_iData, 70 + i);

        a1 = DynamicArrayTestDetail::CreateArray<WConstructionCounterRelocatable, WDefaultAllocatorWrapper>(32, 100);
        W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(32, 8));
        W_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        W_TEST_INT(a1.GetCount(), 32);
        for (WUInt32 i = 0; i < a1.GetCount(); ++i)
          W_TEST_INT(a1[i].m_iData, 100 + i);
      }

      W_TEST_BOOL(WConstructionCounterRelocatable::HasAllDestructed());
      WConstructionCounterRelocatable::Reset();

      auto allocatorStats = proxyAllocator.GetStats();
      W_TEST_BOOL(allocatorStats.m_uiNumAllocations == allocatorStats.m_uiNumDeallocations); // check for memory leak?
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Convert to ArrayPtr")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 100; ++i)
    {
      WInt32 r = rand() % 100000;
      a1.PushBack(r);
    }

    WArrayPtr<WInt32> ap = a1;

    W_TEST_BOOL(ap.GetCount() == a1.GetCount());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator =")
  {
    WDynamicArray<WInt32, DynamicArrayTestDetail::WTestAllocatorWrapper> a1;
    WDynamicArray<WInt32> a2;

    for (WInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    W_TEST_BOOL(a1 == a2);

    WArrayPtr<WInt32> arrayPtr(a1);

    a2 = arrayPtr;

    W_TEST_BOOL(a2 == arrayPtr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator == / !=/ <")
  {
    WDynamicArray<WInt32> a1, a2;

    W_TEST_BOOL(a1 == a1);
    W_TEST_BOOL(a2 == a2);
    W_TEST_BOOL(a1 == a2);

    W_TEST_BOOL((a1 != a1) == false);
    W_TEST_BOOL((a2 != a2) == false);
    W_TEST_BOOL((a1 != a2) == false);

    for (WInt32 i = 0; i < 100; ++i)
    {
      WInt32 r = rand() % 100000;
      a1.PushBack(r);
      a2.PushBack(r);
    }

    W_TEST_BOOL(a1 == a1);
    W_TEST_BOOL(a2 == a2);
    W_TEST_BOOL(a1 == a2);

    W_TEST_BOOL((a1 != a2) == false);

    W_TEST_BOOL((a1 < a2) == false);
    a2.PushBack(100);
    W_TEST_BOOL(a1 < a2);
    a1.PushBack(99);
    W_TEST_BOOL(a1 < a2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Index operator")
  {
    WDynamicArray<WInt32> a1;
    a1.SetCountUninitialized(100);

    for (WInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(a1[i], i);

    WDynamicArray<WInt32> ca1;
    ca1 = a1;

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(ca1[i], i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    WDynamicArray<WInt32> a1;

    W_TEST_BOOL(a1.IsEmpty());

    for (WInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      W_TEST_INT(a1[i], 0);
      a1[i] = i;

      W_TEST_INT(a1.GetCount(), i + 1);
      W_TEST_BOOL(!a1.IsEmpty());
    }

    for (WInt32 i = 0; i < 128; ++i)
      W_TEST_INT(a1[i], i);

    for (WInt32 i = 128; i >= 0; --i)
    {
      a1.SetCount(i);

      W_TEST_INT(a1.GetCount(), i);

      for (WInt32 i2 = 0; i2 < i; ++i2)
        W_TEST_INT(a1[i2], i2);
    }

    W_TEST_BOOL(a1.IsEmpty());

    a1.SetCountUninitialized(32);
    W_TEST_INT(a1.GetCount(), 32);
    a1[31] = 45;
    W_TEST_INT(a1[31], 45);

    // Test SetCount with fill value
    {
      WDynamicArray<WInt32> a2;
      a2.PushBack(5);
      a2.PushBack(3);
      a2.SetCount(10, 42);

      if (W_TEST_INT(a2.GetCount(), 10))
      {
        W_TEST_INT(a2[0], 5);
        W_TEST_INT(a2[1], 3);
        W_TEST_INT(a2[4], 42);
        W_TEST_INT(a2[9], 42);
      }

      a2.Clear();
      a2.PushBack(1);
      a2.PushBack(2);
      a2.PushBack(3);

      a2.SetCount(2, 10);
      if (W_TEST_INT(a2.GetCount(), 2))
      {
        W_TEST_INT(a2[0], 1);
        W_TEST_INT(a2[1], 2);
      }
    }
  }

  // Test SetCount with fill value
  {
    WDynamicArray<WInt32> a2;
    a2.PushBack(5);
    a2.PushBack(3);
    a2.SetCount(10, 42);

    if (W_TEST_INT(a2.GetCount(), 10))
    {
      W_TEST_INT(a2[0], 5);
      W_TEST_INT(a2[1], 3);
      W_TEST_INT(a2[4], 42);
      W_TEST_INT(a2[9], 42);
    }

    a2.Clear();
    a2.PushBack(1);
    a2.PushBack(2);
    a2.PushBack(3);

    a2.SetCount(2, 10);
    if (W_TEST_INT(a2.GetCount(), 2))
    {
      W_TEST_INT(a2[0], 1);
      W_TEST_INT(a2[1], 2);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EnsureCount")
  {
    WDynamicArray<WInt32> a1;

    W_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(0);
    W_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(1);
    W_TEST_INT(a1.GetCount(), 1);

    a1.EnsureCount(2);
    W_TEST_INT(a1.GetCount(), 2);

    a1.EnsureCount(1);
    W_TEST_INT(a1.GetCount(), 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    WDynamicArray<WInt32> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    W_TEST_BOOL(a1.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = -100; i < 100; ++i)
      W_TEST_BOOL(!a1.Contains(i));

    for (WInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);
    for (WInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    for (WInt32 i = 0; i < 100; ++i)
    {
      W_TEST_BOOL(a1.Contains(i));
      W_TEST_INT(a1.IndexOf(i), i);
      W_TEST_INT(a1.IndexOf(i, 100), i + 100);
      W_TEST_INT(a1.LastIndexOf(i), i + 100);
      W_TEST_INT(a1.LastIndexOf(i, 100), i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushBackUnchecked / PushBackRange")
  {
    WDynamicArray<WInt32> a1;
    a1.Reserve(100);

    for (WInt32 i = 0; i < 100; ++i)
      a1.PushBackUnchecked(i);

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(a1[i], i);

    WInt32 temp[] = {100, 101, 102, 103, 104};
    WArrayPtr<WInt32> range(temp);

    a1.PushBackRange(range);

    W_TEST_INT(a1.GetCount(), 105);
    for (WUInt32 i = 0; i < a1.GetCount(); ++i)
      W_TEST_INT(a1[i], i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert")
  {
    WDynamicArray<WInt32> a1;

    // always inserts at the front
    for (WInt32 i = 0; i < 100; ++i)
      a1.InsertAt(0, i);

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(a1[i], 99 - i);

    WUniquePtr<DynamicArrayTestDetail::st> ptr = W_DEFAULT_NEW(DynamicArrayTestDetail::st);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      WDynamicArray<WUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (WUInt32 i = 0; i < 10; ++i)
        a2.InsertAt(0, WUniquePtr<DynamicArrayTestDetail::st>());

      a2.InsertAt(0, std::move(ptr));
      W_TEST_BOOL(ptr == nullptr);
      W_TEST_BOOL(a2[0] != nullptr);

      for (WUInt32 i = 1; i < a2.GetCount(); ++i)
        W_TEST_BOOL(a2[i] == nullptr);
    }

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "InsertRange")
  {
    // Pod element tests
    WDynamicArray<WInt32> intTestRange;
    WDynamicArray<WInt32> a1;

    WInt32 intTemp1[] = {91, 92, 93, 94, 95};
    WArrayPtr<WInt32> intRange1(intTemp1);

    WInt32 intTemp2[] = {96, 97, 98, 99, 100};
    WArrayPtr<WInt32> intRange2(intTemp2);

    WInt32 intTemp3[] = {100, 101, 102, 103, 104};
    WArrayPtr<WInt32> intRange3(intTemp3);

    {
      intTestRange.PushBackRange(intRange3);

      a1.InsertRangeAt(0, intRange3);

      W_TEST_INT(a1.GetCount(), 5);

      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i], intTestRange[i]);
    }

    {
      intTestRange.Clear();
      intTestRange.PushBackRange(intRange1);
      intTestRange.PushBackRange(intRange3);

      a1.InsertRangeAt(0, intRange1);

      W_TEST_INT(a1.GetCount(), 10);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i], intTestRange[i]);
    }

    {
      intTestRange.Clear();
      intTestRange.PushBackRange(intRange1);
      intTestRange.PushBackRange(intRange2);
      intTestRange.PushBackRange(intRange3);

      a1.InsertRangeAt(5, intRange2);

      W_TEST_INT(a1.GetCount(), 15);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i], intTestRange[i]);
    }

    // Class element tests
    WDynamicArray<WDeque<WString>> classTestRange;
    WDynamicArray<WDeque<WString>> a2;

    WDeque<WString> strTemp1[4];
    {
      strTemp1[0].PushBack("One");
      strTemp1[1].PushBack("Two");
      strTemp1[2].PushBack("Three");
      strTemp1[3].PushBack("Four");
    }
    WArrayPtr<WDeque<WString>> classRange1(strTemp1);

    WDeque<WString> strTemp2[3];
    {
      strTemp2[0].PushBack("Five");
      strTemp2[1].PushBack("Six");
      strTemp2[2].PushBack("Seven");
    }
    WArrayPtr<WDeque<WString>> classRange2(strTemp2);

    WDeque<WString> strTemp3[3];
    {
      strTemp3[0].PushBack("Eight");
      strTemp3[1].PushBack("Nine");
      strTemp3[2].PushBack("Ten");
    }
    WArrayPtr<WDeque<WString>> classRange3(strTemp3);

    {
      classTestRange.PushBackRange(classRange3);

      a2.InsertRangeAt(0, classRange3);

      W_TEST_INT(a2.GetCount(), 3);

      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }

    {
      classTestRange.Clear();
      classTestRange.PushBackRange(classRange1);
      classTestRange.PushBackRange(classRange3);

      a2.InsertRangeAt(0, classRange1);

      W_TEST_INT(a2.GetCount(), 7);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }

    {
      classTestRange.Clear();
      classTestRange.PushBackRange(classRange1);
      classTestRange.PushBackRange(classRange2);
      classTestRange.PushBackRange(classRange3);

      a2.InsertRangeAt(4, classRange2);

      W_TEST_INT(a2.GetCount(), 10);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAndCopy")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 100; ++i)
      a1.PushBack(i % 2);

    while (a1.RemoveAndCopy(1))
    {
    }

    W_TEST_BOOL(a1.GetCount() == 50);

    for (WUInt32 i = 0; i < a1.GetCount(); ++i)
      W_TEST_INT(a1[i], 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAndSwap")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAndSwap(9);
    a1.RemoveAndSwap(7);
    a1.RemoveAndSwap(5);
    a1.RemoveAndSwap(3);
    a1.RemoveAndSwap(1);

    W_TEST_INT(a1.GetCount(), 5);

    for (WInt32 i = 0; i < 5; ++i)
      W_TEST_BOOL(WMath::IsEven(a1[i]));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAtAndCopy")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAtAndCopy(9);
    a1.RemoveAtAndCopy(7);
    a1.RemoveAtAndCopy(5);
    a1.RemoveAtAndCopy(3);
    a1.RemoveAtAndCopy(1);

    W_TEST_INT(a1.GetCount(), 5);

    for (WInt32 i = 0; i < 5; ++i)
      W_TEST_INT(a1[i], i * 2);

    WUniquePtr<DynamicArrayTestDetail::st> ptr = W_DEFAULT_NEW(DynamicArrayTestDetail::st);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      WDynamicArray<WUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (WUInt32 i = 0; i < 10; ++i)
        a2.InsertAt(0, WUniquePtr<DynamicArrayTestDetail::st>());

      a2.PushBack(std::move(ptr));
      W_TEST_BOOL(ptr == nullptr);
      W_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndCopy(0);
      W_TEST_BOOL(a2[9] != nullptr);
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));
    }

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAtAndSwap")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAtAndSwap(9);
    a1.RemoveAtAndSwap(7);
    a1.RemoveAtAndSwap(5);
    a1.RemoveAtAndSwap(3);
    a1.RemoveAtAndSwap(1);

    W_TEST_INT(a1.GetCount(), 5);

    for (WInt32 i = 0; i < 5; ++i)
      W_TEST_BOOL(WMath::IsEven(a1[i]));

    WUniquePtr<DynamicArrayTestDetail::st> ptr = W_DEFAULT_NEW(DynamicArrayTestDetail::st);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      WDynamicArray<WUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (WUInt32 i = 0; i < 10; ++i)
        a2.InsertAt(0, WUniquePtr<DynamicArrayTestDetail::st>());

      a2.PushBack(std::move(ptr));
      W_TEST_BOOL(ptr == nullptr);
      W_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndSwap(0);
      W_TEST_BOOL(a2[0] != nullptr);
    }

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 10; ++i)
    {
      a1.PushBack(i);
      W_TEST_INT(a1.PeekBack(), i);
    }

    for (WInt32 i = 9; i >= 0; --i)
    {
      W_TEST_INT(a1.PeekBack(), i);
      a1.PopBack();
    }

    a1.PushBack(23);
    a1.PushBack(2);
    a1.PushBack(3);

    a1.PopBack(2);
    W_TEST_INT(a1.PeekBack(), 23);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandAndGetRef")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 20; ++i)
    {
      WInt32& intRef = a1.ExpandAndGetRef();
      intRef = i * 5;
    }


    W_TEST_BOOL(a1.GetCount() == 20);

    for (WInt32 i = 0; i < 20; ++i)
    {
      W_TEST_INT(a1[i], i * 5);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Construction / Destruction")
  {
    {
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      WDynamicArray<DynamicArrayTestDetail::st> a1;
      WDynamicArray<DynamicArrayTestDetail::st> a2;

      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      a1.PushBack(DynamicArrayTestDetail::st(1));
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.InsertAt(0, DynamicArrayTestDetail::st(2));
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 0)); // two copies

      a1.Clear();
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 2));

      a1.PushBack(DynamicArrayTestDetail::st(3));
      a1.PushBack(DynamicArrayTestDetail::st(4));
      a1.PushBack(DynamicArrayTestDetail::st(5));
      a1.PushBack(DynamicArrayTestDetail::st(6));

      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortingPrimitives")
  {
    WDynamicArray<WUInt32> list;

    list.Sort();

    for (WUInt32 i = 0; i < 450; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    for (WUInt32 i = 1; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(list[i - 1] <= list[i]);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortingObjects")
  {
    WDynamicArray<DynamicArrayTestDetail::Dummy> list;
    list.Reserve(128);

    for (WUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    for (WUInt32 i = 1; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(list[i - 1] <= list[i]);
      W_TEST_BOOL(list[i].s == "Test");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortingMovableObjects")
  {
    {
      WDynamicArray<WUniquePtr<DynamicArrayTestDetail::st>> list;
      list.Reserve(128);

      for (WUInt32 i = 0; i < 100; i++)
      {
        list.PushBack(W_DEFAULT_NEW(DynamicArrayTestDetail::st));
      }
      list.Sort();

      for (WUInt32 i = 1; i < list.GetCount(); i++)
      {
        W_TEST_BOOL(list[i - 1] <= list[i]);
      }
    }

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Various")
  {
    WDynamicArray<DynamicArrayTestDetail::Dummy> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.InsertAt(3, 4);
    list.InsertAt(1, 0);
    list.InsertAt(5, 0);

    W_TEST_BOOL(list[0].a == 1);
    W_TEST_BOOL(list[1].a == 0);
    W_TEST_BOOL(list[2].a == 2);
    W_TEST_BOOL(list[3].a == 3);
    W_TEST_BOOL(list[4].a == 4);
    W_TEST_BOOL(list[5].a == 0);
    W_TEST_BOOL(list.GetCount() == 6);

    list.RemoveAtAndCopy(3);
    list.RemoveAtAndSwap(2);

    W_TEST_BOOL(list[0].a == 1);
    W_TEST_BOOL(list[1].a == 0);
    W_TEST_BOOL(list[2].a == 0);
    W_TEST_BOOL(list[3].a == 4);
    W_TEST_BOOL(list.GetCount() == 4);
    W_TEST_BOOL(list.IndexOf(0) == 1);
    W_TEST_BOOL(list.LastIndexOf(0) == 2);

    list.PushBack(5);
    W_TEST_BOOL(list[4].a == 5);
    DynamicArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    W_TEST_BOOL(d.a == 5);
    W_TEST_BOOL(list.GetCount() == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Assignment")
  {
    WDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    WDynamicArray<DynamicArrayTestDetail::Dummy> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    W_TEST_BOOL(list.GetCount() == list2.GetCount());

    list2.Clear();
    W_TEST_BOOL(list2.GetCount() == 0);

    list2 = list;
    W_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    W_TEST_BOOL(list == list2);

    for (int i = 0; i < 16; i++)
    {
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    W_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    W_TEST_BOOL(list == list2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Count")
  {
    WDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(16);

    list.Compact();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Reserve")
  {
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    WDynamicArray<DynamicArrayTestDetail::st> a;

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.Reserve(100);

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(10);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 0));

    a.Reserve(100);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));

    a.SetCount(100);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(90, 0));

    a.Reserve(200);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compact")
  {
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    WDynamicArray<DynamicArrayTestDetail::st> a;

    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(100);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.SetCount(200);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));

    a.SetCount(10);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(190, 0));

    a.SetCount(10);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    a.SetCount(100);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.Clear();
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    // this will deallocate ALL memory
    W_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    a.SetCount(100);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    W_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STL Iterator")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (WInt32 i = 1; i < 1000; ++i)
    {
      W_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    WInt32 prev = 0;
    WInt32 sum1 = 0;
    for (WInt32 val : a1)
    {
      W_TEST_BOOL(prev <= val);
      prev = val;
      sum1 += val;
    }

    prev = 1000;
    const auto endIt = rend(a1);
    WInt32 sum2 = 0;
    for (auto it = rbegin(a1); it != endIt; ++it)
    {
      W_TEST_BOOL(prev > (*it));
      prev = (*it);
      sum2 += (*it);
    }

    W_TEST_BOOL(sum1 == sum2);

    // const array
    const WDynamicArray<WInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    W_TEST_BOOL(*lb == a2[400]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STL Reverse Iterator")
  {
    WDynamicArray<WInt32> a1;

    for (WInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(rbegin(a1), rend(a1));

    for (WInt32 i = 1; i < 1000; ++i)
    {
      W_TEST_BOOL(a1[i - 1] >= a1[i]);
    }

    // foreach
    WUInt32 prev = 1000;
    for (WUInt32 val : a1)
    {
      W_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const WDynamicArray<WInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    W_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetArrayPtr")
  {
    WDynamicArray<WInt32> a1;
    a1.SetCountUninitialized(10);

    W_TEST_BOOL(a1.GetArrayPtr().GetCount() == 10);
    W_TEST_BOOL(a1.GetArrayPtr().GetPtr() == a1.GetData());

    const WDynamicArray<WInt32>& a1ref = a1;

    W_TEST_BOOL(a1ref.GetArrayPtr().GetCount() == 10);
    W_TEST_BOOL(a1ref.GetArrayPtr().GetPtr() == a1ref.GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WDynamicArray<WInt32> a1, a2;

    WInt32 content1[] = {1, 2, 3, 4};
    WInt32 content2[] = {5, 6, 7, 8, 9};

    a1 = WMakeArrayPtr(content1);
    a2 = WMakeArrayPtr(content2);

    WInt32* a1Ptr = a1.GetData();
    WInt32* a2Ptr = a2.GetData();

    a1.Swap(a2);

    // The pointers should be simply swapped
    W_TEST_BOOL(a2Ptr == a1.GetData());
    W_TEST_BOOL(a1Ptr == a2.GetData());

    // The data should be swapped
    W_TEST_BOOL(a1.GetArrayPtr() == WMakeArrayPtr(content2));
    W_TEST_BOOL(a2.GetArrayPtr() == WMakeArrayPtr(content1));
  }

#if W_ENABLED(W_PLATFORM_64BIT)

  // disabled, because this is a very slow test
  W_TEST_BLOCK(WTestBlock::DisabledNoWarning, "Large Allocation")
  {
    const WUInt32 uiMaxNumElements = 0xFFFFFFFF - 16; // max supported elements due to alignment restrictions

    // this will allocate about 16 GB memory, the pure allocation is really fast
    WDynamicArray<WUInt32> byteArray;
    byteArray.SetCountUninitialized(uiMaxNumElements);

    const WUInt32 uiCheckElements = byteArray.GetCount();
    const WUInt32 uiSkipElements = 1024;

    // this will touch the memory and thus enforce that it is indeed made available by the OS
    // this takes a while
    for (WUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const WUInt32 idx = i & 0xFFFFFFFF;
      byteArray[idx] = idx;
    }

    // check that the assigned values are all correct
    // again, this takes quite a while
    for (WUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const WUInt32 idx = i & 0xFFFFFFFF;
      W_TEST_INT(byteArray[idx], idx);
    }
  }
#endif


  const WUInt32 uiNumSortItems = 1'000'000;

  struct Item
  {
    bool operator<(const Item& rhs) const { return m_iKey < rhs.m_iKey; }

    WInt32 m_iKey = 0;
    WInt32 m_iIndex = 0;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "SortLargeArray (W-sort)")
  {
    WDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (WUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = std::rand();
    }

    WStopwatch sw;
    list.Sort();

    WTime t = sw.GetRunningTotal();
    WStringBuilder s;
    s.SetFormat("W-sort (random keys): {}", t);
    WTestFramework::Output(WTestOutput::Details, s);

    for (WUInt32 i = 1; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortLargeArray (std::sort)")
  {
    WDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (WUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = std::rand();
    }

    WStopwatch sw;
    std::sort(begin(list), end(list));

    WTime t = sw.GetRunningTotal();
    WStringBuilder s;
    s.SetFormat("std::sort (random keys): {}", t);
    WTestFramework::Output(WTestOutput::Details, s);

    for (WUInt32 i = 1; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortLargeArray (equal keys) (W-sort)")
  {
    WDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (WUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = 42;
    }

    WStopwatch sw;
    list.Sort();

    WTime t = sw.GetRunningTotal();
    WStringBuilder s;
    s.SetFormat("W-sort (equal keys): {}", t);
    WTestFramework::Output(WTestOutput::Details, s);

    for (WUInt32 i = 1; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortLargeArray (equal keys) (std::sort)")
  {
    WDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (WUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = 42;
    }

    WStopwatch sw;
    std::sort(begin(list), end(list));

    WTime t = sw.GetRunningTotal();
    WStringBuilder s;
    s.SetFormat("std::sort (equal keys): {}", t);
    WTestFramework::Output(WTestOutput::Details, s);

    for (WUInt32 i = 1; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetCountUninitialized")
  {
    struct POD
    {
      W_DECLARE_POD_TYPE();

      WUInt32 a = 2;
      WUInt32 b = 4;

      POD()
      {
        iCallPodConstructor++;
      }

      // this isn't allowed anymore in types that use W_DECLARE_POD_TYPE
      // unfortunately that means we can't do this kind of check either
      //~POD()
      //{
      //  iCallPodDestructor++;
      //}
    };

    static_assert(std::is_trivial<POD>::value == 0);
    static_assert(WIsPodType<POD>::value == 1);

    struct NonPOD
    {
      WUInt32 a = 3;
      WUInt32 b = 5;

      NonPOD()
      {
        iCallNonPodConstructor++;
      }

      ~NonPOD()
      {
        iCallNonPodDestructor++;
      }
    };

    static_assert(std::is_trivial<NonPOD>::value == 0);
    static_assert(WIsPodType<NonPOD>::value == 0);

    // check that SetCountUninitialized doesn't construct and Clear doesn't destruct POD types
    {
      WDynamicArray<POD> s1a;

      s1a.SetCountUninitialized(16);
      W_TEST_INT(iCallPodConstructor, 0);
      W_TEST_INT(iCallPodDestructor, 0);

      s1a.Clear();
      W_TEST_INT(iCallPodConstructor, 0);
      W_TEST_INT(iCallPodDestructor, 0);
    }

    // check that SetCount constructs and Clear destructs Non-POD types
    {
      WDynamicArray<NonPOD> s2a;

      s2a.SetCount(16);
      W_TEST_INT(iCallNonPodConstructor, 16);
      W_TEST_INT(iCallNonPodDestructor, 0);

      s2a.Clear();
      W_TEST_INT(iCallNonPodConstructor, 16);
      W_TEST_INT(iCallNonPodDestructor, 16);
    }
  }
}
