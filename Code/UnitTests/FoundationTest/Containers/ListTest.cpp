#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/List.h>

W_CREATE_SIMPLE_TEST(Containers, List)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WList<WInt32> l;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushBack() / PeekBack")
  {
    WList<WInt32> l;
    WInt32& val = l.PushBack();

    W_TEST_INT(val, 0);
    W_TEST_INT(l.GetCount(), 1);
    W_TEST_INT(l.PeekBack(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushBack(i) / GetCount")
  {
    WList<WInt32> l;
    W_TEST_BOOL(l.GetHeapMemoryUsage() == 0);

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);

      W_TEST_INT(l.GetCount(), i + 1);
      W_TEST_INT(l.PeekBack(), i);
    }

    W_TEST_BOOL(l.GetHeapMemoryUsage() >= sizeof(WInt32) * 1000);

    WUInt32 i = 0;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      W_TEST_INT(*it, i);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PopBack()")
  {
    WList<WInt32> l;

    WInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    while (!l.IsEmpty())
    {
      --i;
      W_TEST_INT(l.PeekBack(), i);
      l.PopBack();
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushFront() / PeekFront")
  {
    WList<WInt32> l;
    WInt32& val = l.PushFront();

    W_TEST_INT(val, 0);
    W_TEST_INT(l.GetCount(), 1);
    W_TEST_INT(l.PeekFront(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushFront(i) / PeekFront")
  {
    WList<WInt32> l;

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      l.PushFront(i);

      W_TEST_INT(l.GetCount(), i + 1);
      W_TEST_INT(l.PeekFront(), i);
    }

    WUInt32 i2 = 1000;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      --i2;
      W_TEST_INT(*it, i2);
    }

    W_TEST_INT(i2, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PopFront()")
  {
    WList<WInt32> l;

    WInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushFront(i);

    while (!l.IsEmpty())
    {
      --i;
      W_TEST_INT(l.PeekFront(), i);
      l.PopFront();
    }
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Clear / IsEmpty")
  {
    WList<WInt32> l;

    W_TEST_BOOL(l.IsEmpty());

    for (WUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    W_TEST_BOOL(!l.IsEmpty());

    l.Clear();
    W_TEST_BOOL(l.IsEmpty());

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);
      W_TEST_BOOL(!l.IsEmpty());

      l.Clear();
      W_TEST_BOOL(l.IsEmpty());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=")
  {
    WList<WInt32> l, l2;

    for (WUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    l2 = l;

    WUInt32 i = 0;
    for (WList<WInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      W_TEST_INT(*it, i);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor")
  {
    WList<WInt32> l;

    for (WUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    WList<WInt32> l2(l);

    WUInt32 i = 0;
    for (WList<WInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      W_TEST_INT(*it, i);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetCount")
  {
    WList<WInt32> l;
    l.SetCount(1000);
    W_TEST_INT(l.GetCount(), 1000);

    WInt32 i = 1;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      W_TEST_INT(*it, 0);
      *it = i;
      ++i;
    }

    l.SetCount(2000);
    i = 1;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      if (i > 1000)
        W_TEST_INT(*it, 0);
      else
        W_TEST_INT(*it, i);

      ++i;
    }

    l.SetCount(500);
    i = 1;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      W_TEST_INT(*it, i);
      ++i;
    }

    W_TEST_INT(i, 501);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert(item)")
  {
    WList<WInt32> l;

    for (WUInt32 i = 1; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    WInt32 i = 1;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      W_TEST_INT(*it, i + 10000);
      ++it;

      W_TEST_BOOL(it.IsValid());
      W_TEST_INT(*it, i);

      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove(item)")
  {
    WList<WInt32> l;

    WUInt32 i = 1;
    for (; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (WList<WInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    i = 1;

    // now remove every second element and only keep the larger values
    for (WList<WInt32>::Iterator it = l.GetIterator(); it.IsValid();)
    {
      W_TEST_INT(*it, i + 10000);

      ++it;
      it = l.Remove(it);
      ++i;
    }

    i = 1;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(*it, i + 10000);
      ++i;
    }

    W_TEST_INT(i, 1000);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Iterator::IsValid")
  {
    WList<WInt32> l;

    for (WUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    WUInt32 i = 0;
    for (WList<WInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      W_TEST_INT(*it, i);
      ++i;
    }

    W_TEST_BOOL(!l.GetEndIterator().IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Element Constructions / Destructions")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    WList<WConstructionCounter> l;

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    l.PushBack();
    W_TEST_BOOL(WConstructionCounter::HasDone(1, 0));

    l.PushBack(WConstructionCounter(1));
    W_TEST_BOOL(WConstructionCounter::HasDone(2, 1));

    l.SetCount(4);
    W_TEST_BOOL(WConstructionCounter::HasDone(2, 0));

    l.Clear();
    W_TEST_BOOL(WConstructionCounter::HasDone(0, 4));

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator == / !=")
  {
    WList<WInt32> l, l2;

    W_TEST_BOOL(l == l2);

    WInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    W_TEST_BOOL(l != l2);

    l2 = l;

    W_TEST_BOOL(l == l2);
  }
}
