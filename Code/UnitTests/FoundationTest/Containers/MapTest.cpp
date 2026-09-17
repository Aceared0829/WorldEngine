#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>
#include <algorithm>
#include <iterator>

W_CREATE_SIMPLE_TEST(Containers, Map)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Iterator")
  {
    WMap<WUInt32, WUInt32> m;
    for (WUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // W_TEST_INT(std::find(begin(m), end(m), 500).Key(), 499);

    auto itfound = std::find_if(begin(m), end(m), [](WMap<WUInt32, WUInt32>::ConstIterator val)
      { return val.Value() == 500; });

    // W_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    WUInt32 prev = begin(m).Key();
    for (auto it : m)
    {
      W_TEST_BOOL(it.Value() >= prev);
      prev = it.Value();
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WMap<WUInt32, WUInt32> m;
    WMap<WConstructionCounter, WUInt32> m2;
    WMap<WConstructionCounter, WConstructionCounter> m3;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEmpty")
  {
    WMap<WUInt32, WUInt32> m;
    W_TEST_BOOL(m.IsEmpty());

    m[1] = 2;
    W_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    W_TEST_BOOL(m.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCount")
  {
    WMap<WUInt32, WUInt32> m;
    W_TEST_INT(m.GetCount(), 0);

    m[0] = 1;
    W_TEST_INT(m.GetCount(), 1);

    m[1] = 2;
    W_TEST_INT(m.GetCount(), 2);

    m[2] = 3;
    W_TEST_INT(m.GetCount(), 3);

    m[0] = 1;
    W_TEST_INT(m.GetCount(), 3);

    m.Clear();
    W_TEST_INT(m.GetCount(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      WMap<WUInt32, WConstructionCounter> m1;
      m1[0] = WConstructionCounter(1);
      W_TEST_BOOL(WConstructionCounter::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[1] = WConstructionCounter(3);
      W_TEST_BOOL(WConstructionCounter::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[0] = WConstructionCounter(2);
      W_TEST_BOOL(WConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      W_TEST_BOOL(WConstructionCounter::HasDone(0, 2));
      W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    }

    {
      WMap<WConstructionCounter, WUInt32> m1;
      m1[WConstructionCounter(0)] = 1;
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary

      m1[WConstructionCounter(1)] = 3;
      W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary

      m1[WConstructionCounter(0)] = 2;
      W_TEST_BOOL(WConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      W_TEST_BOOL(WConstructionCounter::HasDone(0, 2));
      W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert")
  {
    WMap<WUInt32, WUInt32> m;

    W_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    W_TEST_BOOL(m.Insert(1, 10).IsValid());
    W_TEST_BOOL(m.Insert(1, 10).IsValid());
    m.Insert(3, 30);
    auto it7 = m.Insert(7, 70);
    m.Insert(9, 90);
    m.Insert(4, 40);
    m.Insert(2, 20);
    m.Insert(8, 80);
    m.Insert(5, 50);
    m.Insert(6, 60);

    W_TEST_BOOL(m.Insert(7, 70).Value() == 70);
    W_TEST_BOOL(m.Insert(7, 70) == it7);

    W_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(WUInt32) * 2 * 9);

    W_TEST_INT(m[1], 10);
    W_TEST_INT(m[2], 20);
    W_TEST_INT(m[3], 30);
    W_TEST_INT(m[4], 40);
    W_TEST_INT(m[5], 50);
    W_TEST_INT(m[6], 60);
    W_TEST_INT(m[7], 70);
    W_TEST_INT(m[8], 80);
    W_TEST_INT(m[9], 90);

    W_TEST_INT(m.GetCount(), 9);

    for (WUInt32 i = 0; i < 1000000; ++i)
      m[i] = i;

    W_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(WUInt32) * 2 * 1000000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Find")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (WInt32 i = 1000 - 1; i >= 0; --i)
      W_TEST_INT(m.Find(i).Value(), i * 10);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetValue/TryGetValue")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    for (WInt32 i = 100 - 1; i >= 0; --i)
    {
      W_TEST_INT(*m.GetValue(i), i * 10);

      WUInt32 v = 0;
      W_TEST_BOOL(m.TryGetValue(i, v));
      W_TEST_INT(v, i * 10);

      WUInt32* pV = nullptr;
      W_TEST_BOOL(m.TryGetValue(i, pV));
      W_TEST_INT(*pV, i * 10);
    }

    W_TEST_BOOL(m.GetValue(101) == nullptr);

    WUInt32 v = 0;
    W_TEST_BOOL(m.TryGetValue(101, v) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetValue/TryGetValue (const)")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    const WMap<WUInt32, WUInt32>& mConst = m;

    for (WInt32 i = 100 - 1; i >= 0; --i)
    {
      W_TEST_INT(*mConst.GetValue(i), i * 10);

      WUInt32 v = 0;
      W_TEST_BOOL(m.TryGetValue(i, v));
      W_TEST_INT(v, i * 10);

      WUInt32* pV = nullptr;
      W_TEST_BOOL(m.TryGetValue(i, pV));
      W_TEST_INT(*pV, i * 10);
    }

    W_TEST_BOOL(mConst.GetValue(101) == nullptr);

    WUInt32 v = 0;
    W_TEST_BOOL(mConst.TryGetValue(101, v) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetValueOrDefault")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    for (WInt32 i = 100 - 1; i >= 0; --i)
      W_TEST_INT(m.GetValueOrDefault(i, 999), i * 10);

    W_TEST_BOOL(m.GetValueOrDefault(101, 999) == 999);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; i += 2)
      m[i] = i * 10;

    for (WInt32 i = 0; i < 1000; i += 2)
    {
      W_TEST_BOOL(m.Contains(i));
      W_TEST_BOOL(!m.Contains(i + 1));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindOrAdd")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
    {
      bool bExisted = true;
      m.FindOrAdd(i, &bExisted).Value() = i * 10;
      W_TEST_BOOL(!bExisted);
    }

    for (WInt32 i = 1000 - 1; i >= 0; --i)
    {
      bool bExisted = false;
      W_TEST_INT(m.FindOrAdd(i, &bExisted).Value(), i * 10);
      W_TEST_BOOL(bExisted);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator[]")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (WInt32 i = 1000 - 1; i >= 0; --i)
      W_TEST_INT(m[i], i * 10);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove (non-existing)")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
    {
      W_TEST_BOOL(!m.Remove(i));
    }

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (WInt32 i = 0; i < 1000; ++i)
    {
      W_TEST_BOOL(m.Remove(i + 500) == (i < 500));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove (Iterator)")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (WInt32 i = 0; i < 1000 - 1; ++i)
    {
      WMap<WUInt32, WUInt32>::Iterator itNext = m.Remove(m.Find(i));
      W_TEST_BOOL(!m.Find(i).IsValid());
      W_TEST_BOOL(itNext.Key() == i + 1);

      W_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove (Key)")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (WInt32 i = 0; i < 1000; ++i)
    {
      W_TEST_BOOL(m.Remove(i));
      W_TEST_BOOL(!m.Find(i).IsValid());

      W_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=")
  {
    WMap<WUInt32, WUInt32> m, m2;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    m2 = m;

    for (WInt32 i = 1000 - 1; i >= 0; --i)
      W_TEST_INT(m2[i], i * 10);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    WMap<WUInt32, WUInt32> m2(m);

    for (WInt32 i = 1000 - 1; i >= 0; --i)
      W_TEST_INT(m2[i], i * 10);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    WInt32 i = 0;
    for (WMap<WUInt32, WUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      W_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const WMap<WUInt32, WUInt32> m2(m);

    WInt32 i = 0;
    for (WMap<WUInt32, WUInt32>::ConstIterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      W_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LowerBound")
  {
    WMap<WInt32, WInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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
    WMap<WInt32, WInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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

    WMap<WInt32, WInt32> m;

    for (WUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (WUInt32 i = 0; i < 10000; ++i)
        m.Insert(i, i * 10);

      W_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (WUInt32 i = 0; i < 5000; ++i)
        W_TEST_BOOL(m.Remove(i));

      // Insert others
      for (WUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j, j);

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

  W_TEST_BLOCK(WTestBlock::Enabled, "operator == / !=")
  {
    WMap<WUInt32, WUInt32> m, m2;

    W_TEST_BOOL(m == m2);

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    W_TEST_BOOL(m != m2);

    m2 = m;

    W_TEST_BOOL(m == m2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompatibleKeyType")
  {
    {
      WMap<WString, int> stringTable;
      const char* szChar = "Char";
      const char* szString = "ViewBla";
      WStringView sView(szString, szString + 4);
      WStringBuilder sBuilder("Builder");
      WString sString("String");
      stringTable.Insert(szChar, 1);
      stringTable.Insert(sView, 2);
      stringTable.Insert(sBuilder, 3);
      stringTable.Insert(sString, 4);

      W_TEST_BOOL(stringTable.Contains(szChar));
      W_TEST_BOOL(stringTable.Contains(sView));
      W_TEST_BOOL(stringTable.Contains(sBuilder));
      W_TEST_BOOL(stringTable.Contains(sString));

      W_TEST_INT(*stringTable.GetValue(szChar), 1);
      W_TEST_INT(*stringTable.GetValue(sView), 2);
      W_TEST_INT(*stringTable.GetValue(sBuilder), 3);
      W_TEST_INT(*stringTable.GetValue(sString), 4);

      W_TEST_BOOL(stringTable.Remove(szChar));
      W_TEST_BOOL(stringTable.Remove(sView));
      W_TEST_BOOL(stringTable.Remove(sBuilder));
      W_TEST_BOOL(stringTable.Remove(sString));
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

      WMap<TestDynArray, int> arrayTable;
      arrayTable.Insert(a, 1);
      arrayTable.Insert(b, 2);

      WArrayPtr<const int> aPtr = a.GetArrayPtr();
      WArrayPtr<const int> bPtr = b.GetArrayPtr();

      WUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

      bool existed;
      auto it = arrayTable.FindOrAdd(aPtr, &existed);
      W_TEST_BOOL(existed);

      W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      W_TEST_BOOL(arrayTable.Contains(aPtr));
      W_TEST_BOOL(arrayTable.Contains(bPtr));
      W_TEST_BOOL(arrayTable.Contains(a));

      W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      W_TEST_INT(*arrayTable.GetValue(aPtr), 1);
      W_TEST_INT(*arrayTable.GetValue(bPtr), 2);
      W_TEST_INT(*arrayTable.GetValue(a), 1);

      W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      W_TEST_BOOL(arrayTable.Remove(aPtr));
      W_TEST_BOOL(arrayTable.Remove(bPtr));

      W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WStringBuilder tmp;
    WMap<WString, WInt32> map1;
    WMap<WString, WInt32> map2;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map1[tmp] = i;

      tmp.SetFormat("{0}{0}{0}", i);
      map2[tmp] = i;
    }

    map1.Swap(map2);

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      W_TEST_BOOL(map2.Contains(tmp));
      W_TEST_INT(map2[tmp], i);

      tmp.SetFormat("{0}{0}{0}", i);
      W_TEST_BOOL(map1.Contains(tmp));
      W_TEST_INT(map1[tmp], i);
    }
  }

  constexpr WUInt32 uiMapSize = sizeof(WMap<WString, WInt32>);

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WUInt8 map1Mem[uiMapSize];
    WUInt8 map2Mem[uiMapSize];
    WMemoryUtils::PatternFill(map1Mem, 0xCA, uiMapSize);
    WMemoryUtils::PatternFill(map2Mem, 0xCA, uiMapSize);

    WStringBuilder tmp;
    WMap<WString, WInt32>* map1 = new (map1Mem)(WMap<WString, WInt32>);
    WMap<WString, WInt32>* map2 = new (map2Mem)(WMap<WString, WInt32>);

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map1->Insert(tmp, i);

      tmp.SetFormat("{0}{0}{0}", i);
      map2->Insert(tmp, i);
    }

    map1->Swap(*map2);

    // test swapped elements
    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      W_TEST_BOOL(map2->Contains(tmp));
      W_TEST_INT((*map2)[tmp], i);

      tmp.SetFormat("{0}{0}{0}", i);
      W_TEST_BOOL(map1->Contains(tmp));
      W_TEST_INT((*map1)[tmp], i);
    }

    // test iterators after swap
    {
      for (auto it : *map1)
      {
        W_TEST_BOOL(!map2->Contains(it.Key()));
      }

      for (auto it : *map2)
      {
        W_TEST_BOOL(!map1->Contains(it.Key()));
      }
    }

    // due to a compiler bug in VS 2017, PatternFill cannot be called here, because it will move the memset BEFORE the destructor call!
    // seems to be fixed in VS 2019 though

    map1->~WMap<WString, WInt32>();
    // WMemoryUtils::PatternFill(map1Mem, 0xBA, uiSetSize);

    map2->~WMap<WString, WInt32>();
    WMemoryUtils::PatternFill(map2Mem, 0xBA, uiMapSize);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap Empty")
  {
    WUInt8 map1Mem[uiMapSize];
    WUInt8 map2Mem[uiMapSize];
    WMemoryUtils::PatternFill(map1Mem, 0xCA, uiMapSize);
    WMemoryUtils::PatternFill(map2Mem, 0xCA, uiMapSize);

    WStringBuilder tmp;
    WMap<WString, WInt32>* map1 = new (map1Mem)(WMap<WString, WInt32>);
    WMap<WString, WInt32>* map2 = new (map2Mem)(WMap<WString, WInt32>);

    for (WUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map1->Insert(tmp, i);
    }

    map1->Swap(*map2);
    W_TEST_BOOL(map1->IsEmpty());

    map1->~WMap<WString, WInt32>();
    WMemoryUtils::PatternFill(map1Mem, 0xBA, uiMapSize);

    // test swapped elements
    for (WUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      W_TEST_BOOL(map2->Contains(tmp));
    }

    // test iterators after swap
    {
      for (auto it : *map2)
      {
        W_TEST_BOOL(map2->Contains(it.Key()));
      }
    }

    map2->~WMap<WString, WInt32>();
    WMemoryUtils::PatternFill(map2Mem, 0xBA, uiMapSize);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetReverseIterator")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    WInt32 i = 1000 - 1;
    for (WMap<WUInt32, WUInt32>::ReverseIterator it = m.GetReverseIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      W_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetReverseIterator (const)")
  {
    WMap<WUInt32, WUInt32> m;

    for (WInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const WMap<WUInt32, WUInt32> m2(m);

    WInt32 i = 1000 - 1;
    for (WMap<WUInt32, WUInt32>::ConstReverseIterator it = m2.GetReverseIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(it.Key(), i);
      W_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }
}
