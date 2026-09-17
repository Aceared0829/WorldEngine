#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace HybridArrayTestDetail
{

  class Dummy
  {
  public:
    int a = 0;
    std::string s = "Test";

    Dummy() = default;

    Dummy(int a)
      : a(a)
    {
    }

    Dummy(const Dummy& other) = default;
    ~Dummy() = default;

    Dummy& operator=(const Dummy& other) = default;

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };

  class NonMovableClass
  {
  public:
    NonMovableClass(int iVal)
    {
      m_val = iVal;
      m_pVal = &m_val;
    }

    NonMovableClass(const NonMovableClass& other)
    {
      m_val = other.m_val;
      m_pVal = &m_val;
    }

    void operator=(const NonMovableClass& other) { m_val = other.m_val; }

    int m_val = 0;
    int* m_pVal = nullptr;
  };

  template <typename T>
  static WHybridArray<T, 16> CreateArray(WUInt32 uiSize, WUInt32 uiOffset)
  {
    WHybridArray<T, 16> a;
    a.SetCount(uiSize);

    for (WUInt32 i = 0; i < uiSize; ++i)
      a[i] = T(uiOffset + i);

    return a;
  }

  struct ExternalCounter
  {
    W_DECLARE_MEM_RELOCATABLE_TYPE();

    ExternalCounter() = default;

    ExternalCounter(int& ref_iCounter)
      : m_counter{&ref_iCounter}
    {
    }

    ~ExternalCounter()
    {
      if (m_counter)
        (*m_counter)++;
    }

    int* m_counter{};
  };
} // namespace HybridArrayTestDetail

static void TakesDynamicArray(WDynamicArray<int>& ref_ar, int iNum, int iStart);

#if W_ENABLED(W_PLATFORM_64BIT)
static_assert(sizeof(WHybridArray<WInt32, 1>) == 32);
#else
static_assert(sizeof(WHybridArray<WInt32, 1>) == 20);
#endif

static_assert(WGetTypeClass<WHybridArray<WInt32, 1>>::value == WTypeIsClass::value);
static_assert(WGetTypeClass<WHybridArray<HybridArrayTestDetail::NonMovableClass, 1>>::value == WTypeIsClass::value);

W_CREATE_SIMPLE_TEST(Containers, HybridArray)
{
  WConstructionCounter::Reset();

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WHybridArray<WInt32, 16> a1;
    WHybridArray<WConstructionCounter, 16> a2;

    W_TEST_BOOL(a1.GetCount() == 0);
    W_TEST_BOOL(a2.GetCount() == 0);
    W_TEST_BOOL(a1.IsEmpty());
    W_TEST_BOOL(a2.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor")
  {
    WHybridArray<WInt32, 16> a1;

    W_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (WInt32 i = 0; i < 32; ++i)
    {
      a1.PushBack(rand() % 100000);

      if (i < 16)
      {
        W_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);
      }
      else
      {
        W_TEST_BOOL(a1.GetHeapMemoryUsage() >= i * sizeof(WInt32));
      }
    }

    WHybridArray<WInt32, 16> a2 = a1;
    WHybridArray<WInt32, 16> a3(a1);

    W_TEST_BOOL(a1 == a2);
    W_TEST_BOOL(a1 == a3);
    W_TEST_BOOL(a2 == a3);

    WInt32 test[] = {1, 2, 3, 4};
    WArrayPtr<WInt32> aptr(test);

    WHybridArray<WInt32, 16> a4(aptr);

    W_TEST_BOOL(a4 == aptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move Constructor / Operator")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      // move constructor external storage
      WHybridArray<WConstructionCounter, 16> a1(HybridArrayTestDetail::CreateArray<WConstructionCounter>(100, 20));

      W_TEST_INT(a1.GetCount(), 100);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator external storage
      a1 = HybridArrayTestDetail::CreateArray<WConstructionCounter>(200, 50);

      W_TEST_INT(a1.GetCount(), 200);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 50 + i);
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    WConstructionCounter::Reset();

    {
      // move constructor internal storage
      WHybridArray<WConstructionCounter, 16> a2(HybridArrayTestDetail::CreateArray<WConstructionCounter>(10, 30));

      W_TEST_INT(a2.GetCount(), 10);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 30 + i);

      // move operator internal storage
      a2 = HybridArrayTestDetail::CreateArray<WConstructionCounter>(8, 70);

      W_TEST_INT(a2.GetCount(), 8);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 70 + i);
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    WConstructionCounter::Reset();

    WConstructionCounterRelocatable::Reset();
    {
      // move constructor external storage relocatable
      WHybridArray<WConstructionCounterRelocatable, 16> a1(HybridArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(100, 20));

      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(100, 0));

      W_TEST_INT(a1.GetCount(), 100);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator external storage
      a1 = HybridArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(200, 50);
      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(200, 100));

      W_TEST_INT(a1.GetCount(), 200);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 50 + i);
    }

    W_TEST_BOOL(WConstructionCounterRelocatable::HasAllDestructed());
    WConstructionCounterRelocatable::Reset();

    {
      // move constructor internal storage relocatable
      WHybridArray<WConstructionCounterRelocatable, 16> a2(HybridArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(10, 30));
      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(10, 0));

      W_TEST_INT(a2.GetCount(), 10);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 30 + i);

      // move operator internal storage
      a2 = HybridArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(8, 70);
      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(8, 10));

      W_TEST_INT(a2.GetCount(), 8);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 70 + i);
    }

    W_TEST_BOOL(WConstructionCounterRelocatable::HasAllDestructed());
    WConstructionCounterRelocatable::Reset();

    {
      // move constructor with different allocators
      WProxyAllocator proxyAllocator("test allocator", WFoundation::GetDefaultAllocator());
      {
        WHybridArray<WConstructionCounterRelocatable, 16> a1(&proxyAllocator);

        a1 = HybridArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(8, 70);
        W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(8, 0));
        W_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        W_TEST_INT(a1.GetCount(), 8);
        for (WUInt32 i = 0; i < a1.GetCount(); ++i)
          W_TEST_INT(a1[i].m_iData, 70 + i);

        a1 = HybridArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(32, 100);
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
    WHybridArray<WInt32, 16> a1;

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
    WHybridArray<WInt32, 16> a1, a2;

    for (WInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    W_TEST_BOOL(a1 == a2);

    WArrayPtr<WInt32> arrayPtr(a1);

    a2 = arrayPtr;

    W_TEST_BOOL(a2 == arrayPtr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator == / !=")
  {
    WHybridArray<WInt32, 16> a1, a2;

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
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Index operator")
  {
    WHybridArray<WInt32, 16> a1;
    a1.SetCountUninitialized(100);

    for (WInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(a1[i], i);

    const WHybridArray<WInt32, 16> ca1 = a1;

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(ca1[i], i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    WHybridArray<WInt32, 16> a1;

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
      WHybridArray<WInt32, 2> a2;
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
    WHybridArray<WInt32, 2> a2;
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

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    WHybridArray<WInt32, 16> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    W_TEST_BOOL(a1.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    WHybridArray<WInt32, 16> a1;

    for (WInt32 i = -100; i < 100; ++i)
      W_TEST_BOOL(!a1.Contains(i));

    for (WInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    for (WInt32 i = 0; i < 100; ++i)
    {
      W_TEST_BOOL(a1.Contains(i));
      W_TEST_INT(a1.IndexOf(i), i);
      W_TEST_INT(a1.LastIndexOf(i), i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "InsertAt")
  {
    WHybridArray<WInt32, 16> a1;

    // always inserts at the front
    for (WInt32 i = 0; i < 100; ++i)
      a1.InsertAt(0, i);

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(a1[i], 99 - i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAndCopy")
  {
    WHybridArray<WInt32, 16> a1;

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
    WHybridArray<WInt32, 16> a1;

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
    WHybridArray<WInt32, 16> a1;

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
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAtAndSwap")
  {
    WHybridArray<WInt32, 16> a1;

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
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    WHybridArray<WInt32, 16> a1;

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
    WHybridArray<WInt32, 16> a1;

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
      W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

      WHybridArray<WConstructionCounter, 16> a1;
      WHybridArray<WConstructionCounter, 16> a2;

      W_TEST_BOOL(WConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
      W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

      a1.PushBack(WConstructionCounter(1));
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.InsertAt(0, WConstructionCounter(2));
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 0)); // two copies

      a1.Clear();
      W_TEST_BOOL(WConstructionCounter::HasDone(0, 2));

      a1.PushBack(WConstructionCounter(3));
      a1.PushBack(WConstructionCounter(4));
      a1.PushBack(WConstructionCounter(5));
      a1.PushBack(WConstructionCounter(6));

      W_TEST_BOOL(WConstructionCounter::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(WConstructionCounter(3));
      W_TEST_BOOL(WConstructionCounter::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(WConstructionCounter(3));
      W_TEST_BOOL(WConstructionCounter::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      W_TEST_BOOL(WConstructionCounter::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      W_TEST_BOOL(WConstructionCounter::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compact")
  {
    WHybridArray<WInt32, 16> a;

    for (WInt32 i = 0; i < 1008; ++i)
    {
      a.PushBack(i);
      W_TEST_INT(a.GetCount(), i + 1);
    }

    W_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    W_TEST_BOOL(a.GetHeapMemoryUsage() > 0);

    for (WInt32 i = 0; i < 1008; ++i)
      W_TEST_INT(a[i], i);

    // this tests whether the static array is reused properly (not the case anymore with new implementation that derives from WDynamicArray)
    a.SetCount(15);
    a.Compact();
    // W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
    W_TEST_BOOL(a.GetHeapMemoryUsage() > 0);

    for (WInt32 i = 0; i < 15; ++i)
      W_TEST_INT(a[i], i);

    a.Clear();
    a.Compact();
    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortingPrimitives")
  {
    WHybridArray<WUInt32, 16> list;

    list.Sort();

    for (WUInt32 i = 0; i < 45; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    WUInt32 last = 0;
    for (WUInt32 i = 0; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortingObjects")
  {
    WHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    list.Reserve(128);

    for (WUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    HybridArrayTestDetail::Dummy last = 0;
    for (WUInt32 i = 0; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Various")
  {
    WHybridArray<HybridArrayTestDetail::Dummy, 16> list;
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
    HybridArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    W_TEST_BOOL(d.a == 5);
    W_TEST_BOOL(list.GetCount() == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Assignment")
  {
    WHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }

    WHybridArray<HybridArrayTestDetail::Dummy, 16> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(HybridArrayTestDetail::Dummy(rand()));
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
      list2.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    W_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    W_TEST_BOOL(list == list2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Count")
  {
    WHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Reserve")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    WHybridArray<WConstructionCounter, 16> a;

    W_TEST_BOOL(WConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    a.Reserve(100);

    W_TEST_BOOL(WConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    a.SetCount(10);
    W_TEST_BOOL(WConstructionCounter::HasDone(10, 0));

    a.Reserve(100);
    W_TEST_BOOL(WConstructionCounter::HasDone(0, 0));

    a.SetCount(100);
    W_TEST_BOOL(WConstructionCounter::HasDone(90, 0));

    a.Reserve(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(100, 0));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compact")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    WHybridArray<WConstructionCounter, 16> a;

    W_TEST_BOOL(WConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    a.SetCount(100);
    W_TEST_BOOL(WConstructionCounter::HasDone(100, 0));

    a.SetCount(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(200, 100));

    a.SetCount(10);
    W_TEST_BOOL(WConstructionCounter::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(190, 0));

    a.SetCount(10);
    W_TEST_BOOL(WConstructionCounter::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    W_TEST_BOOL(WConstructionCounter::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    W_TEST_BOOL(WConstructionCounter::HasDone(0, 200));

    a.SetCount(100);
    W_TEST_BOOL(WConstructionCounter::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(100, 0));

    a.Clear();
    W_TEST_BOOL(WConstructionCounter::HasDone(0, 200));

    // this will deallocate ALL memory
    a.Compact();

    a.SetCount(100);
    W_TEST_BOOL(WConstructionCounter::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(200, 100));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STL Iterator")
  {
    WHybridArray<WInt32, 16> a1;

    for (WInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (WInt32 i = 1; i < 1000; ++i)
    {
      W_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    WUInt32 prev = 0;
    for (WUInt32 val : a1)
    {
      W_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const WHybridArray<WInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    W_TEST_BOOL(*lb == a2[400]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STL Reverse Iterator")
  {
    WHybridArray<WInt32, 16> a1;

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
    const WHybridArray<WInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    W_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {

    WInt32 content1[] = {1, 2, 3, 4};
    WInt32 content2[] = {5, 6, 7, 8, 9};
    WInt32 contentHeap1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    WInt32 contentHeap2[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 110, 111, 112, 113};

    {
      // local <-> local
      WHybridArray<WInt32, 8> a1;
      WHybridArray<WInt32, 16> a2;
      a1 = WMakeArrayPtr(content1);
      a2 = WMakeArrayPtr(content2);

      WInt32* a1Ptr = a1.GetData();
      WInt32* a2Ptr = a2.GetData();

      a1.Swap(a2);

      // Because the data points to the internal storage the pointers shouldn't change when swapping
      W_TEST_BOOL(a1Ptr == a1.GetData());
      W_TEST_BOOL(a2Ptr == a2.GetData());

      // The data however should be swapped
      W_TEST_BOOL(a1.GetArrayPtr() == WMakeArrayPtr(content2));
      W_TEST_BOOL(a2.GetArrayPtr() == WMakeArrayPtr(content1));

      W_TEST_INT(a1.GetCapacity(), 8);
      W_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // local <-> heap
      WHybridArray<WInt32, 8> a1;
      WDynamicArray<WInt32> a2;
      a1 = WMakeArrayPtr(content1);
      a2 = WMakeArrayPtr(contentHeap1);
      WInt32* a1Ptr = a1.GetData();
      WInt32* a2Ptr = a2.GetData();
      a1.Swap(a2);
      W_TEST_BOOL(a1Ptr != a1.GetData());
      W_TEST_BOOL(a2Ptr != a2.GetData());
      W_TEST_BOOL(a1.GetArrayPtr() == WMakeArrayPtr(contentHeap1));
      W_TEST_BOOL(a2.GetArrayPtr() == WMakeArrayPtr(content1));

      W_TEST_INT(a1.GetCapacity(), 16);
      W_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // heap <-> local
      WHybridArray<WInt32, 8> a1;
      WHybridArray<WInt32, 7> a2;
      a1 = WMakeArrayPtr(content1);
      a2 = WMakeArrayPtr(contentHeap1);
      WInt32* a1Ptr = a1.GetData();
      WInt32* a2Ptr = a2.GetData();
      a2.Swap(a1); // Swap is opposite direction as before
      W_TEST_BOOL(a1Ptr != a1.GetData());
      W_TEST_BOOL(a2Ptr != a2.GetData());
      W_TEST_BOOL(a1.GetArrayPtr() == WMakeArrayPtr(contentHeap1));
      W_TEST_BOOL(a2.GetArrayPtr() == WMakeArrayPtr(content1));

      W_TEST_INT(a1.GetCapacity(), 16);
      W_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // heap <-> heap
      WDynamicArray<WInt32> a1;
      WHybridArray<WInt32, 8> a2;
      a1 = WMakeArrayPtr(contentHeap1);
      a2 = WMakeArrayPtr(contentHeap2);
      WInt32* a1Ptr = a1.GetData();
      WInt32* a2Ptr = a2.GetData();
      a2.Swap(a1);
      W_TEST_BOOL(a1Ptr != a1.GetData());
      W_TEST_BOOL(a2Ptr != a2.GetData());
      W_TEST_BOOL(a1.GetArrayPtr() == WMakeArrayPtr(contentHeap2));
      W_TEST_BOOL(a2.GetArrayPtr() == WMakeArrayPtr(contentHeap1));

      W_TEST_INT(a1.GetCapacity(), 16);
      W_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // empty <-> local
      WHybridArray<WInt32, 8> a1, a2;
      a2 = WMakeArrayPtr(content2);
      a1.Swap(a2);
      W_TEST_BOOL(a1.GetArrayPtr() == WMakeArrayPtr(content2));
      W_TEST_BOOL(a2.IsEmpty());

      W_TEST_INT(a1.GetCapacity(), 8);
      W_TEST_INT(a2.GetCapacity(), 8);
    }

    {
      // empty <-> empty
      WHybridArray<WInt32, 8> a1, a2;
      a1.Swap(a2);
      W_TEST_BOOL(a1.IsEmpty());
      W_TEST_BOOL(a2.IsEmpty());

      W_TEST_INT(a1.GetCapacity(), 8);
      W_TEST_INT(a2.GetCapacity(), 8);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move")
  {
    int counter = 0;
    {
      WHybridArray<HybridArrayTestDetail::ExternalCounter, 2> a, b;
      W_TEST_BOOL(counter == 0);

      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      W_TEST_BOOL(counter == 1);

      b = std::move(a);
      W_TEST_BOOL(counter == 1);
    }
    W_TEST_BOOL(counter == 2);

    counter = 0;
    {
      WHybridArray<HybridArrayTestDetail::ExternalCounter, 2> a, b;
      W_TEST_BOOL(counter == 0);

      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      W_TEST_BOOL(counter == 4);

      b = std::move(a);
      W_TEST_BOOL(counter == 4);
    }
    W_TEST_BOOL(counter == 8);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Use WHybridArray with WDynamicArray")
  {
    WHybridArray<int, 16> a;

    TakesDynamicArray(a, 4, a.GetCount());
    W_TEST_INT(a.GetCount(), 4);
    W_TEST_INT(a.GetCapacity(), 16);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      W_TEST_INT(a[i], i);
    }

    TakesDynamicArray(a, 12, a.GetCount());
    W_TEST_INT(a.GetCount(), 16);
    W_TEST_INT(a.GetCapacity(), 16);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      W_TEST_INT(a[i], i);
    }

    TakesDynamicArray(a, 8, a.GetCount());
    W_TEST_INT(a.GetCount(), 24);
    W_TEST_INT(a.GetCapacity(), 32);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      W_TEST_INT(a[i], i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Nested arrays")
  {
    WDynamicArray<WHybridArray<HybridArrayTestDetail::NonMovableClass, 4>> a;

    for (int i = 0; i < 100; ++i)
    {
      WHybridArray<HybridArrayTestDetail::NonMovableClass, 4> b;
      b.PushBack(HybridArrayTestDetail::NonMovableClass(i));

      a.PushBack(std::move(b));
    }

    for (int i = 0; i < 100; ++i)
    {
      auto& nonMoveable = a[i][0];

      W_TEST_INT(nonMoveable.m_val, i);
      W_TEST_BOOL(nonMoveable.m_pVal == &nonMoveable.m_val);
    }
  }
}

void TakesDynamicArray(WDynamicArray<int>& ref_ar, int iNum, int iStart)
{
  for (int i = 0; i < iNum; ++i)
  {
    ref_ar.PushBack(iStart + i);
  }
}
