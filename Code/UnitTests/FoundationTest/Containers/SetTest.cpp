#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Set.h>
#include <Foundation/Memory/CommonAllocators.h>

W_CREATE_SIMPLE_TEST(Containers, Set)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WSet<WUInt32> m;
    WSet<WConstructionCounter, WUInt32> m2;
    WSet<WConstructionCounter, WConstructionCounter> m3;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEmpty")
  {
    WSet<WUInt32> m;
    W_TEST_BOOL(m.IsEmpty());

    m.Insert(1);
    W_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    W_TEST_BOOL(m.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCount")
  {
    WSet<WUInt32> m;
    W_TEST_INT(m.GetCount(), 0);

    m.Insert(0);
    W_TEST_INT(m.GetCount(), 1);

    m.Insert(1);
    W_TEST_INT(m.GetCount(), 2);

    m.Insert(2);
    W_TEST_INT(m.GetCount(), 3);

    m.Insert(1);
    W_TEST_INT(m.GetCount(), 3);

    m.Clear();
    W_TEST_INT(m.GetCount(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      WSet<WConstructionCounter> m1;
      m1.Insert(WConstructionCounter(1));
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1));

      m1.Insert(WConstructionCounter(3));
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1));

      m1.Insert(WConstructionCounter(1));
      W_TEST_BOOL(WConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      W_TEST_BOOL(WConstructionCounter::HasDone(0, 2));
      W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    }

    {
      WSet<WConstructionCounter> m1;
      m1.Insert(WConstructionCounter(0));
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(WConstructionCounter(1));
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(WConstructionCounter(0));
      W_TEST_BOOL(WConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      W_TEST_BOOL(WConstructionCounter::HasDone(0, 2));
      W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert")
  {
    WSet<WUInt32> m;
    W_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    W_TEST_BOOL(m.Insert(1).IsValid());
    W_TEST_BOOL(m.Insert(1).IsValid());

    m.Insert(3);
    auto it7 = m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    W_TEST_BOOL(m.Insert(1).Key() == 1);
    W_TEST_BOOL(m.Insert(3).Key() == 3);
    W_TEST_BOOL(m.Insert(7) == it7);

    W_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(WUInt32) * 1 * 9);

    W_TEST_BOOL(m.Find(1).IsValid());
    W_TEST_BOOL(m.Find(2).IsValid());
    W_TEST_BOOL(m.Find(3).IsValid());
    W_TEST_BOOL(m.Find(4).IsValid());
    W_TEST_BOOL(m.Find(5).IsValid());
    W_TEST_BOOL(m.Find(6).IsValid());
    W_TEST_BOOL(m.Find(7).IsValid());
    W_TEST_BOOL(m.Find(8).IsValid());
    W_TEST_BOOL(m.Find(9).IsValid());

    W_TEST_BOOL(!m.Find(0).IsValid());
    W_TEST_BOOL(!m.Find(10).IsValid());

    W_TEST_INT(m.GetCount(), 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains")
  {
    WSet<WUInt32> m;
    m.Insert(1);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    W_TEST_BOOL(m.Contains(1));
    W_TEST_BOOL(m.Contains(2));
    W_TEST_BOOL(m.Contains(3));
    W_TEST_BOOL(m.Contains(4));
    W_TEST_BOOL(m.Contains(5));
    W_TEST_BOOL(m.Contains(6));
    W_TEST_BOOL(m.Contains(7));
    W_TEST_BOOL(m.Contains(8));
    W_TEST_BOOL(m.Contains(9));

    W_TEST_BOOL(!m.Contains(0));
    W_TEST_BOOL(!m.Contains(10));

    W_TEST_INT(m.GetCount(), 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Set Operations")
  {
    WSet<WUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    WSet<WUInt32> empty;

    WSet<WUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    WSet<WUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    WSet<WUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    WSet<WUInt32> nonDisjunctNonEmptySubSet;
    nonDisjunctNonEmptySubSet.Insert(1);
    nonDisjunctNonEmptySubSet.Insert(4);
    nonDisjunctNonEmptySubSet.Insert(5);

    // ContainsSet
    W_TEST_BOOL(base.ContainsSet(base));

    W_TEST_BOOL(base.ContainsSet(empty));
    W_TEST_BOOL(!empty.ContainsSet(base));

    W_TEST_BOOL(!base.ContainsSet(disjunct));
    W_TEST_BOOL(!disjunct.ContainsSet(base));

    W_TEST_BOOL(base.ContainsSet(subSet));
    W_TEST_BOOL(!subSet.ContainsSet(base));

    W_TEST_BOOL(!base.ContainsSet(superSet));
    W_TEST_BOOL(superSet.ContainsSet(base));

    W_TEST_BOOL(!base.ContainsSet(nonDisjunctNonEmptySubSet));
    W_TEST_BOOL(!nonDisjunctNonEmptySubSet.ContainsSet(base));

    // Union
    {
      WSet<WUInt32> res;

      res.Union(base);
      W_TEST_BOOL(res.ContainsSet(base));
      W_TEST_BOOL(base.ContainsSet(res));
      res.Union(subSet);
      W_TEST_BOOL(res.ContainsSet(base));
      W_TEST_BOOL(res.ContainsSet(subSet));
      W_TEST_BOOL(base.ContainsSet(res));
      res.Union(superSet);
      W_TEST_BOOL(res.ContainsSet(base));
      W_TEST_BOOL(res.ContainsSet(subSet));
      W_TEST_BOOL(res.ContainsSet(superSet));
      W_TEST_BOOL(superSet.ContainsSet(res));
    }

    // Difference
    {
      WSet<WUInt32> res;
      res.Union(base);
      res.Difference(empty);
      W_TEST_BOOL(res.ContainsSet(base));
      W_TEST_BOOL(base.ContainsSet(res));
      res.Difference(disjunct);
      W_TEST_BOOL(res.ContainsSet(base));
      W_TEST_BOOL(base.ContainsSet(res));
      res.Difference(subSet);
      W_TEST_INT(res.GetCount(), 1);
      W_TEST_BOOL(res.Contains(3));
    }

    // Intersection
    {
      WSet<WUInt32> res;
      res.Union(base);
      res.Intersection(disjunct);
      W_TEST_BOOL(res.IsEmpty());
      res.Union(base);
      res.Intersection(subSet);
      W_TEST_BOOL(base.ContainsSet(subSet));
      W_TEST_BOOL(res.ContainsSet(subSet));
      W_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(superSet);
      W_TEST_BOOL(superSet.ContainsSet(res));
      W_TEST_BOOL(res.ContainsSet(subSet));
      W_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(empty);
      W_TEST_BOOL(res.IsEmpty());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Find")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (WInt32 i = 1000 - 1; i >= 0; --i)
      W_TEST_INT(m.Find(i).Key(), i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove (non-existing)")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      W_TEST_BOOL(!m.Remove(i));

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (WInt32 i = 0; i < 1000; ++i)
      W_TEST_BOOL(m.Remove(i + 500) == (i < 500));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove (Iterator)")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (WInt32 i = 0; i < 1000 - 1; ++i)
    {
      WSet<WUInt32>::Iterator itNext = m.Remove(m.Find(i));
      W_TEST_BOOL(!m.Find(i).IsValid());
      W_TEST_BOOL(itNext.Key() == i + 1);

      W_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove (Key)")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (WInt32 i = 0; i < 1000; ++i)
    {
      W_TEST_BOOL(m.Remove(i));
      W_TEST_BOOL(!m.Find(i).IsValid());

      W_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=")
  {
    WSet<WUInt32> m, m2;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    m2 = m;

    for (WInt32 i = 1000 - 1; i >= 0; --i)
      W_TEST_BOOL(m2.Find(i).IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    WSet<WUInt32> m2(m);

    for (WInt32 i = 1000 - 1; i >= 0; --i)
      W_TEST_BOOL(m2.Find(i).IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    WInt32 i = 0;
    for (WSet<WUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const WSet<WUInt32> m2(m);

    WInt32 i = 0;
    for (WSet<WUInt32>::Iterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LowerBound")
  {
    WSet<WInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

    W_TEST_INT(m.LowerBound(-1).Key(), 0);
    W_TEST_INT(m.LowerBound(0).Key(), 0);
    W_TEST_INT(m.LowerBound(1).Key(), 3);
    W_TEST_INT(m.LowerBound(2).Key(), 3);
    W_TEST_INT(m.LowerBound(3).Key(), 3);
    W_TEST_INT(m.LowerBound(4).Key(), 7);
    W_TEST_INT(m.LowerBound(5).Key(), 7);
    W_TEST_INT(m.LowerBound(6).Key(), 7);
    W_TEST_INT(m.LowerBound(7).Key(), 7);
    W_TEST_INT(m.LowerBound(8).Key(), 9);
    W_TEST_INT(m.LowerBound(9).Key(), 9);

    W_TEST_BOOL(!m.LowerBound(10).IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UpperBound")
  {
    WSet<WInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

    W_TEST_INT(m.UpperBound(-1).Key(), 0);
    W_TEST_INT(m.UpperBound(0).Key(), 3);
    W_TEST_INT(m.UpperBound(1).Key(), 3);
    W_TEST_INT(m.UpperBound(2).Key(), 3);
    W_TEST_INT(m.UpperBound(3).Key(), 7);
    W_TEST_INT(m.UpperBound(4).Key(), 7);
    W_TEST_INT(m.UpperBound(5).Key(), 7);
    W_TEST_INT(m.UpperBound(6).Key(), 7);
    W_TEST_INT(m.UpperBound(7).Key(), 9);
    W_TEST_INT(m.UpperBound(8).Key(), 9);
    W_TEST_BOOL(!m.UpperBound(9).IsValid());
    W_TEST_BOOL(!m.UpperBound(10).IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert / Remove")
  {
    // Tests whether reusing of elements makes problems

    WSet<WInt32> m;

    for (WUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (WUInt32 i = 0; i < 10000; ++i)
        m.Insert(i);

      W_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (WUInt32 i = 0; i < 5000; ++i)
        W_TEST_BOOL(m.Remove(i));

      // Insert others
      for (WUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j);

      // Remove
      for (WUInt32 i = 0; i < 5000; ++i)
        W_TEST_BOOL(m.Remove(5000 + i));

      // Remove others
      for (WUInt32 j = 1; j < 1000; ++j)
      {
        W_TEST_BOOL(m.Find(20000 * j).IsValid());
        W_TEST_BOOL(m.Remove(20000 * j));
      }
    }

    W_TEST_BOOL(m.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Iterator")
  {
    WSet<WUInt32> m;
    for (WUInt32 i = 0; i < 1000; ++i)
      m.Insert(i + 1);

    W_TEST_INT(std::find(begin(m), end(m), 500).Key(), 500);

    auto itfound = std::find_if(begin(m), end(m), [](WUInt32 uiVal)
      { return uiVal == 500; });

    W_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    WUInt32 prev = *begin(m);
    for (WUInt32 val : m)
    {
      W_TEST_BOOL(val >= prev);
      prev = val;
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator == / !=")
  {
    WSet<WUInt32> m, m2;

    W_TEST_BOOL(m == m2);

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i * 10);

    W_TEST_BOOL(m != m2);

    m2 = m;

    W_TEST_BOOL(m == m2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompatibleKeyType")
  {
    {
      WSet<WString> stringSet;
      const char* szChar = "Char";
      const char* szString = "ViewBla";
      WStringView sView(szString, szString + 4);
      WStringBuilder sBuilder("Builder");
      WString sString("String");
      stringSet.Insert(szChar);
      stringSet.Insert(sView);
      stringSet.Insert(sBuilder);
      stringSet.Insert(sString);

      W_TEST_BOOL(stringSet.Contains(szChar));
      W_TEST_BOOL(stringSet.Contains(sView));
      W_TEST_BOOL(stringSet.Contains(sBuilder));
      W_TEST_BOOL(stringSet.Contains(sString));

      W_TEST_BOOL(stringSet.Remove(szChar));
      W_TEST_BOOL(stringSet.Remove(sView));
      W_TEST_BOOL(stringSet.Remove(sBuilder));
      W_TEST_BOOL(stringSet.Remove(sString));
    }

    // dynamic array as key, check for allocations in comparisons
    {
      WProxyAllocator testAllocator("Test", WFoundation::GetDefaultAllocator());
      WLocalAllocatorWrapper allocWrapper(&testAllocator);
      using TestDynArray = WDynamicArray<int, WLocalAllocatorWrapper>;
      TestDynArray a;
      TestDynArray b;
      for (int i = 0; i < 10; ++i)
      {
        a.PushBack(i);
        b.PushBack(i * 2);
      }

      WSet<TestDynArray> arraySet;
      arraySet.Insert(a);
      arraySet.Insert(b);

      WArrayPtr<const int> aPtr = a.GetArrayPtr();
      WArrayPtr<const int> bPtr = b.GetArrayPtr();

      WUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

      W_TEST_BOOL(arraySet.Contains(aPtr));
      W_TEST_BOOL(arraySet.Contains(bPtr));
      W_TEST_BOOL(arraySet.Contains(a));

      W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      W_TEST_BOOL(arraySet.Remove(aPtr));
      W_TEST_BOOL(arraySet.Remove(bPtr));

      W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
    }
  }

  constexpr WUInt32 uiSetSize = sizeof(WSet<WString>);

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WUInt8 set1Mem[uiSetSize];
    WUInt8 set2Mem[uiSetSize];
    WMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    WMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    WStringBuilder tmp;
    WSet<WString>* set1 = new (set1Mem)(WSet<WString>);
    WSet<WString>* set2 = new (set2Mem)(WSet<WString>);

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1->Insert(tmp);

      tmp.SetFormat("{0}{0}{0}", i);
      set2->Insert(tmp);
    }

    set1->Swap(*set2);

    // test swapped elements
    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      W_TEST_BOOL(set2->Contains(tmp));

      tmp.SetFormat("{0}{0}{0}", i);
      W_TEST_BOOL(set1->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set1)
      {
        W_TEST_BOOL(!set2->Contains(element));
      }

      for (const auto& element : *set2)
      {
        W_TEST_BOOL(!set1->Contains(element));
      }
    }

    // due to a compiler bug in VS 2017, PatternFill cannot be called here, because it will move the memset BEFORE the destructor call!
    // seems to be fixed in VS 2019 though

    set1->~WSet<WString>();
    // WMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    set2->~WSet<WString>();
    WMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap Empty")
  {
    WUInt8 set1Mem[uiSetSize];
    WUInt8 set2Mem[uiSetSize];
    WMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    WMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    WStringBuilder tmp;
    WSet<WString>* set1 = new (set1Mem)(WSet<WString>);
    WSet<WString>* set2 = new (set2Mem)(WSet<WString>);

    for (WUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1->Insert(tmp);
    }

    set1->Swap(*set2);
    W_TEST_BOOL(set1->IsEmpty());

    set1->~WSet<WString>();
    WMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    // test swapped elements
    for (WUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      W_TEST_BOOL(set2->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set2)
      {
        W_TEST_BOOL(set2->Contains(element));
      }
    }

    set2->~WSet<WString>();
    WMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetReverseIterator")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    WInt32 i = 1000 - 1;
    for (WSet<WUInt32>::ReverseIterator it = m.GetReverseIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      --i;
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetReverseIterator (const)")
  {
    WSet<WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const WSet<WUInt32> m2(m);

    WInt32 i = 1000 - 1;
    for (WSet<WUInt32>::ReverseIterator it = m2.GetReverseIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      --i;
    }
  }
}
