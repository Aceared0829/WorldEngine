#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace SmallArrayTestDetail
{

  class Dummy
  {
  public:
    int a;
    std::string s;

    Dummy()
      : a(0)
      , s("Test")
    {
    }
    Dummy(int a)
      : a(a)
      , s("Test")
    {
    }
    Dummy(const Dummy& other)

      = default;
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
  static WSmallArray<T, 16> CreateArray(WUInt32 uiSize, WUInt32 uiOffset, WUInt32 uiUserData)
  {
    WSmallArray<T, 16> a;
    a.SetCount(static_cast<WUInt16>(uiSize));

    for (WUInt32 i = 0; i < uiSize; ++i)
    {
      a[i] = T(uiOffset + i);
    }

    a.template GetUserData<WUInt32>() = uiUserData;

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
} // namespace SmallArrayTestDetail

static void TakesDynamicArray(WDynamicArray<int>& ref_ar, int iNum, int iStart);

#if W_ENABLED(W_PLATFORM_64BIT)
static_assert(sizeof(WSmallArray<WInt32, 1>) == 16);
#else
static_assert(sizeof(WSmallArray<WInt32, 1>) == 12);
#endif

static_assert(WGetTypeClass<WSmallArray<WInt32, 1>>::value == WTypeIsMemRelocatable::value);
static_assert(WGetTypeClass<WSmallArray<SmallArrayTestDetail::NonMovableClass, 1>>::value == WTypeIsClass::value);

W_CREATE_SIMPLE_TEST(Containers, SmallArray)
{
  WConstructionCounter::Reset();

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WSmallArray<WInt32, 16> a1;
    WSmallArray<WConstructionCounter, 16> a2;

    W_TEST_BOOL(a1.GetCount() == 0);
    W_TEST_BOOL(a2.GetCount() == 0);
    W_TEST_BOOL(a1.IsEmpty());
    W_TEST_BOOL(a2.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor")
  {
    WSmallArray<WInt32, 16> a1;

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

    a1.GetUserData<WUInt32>() = 11;

    WSmallArray<WInt32, 16> a2 = a1;
    WSmallArray<WInt32, 16> a3(a1);

    W_TEST_BOOL(a1 == a2);
    W_TEST_BOOL(a1 == a3);
    W_TEST_BOOL(a2 == a3);

    W_TEST_INT(a2.GetUserData<WUInt32>(), 11);
    W_TEST_INT(a3.GetUserData<WUInt32>(), 11);

    WInt32 test[] = {1, 2, 3, 4};
    WArrayPtr<WInt32> aptr(test);

    WSmallArray<WInt32, 16> a4(aptr);

    W_TEST_BOOL(a4 == aptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move Constructor / Operator")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      // move constructor external storage
      WSmallArray<WConstructionCounter, 16> a1(SmallArrayTestDetail::CreateArray<WConstructionCounter>(100, 20, 11));

      W_TEST_INT(a1.GetCount(), 100);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 20 + i);

      W_TEST_INT(a1.GetUserData<WUInt32>(), 11);

      // move operator external storage
      a1 = SmallArrayTestDetail::CreateArray<WConstructionCounter>(200, 50, 22);

      W_TEST_INT(a1.GetCount(), 200);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 50 + i);

      W_TEST_INT(a1.GetUserData<WUInt32>(), 22);
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    WConstructionCounter::Reset();

    {
      // move constructor internal storage
      WSmallArray<WConstructionCounter, 16> a2(SmallArrayTestDetail::CreateArray<WConstructionCounter>(10, 30, 11));

      W_TEST_INT(a2.GetCount(), 10);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 30 + i);

      W_TEST_INT(a2.GetUserData<WUInt32>(), 11);

      // move operator internal storage
      a2 = SmallArrayTestDetail::CreateArray<WConstructionCounter>(8, 70, 22);

      W_TEST_INT(a2.GetCount(), 8);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 70 + i);

      W_TEST_INT(a2.GetUserData<WUInt32>(), 22);
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    WConstructionCounter::Reset();

    WConstructionCounterRelocatable::Reset();
    {
      // move constructor external storage relocatable
      WSmallArray<WConstructionCounterRelocatable, 16> a1(SmallArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(100, 20, 11));

      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(100, 0));

      W_TEST_INT(a1.GetCount(), 100);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 20 + i);

      W_TEST_INT(a1.GetUserData<WUInt32>(), 11);

      // move operator external storage
      a1 = SmallArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(200, 50, 22);
      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(200, 100));

      W_TEST_INT(a1.GetCount(), 200);
      for (WUInt32 i = 0; i < a1.GetCount(); ++i)
        W_TEST_INT(a1[i].m_iData, 50 + i);

      W_TEST_INT(a1.GetUserData<WUInt32>(), 22);
    }

    W_TEST_BOOL(WConstructionCounterRelocatable::HasAllDestructed());
    WConstructionCounterRelocatable::Reset();

    {
      // move constructor internal storage relocatable
      WSmallArray<WConstructionCounterRelocatable, 16> a2(SmallArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(10, 30, 11));
      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(10, 0));

      W_TEST_INT(a2.GetCount(), 10);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 30 + i);

      W_TEST_INT(a2.GetUserData<WUInt32>(), 11);

      // move operator internal storage
      a2 = SmallArrayTestDetail::CreateArray<WConstructionCounterRelocatable>(8, 70, 22);
      W_TEST_BOOL(WConstructionCounterRelocatable::HasDone(8, 10));

      W_TEST_INT(a2.GetCount(), 8);
      for (WUInt32 i = 0; i < a2.GetCount(); ++i)
        W_TEST_INT(a2[i].m_iData, 70 + i);

      W_TEST_INT(a2.GetUserData<WUInt32>(), 22);
    }

    W_TEST_BOOL(WConstructionCounterRelocatable::HasAllDestructed());
    WConstructionCounterRelocatable::Reset();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Convert to ArrayPtr")
  {
    WSmallArray<WInt32, 16> a1;

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
    WSmallArray<WInt32, 16> a1, a2;

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
    WSmallArray<WInt32, 16> a1, a2;

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
    WSmallArray<WInt32, 16> a1;
    a1.SetCountUninitialized(100);

    for (WInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(a1[i], i);

    const WSmallArray<WInt32, 16> ca1 = a1;

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(ca1[i], i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    WSmallArray<WInt32, 16> a1;

    W_TEST_BOOL(a1.IsEmpty());

    for (WInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(static_cast<WUInt16>(i + 1));
      W_TEST_INT(a1[i], 0);
      a1[i] = i;

      W_TEST_INT(a1.GetCount(), i + 1);
      W_TEST_BOOL(!a1.IsEmpty());
    }

    for (WInt32 i = 0; i < 128; ++i)
      W_TEST_INT(a1[i], i);

    for (WInt32 i = 128; i >= 0; --i)
    {
      a1.SetCount(static_cast<WUInt16>(i));

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
      WSmallArray<WInt32, 2> a2;
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
    WSmallArray<WInt32, 2> a2;
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
    WSmallArray<WInt32, 16> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    W_TEST_BOOL(a1.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    WSmallArray<WInt32, 16> a1;

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

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert")
  {
    WSmallArray<WInt32, 16> a1;

    // always inserts at the front
    for (WInt32 i = 0; i < 100; ++i)
      a1.InsertAt(0, i);

    for (WInt32 i = 0; i < 100; ++i)
      W_TEST_INT(a1[i], 99 - i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveAndCopy")
  {
    WSmallArray<WInt32, 16> a1;

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
    WSmallArray<WInt32, 16> a1;

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
    WSmallArray<WInt32, 16> a1;

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
    WSmallArray<WInt32, 16> a1;

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
    WSmallArray<WInt32, 16> a1;

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
    WSmallArray<WInt32, 16> a1;

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

      WSmallArray<WConstructionCounter, 16> a1;
      WSmallArray<WConstructionCounter, 16> a2;

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
    WSmallArray<WInt32, 16> a;

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

    // this tests whether the static array is reused properly
    a.SetCount(15);
    a.Compact();
    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (WInt32 i = 0; i < 15; ++i)
      W_TEST_INT(a[i], i);

    a.Clear();
    a.Compact();
    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SortingPrimitives")
  {
    WSmallArray<WUInt32, 16> list;

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
    WSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    list.Reserve(128);

    for (WUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    SmallArrayTestDetail::Dummy last = 0;
    for (WUInt32 i = 0; i < list.GetCount(); i++)
    {
      W_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Various")
  {
    WSmallArray<SmallArrayTestDetail::Dummy, 16> list;
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
    SmallArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    W_TEST_BOOL(d.a == 5);
    W_TEST_BOOL(list.GetCount() == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Assignment")
  {
    WSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.GetUserData<WUInt32>() = 11;

    WSmallArray<SmallArrayTestDetail::Dummy, 16> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list2.GetUserData<WUInt32>() = 22;

    list = list2;
    W_TEST_INT(list.GetCount(), list2.GetCount());
    W_TEST_INT(list.GetUserData<WUInt32>(), list2.GetUserData<WUInt32>());

    list2.Clear();
    W_TEST_BOOL(list2.GetCount() == 0);

    list2 = list;
    W_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    W_TEST_BOOL(list == list2);

    for (int i = 0; i < 16; i++)
    {
      list2.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    W_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    W_TEST_BOOL(list == list2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Count")
  {
    WSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Reserve")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    WSmallArray<WConstructionCounter, 16> a;

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

    WSmallArray<WConstructionCounter, 16> a;

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

    a.SetCount(10);
    W_TEST_BOOL(WConstructionCounter::HasDone(10, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    W_TEST_BOOL(WConstructionCounter::HasDone(200, 10));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STL Iterator")
  {
    WSmallArray<WInt32, 16> a1;

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
    const WSmallArray<WInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    W_TEST_BOOL(*lb == a2[400]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "STL Reverse Iterator")
  {
    WSmallArray<WInt32, 16> a1;

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
    const WSmallArray<WInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    W_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move")
  {
    int counter = 0;
    {
      WSmallArray<SmallArrayTestDetail::ExternalCounter, 2> a, b;
      W_TEST_BOOL(counter == 0);

      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      W_TEST_BOOL(counter == 1);

      b = std::move(a);
      W_TEST_BOOL(counter == 1);
    }
    W_TEST_BOOL(counter == 2);

    counter = 0;
    {
      WSmallArray<SmallArrayTestDetail::ExternalCounter, 2> a, b;
      W_TEST_BOOL(counter == 0);

      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      W_TEST_BOOL(counter == 4);

      b = std::move(a);
      W_TEST_BOOL(counter == 4);
    }
    W_TEST_BOOL(counter == 8);
  }
}
