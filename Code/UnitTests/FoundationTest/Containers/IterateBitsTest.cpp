#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/IterateBits.h>

namespace
{
  template <typename T>
  void TestEmptyIntegerBitValues()
  {
    WUInt32 uiNextBit = 1;
    for (auto bit : WIterateBitValues(static_cast<T>(0)))
    {
      W_TEST_BOOL_MSG(false, "No bit should be present");
    }
  }

  template <typename T>
  void TestFullIntegerBitValues()
  {
    constexpr WUInt64 uiBitCount = sizeof(T) * 8;
    WUInt64 uiNextBit = 1;
    WUInt64 uiCount = 0;
    for (auto bit : WIterateBitValues(WMath::MaxValue<T>()))
    {
      W_TEST_INT(bit, uiNextBit);
      uiNextBit *= 2;
      uiCount++;
    }
    W_TEST_INT(uiBitCount, uiCount);
  }

  template <typename T>
  void TestEmptyIntegerBitIndices()
  {
    WUInt32 uiNextBit = 1;
    for (auto bit : WIterateBitIndices(static_cast<T>(0)))
    {
      W_TEST_BOOL_MSG(false, "No bit should be present");
    }
  }

  template <typename T>
  void TestFullIntegerBitIndices()
  {
    constexpr WUInt64 uiBitCount = sizeof(T) * 8;
    WUInt64 uiNextBitIndex = 0;
    for (auto bit : WIterateBitIndices(WMath::MaxValue<T>()))
    {
      W_TEST_INT(bit, uiNextBitIndex);
      ++uiNextBitIndex;
    }
    W_TEST_INT(uiBitCount, uiNextBitIndex);
  }
} // namespace

W_CREATE_SIMPLE_TEST(Containers, IterateBits)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "WIterateBitValues")
  {
    {
      // Empty set
      TestEmptyIntegerBitValues<WUInt8>();
      TestEmptyIntegerBitValues<WUInt16>();
      TestEmptyIntegerBitValues<WUInt32>();
      TestEmptyIntegerBitValues<WUInt64>();
    }

    {
      // Full sets
      TestFullIntegerBitValues<WUInt8>();
      TestFullIntegerBitValues<WUInt16>();
      TestFullIntegerBitValues<WUInt32>();
      TestFullIntegerBitValues<WUInt64>();
    }

    {
      // Some bits set
      WUInt64 uiBitMask = 0b1101;
      WTempHybridArray<WUInt64, 3> bits;
      bits.PushBack(0b0001);
      bits.PushBack(0b0100);
      bits.PushBack(0b1000);

      for (WUInt64 bit : WIterateBitValues(uiBitMask))
      {
        W_TEST_INT(bit, bits[0]);
        bits.RemoveAtAndCopy(0);
      }
      W_TEST_BOOL(bits.IsEmpty());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WIterateBitIndices")
  {
    {
      // Empty set
      TestEmptyIntegerBitIndices<WUInt8>();
      TestEmptyIntegerBitIndices<WUInt16>();
      TestEmptyIntegerBitIndices<WUInt32>();
      TestEmptyIntegerBitIndices<WUInt64>();
    }

    {
      // Full sets
      TestFullIntegerBitIndices<WUInt8>();
      TestFullIntegerBitIndices<WUInt16>();
      TestFullIntegerBitIndices<WUInt32>();
      TestFullIntegerBitIndices<WUInt64>();
    }

    {
      // Some bits set
      WUInt64 uiBitMask = 0b1101;
      WTempHybridArray<WUInt64, 3> bits;
      bits.PushBack(0);
      bits.PushBack(2);
      bits.PushBack(3);

      for (WUInt64 bit : WIterateBitIndices(uiBitMask))
      {
        W_TEST_INT(bit, bits[0]);
        bits.RemoveAtAndCopy(0);
      }
      W_TEST_BOOL(bits.IsEmpty());
    }
  }
}
