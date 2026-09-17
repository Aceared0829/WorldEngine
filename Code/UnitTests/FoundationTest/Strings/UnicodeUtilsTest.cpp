#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

W_CREATE_SIMPLE_TEST(Strings, UnicodeUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "IsASCII")
  {
    // test all ASCII Characters
    for (WUInt32 i = 0; i < 128; ++i)
      W_TEST_BOOL(WUnicodeUtils::IsASCII(i));

    for (WUInt32 i = 128; i < 0xFFFFF; ++i)
      W_TEST_BOOL(!WUnicodeUtils::IsASCII(i));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsUtf8StartByte")
  {
    WStringUtf8 s(L"äöü€");
    // ä
    W_TEST_BOOL(WUnicodeUtils::IsUtf8StartByte(s.GetData()[0]));
    W_TEST_BOOL(!WUnicodeUtils::IsUtf8StartByte(s.GetData()[1]));

    // ö
    W_TEST_BOOL(WUnicodeUtils::IsUtf8StartByte(s.GetData()[2]));
    W_TEST_BOOL(!WUnicodeUtils::IsUtf8StartByte(s.GetData()[3]));

    // ü
    W_TEST_BOOL(WUnicodeUtils::IsUtf8StartByte(s.GetData()[4]));
    W_TEST_BOOL(!WUnicodeUtils::IsUtf8StartByte(s.GetData()[5]));

    // €
    W_TEST_BOOL(WUnicodeUtils::IsUtf8StartByte(s.GetData()[6]));
    W_TEST_BOOL(!WUnicodeUtils::IsUtf8StartByte(s.GetData()[7]));
    W_TEST_BOOL(!WUnicodeUtils::IsUtf8StartByte(s.GetData()[8]));

    // \0
    W_TEST_BOOL(WUnicodeUtils::IsUtf8StartByte(s.GetData()[9]));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsUtf8ContinuationByte")
  {
    // all ASCII Characters are not continuation bytes
    for (char i = 0; i < 127; ++i)
    {
      W_TEST_BOOL(!WUnicodeUtils::IsUtf8ContinuationByte(i));
    }

    for (WUInt32 i = 0; i < 255u; ++i)
    {
      const char uiContByte = static_cast<char>(0x80 | (i & 0x3f));
      const char uiNoContByte1 = static_cast<char>(i | 0x40);
      const char uiNoContByte2 = static_cast<char>(i | 0xC0);

      W_TEST_BOOL(WUnicodeUtils::IsUtf8ContinuationByte(uiContByte));
      W_TEST_BOOL(!WUnicodeUtils::IsUtf8ContinuationByte(uiNoContByte1));
      W_TEST_BOOL(!WUnicodeUtils::IsUtf8ContinuationByte(uiNoContByte2));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetUtf8SequenceLength")
  {
    // All ASCII characters are 1 byte in length
    for (char i = 0; i < 127; ++i)
    {
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(i), 1);
    }

    {
      WStringUtf8 s(L"ä");
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      WStringUtf8 s(L"ß");
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      WStringUtf8 s(L"€");
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 3);
    }

    {
      WStringUtf8 s(L"з");
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      WStringUtf8 s(L"г");
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      WStringUtf8 s(L"ы");
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      WUInt32 u[2] = {L'\u0B87', 0};
      WStringUtf8 s(u);
      W_TEST_INT(WUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 3);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ConvertUtf8ToUtf32")
  {
    // Just wraps around 'utf8::peek_next'
    // I think we can assume that that works.
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetSizeForCharacterInUtf8")
  {
    // All ASCII characters are 1 byte in length
    for (WUInt32 i = 0; i < 128; ++i)
      W_TEST_INT(WUnicodeUtils::GetSizeForCharacterInUtf8(i), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Decode")
  {
    char utf8[] = {(char)0xc3, (char)0xb6, 0};
    WUInt16 utf16[] = {0xf6, 0};
    wchar_t wchar[] = {L'ö', 0};

    char* szUtf8 = &utf8[0];
    WUInt16* szUtf16 = &utf16[0];
    wchar_t* szWChar = &wchar[0];

    WUInt32 uiUtf321 = WUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);
    WUInt32 uiUtf322 = WUnicodeUtils::DecodeUtf16ToUtf32(szUtf16);
    WUInt32 uiUtf323 = WUnicodeUtils::DecodeWCharToUtf32(szWChar);

    W_TEST_INT(uiUtf321, uiUtf322);
    W_TEST_INT(uiUtf321, uiUtf323);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Encode")
  {
    char utf8[4] = {0};
    WUInt16 utf16[4] = {0};
    wchar_t wchar[4] = {0};

    char* szUtf8 = &utf8[0];
    WUInt16* szUtf16 = &utf16[0];
    wchar_t* szWChar = &wchar[0];

    WUnicodeUtils::EncodeUtf32ToUtf8(0xf6, szUtf8);
    WUnicodeUtils::EncodeUtf32ToUtf16(0xf6, szUtf16);
    WUnicodeUtils::EncodeUtf32ToWChar(0xf6, szWChar);

    W_TEST_BOOL(utf8[0] == (char)0xc3 && utf8[1] == (char)0xb6);
    W_TEST_BOOL(utf16[0] == 0xf6);
    W_TEST_BOOL(wchar[0] == L'ö');
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MoveToNextUtf8")
  {
    WStringUtf8 s(L"aböäß€de");

    W_TEST_INT(s.GetElementCount(), 13);

    const char* sz = s.GetData();

    // test how far it skips ahead

    WUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[1]);

    WUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[2]);

    WUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[4]);

    WUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[6]);

    WUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[8]);

    WUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[11]);

    WUnicodeUtils::MoveToNextUtf8(sz).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[12]);

    sz = s.GetData();
    const char* szEnd = s.GetView().GetEndPointer();


    WUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[1]);

    WUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[2]);

    WUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[4]);

    WUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[6]);

    WUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[8]);

    WUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[11]);

    WUnicodeUtils::MoveToNextUtf8(sz, szEnd).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[12]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MoveToPriorUtf8")
  {
    WStringUtf8 s(L"aböäß€de");

    const char* sz = &s.GetData()[13];

    W_TEST_INT(s.GetElementCount(), 13);

    // test how far it skips ahead

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[12]);

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[11]);

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[8]);

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[6]);

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[4]);

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[2]);

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[1]);

    WUnicodeUtils::MoveToPriorUtf8(sz, s.GetData()).AssertSuccess();
    W_TEST_BOOL(sz == &s.GetData()[0]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SkipUtf8Bom")
  {
    // C++ is really stupid, chars are signed, but Utf8 only works with unsigned values ... argh!

    char szWithBom[] = {(char)0xef, (char)0xbb, (char)0xbf, 'a'};
    char szNoBom[] = {'a'};
    const char* pString = szWithBom;

    W_TEST_BOOL(WUnicodeUtils::SkipUtf8Bom(pString) == true);
    W_TEST_BOOL(pString == &szWithBom[3]);

    pString = szNoBom;

    W_TEST_BOOL(WUnicodeUtils::SkipUtf8Bom(pString) == false);
    W_TEST_BOOL(pString == szNoBom);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SkipUtf16BomLE")
  {
    WUInt16 szWithBom[] = {0xfeff, 'a'};
    WUInt16 szNoBom[] = {'a'};

    const WUInt16* pString = szWithBom;

    W_TEST_BOOL(WUnicodeUtils::SkipUtf16BomLE(pString) == true);
    W_TEST_BOOL(pString == &szWithBom[1]);

    pString = szNoBom;

    W_TEST_BOOL(WUnicodeUtils::SkipUtf16BomLE(pString) == false);
    W_TEST_BOOL(pString == szNoBom);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SkipUtf16BomBE")
  {
    WUInt16 szWithBom[] = {0xfffe, 'a'};
    WUInt16 szNoBom[] = {'a'};

    const WUInt16* pString = szWithBom;

    W_TEST_BOOL(WUnicodeUtils::SkipUtf16BomBE(pString) == true);
    W_TEST_BOOL(pString == &szWithBom[1]);

    pString = szNoBom;

    W_TEST_BOOL(WUnicodeUtils::SkipUtf16BomBE(pString) == false);
    W_TEST_BOOL(pString == szNoBom);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsUtf16Surrogate")
  {
    WUInt16 szNoSurrogate[] = {0x2AD7};
    WUInt16 szSurrogate[] = {0xd83e};

    W_TEST_BOOL(WUnicodeUtils::IsUtf16Surrogate(szNoSurrogate) == false);
    W_TEST_BOOL(WUnicodeUtils::IsUtf16Surrogate(szSurrogate) == true);
  }
}
