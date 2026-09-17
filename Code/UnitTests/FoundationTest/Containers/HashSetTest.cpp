#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Memory/CommonAllocators.h>

namespace
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
} // namespace

template <>
struct WHashHelper<Collision>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const Collision& value) { return value.hash; }

  W_ALWAYS_INLINE static bool Equal(const Collision& a, const Collision& b) { return a == b; }
};

template <>
struct WHashHelper<OnlyMovable>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const OnlyMovable& value) { return value.hash; }

  W_ALWAYS_INLINE static bool Equal(const OnlyMovable& a, const OnlyMovable& b) { return a.hash == b.hash; }
};

W_CREATE_SIMPLE_TEST(Containers, HashSet)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WHashSet<WInt32> table1;

    W_TEST_BOOL(table1.GetCount() == 0);
    W_TEST_BOOL(table1.IsEmpty());

    WUInt32 counter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    W_TEST_INT(counter, 0);

    W_TEST_BOOL(begin(table1) == end(table1));
    W_TEST_BOOL(cbegin(table1) == cend(table1));
    table1.Reserve(10);
    W_TEST_BOOL(begin(table1) == end(table1));
    W_TEST_BOOL(cbegin(table1) == cend(table1));

    for (auto value : table1)
    {
      ++counter;
    }
    W_TEST_INT(counter, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    WHashSet<WInt32> table1;

    for (WInt32 i = 0; i < 64; ++i)
    {
      WInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key);
    }

    // insert an element at the very end
    table1.Insert(47);

    WHashSet<WInt32> table2;
    table2 = table1;
    WHashSet<WInt32> table3(table1);

    W_TEST_INT(table1.GetCount(), 65);
    W_TEST_INT(table2.GetCount(), 65);
    W_TEST_INT(table3.GetCount(), 65);
    W_TEST_BOOL(begin(table1) != end(table1));
    W_TEST_BOOL(cbegin(table1) != cend(table1));

    WUInt32 uiCounter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      WConstructionCounter value;
      W_TEST_BOOL(table2.Contains(it.Key()));
      W_TEST_BOOL(table3.Contains(it.Key()));
      ++uiCounter;
    }
    W_TEST_INT(uiCounter, table1.GetCount());

    uiCounter = 0;
    for (const auto& value : table1)
    {
      W_TEST_BOOL(table2.Contains(value));
      W_TEST_BOOL(table3.Contains(value));
      ++uiCounter;
    }
    W_TEST_INT(uiCounter, table1.GetCount());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    WHashSet<st> set1;
    for (WInt32 i = 0; i < 64; ++i)
    {
      set1.Insert(WConstructionCounter(i));
    }

    WUInt64 memoryUsage = set1.GetHeapMemoryUsage();

    WHashSet<st> set2;
    set2 = std::move(set1);

    W_TEST_INT(set1.GetCount(), 0);
    W_TEST_INT(set1.GetHeapMemoryUsage(), 0);
    W_TEST_INT(set2.GetCount(), 64);
    W_TEST_INT(set2.GetHeapMemoryUsage(), memoryUsage);

    WHashSet<st> set3(std::move(set2));

    W_TEST_INT(set2.GetCount(), 0);
    W_TEST_INT(set2.GetHeapMemoryUsage(), 0);
    W_TEST_INT(set3.GetCount(), 64);
    W_TEST_INT(set3.GetHeapMemoryUsage(), memoryUsage);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Collision Tests")
  {
    WHashSet<Collision> set2;

    set2.Insert(Collision(0, 0));
    set2.Insert(Collision(1, 1));
    set2.Insert(Collision(0, 2));
    set2.Insert(Collision(1, 3));
    set2.Insert(Collision(1, 4));
    set2.Insert(Collision(0, 5));

    W_TEST_BOOL(set2.Contains(Collision(0, 0)));
    W_TEST_BOOL(set2.Contains(Collision(1, 1)));
    W_TEST_BOOL(set2.Contains(Collision(0, 2)));
    W_TEST_BOOL(set2.Contains(Collision(1, 3)));
    W_TEST_BOOL(set2.Contains(Collision(1, 4)));
    W_TEST_BOOL(set2.Contains(Collision(0, 5)));

    W_TEST_BOOL(set2.Remove(Collision(0, 0)));
    W_TEST_BOOL(set2.Remove(Collision(1, 1)));

    W_TEST_BOOL(!set2.Contains(Collision(0, 0)));
    W_TEST_BOOL(!set2.Contains(Collision(1, 1)));
    W_TEST_BOOL(set2.Contains(Collision(0, 2)));
    W_TEST_BOOL(set2.Contains(Collision(1, 3)));
    W_TEST_BOOL(set2.Contains(Collision(1, 4)));
    W_TEST_BOOL(set2.Contains(Collision(0, 5)));

    set2.Insert(Collision(0, 6));
    set2.Insert(Collision(1, 7));

    W_TEST_BOOL(set2.Contains(Collision(0, 2)));
    W_TEST_BOOL(set2.Contains(Collision(1, 3)));
    W_TEST_BOOL(set2.Contains(Collision(1, 4)));
    W_TEST_BOOL(set2.Contains(Collision(0, 5)));
    W_TEST_BOOL(set2.Contains(Collision(0, 6)));
    W_TEST_BOOL(set2.Contains(Collision(1, 7)));

    W_TEST_BOOL(set2.Remove(Collision(1, 4)));
    W_TEST_BOOL(set2.Remove(Collision(0, 6)));

    W_TEST_BOOL(!set2.Contains(Collision(1, 4)));
    W_TEST_BOOL(!set2.Contains(Collision(0, 6)));
    W_TEST_BOOL(set2.Contains(Collision(0, 2)));
    W_TEST_BOOL(set2.Contains(Collision(1, 3)));
    W_TEST_BOOL(set2.Contains(Collision(0, 5)));
    W_TEST_BOOL(set2.Contains(Collision(1, 7)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    W_TEST_BOOL(st::HasAllDestructed());

    {
      WHashSet<st> m1;
      m1.Insert(st(1));
      W_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1.Insert(st(3));
      W_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1.Insert(st(1));
      W_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      W_TEST_BOOL(st::HasDone(0, 2));
      W_TEST_BOOL(st::HasAllDestructed());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert")
  {
    WHashSet<WInt32> a1;

    for (WInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(!a1.Insert(i));
    }

    for (WInt32 i = 0; i < 10; ++i)
    {
      W_TEST_BOOL(a1.Insert(i));
    }
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Move Insert")
  {
    OnlyMovable noCopyObject(42);

    WHashSet<OnlyMovable> noCopyKey;
    // noCopyKey.Insert(noCopyObject); // Should not compile
    noCopyKey.Insert(std::move(noCopyObject));
    W_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
    W_TEST_BOOL(noCopyKey.Contains(noCopyObject));
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Remove/Compact")
  {
    WHashSet<WInt32> a;

    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (WInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i);
      W_TEST_INT(a.GetCount(), i + 1);
    }

    W_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(WInt32)));

    a.Compact();

    for (WInt32 i = 0; i < 500; ++i)
    {
      W_TEST_BOOL(a.Remove(i));
    }

    a.Compact();

    for (WInt32 i = 500; i < 1000; ++i)
    {
      W_TEST_BOOL(a.Contains(i));
    }

    a.Clear();
    a.Compact();

    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove (Iterator)")
  {
    WHashSet<WInt32> a;

    W_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
    for (WInt32 i = 0; i < 1000; ++i)
      a.Insert(i);

    WHashSet<WInt32>::ConstIterator it = a.GetIterator();

    for (WInt32 i = 0; i < 1000 - 1; ++i)
    {
      WInt32 value = it.Key();
      it = a.Remove(it);
      W_TEST_BOOL(!a.Contains(value));
      W_TEST_BOOL(it.IsValid());
      W_TEST_INT(a.GetCount(), 1000 - 1 - i);
    }
    it = a.Remove(it);
    W_TEST_BOOL(!it.IsValid());
    W_TEST_BOOL(a.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Set Operations")
  {
    WHashSet<WUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    WHashSet<WUInt32> empty;

    WHashSet<WUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    WHashSet<WUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    WHashSet<WUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    WHashSet<WUInt32> nonDisjunctNonEmptySubSet;
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
      WHashSet<WUInt32> res;

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
      WHashSet<WUInt32> res;
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
      WHashSet<WUInt32> res;
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

  W_TEST_BLOCK(WTestBlock::Enabled, "operator==/!=")
  {
    WStaticArray<WInt32, 64> keys[2];

    for (WUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    WHashSet<WInt32> t[2];

    for (WUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const WUInt32 uiIndex = rand() % keys[i].GetCount();
        const WInt32 key = keys[i][uiIndex];
        t[i].Insert(key);

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    W_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32);
    W_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32);
    W_TEST_BOOL(t[0] == t[1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompatibleKeyType")
  {
    WProxyAllocator testAllocator("Test", WFoundation::GetDefaultAllocator());
    WLocalAllocatorWrapper allocWrapper(&testAllocator);
    using TestString = WHybridString<32, WLocalAllocatorWrapper>;

    WHashSet<TestString> stringSet;
    const char* szChar = "VeryLongStringDefinitelyMoreThan32Chars1111elf!!!!";
    const char* szString = "AnotherVeryLongStringThisTimeUsedForStringView!!!!";
    WStringView sView(szString);
    WStringBuilder sBuilder("BuilderAlsoNeedsToBeAVeryLongStringToTriggerAllocation");
    WString sString("String");
    W_TEST_BOOL(!stringSet.Insert(szChar));
    W_TEST_BOOL(!stringSet.Insert(sView));
    W_TEST_BOOL(!stringSet.Insert(sBuilder));
    W_TEST_BOOL(!stringSet.Insert(sString));
    W_TEST_BOOL(stringSet.Insert(szString));

    WUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

    W_TEST_BOOL(stringSet.Contains(szChar));
    W_TEST_BOOL(stringSet.Contains(sView));
    W_TEST_BOOL(stringSet.Contains(sBuilder));
    W_TEST_BOOL(stringSet.Contains(sString));

    W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    W_TEST_BOOL(stringSet.Remove(szChar));
    W_TEST_BOOL(stringSet.Remove(sView));
    W_TEST_BOOL(stringSet.Remove(sBuilder));
    W_TEST_BOOL(stringSet.Remove(sString));

    W_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WStringBuilder tmp;
    WHashSet<WString> set1;
    WHashSet<WString> set2;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1.Insert(tmp);

      tmp.SetFormat("{0}{0}{0}", i);
      set2.Insert(tmp);
    }

    set1.Swap(set2);

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      W_TEST_BOOL(set2.Contains(tmp));

      tmp.SetFormat("{0}{0}{0}", i);
      W_TEST_BOOL(set1.Contains(tmp));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "foreach")
  {
    WStringBuilder tmp;
    WHashSet<WString> set;
    WHashSet<WString> set2;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set.Insert(tmp);
    }

    W_TEST_INT(set.GetCount(), 1000);

    set2 = set;
    W_TEST_INT(set2.GetCount(), set.GetCount());

    for (WHashSet<WString>::ConstIterator it = begin(set); it != end(set); ++it)
    {
      const WString& k = it.Key();
      set2.Remove(k);
    }

    W_TEST_BOOL(set2.IsEmpty());
    set2 = set;

    for (auto key : set)
    {
      set2.Remove(key);
    }

    W_TEST_BOOL(set2.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Find")
  {
    WStringBuilder tmp;
    WHashSet<WString> set;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set.Insert(tmp);
    }

    for (WInt32 i = set.GetCount() - 1; i > 0; --i)
    {
      tmp.SetFormat("stuff{}bla", i);

      auto it = set.Find(tmp);

      W_TEST_STRING(it.Key(), tmp);

      int allowedIterations = set.GetCount();
      for (auto it2 = it; it2.IsValid(); ++it2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        W_TEST_BOOL(allowedIterations >= 0);
      }

      set.Remove(it);
    }
  }
}
