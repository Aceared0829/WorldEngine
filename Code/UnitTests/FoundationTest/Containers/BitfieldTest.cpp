#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Strings/String.h>

W_CREATE_SIMPLE_TEST(Containers, Bitfield)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "GetCount / IsEmpty / Clear")
  {
    WDynamicBitfield bf; // using a dynamic array

    W_TEST_INT(bf.GetCount(), 0);
    W_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(15, false);

    W_TEST_INT(bf.GetCount(), 15);
    W_TEST_BOOL(!bf.IsEmpty());

    bf.Clear();

    W_TEST_INT(bf.GetCount(), 0);
    W_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(37, false);

    W_TEST_INT(bf.GetCount(), 37);
    W_TEST_BOOL(!bf.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetCount / SetAllBits / ClearAllBits")
  {
    WHybridBitfield<512> bf; // using a hybrid array

    bf.SetCount(249, false);
    W_TEST_INT(bf.GetCount(), 249);

    for (WUInt32 i = 0; i < bf.GetCount(); ++i)
      W_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();
    W_TEST_INT(bf.GetCount(), 249);

    for (WUInt32 i = 0; i < bf.GetCount(); ++i)
      W_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();
    W_TEST_INT(bf.GetCount(), 249);

    for (WUInt32 i = 0; i < bf.GetCount(); ++i)
      W_TEST_BOOL(!bf.IsBitSet(i));


    bf.SetCount(349, true);
    W_TEST_INT(bf.GetCount(), 349);

    for (WUInt32 i = 0; i < 249; ++i)
      W_TEST_BOOL(!bf.IsBitSet(i));

    for (WUInt32 i = 249; i < bf.GetCount(); ++i)
      W_TEST_BOOL(bf.IsBitSet(i));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetCount / SetBit / FlipBit / ClearBit / SetBitValue / SetCountUninitialized")
  {
    WHybridBitfield<512> bf; // using a hybrid array

    bf.SetCount(100, false);
    W_TEST_INT(bf.GetCount(), 100);

    for (WUInt32 i = 0; i < bf.GetCount(); ++i)
      W_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetCount(200, true);
    W_TEST_INT(bf.GetCount(), 200);

    for (WUInt32 i = 100; i < bf.GetCount(); ++i)
      W_TEST_BOOL(bf.IsBitSet(i));

    bf.SetCountUninitialized(250);
    W_TEST_INT(bf.GetCount(), 250);

    bf.ClearAllBits();

    for (WUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    for (WUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      W_TEST_BOOL(bf.IsBitSet(i));
      W_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (WUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (WUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      W_TEST_BOOL(!bf.IsBitSet(i));
      W_TEST_BOOL(bf.IsBitSet(i + 1));
    }

    for (WUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (WUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      W_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }

    for (WUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.FlipBit(i);
    }

    for (WUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      W_TEST_BOOL(bf.IsBitSet(i) == (((0b011100 >> (i % 6)) & 1) == 1));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetBitRange")
  {
    for (WUInt32 size = 1; size < 1024; ++size)
    {
      WBitfield<WDeque<WUInt32>> bf; // using a deque
      bf.SetCount(size, false);

      W_TEST_INT(bf.GetCount(), size);

      for (WUInt32 count = 0; count < bf.GetCount(); ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));

      WUInt32 uiStart = size / 2;
      WUInt32 uiEnd = WMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (WUInt32 count = 0; count < uiStart; ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));
      for (WUInt32 count = uiStart; count <= uiEnd; ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
      for (WUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClearBitRange")
  {
    for (WUInt32 size = 1; size < 1024; ++size)
    {
      WBitfield<WDeque<WUInt32>> bf; // using a deque
      bf.SetCount(size, true);

      W_TEST_INT(bf.GetCount(), size);

      for (WUInt32 count = 0; count < bf.GetCount(); ++count)
        W_TEST_BOOL(bf.IsBitSet(count));

      WUInt32 uiStart = size / 2;
      WUInt32 uiEnd = WMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (WUInt32 count = 0; count < uiStart; ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
      for (WUInt32 count = uiStart; count <= uiEnd; ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));
      for (WUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FlipBitRange")
  {
    for (WUInt32 size = 1; size < 1024; ++size)
    {
      WBitfield<WDeque<WUInt32>> bf; // using a deque
      bf.SetCount(size, true);

      W_TEST_INT(bf.GetCount(), size);

      for (WUInt32 count = 0; count < bf.GetCount(); ++count)
        W_TEST_BOOL(bf.IsBitSet(count));

      WUInt32 uiStart = size / 2;
      WUInt32 uiEnd = WMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.FlipBitRange(uiStart, uiEnd - uiStart + 1);

      for (WUInt32 count = 0; count < uiStart; ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
      for (WUInt32 count = uiStart; count <= uiEnd; ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));
      for (WUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    WHybridBitfield<512> bf;                  // using a hybrid array

    W_TEST_BOOL(bf.IsEmpty() == true);
    W_TEST_BOOL(bf.IsAnyBitSet() == false);   // empty
    W_TEST_BOOL(bf.IsNoBitSet() == true);
    W_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    bf.SetCount(250, false);

    W_TEST_BOOL(bf.IsEmpty() == false);
    W_TEST_BOOL(bf.IsAnyBitSet() == false);
    W_TEST_BOOL(bf.IsNoBitSet() == true);
    W_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (WUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    W_TEST_BOOL(bf.IsEmpty() == false);
    W_TEST_BOOL(bf.IsAnyBitSet() == true);
    W_TEST_BOOL(bf.IsNoBitSet() == false);
    W_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (WUInt32 i = 0; i < bf.GetCount(); i++)
      bf.SetBit(i);

    W_TEST_BOOL(bf.IsAnyBitSet() == true);
    W_TEST_BOOL(bf.IsNoBitSet() == false);
    W_TEST_BOOL(bf.AreAllBitsSet() == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WHybridBitfield<512> bf0; // using a hybrid array
    WHybridBitfield<512> bf1; // using a hybrid array

    WUInt32 bitFieldCount0 = 100;
    WUInt32 bitFieldCount1 = 999;
    WUInt32 bitIndexSet0 = 2;
    WUInt32 bitIndexSet1 = 555;

    bf0.SetCount(bitFieldCount0, false);
    bf1.SetCount(bitFieldCount1, false);
    bf0.SetBit(bitIndexSet0);
    bf1.SetBit(bitIndexSet1);

    W_TEST_BOOL(bitFieldCount0 == bf0.GetCount());
    for (WUInt32 i = 0; i < bf0.GetCount(); ++i)
    {
      W_TEST_BOOL(bf0.IsBitSet(i) == (bitIndexSet0 == i));
    }

    W_TEST_BOOL(bitFieldCount1 == bf1.GetCount());
    for (WUInt32 i = 0; i < bf1.GetCount(); ++i)
    {
      W_TEST_BOOL(bf1.IsBitSet(i) == (bitIndexSet1 == i));
    }

    bf0.Swap(bf1);
    WMath::Swap(bitIndexSet0, bitIndexSet1);
    WMath::Swap(bitFieldCount0, bitFieldCount1);

    W_TEST_BOOL(bitFieldCount0 == bf0.GetCount());
    for (WUInt32 i = 0; i < bf0.GetCount(); ++i)
    {
      W_TEST_BOOL(bf0.IsBitSet(i) == (bitIndexSet0 == i));
    }

    W_TEST_BOOL(bitFieldCount1 == bf1.GetCount());
    for (WUInt32 i = 0; i < bf1.GetCount(); ++i)
    {
      W_TEST_BOOL(bf1.IsBitSet(i) == (bitIndexSet1 == i));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Iterator")
  {
    {
      // Check empty bitfields of varying sizes.
      for (WUInt32 uiNumBits = 0; uiNumBits <= 65; ++uiNumBits)
      {
        WHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, true);
        for (WUInt32 b = 0; b < uiNumBits; ++b)
        {
          bitfield.ClearBit(b);
        }
        for (WUInt32 uiBit : bitfield)
        {
          W_TEST_BOOL_MSG(false, "No bit should be set");
        }

        for (auto it = bitfield.GetIterator(); it.IsValid(); it.Next())
        {
          W_TEST_BOOL_MSG(false, "No bit should be set");
        }
        W_TEST_BOOL(bitfield.GetIterator() == bitfield.GetEndIterator());
        W_TEST_BOOL(!bitfield.GetIterator().IsValid());
        W_TEST_BOOL(!bitfield.GetEndIterator().IsValid());
      }
    }

    {
      // Full bits.
      for (WUInt32 uiNumBits = 0; uiNumBits <= 65; ++uiNumBits)
      {
        WHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, true);
        WUInt32 uiNextBit = 0;
        for (WUInt32 uiBit : bitfield)
        {
          W_TEST_INT(uiBit, uiNextBit);
          uiNextBit++;
        }
        W_TEST_INT(uiNumBits, uiNextBit);

        uiNextBit = 0;
        for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
        {
          W_TEST_INT(it.Value(), uiNextBit);
          W_TEST_INT(*it, uiNextBit);
          W_TEST_BOOL(it.IsValid());
          uiNextBit++;
        }
        W_TEST_INT(uiNumBits, uiNextBit);
      }
    }

    {
      // Partial bits set.
      WRandom rnd;
      rnd.Initialize(42);

      for (WUInt32 uiNumBits = 2; uiNumBits <= 65; ++uiNumBits)
      {
        WHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, false);

        // Add some random bits and ensure they appear in the iterator in order.
        WTempHybridArray<WUInt32, 3> bits;
        for (int i = 0; i < uiNumBits / 2; ++i)
        {
          WUInt32 bit = (WUInt32)rnd.IntMinMax(0, uiNumBits - 1);
          if (!bitfield.IsBitSet(bit))
          {
            bits.PushBack(bit);
            bitfield.SetBit(bit);
          }
        }
        bits.Sort();

        for (WUInt32 uiBit : bitfield)
        {
          W_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        W_TEST_BOOL(bits.IsEmpty());
      }
    }
  }
}


W_CREATE_SIMPLE_TEST(Containers, StaticBitfield)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "SetAllBits / ClearAllBits")
  {
    WStaticBitfield64 bf;

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      W_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      W_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      W_TEST_BOOL(!bf.IsBitSet(i));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetBit / ClearBit / SetBitValue")
  {
    WStaticBitfield32 bf;

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      W_TEST_BOOL(!bf.IsBitSet(i));

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
      bf.SetBit(i);

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      W_TEST_BOOL(bf.IsBitSet(i));
      W_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      W_TEST_BOOL(!bf.IsBitSet(i));
      W_TEST_BOOL(bf.IsBitSet(i + 1));
    }

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
    {
      W_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetBitRange")
  {
    for (WUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      WStaticBitfield64 bf;

      for (WUInt32 count = 0; count < bf.GetStorageTypeBitCount(); ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));

      WUInt32 uiEnd = uiStart + 3;

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (WUInt32 count = 0; count < uiStart; ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));
      for (WUInt32 count = uiStart; count <= uiEnd; ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
      for (WUInt32 count = uiEnd + 1; count < bf.GetStorageTypeBitCount(); ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClearBitRange")
  {
    for (WUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      WStaticBitfield64 bf;
      bf.SetAllBits();

      for (WUInt32 count = 0; count < bf.GetStorageTypeBitCount(); ++count)
        W_TEST_BOOL(bf.IsBitSet(count));

      WUInt32 uiEnd = uiStart + 3;

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (WUInt32 count = 0; count < uiStart; ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
      for (WUInt32 count = uiStart; count <= uiEnd; ++count)
        W_TEST_BOOL(!bf.IsBitSet(count));
      for (WUInt32 count = uiEnd + 1; count < bf.GetStorageTypeBitCount(); ++count)
        W_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    WStaticBitfield8 bf;

    W_TEST_BOOL(bf.IsAnyBitSet() == false);   // empty
    W_TEST_BOOL(bf.IsNoBitSet() == true);
    W_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
      bf.SetBit(i);

    W_TEST_BOOL(bf.IsAnyBitSet() == true);
    W_TEST_BOOL(bf.IsNoBitSet() == false);
    W_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (WUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i++)
      bf.SetBit(i);

    W_TEST_BOOL(bf.IsAnyBitSet() == true);
    W_TEST_BOOL(bf.IsNoBitSet() == false);
    W_TEST_BOOL(bf.AreAllBitsSet() == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetNumBitsSet")
  {
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0).GetNumBitsSet(), 0);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xff).GetNumBitsSet(), 8);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xffff).GetNumBitsSet(), 16);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xffffffffu).GetNumBitsSet(), 32);
    W_TEST_INT(WStaticBitfield64::MakeFromMask(0).GetNumBitsSet(), 0);
    W_TEST_INT(WStaticBitfield64::MakeFromMask(0xff).GetNumBitsSet(), 8);
    W_TEST_INT(WStaticBitfield64::MakeFromMask(0xffff).GetNumBitsSet(), 16);
    W_TEST_INT(WStaticBitfield64::MakeFromMask(0xffffffffu).GetNumBitsSet(), 32);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLowestBitSet")
  {
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0u).GetLowestBitSet(), 32);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(1u).GetLowestBitSet(), 0);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xffu).GetLowestBitSet(), 0);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xff00u).GetLowestBitSet(), 8);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xff0000u).GetLowestBitSet(), 16);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xff000000u).GetLowestBitSet(), 24);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0x80000000u).GetLowestBitSet(), 31);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xffffffffu).GetLowestBitSet(), 0);
    W_TEST_INT(WStaticBitfield64::MakeFromMask(0xffffffffffffffffull).GetLowestBitSet(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetHighestBitSet")
  {
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0u).GetHighestBitSet(), 32);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(1u).GetHighestBitSet(), 0);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xffu).GetHighestBitSet(), 7);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xff00u).GetHighestBitSet(), 15);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xff0000u).GetHighestBitSet(), 23);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xff000000u).GetHighestBitSet(), 31);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0x80000000u).GetHighestBitSet(), 31);
    W_TEST_INT(WStaticBitfield32::MakeFromMask(0xffffffffu).GetHighestBitSet(), 31);
    W_TEST_INT(WStaticBitfield64::MakeFromMask(0xffffffffffffffffull).GetHighestBitSet(), 63);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Iterator")
  {
    {
      // Empty bitfield
      WStaticBitfield32 bitfield = WStaticBitfield32::MakeFromMask(0u);
      for (WUInt32 uiBit : bitfield)
      {
        W_TEST_BOOL_MSG(false, "No bit should be set");
      }
      for (auto it = bitfield.GetIterator(); it.IsValid(); it.Next())
      {
        W_TEST_BOOL_MSG(false, "No bit should be set");
      }
      W_TEST_BOOL(bitfield.GetIterator() == bitfield.GetEndIterator());
      W_TEST_BOOL(!bitfield.GetIterator().IsValid());
      W_TEST_BOOL(!bitfield.GetEndIterator().IsValid());

      WStaticBitfield64 bitfield64 = WStaticBitfield64::MakeFromMask(0u);
      for (WUInt32 uiBit : bitfield64)
      {
        W_TEST_BOOL_MSG(false, "No bit should be set");
      }
      for (auto it = bitfield64.GetIterator(); it.IsValid(); it.Next())
      {
        W_TEST_BOOL_MSG(false, "No bit should be set");
      }
      W_TEST_BOOL(bitfield64.GetIterator() == bitfield64.GetEndIterator());
      W_TEST_BOOL(!bitfield64.GetIterator().IsValid());
      W_TEST_BOOL(!bitfield64.GetEndIterator().IsValid());
    }

    {
      // Full 32 bits
      WStaticBitfield32 bitfield = WStaticBitfield32::MakeFromMask(0xffffffffu);
      WUInt32 uiNextBit = 0;
      for (WUInt32 uiBit : bitfield)
      {
        W_TEST_INT(uiBit, uiNextBit);
        uiNextBit++;
      }
      W_TEST_INT(32, uiNextBit);

      uiNextBit = 0;
      for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
      {
        W_TEST_INT(it.Value(), uiNextBit);
        W_TEST_INT(*it, uiNextBit);
        W_TEST_BOOL(it.IsValid());
        uiNextBit++;
      }
      W_TEST_INT(32, uiNextBit);
    }

    {
      // Full 64 bits
      WStaticBitfield64 bitfield = WStaticBitfield64::MakeFromMask(0xffffffffffffffffull);
      WUInt32 uiNextBit = 0;
      for (WUInt32 uiBit : bitfield)
      {
        W_TEST_INT(uiBit, uiNextBit);
        uiNextBit++;
      }
      W_TEST_INT(64, uiNextBit);

      uiNextBit = 0;
      for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
      {
        W_TEST_INT(it.Value(), uiNextBit);
        W_TEST_INT(*it, uiNextBit);
        W_TEST_BOOL(it.IsValid());
        uiNextBit++;
      }
      W_TEST_INT(64, uiNextBit);
    }

    {
      // Partial bits set 32 bit.
      WRandom rnd;
      rnd.Initialize(42);

      for (WUInt32 uiNumBits = 2; uiNumBits <= 32; ++uiNumBits)
      {
        // Add some random bits and ensure they appear in the iterator in order.
        WTempHybridArray<WUInt32, 3> bits;
        WUInt32 uiBits = 0;
        for (int i = 0; i < uiNumBits; ++i)
        {
          const WUInt32 bit = (WUInt32)rnd.IntMinMax(0, 31);
          if (!bits.Contains(bit))
          {
            bits.PushBack(bit);
            uiBits |= W_BIT(bit);
          }
        }
        bits.Sort();

        WStaticBitfield32 bitfield = WStaticBitfield32::MakeFromMask(uiBits);

        for (WUInt32 uiBit : bitfield)
        {
          W_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        W_TEST_BOOL(bits.IsEmpty());
      }
    }

    {
      // Partial bits set 64 bit.
      WRandom rnd;
      rnd.Initialize(42);

      for (WUInt32 uiNumBits = 2; uiNumBits <= 63; ++uiNumBits)
      {
        // Add some random bits and ensure they appear in the iterator in order.
        WTempHybridArray<WUInt32, 3> bits;
        WUInt64 uiBits = 0;
        for (int i = 0; i < uiNumBits; ++i)
        {
          const WUInt32 bit = (WUInt32)rnd.IntMinMax(0, 63);
          if (!bits.Contains(bit))
          {
            bits.PushBack(bit);
            uiBits |= W_BIT(bit);
          }
        }
        bits.Sort();

        WStaticBitfield64 bitfield = WStaticBitfield64::MakeFromMask(uiBits);

        for (WUInt32 uiBit : bitfield)
        {
          W_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        W_TEST_BOOL(bits.IsEmpty());
      }
    }
  }
}
