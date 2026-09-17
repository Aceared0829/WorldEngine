#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/IterateBits.h>

namespace
{
  // declare bitflags using macro magic
  W_DECLARE_FLAGS(WUInt32, AutoFlags, Bit1, Bit2, Bit3, Bit4);

  // declare bitflags manually
  struct ManualFlags
  {
    using StorageType = WUInt32;

    enum Enum
    {
      Bit1 = W_BIT(0),
      Bit2 = W_BIT(1),
      Bit3 = W_BIT(2),
      Bit4 = W_BIT(3),

      Default = Bit1 | Bit2
    };

    struct Bits
    {
      StorageType Bit1 : 1;
      StorageType Bit2 : 1;
      StorageType Bit3 : 1;
      StorageType Bit4 : 1;
    };
  };

  W_DECLARE_FLAGS_OPERATORS(ManualFlags);
} // namespace

W_DEFINE_AS_POD_TYPE(AutoFlags::Enum);
static_assert(sizeof(WBitflags<AutoFlags>) == 4);


W_CREATE_SIMPLE_TEST(Basics, Bitflags)
{
  W_TEST_BOOL(AutoFlags::Count == 4);

  {
    WBitflags<AutoFlags> flags = AutoFlags::Bit1 | AutoFlags::Bit4;

    W_TEST_BOOL(flags.IsSet(AutoFlags::Bit4));
    W_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit1 | AutoFlags::Bit4));
    W_TEST_BOOL(flags.IsAnySet(AutoFlags::Bit1 | AutoFlags::Bit2));
    W_TEST_BOOL(!flags.IsAnySet(AutoFlags::Bit2 | AutoFlags::Bit3));
    W_TEST_BOOL(flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit3));
    W_TEST_BOOL(!flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit4));

    flags.Add(AutoFlags::Bit3);
    W_TEST_BOOL(flags.IsSet(AutoFlags::Bit3));

    flags.Remove(AutoFlags::Bit1);
    W_TEST_BOOL(!flags.IsSet(AutoFlags::Bit1));

    flags.Toggle(AutoFlags::Bit4);
    W_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit3));

    flags.AddOrRemove(AutoFlags::Bit2, true);
    flags.AddOrRemove(AutoFlags::Bit3, false);
    W_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit2));

    flags.Add(AutoFlags::Bit1);

    WBitflags<ManualFlags> manualFlags = ManualFlags::Default;
    W_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Bit1 | ManualFlags::Bit2));
    W_TEST_BOOL(manualFlags.GetValue() == flags.GetValue());
    W_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Default & ManualFlags::Bit2));

    W_TEST_BOOL(flags.IsAnyFlagSet());
    flags.Clear();
    W_TEST_BOOL(flags.IsNoFlagSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator&")
  {
    WBitflags<AutoFlags> flags2 = AutoFlags::Bit1 & AutoFlags::Bit4;
    W_TEST_BOOL(flags2.GetValue() == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetValue")
  {
    WBitflags<AutoFlags> flags;
    flags.SetValue(17);
    W_TEST_BOOL(flags.GetValue() == 17);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator|=")
  {
    WBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2;
    f |= AutoFlags::Bit3;

    W_TEST_BOOL(f.GetValue() == (AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3).GetValue());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator&=")
  {
    WBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3;
    f &= AutoFlags::Bit3;

    W_TEST_BOOL(f.GetValue() == AutoFlags::Bit3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Iterator")
  {
    {
      // Empty
      WBitflags<AutoFlags> f;
      auto it = f.GetIterator();
      W_TEST_BOOL(it == f.GetEndIterator());
      W_TEST_BOOL(!it.IsValid());

      for (AutoFlags::Enum flag : f)
      {
        W_TEST_BOOL_MSG(false, "No bit should be set");
      }
    }

    {
      // All flags
      WBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3 | AutoFlags::Bit4;
      WTempHybridArray<AutoFlags::Enum, 4> flags;
      flags.PushBack(AutoFlags::Bit1);
      flags.PushBack(AutoFlags::Bit2);
      flags.PushBack(AutoFlags::Bit3);
      flags.PushBack(AutoFlags::Bit4);

      WUInt32 uiIndex = 0;
      // Iterator
      for (auto it = f.GetIterator(); it.IsValid(); ++it)
      {
        W_TEST_INT(*it, flags[uiIndex]);
        W_TEST_INT(it.Value(), flags[uiIndex]);
        W_TEST_BOOL(it.IsValid());
        ++uiIndex;
      }
      W_TEST_INT(uiIndex, 4);

      // Range-base for loop
      uiIndex = 0;
      for (AutoFlags::Enum flag : f)
      {
        W_TEST_INT(flag, flags[uiIndex]);
        ++uiIndex;
      }
      W_TEST_INT(uiIndex, 4);
    }
  }
}


//////////////////////////////////////////////////////////////////////////

namespace
{
  struct TypelessFlags1
  {
    enum Enum
    {
      Bit1 = W_BIT(0),
      Bit2 = W_BIT(1),
    };
  };

  struct TypelessFlags2
  {
    enum Enum
    {
      Bit3 = W_BIT(2),
      Bit4 = W_BIT(3),
    };
  };
} // namespace
