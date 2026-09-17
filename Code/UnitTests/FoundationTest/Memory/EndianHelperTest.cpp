#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/EndianHelper.h>

namespace
{
  struct TempStruct
  {
    float fVal;
    WUInt32 uiDVal;
    WUInt16 uiWVal1;
    WUInt16 uiWVal2;
    char pad[4];
  };

  struct FloatAndInt
  {
    union
    {
      float fVal;
      WUInt32 uiVal;
    };
  };
} // namespace


W_CREATE_SIMPLE_TEST(Memory, Endian)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Basics")
  {
// Test if the IsBigEndian() delivers the same result as the #define
#if W_ENABLED(W_PLATFORM_LITTLE_ENDIAN)
    W_TEST_BOOL(!WEndianHelper::IsBigEndian());
#elif W_ENABLED(W_PLATFORM_BIG_ENDIAN)
    W_TEST_BOOL(WEndianHelper::IsBigEndian());
#endif

    // Test conversion functions for single elements
    W_TEST_BOOL(WEndianHelper::Switch(static_cast<WUInt16>(0x15FF)) == 0xFF15);
    W_TEST_BOOL(WEndianHelper::Switch(static_cast<WUInt32>(0x34AA12FF)) == 0xFF12AA34);
    W_TEST_BOOL(WEndianHelper::Switch(static_cast<WUInt64>(0x34AA12FFABC3421E)) == 0x1E42C3ABFF12AA34);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Switching Arrays")
  {
    WArrayPtr<WUInt16> p16BitArray = W_DEFAULT_NEW_ARRAY(WUInt16, 1024);
    WArrayPtr<WUInt16> p16BitArrayCopy = W_DEFAULT_NEW_ARRAY(WUInt16, 1024);

    WArrayPtr<WUInt32> p32BitArray = W_DEFAULT_NEW_ARRAY(WUInt32, 1024);
    WArrayPtr<WUInt32> p32BitArrayCopy = W_DEFAULT_NEW_ARRAY(WUInt32, 1024);

    WArrayPtr<WUInt64> p64BitArray = W_DEFAULT_NEW_ARRAY(WUInt64, 1024);
    WArrayPtr<WUInt64> p64BitArrayCopy = W_DEFAULT_NEW_ARRAY(WUInt64, 1024);

    for (WUInt32 i = 0; i < 1024; i++)
    {
      WInt32 iRand = rand();
      p16BitArray[i] = static_cast<WUInt16>(iRand);
      p32BitArray[i] = static_cast<WUInt32>(iRand);
      p64BitArray[i] = static_cast<WUInt64>(iRand | static_cast<WUInt64>((iRand % 3)) << 32);
    }

    p16BitArrayCopy.CopyFrom(p16BitArray);
    p32BitArrayCopy.CopyFrom(p32BitArray);
    p64BitArrayCopy.CopyFrom(p64BitArray);

    WEndianHelper::SwitchWords(p16BitArray.GetPtr(), 1024);
    WEndianHelper::SwitchDWords(p32BitArray.GetPtr(), 1024);
    WEndianHelper::SwitchQWords(p64BitArray.GetPtr(), 1024);

    for (WUInt32 i = 0; i < 1024; i++)
    {
      W_TEST_BOOL(p16BitArray[i] == WEndianHelper::Switch(p16BitArrayCopy[i]));
      W_TEST_BOOL(p32BitArray[i] == WEndianHelper::Switch(p32BitArrayCopy[i]));
      W_TEST_BOOL(p64BitArray[i] == WEndianHelper::Switch(p64BitArrayCopy[i]));

      // Test in place switcher
      WEndianHelper::SwitchInPlace(&p16BitArrayCopy[i]);
      W_TEST_BOOL(p16BitArray[i] == p16BitArrayCopy[i]);

      WEndianHelper::SwitchInPlace(&p32BitArrayCopy[i]);
      W_TEST_BOOL(p32BitArray[i] == p32BitArrayCopy[i]);

      WEndianHelper::SwitchInPlace(&p64BitArrayCopy[i]);
      W_TEST_BOOL(p64BitArray[i] == p64BitArrayCopy[i]);
    }


    W_DEFAULT_DELETE_ARRAY(p16BitArray);
    W_DEFAULT_DELETE_ARRAY(p16BitArrayCopy);

    W_DEFAULT_DELETE_ARRAY(p32BitArray);
    W_DEFAULT_DELETE_ARRAY(p32BitArrayCopy);

    W_DEFAULT_DELETE_ARRAY(p64BitArray);
    W_DEFAULT_DELETE_ARRAY(p64BitArrayCopy);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Switching Structs")
  {
    TempStruct instance = {42.0f, 0x34AA12FF, 0x15FF, 0x23FF, {'E', 'Z', 'F', 'T'}};

    WEndianHelper::SwitchStruct(&instance, "ddwwcccc");

    WIntFloatUnion floatHelper(42.0f);
    WIntFloatUnion floatHelper2(instance.fVal);

    W_TEST_BOOL(floatHelper2.i == WEndianHelper::Switch(floatHelper.i));
    W_TEST_BOOL(instance.uiDVal == WEndianHelper::Switch(static_cast<WUInt32>(0x34AA12FF)));
    W_TEST_BOOL(instance.uiWVal1 == WEndianHelper::Switch(static_cast<WUInt16>(0x15FF)));
    W_TEST_BOOL(instance.uiWVal2 == WEndianHelper::Switch(static_cast<WUInt16>(0x23FF)));
    W_TEST_BOOL(instance.pad[0] == 'E');
    W_TEST_BOOL(instance.pad[1] == 'Z');
    W_TEST_BOOL(instance.pad[2] == 'F');
    W_TEST_BOOL(instance.pad[3] == 'T');
  }
}
