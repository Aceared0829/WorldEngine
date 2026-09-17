#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace HashTableTestDetail
{
  using st = WConstructionCounter;

  struct Collision
  {
    WUInt32 hash;
    int key;

    inline Collision(WUInt32 uiHash, int iKey)
    {
      this->hash = uiHash;
      this->key = iKey;
    }

    inline bool operator==(const Collision& other) const { return key == other.key; }

    W_DECLARE_POD_TYPE();
  };

  class OnlyMovable
  {
  public:
    OnlyMovable(WUInt32 uiHash)
      : hash(uiHash)

    {
    }
    OnlyMovable(OnlyMovable&& other) { *this = std::move(other); }

    void operator=(OnlyMovable&& other)
    {
      hash = other.hash;
      m_NumTimesMoved = 0;
      ++other.m_NumTimesMoved;
    }

    bool operator==(const OnlyMovable& other) const { return hash == other.hash; }

    int m_NumTimesMoved = 0;
    WUInt32 hash;

  private:
    OnlyMovable(const OnlyMovable&);
    void operator=(const OnlyMovable&);
  };
} // namespace HashTableTestDetail

template <>
struct WHashHelper<HashTableTestDetail::Collision>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const HashTableTestDetail::Collision& value) { return value.hash; }

  W_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::Collision& a, const HashTableTestDetail::Collision& b) { return a == b; }
};

template <>
struct WHashHelper<HashTableTestDetail::OnlyMovable>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const HashTableTestDetail::OnlyMovable& value) { return value.hash; }

  W_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::OnlyMovable& a, const HashTableTestDetail::OnlyMovable& b)
  {
    return a.hash == b.hash;
  }
};

W_CREATE_SIMPLE_TEST(Containers, HashTable)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WHashTable<WInt32, HashTableTestDetail::st> table1;

    W_TEST_BOOL(table1.GetCount() == 0);
    W_TEST_BOOL(table1.IsEmpty());

    WUInt32 counter = 0;
    for (WHashTable<WInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    W_TEST_INT(counter, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    WHashTable<WInt32, HashTableTestDetail::st> table1;

    for (WInt32 i = 0; i < 64; ++i)
    {
      WInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key, WConstructionCounter(i));
    }

    // insert an element at the very end
    table1.Insert(47, WConstructionCounter(64));

    WHashTable<WInt32, HashTableTestDetail::st> table2;
    table2 = table1;
    WHashTable<WInt32, HashTableTestDetail::st> table3(table1);

    W_TEST_INT(table1.GetCount(), 65);
    W_TEST_INT(table2.GetCount(), 65);
    W_TEST_INT(table3.GetCount(), 65);

    WUInt32 uiCounter = 0;
    for (WHashTable<WInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      WConstructionCounter value;

      W_TEST_BOOL(table2.TryGetValue(it.Key(), value));
      W_TEST_BOOL(it.Value() == value);
      W_TEST_BOOL(*table2.GetValue(it.Key()) == it.Value());

      W_TEST_BOOL(table3.TryGetValue(it.Key(), value));
      W_TEST_BOOL(it.Value() == value);
      W_TEST_BOOL(*table3.GetValue(it.Key()) == it.Value());

      ++uiCounter;
    }
    W_TEST_INT(uiCounter, table1.GetCount());

    for (WHashTable<WInt32, HashTableTestDetail::st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      it.Value() = HashTableTestDetail::st(42);
    }

    for (WHashTable<WInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      WConstructionCounter value;

      W_TEST_BOOL(table1.TryGetValue(it.Key(), value));
      W_TEST_BOOL(it.Value() == value);
      W_TEST_BOOL(value.m_iData == 42);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    WHashTable<WInt32, HashTableTestDetail::st> table1;
    for (WInt32 i = 0; i < 64; ++i)
    {
      table1.Insert(i, WConstructionCounter(i));
    }

    WUInt64 memoryUsage = table1.GetHeapMemoryUsage();

    WHashTable<WInt32, HashTableTestDetail::st> table2;
    table2 = std::move(table1);

    W_TEST_INT(table1.GetCount(), 0);
    W_TEST_INT(table1.GetHeapMemoryUsage(), 0);
    W_TEST_INT(table2.GetCount(), 64);
    W_TEST_INT(table2.GetHeapMemoryUsage(), memoryUsage);

    WHashTable<WInt32, HashTableTestDetail::st> table3(std::move(table2));

    W_TEST_INT(table2.GetCount(), 0);
    W_TEST_INT(table2.GetHeapMemoryUsage(), 0);
    W_TEST_INT(table3.GetCount(), 64);
    W_TEST_INT(table3.GetHeapMemoryUsage(), memoryUsage);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move Insert")
  {
    HashTableTestDetail::OnlyMovable noCopyObject(42);

    {
      WHashTable<HashTableTestDetail::OnlyMovable, int> noCopyKey;
      // noCopyKey.Insert(noCopyObject, 10); // Should not compile
      noCopyKey.Insert(std::move(noCopyObject), 10);
      W_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
      W_TEST_BOOL(noCopyKey.Contains(noCopyObject));
    }

    {
      WHashTable<int, HashTableTestDetail::OnlyMovable> noCopyValue;
      // noCopyValue.Insert(10, noCopyObject); // Should not compile
      noCopyValue.Insert(10, std::move(noCopyObject));
      W_TEST_INT(noCopyObject.m_NumTimesMoved, 2);
      W_TEST_BOOL(noCopyValue.Contains(10));
    }

    {
      WHashTable<HashTableTestDetail::OnlyMovable, HashTableTestDetail::OnlyMovable> noCopyAnything;
      // noCopyAnything.Insert(10, noCopyObject); // Should not compile
      // noCopyAnything.Insert(noCopyObject, 10); // Should not compile
      noCopyAnything.Insert(std::move(noCopyObject), std::move(noCopyObject));
      W_TEST_INT(noCopyObject.m_NumTimesMoved, 4);
      W_TEST_BOOL(noCopyAnything.Contains(noCopyObject));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Collision Tests")
  {
    WHashTable<HashTableTestDetail::Collision, int> map2;

    map2[HashTableTestDetail::Collision(0, 0)] = 0;
    map2[HashTableTestDetail::Collision(1, 1)] = 1;
    map2[HashTableTestDetail::Collision(0, 2)] = 2;
    map2[HashTableTestDetail::Collision(1, 3)] = 3;
    map2[HashTableTestDetail::Collision(1, 4)] = 4;
    map2[HashTableTestDetail::Collision(0, 5)] = 5;

    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 0)] == 0);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 1)] == 1);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 0)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 1)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    W_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 0)));
    W_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 1)));

    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    W_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 0)));
    W_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 1)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    map2[HashTableTestDetail::Collision(0, 6)] = 6;
    map2[HashTableTestDetail::Collision(1, 7)] = 7;

    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 6)] == 6);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 6)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    W_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 4)));
    W_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 6)));

    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    W_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 4)));
    W_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 6)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    W_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    map2[HashTableTestDetail::Collision(0, 2)] = 3;
    map2[HashTableTestDetail::Collision(0, 5)] = 6;
    map2[HashTableTestDetail::Collision(1, 3)] = 4;

    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 3);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 6);
    W_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    W_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());

    {
      WHashTable<WUInt32, HashTableTestDetail::st> m1;
      m1[0] = HashTableTestDetail::st(1);
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1[1] = HashTableTestDetail::st(3);
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1[0] = HashTableTestDetail::st(2);
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      W_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }

    {
      WHashTable<HashTableTestDetail::st, WUInt32> m1;
      m1[HashTableTestDetail::st(0)] = 1;
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(1)] = 3;
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(0)] = 2;
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      W_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      W_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert/TryGetValue/GetValue")
  {
    WHashTable<WInt32, HashTableTestDetail::st> a1;

    for (WInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(!a1.Insert(i, i - 20));
    }

    for (WInt32 i = 0; i < 10; ++i)
    {
      HashTableTestDetail::st oldValue;
      W_TEST_BOOL(a1.Insert(i, i, &oldValue));
      W_TEST_INT(oldValue.m_iData, i - 20);
    }

    HashTableTestDetail::st value;
    W_TEST_BOOL(a1.TryGetValue(9, value));
    W_TEST_INT(value.m_iData, 9);
    W_TEST_INT(a1.GetValue(9)->m_iData, 9);

    W_TEST_BOOL(!a1.TryGetValue(11, value));
    W_TEST_INT(value.m_iData, 9);
    W_TEST_BOOL(a1.GetValue(11) == nullptr);

    HashTableTestDetail::st* pValue;
    W_TEST_BOOL(a1.TryGetValue(9, pValue));
    W_TEST_INT(pValue->m_iData, 9);

    pValue->m_iData = 20;
    W_TEST_INT(a1[9].m_iData, 20);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove/Compact")
  {
    WHashTable<WInt32, HashTableTestDetail::st> a;

    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (WInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i, i);
      W_TEST_INT(a.GetCount(), i + 1);
    }

    W_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(WInt32) + sizeof(HashTableTestDetail::st)));

    a.Compact();

    for (WInt32 i = 0; i < 1000; ++i)
      W_TEST_INT(a[i].m_iData, i);


    for (WInt32 i = 0; i < 250; ++i)
    {
      HashTableTestDetail::st oldValue;
      W_TEST_BOOL(a.Remove(i, &oldValue));
      W_TEST_INT(oldValue.m_iData, i);
    }
    W_TEST_INT(a.GetCount(), 750);

    for (WHashTable<WInt32, HashTableTestDetail::st>::Iterator it = a.GetIterator(); it.IsValid();)
    {
      if (it.Key() < 500)
        it = a.Remove(it);
      else
        ++it;
    }
    W_TEST_INT(a.GetCount(), 500);
    a.Compact();

    for (WInt32 i = 500; i < 1000; ++i)
      W_TEST_INT(a[i].m_iData, i);

    a.Clear();
    a.Compact();

    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator[]")
  {
    WHashTable<WInt32, WInt32> a;

    a.Insert(4, 20);
    a[2] = 30;

    W_TEST_INT(a[4], 20);
    W_TEST_INT(a[2], 30);
    W_TEST_INT(a[1], 0); // new values are default constructed
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator==/!=")
  {
    WStaticArray<WInt32, 64> keys[2];

    for (WUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    WHashTable<WInt32, HashTableTestDetail::st> t[2];

    for (WUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const WUInt32 uiIndex = rand() % keys[i].GetCount();
        const WInt32 key = keys[i][uiIndex];
        t[i].Insert(key, HashTableTestDetail::st(key * 3456));

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    W_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32, HashTableTestDetail::st(64));
    W_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32, HashTableTestDetail::st(47));
    W_TEST_BOOL(t[0] != t[1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompatibleKeyType")
  {
    WProxyAllocator testAllocator("Test", WFoundation::GetDefaultAllocator());
    WLocalAllocatorWrapper allocWrapper(&testAllocator);
    using TestString = WHybridString<32, WLocalAllocatorWrapper>;

    WHashTable<TestString, int> stringTable;
    const char* szChar = "VeryLongStringDefinitelyMoreThan32Chars1111elf!!!!";
    const char* szString = "AnotherVeryLongStringThisTimeUsedForStringView!!!!";
    WStringView sView(szString);
    WStringBuilder sBuilder("BuilderAlsoNeedsToBeAVeryLongStringToTriggerAllocation");
    WString sString("String");
    W_TEST_BOOL(!stringTable.Insert(szChar, 1));
    W_TEST_BOOL(!stringTable.Insert(sView, 2));
    W_TEST_BOOL(!stringTable.Insert(sBuilder, 3));
    W_TEST_BOOL(!stringTable.Insert(sString, 4));
    W_TEST_BOOL(stringTable.Insert(szString, 2));

    WUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

    W_TEST_BOOL(stringTable.Contains(szChar));
    W_TEST_BOOL(stringTable.Contains(sView));
    W_TEST_BOOL(stringTable.Contains(sBuilder));
    W_TEST_BOOL(stringTable.Contains(sString));

    W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    W_TEST_INT(*stringTable.GetValue(szChar), 1);
    W_TEST_INT(*stringTable.GetValue(sView), 2);
    W_TEST_INT(*stringTable.GetValue(sBuilder), 3);
    W_TEST_INT(*stringTable.GetValue(sString), 4);

    W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    W_TEST_BOOL(stringTable.Remove(szChar));
    W_TEST_BOOL(stringTable.Remove(sView));
    W_TEST_BOOL(stringTable.Remove(sBuilder));
    W_TEST_BOOL(stringTable.Remove(sString));

    W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WStringBuilder tmp;
    WHashTable<WString, WInt32> map1;
    WHashTable<WString, WInt32> map2;

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

  W_TEST_BLOCK(WTestBlock::Enabled, "foreach")
  {
    WStringBuilder tmp;
    WHashTable<WString, WInt32> map;
    WHashTable<WString, WInt32> map2;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map[tmp] = i;
    }

    W_TEST_INT(map.GetCount(), 1000);

    map2 = map;
    W_TEST_INT(map2.GetCount(), map.GetCount());

    for (WHashTable<WString, WInt32>::Iterator it = begin(map); it != end(map); ++it)
    {
      const WString& k = it.Key();
      WInt32 v = it.Value();

      map2.Remove(k);
    }

    W_TEST_BOOL(map2.IsEmpty());
    map2 = map;

    for (auto it : map)
    {
      const WString& k = it.Key();
      WInt32 v = it.Value();

      map2.Remove(k);
    }

    W_TEST_BOOL(map2.IsEmpty());
    map2 = map;

    // just check that this compiles
    for (auto it : static_cast<const WHashTable<WString, WInt32>&>(map))
    {
      const WString& k = it.Key();
      WInt32 v = it.Value();

      map2.Remove(k);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Find")
  {
    WStringBuilder tmp;
    WHashTable<WString, WInt32> map;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map[tmp] = i;
    }

    for (WInt32 i = map.GetCount() - 1; i > 0; --i)
    {
      tmp.SetFormat("stuff{}bla", i);

      auto it = map.Find(tmp);
      auto cit = static_cast<const WHashTable<WString, WInt32>&>(map).Find(tmp);

      W_TEST_STRING(it.Key(), tmp);
      W_TEST_INT(it.Value(), i);

      W_TEST_STRING(cit.Key(), tmp);
      W_TEST_INT(cit.Value(), i);

      int allowedIterations = map.GetCount();
      for (auto it2 = it; it2.IsValid(); ++it2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        W_TEST_BOOL(allowedIterations >= 0);
      }

      allowedIterations = map.GetCount();
      for (auto cit2 = cit; cit2.IsValid(); ++cit2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        W_TEST_BOOL(allowedIterations >= 0);
      }

      map.Remove(it);
    }
  }
  W_TEST_BLOCK(WTestBlock::Enabled, "Find")
  {
    WStringBuilder tmp;
    WHashTable<WString, WInt32> map;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map[tmp] = i;
    }

    for (WInt32 i = map.GetCount() - 1; i > 0; --i)
    {
      tmp.SetFormat("stuff{}bla", i);

      auto it = map.Find(tmp);
      auto cit = static_cast<const WHashTable<WString, WInt32>&>(map).Find(tmp);

      W_TEST_STRING(it.Key(), tmp);
      W_TEST_INT(it.Value(), i);

      W_TEST_STRING(cit.Key(), tmp);
      W_TEST_INT(cit.Value(), i);

      int allowedIterations = map.GetCount();
      for (auto it2 = it; it2.IsValid(); ++it2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        W_TEST_BOOL(allowedIterations >= 0);
      }

      allowedIterations = map.GetCount();
      for (auto cit2 = cit; cit2.IsValid(); ++cit2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        W_TEST_BOOL(allowedIterations >= 0);
      }

      map.Remove(it);
    }
  }
}
