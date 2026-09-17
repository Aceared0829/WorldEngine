#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Strings/HashedString.h>

W_CREATE_SIMPLE_TEST_GROUP(Algorithm);

// Warning for overflow in compile time executed static_assert(WHashingUtils::MurmurHash32...)
// Todo: Why is this not happening elsewhere?
#pragma warning(disable : 4307)

W_CREATE_SIMPLE_TEST(Algorithm, Hashing)
{
  // check whether compile time hashing gives the same value as runtime hashing
  const char* szString = "This is a test string. 1234";
  const char* szStringLower = "this is a test string. 1234";
  const char* szString2 = "THiS iS A TESt sTrInG. 1234";
  WStringBuilder sb = szString;

  W_TEST_BLOCK(WTestBlock::Enabled, "Hashfunction")
  {
    WUInt32 uiHashRT = WHashingUtils::MurmurHash32String(sb.GetData());
    constexpr WUInt32 uiHashCT = WHashingUtils::MurmurHash32String("This is a test string. 1234");
    W_TEST_INT(uiHashRT, 0xb999d6c4);
    W_TEST_INT(uiHashRT, uiHashCT);

    // Static assert to ensure this is happening at compile time!
    static_assert(WHashingUtils::MurmurHash32String("This is a test string. 1234") == static_cast<WUInt32>(0xb999d6c4), "Error in compile time murmur hash calculation!");

    {
      // Test short inputs (< 16 characters) of xx hash at compile time
      WUInt32 uixxHashRT = WHashingUtils::xxHash32("Test string", 11, 0);
      WUInt32 uixxHashCT = WHashingUtils::xxHash32String("Test string", 0);
      W_TEST_INT(uixxHashRT, uixxHashCT);
      static_assert(WHashingUtils::xxHash32String("Test string") == 0x1b50ee03);

      // Test long inputs ( > 16 characters) of xx hash at compile time
      WUInt32 uixxHashRTLong = WHashingUtils::xxHash32String(sb.GetData());
      WUInt32 uixxHashCTLong = WHashingUtils::xxHash32String("This is a test string. 1234");
      W_TEST_INT(uixxHashRTLong, uixxHashCTLong);
      static_assert(WHashingUtils::xxHash32String("This is a test string. 1234") == 0xff35b049);
    }

    {
      // Test short inputs (< 32 characters) of xx hash 64 at compile time
      WUInt64 uixxHash64RT = WHashingUtils::xxHash64("Test string", 11, 0);
      WUInt64 uixxHash64CT = WHashingUtils::xxHash64String("Test string", 0);
      W_TEST_INT(uixxHash64RT, uixxHash64CT);
      static_assert(WHashingUtils::xxHash64String("Test string") == 0xcf0f91eece7c88feULL);

      // Test long inputs ( > 32 characters) of xx hash 64 at compile time
      WUInt64 uixxHash64RTLong = WHashingUtils::xxHash64String(WStringView("This is a longer test string for 64-bit. 123456"));
      WUInt64 uixxHash64CTLong = WHashingUtils::xxHash64String("This is a longer test string for 64-bit. 123456");
      W_TEST_INT(uixxHash64RTLong, uixxHash64CTLong);
      static_assert(WHashingUtils::xxHash64String("This is a longer test string for 64-bit. 123456") == 0xb85d007925299bacULL);
    }

    {
      // Test short inputs (< 32 characters) of xx hash 64 at compile time
      WUInt64 uixxHash64RT = WHashingUtils::StringHash(WStringView("Test string"));
      WUInt64 uixxHash64CT = WHashingUtils::StringHash("Test string");
      W_TEST_INT(uixxHash64RT, uixxHash64CT);
      static_assert(WHashingUtils::StringHash("Test string") == 0xcf0f91eece7c88feULL);

      // Test long inputs ( > 32 characters) of xx hash 64 at compile time
      WUInt64 uixxHash64RTLong = WHashingUtils::StringHash(WStringView("This is a longer test string for 64-bit. 123456"));
      WUInt64 uixxHash64CTLong = WHashingUtils::StringHash("This is a longer test string for 64-bit. 123456");
      W_TEST_INT(uixxHash64RTLong, uixxHash64CTLong);
      static_assert(WHashingUtils::StringHash("This is a longer test string for 64-bit. 123456") == 0xb85d007925299bacULL);
    }

    // Check MurmurHash for unaligned inputs
    const char* alignmentTestString = "12345678_12345678__12345678___12345678";
    WUInt32 uiHash1 = WHashingUtils::MurmurHash32(alignmentTestString, 8);
    WUInt32 uiHash2 = WHashingUtils::MurmurHash32(alignmentTestString + 9, 8);
    WUInt32 uiHash3 = WHashingUtils::MurmurHash32(alignmentTestString + 19, 8);
    WUInt32 uiHash4 = WHashingUtils::MurmurHash32(alignmentTestString + 30, 8);
    W_TEST_INT(uiHash1, uiHash2);
    W_TEST_INT(uiHash1, uiHash3);
    W_TEST_INT(uiHash1, uiHash4);

    // check 64bit hashes
    const WUInt64 uiMurmurHash64 = WHashingUtils::MurmurHash64(sb.GetData(), sb.GetElementCount());
    W_TEST_INT(uiMurmurHash64, 0xf8ebc5e8cb110786);

    // Check MurmurHash64 for unaligned inputs
    WUInt64 uiHash1_64 = WHashingUtils::MurmurHash64(alignmentTestString, 8);
    WUInt64 uiHash2_64 = WHashingUtils::MurmurHash64(alignmentTestString + 9, 8);
    WUInt64 uiHash3_64 = WHashingUtils::MurmurHash64(alignmentTestString + 19, 8);
    WUInt64 uiHash4_64 = WHashingUtils::MurmurHash64(alignmentTestString + 30, 8);
    W_TEST_INT(uiHash1_64, uiHash2_64);
    W_TEST_INT(uiHash1_64, uiHash3_64);
    W_TEST_INT(uiHash1_64, uiHash4_64);

    // test crc32
    const WUInt32 uiCrc32 = WHashingUtils::CRC32Hash(sb.GetData(), sb.GetElementCount());
    W_TEST_INT(uiCrc32, 0x73b5e898);

    // Check crc32 for unaligned inputs
    uiHash1 = WHashingUtils::CRC32Hash(alignmentTestString, 8);
    uiHash2 = WHashingUtils::CRC32Hash(alignmentTestString + 9, 8);
    uiHash3 = WHashingUtils::CRC32Hash(alignmentTestString + 19, 8);
    uiHash4 = WHashingUtils::CRC32Hash(alignmentTestString + 30, 8);
    W_TEST_INT(uiHash1, uiHash2);
    W_TEST_INT(uiHash1, uiHash3);
    W_TEST_INT(uiHash1, uiHash4);

    // 32 Bit xxHash
    const WUInt32 uiXXHash32 = WHashingUtils::xxHash32(sb.GetData(), sb.GetElementCount());
    W_TEST_INT(uiXXHash32, 0xff35b049);

    // Check xxHash for unaligned inputs
    uiHash1 = WHashingUtils::xxHash32(alignmentTestString, 8);
    uiHash2 = WHashingUtils::xxHash32(alignmentTestString + 9, 8);
    uiHash3 = WHashingUtils::xxHash32(alignmentTestString + 19, 8);
    uiHash4 = WHashingUtils::xxHash32(alignmentTestString + 30, 8);
    W_TEST_INT(uiHash1, uiHash2);
    W_TEST_INT(uiHash1, uiHash3);
    W_TEST_INT(uiHash1, uiHash4);

    // 64 Bit xxHash
    const WUInt64 uiXXHash64 = WHashingUtils::xxHash64(sb.GetData(), sb.GetElementCount());
    W_TEST_INT(uiXXHash64, 0x141fb89c0bf32020);
    // Check xxHash64 for unaligned inputs
    uiHash1_64 = WHashingUtils::xxHash64(alignmentTestString, 8);
    uiHash2_64 = WHashingUtils::xxHash64(alignmentTestString + 9, 8);
    uiHash3_64 = WHashingUtils::xxHash64(alignmentTestString + 19, 8);
    uiHash4_64 = WHashingUtils::xxHash64(alignmentTestString + 30, 8);
    W_TEST_INT(uiHash1_64, uiHash2_64);
    W_TEST_INT(uiHash1_64, uiHash3_64);
    W_TEST_INT(uiHash1_64, uiHash4_64);

    WUInt32 uixxHash32RTEmpty = WHashingUtils::xxHash32("", 0, 0);
    WUInt32 uixxHash32CTEmpty = WHashingUtils::xxHash32String("", 0);
    W_TEST_BOOL(uixxHash32RTEmpty == uixxHash32CTEmpty);

    WUInt64 uixxHash64RTEmpty = WHashingUtils::xxHash64("", 0, 0);
    WUInt64 uixxHash64CTEmpty = WHashingUtils::xxHash64String("", 0);
    W_TEST_BOOL(uixxHash64RTEmpty == uixxHash64CTEmpty);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HashHelper")
  {
    WUInt32 uiHash = WHashHelper<WStringBuilder>::Hash(sb);
    W_TEST_INT(uiHash, 0x0bf32020);

    const char* szTest = "This is a test string. 1234";
    uiHash = WHashHelper<const char*>::Hash(szTest);
    W_TEST_INT(uiHash, 0x0bf32020);
    W_TEST_BOOL(WHashHelper<const char*>::Equal(szTest, sb.GetData()));

    WHashedString hs;
    hs.Assign(szTest);
    uiHash = WHashHelper<WHashedString>::Hash(hs);
    W_TEST_INT(uiHash, 0x0bf32020);

    WTempHashedString ths(szTest);
    uiHash = WHashHelper<WHashedString>::Hash(ths);
    W_TEST_INT(uiHash, 0x0bf32020);
    W_TEST_BOOL(WHashHelper<WHashedString>::Equal(hs, ths));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HashHelperString_NoCase")
  {
    const WUInt32 uiHash = WHashHelper<const char*>::Hash(szStringLower);
    W_TEST_INT(uiHash, 0x19404167);
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(szString));
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(szStringLower));
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(szString2));
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(sb));
    WStringBuilder sb2 = szString2;
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(sb2));
    WString sL = szStringLower;
    WString s1 = sb;
    WString s2 = sb2;
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(s1));
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(s2));
    WStringView svL = szStringLower;
    WStringView sv1 = szString;
    WStringView sv2 = szString2;
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(svL));
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(sv1));
    W_TEST_INT(uiHash, WHashHelperString_NoCase::Hash(sv2));

    W_TEST_BOOL(WHashHelperString_NoCase::Equal(sb, sb2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(sb, szString2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(sb, sv2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(s1, sb2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(s1, szString2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(s1, sv2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(sv1, sb2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(sv1, szString2));
    W_TEST_BOOL(WHashHelperString_NoCase::Equal(sv1, sv2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HashStream32")
  {
    const char* szTest = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool bFlush, WUInt32* pHash)
    {
      WHashStreamWriter32 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest)).IgnoreResult();
      if (bFlush)
      {
        writer1.Flush().IgnoreResult();
      }

      const WUInt32 uiHash1 = writer1.GetHashValue();

      WHashStreamWriter32 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1)).IgnoreResult();
      if (bFlush)
      {
        writer2.Flush().IgnoreResult();
      }

      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2)).IgnoreResult();
      if (bFlush)
      {
        writer2.Flush().IgnoreResult();
      }

      const WUInt32 uiHash2 = writer2.GetHashValue();

      WHashStreamWriter32 writer3;
      for (WUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1).IgnoreResult();

        if (bFlush)
        {
          writer3.Flush().IgnoreResult();
        }
      }
      const WUInt32 uiHash3 = writer3.GetHashValue();

      W_TEST_INT(uiHash1, uiHash2);
      W_TEST_INT(uiHash1, uiHash3);

      *pHash = uiHash1;
    };

    WUInt32 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    W_TEST_INT(uiHash1, uiHash2);

    const WUInt64 uiHash3 = WHashingUtils::xxHash32(szTest, std::strlen(szTest));
    W_TEST_INT(uiHash1, uiHash3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HashStream64")
  {
    const char* szTest = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool bFlush, WUInt64* pHash)
    {
      WHashStreamWriter64 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest)).IgnoreResult();

      if (bFlush)
      {
        writer1.Flush().IgnoreResult();
      }

      const WUInt64 uiHash1 = writer1.GetHashValue();

      WHashStreamWriter64 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1)).IgnoreResult();
      if (bFlush)
        writer2.Flush().IgnoreResult();
      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2)).IgnoreResult();
      if (bFlush)
        writer2.Flush().IgnoreResult();

      const WUInt64 uiHash2 = writer2.GetHashValue();

      WHashStreamWriter64 writer3;
      for (WUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1).IgnoreResult();
        if (bFlush)
          writer3.Flush().IgnoreResult();
      }
      const WUInt64 uiHash3 = writer3.GetHashValue();

      W_TEST_INT(uiHash1, uiHash2);
      W_TEST_INT(uiHash1, uiHash3);

      *pHash = uiHash1;
    };

    WUInt64 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    W_TEST_INT(uiHash1, uiHash2);

    const WUInt64 uiHash3 = WHashingUtils::xxHash64(szTest, std::strlen(szTest));
    W_TEST_INT(uiHash1, uiHash3);
  }
}

struct SimpleHashableStruct : public WHashableStruct<SimpleHashableStruct>
{
  WUInt32 m_uiTestMember1;
  WUInt8 m_uiTestMember2;
  WUInt64 m_uiTestMember3;
};

struct SimpleStruct
{
  WUInt32 m_uiTestMember1;
  WUInt8 m_uiTestMember2;
  WUInt64 m_uiTestMember3;
};

W_CREATE_SIMPLE_TEST(Algorithm, HashableStruct)
{
  SimpleHashableStruct AutomaticInst;
  W_TEST_INT(AutomaticInst.m_uiTestMember1, 0);
  W_TEST_INT(AutomaticInst.m_uiTestMember2, 0);
  W_TEST_INT(AutomaticInst.m_uiTestMember3, 0);

  SimpleStruct NonAutomaticInst;
  WMemoryUtils::ZeroFill(&NonAutomaticInst, 1);

  static_assert(sizeof(AutomaticInst) == sizeof(NonAutomaticInst));

  W_TEST_INT(WMemoryUtils::Compare<WUInt8>((WUInt8*)&AutomaticInst, (WUInt8*)&NonAutomaticInst, sizeof(AutomaticInst)), 0);

  AutomaticInst.m_uiTestMember2 = 0x42u;
  AutomaticInst.m_uiTestMember3 = 0x23u;

  WUInt32 uiAutomaticHash = AutomaticInst.CalculateHash();

  NonAutomaticInst.m_uiTestMember2 = 0x42u;
  NonAutomaticInst.m_uiTestMember3 = 0x23u;

  WUInt32 uiNonAutomaticHash = WHashingUtils::xxHash32(&NonAutomaticInst, sizeof(NonAutomaticInst));

  W_TEST_INT(uiAutomaticHash, uiNonAutomaticHash);

  AutomaticInst.m_uiTestMember1 = 0x5u;
  uiAutomaticHash = AutomaticInst.CalculateHash();

  W_TEST_BOOL(uiAutomaticHash != uiNonAutomaticHash);

  W_TEST_BLOCK(WTestBlock::Enabled, "Compare")
  {
    SimpleHashableStruct LeftSide;
    LeftSide.m_uiTestMember1 = 1;
    LeftSide.m_uiTestMember2 = 2;
    LeftSide.m_uiTestMember3 = 3;
    SimpleHashableStruct RightSide = LeftSide;
    W_TEST_BOOL(LeftSide == RightSide);
    W_TEST_BOOL(!(LeftSide != RightSide));
    W_TEST_BOOL(!(LeftSide < RightSide));
    W_TEST_BOOL(!(RightSide < LeftSide));
    W_TEST_BOOL(LeftSide.CalculateHash() == RightSide.CalculateHash());

    LeftSide.m_uiTestMember3 = 2;
    W_TEST_BOOL(!(LeftSide == RightSide));
    W_TEST_BOOL(LeftSide != RightSide);
    W_TEST_BOOL(LeftSide < RightSide);
    W_TEST_BOOL(!(RightSide < LeftSide));
    W_TEST_BOOL(LeftSide.CalculateHash() != RightSide.CalculateHash());

    RightSide.m_uiTestMember1 = 0;
    W_TEST_BOOL(!(LeftSide == RightSide));
    W_TEST_BOOL(LeftSide != RightSide);
    W_TEST_BOOL(!(LeftSide < RightSide));
    W_TEST_BOOL(RightSide < LeftSide);
    W_TEST_BOOL(LeftSide.CalculateHash() != RightSide.CalculateHash());
  }
}
