#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticRingBuffer.h>

using cc = WConstructionCounter;

W_CREATE_SIMPLE_TEST(Containers, StaticRingBuffer)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      WStaticRingBuffer<WInt32, 32> r1;
      WStaticRingBuffer<WInt32, 16> r2;
      WStaticRingBuffer<cc, 2> r3;
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy Constructor / Operator=")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      WStaticRingBuffer<cc, 16> r1;

      for (WUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      WStaticRingBuffer<cc, 16> r2(r1);

      for (WUInt32 i = 0; i < 16; ++i)
        W_TEST_BOOL(r2[i] == cc(i));

      WStaticRingBuffer<cc, 16> r3;
      r3 = r1;

      for (WUInt32 i = 0; i < 16; ++i)
        W_TEST_BOOL(r3[i] == cc(i));
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operator==")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      WStaticRingBuffer<cc, 16> r1;

      for (WUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      WStaticRingBuffer<cc, 16> r2(r1);
      WStaticRingBuffer<cc, 16> r3(r1);
      r3.PeekFront() = cc(3);

      W_TEST_BOOL(r1 == r1);
      W_TEST_BOOL(r2 == r2);
      W_TEST_BOOL(r3 == r3);

      W_TEST_BOOL(r1 == r2);
      W_TEST_BOOL(r1 != r3);
      W_TEST_BOOL(r2 != r3);
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PushBack / operator[] / CanAppend")
  {
    WStaticRingBuffer<WInt32, 16> r;

    for (WUInt32 i = 0; i < 16; ++i)
    {
      W_TEST_BOOL(r.CanAppend());
      r.PushBack(i);
    }

    W_TEST_BOOL(!r.CanAppend());

    for (WUInt32 i = 0; i < 16; ++i)
      W_TEST_INT(r[i], i);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCount / IsEmpty")
  {
    WStaticRingBuffer<WInt32, 16> r;

    W_TEST_BOOL(r.IsEmpty());

    for (WUInt32 i = 0; i < 16; ++i)
    {
      W_TEST_INT(r.GetCount(), i);
      r.PushBack(i);
      W_TEST_INT(r.GetCount(), i + 1);

      W_TEST_BOOL(!r.IsEmpty());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear / IsEmpty")
  {
    WStaticRingBuffer<WInt32, 16> r;

    W_TEST_BOOL(r.IsEmpty());

    for (WUInt32 i = 0; i < 16; ++i)
      r.PushBack(i);

    W_TEST_BOOL(!r.IsEmpty());

    r.Clear();

    W_TEST_BOOL(r.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Cycle Items / PeekFront")
  {
    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());

    {
      WStaticRingBuffer<WConstructionCounter, 16> r;

      for (WUInt32 i = 0; i < 16; ++i)
      {
        r.PushBack(WConstructionCounter(i));
        W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (WUInt32 i = 16; i < 1000; ++i)
      {
        W_TEST_BOOL(r.PeekFront() == WConstructionCounter(i - 16));
        W_TEST_BOOL(WConstructionCounter::HasDone(1, 1)); // one temporary

        W_TEST_BOOL(!r.CanAppend());

        r.PopFront();
        W_TEST_BOOL(WConstructionCounter::HasDone(0, 1));

        W_TEST_BOOL(r.CanAppend());

        r.PushBack(WConstructionCounter(i));
        W_TEST_BOOL(WConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (WUInt32 i = 1000; i < 1016; ++i)
      {
        W_TEST_BOOL(r.PeekFront() == WConstructionCounter(i - 16));
        W_TEST_BOOL(WConstructionCounter::HasDone(1, 1)); // one temporary

        r.PopFront();
        W_TEST_BOOL(WConstructionCounter::HasDone(0, 1)); // one temporary
      }

      W_TEST_BOOL(r.IsEmpty());
    }

    W_TEST_BOOL(WConstructionCounter::HasAllDestructed());
  }
}
