#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/String.h>

W_CREATE_SIMPLE_TEST_GROUP(Strings);

W_CREATE_SIMPLE_TEST(Strings, StringUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "IsNullOrEmpty")
  {
    W_TEST_BOOL(WStringUtils::IsNullOrEmpty((char*)nullptr) == true);
    W_TEST_BOOL(WStringUtils::IsNullOrEmpty("") == true);

    // all other characters are not empty
    for (WUInt8 c = 1; c < 255; c++)
      W_TEST_BOOL(WStringUtils::IsNullOrEmpty(&c) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetStringElementCount")
  {
    W_TEST_INT(WStringUtils::GetStringElementCount((char*)nullptr), 0);

    // Counts the Bytes
    W_TEST_INT(WStringUtils::GetStringElementCount(""), 0);
    W_TEST_INT(WStringUtils::GetStringElementCount("a"), 1);
    W_TEST_INT(WStringUtils::GetStringElementCount("ab"), 2);
    W_TEST_INT(WStringUtils::GetStringElementCount("abc"), 3);

    // Counts the number of wchar_t's
    W_TEST_INT(WStringUtils::GetStringElementCount(L""), 0);
    W_TEST_INT(WStringUtils::GetStringElementCount(L"a"), 1);
    W_TEST_INT(WStringUtils::GetStringElementCount(L"ab"), 2);
    W_TEST_INT(WStringUtils::GetStringElementCount(L"abc"), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    W_TEST_INT(WStringUtils::GetStringElementCount(sz, sz + 0), 0);
    W_TEST_INT(WStringUtils::GetStringElementCount(sz, sz + 3), 3);
    W_TEST_INT(WStringUtils::GetStringElementCount(sz, sz + 6), 6);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UpdateStringEnd")
  {
    const char* sz = "Test test";
    const char* szEnd = WUnicodeUtils::GetMaxStringEnd<char>();

    WStringUtils::UpdateStringEnd(sz, szEnd);
    W_TEST_BOOL(szEnd == sz + WStringUtils::GetStringElementCount(sz));

    WStringUtils::UpdateStringEnd(sz, szEnd);
    W_TEST_BOOL(szEnd == sz + WStringUtils::GetStringElementCount(sz));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCharacterCount")
  {
    W_TEST_INT(WStringUtils::GetCharacterCount(nullptr), 0);
    W_TEST_INT(WStringUtils::GetCharacterCount(""), 0);
    W_TEST_INT(WStringUtils::GetCharacterCount("a"), 1);
    W_TEST_INT(WStringUtils::GetCharacterCount("abc"), 3);

    WStringUtf8 s(L"äöü"); // 6 Bytes

    W_TEST_INT(WStringUtils::GetStringElementCount(s.GetData()), 6);
    W_TEST_INT(WStringUtils::GetCharacterCount(s.GetData()), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    W_TEST_INT(WStringUtils::GetCharacterCount(sz, sz + 0), 0);
    W_TEST_INT(WStringUtils::GetCharacterCount(sz, sz + 3), 3);
    W_TEST_INT(WStringUtils::GetCharacterCount(sz, sz + 6), 6);

    W_TEST_INT(WStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 0), 0);
    W_TEST_INT(WStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 2), 1);
    W_TEST_INT(WStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 4), 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCharacterAndElementCount")
  {
    WUInt32 uiCC, uiEC;

    WStringUtils::GetCharacterAndElementCount(nullptr, uiCC, uiEC);
    W_TEST_INT(uiCC, 0);
    W_TEST_INT(uiEC, 0);

    WStringUtils::GetCharacterAndElementCount("", uiCC, uiEC);
    W_TEST_INT(uiCC, 0);
    W_TEST_INT(uiEC, 0);

    WStringUtils::GetCharacterAndElementCount("a", uiCC, uiEC);
    W_TEST_INT(uiCC, 1);
    W_TEST_INT(uiEC, 1);

    WStringUtils::GetCharacterAndElementCount("abc", uiCC, uiEC);
    W_TEST_INT(uiCC, 3);
    W_TEST_INT(uiEC, 3);

    WStringUtf8 s(L"äöü"); // 6 Bytes

    WStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC);
    W_TEST_INT(uiCC, 3);
    W_TEST_INT(uiEC, 6);

    WStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 0);
    W_TEST_INT(uiCC, 0);
    W_TEST_INT(uiEC, 0);

    WStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 4);
    W_TEST_INT(uiCC, 2);
    W_TEST_INT(uiEC, 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy (full)")
  {
    char szDest[256] = "";

    // large enough
    W_TEST_INT(WStringUtils::Copy(szDest, 256, "Test ABC"), 8);
    W_TEST_BOOL(WStringUtils::IsEqual(szDest, "Test ABC"));

    // exactly fitting
    W_TEST_INT(WStringUtils::Copy(szDest, 13, "Humpf, humpf"), 12);
    W_TEST_BOOL(WStringUtils::IsEqual(szDest, "Humpf, humpf"));

    // too small
    W_TEST_INT(WStringUtils::Copy(szDest, 8, "Test ABC"), 7);
    W_TEST_BOOL(WStringUtils::IsEqual(szDest, "Test AB"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    // large enough
    W_TEST_INT(WStringUtils::Copy(szDest, 256, szUTF8), 9);
    W_TEST_BOOL(WStringUtils::IsEqual(szDest, szUTF8));

    // exactly fitting
    W_TEST_INT(WStringUtils::Copy(szDest, 10, szUTF8), 9);
    W_TEST_BOOL(WStringUtils::IsEqual(szDest, szUTF8));

    // These tests are disabled as previously valid behavior was now turned into an assert.
    // Comment them in to test the assert.
    // too small 1
    /*W_TEST_INT(WStringUtils::Copy(szDest, 9, szUTF8), 7);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 5)); // one character less

    // too small 2
    W_TEST_INT(WStringUtils::Copy(szDest, 7, szUTF8), 4);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 4)); // two characters less*/


    // copy only from a subset
    W_TEST_INT(WStringUtils::Copy(szDest, 256, szUTF8, szUTF8 + 7), 7);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 5)); // two characters less
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CopyN")
  {
    char szDest[256] = "";

    W_TEST_INT(WStringUtils::CopyN(szDest, 256, "Test ABC", 4), 4);
    W_TEST_BOOL(WStringUtils::IsEqual(szDest, "Test"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    W_TEST_INT(WStringUtils::CopyN(szDest, 256, szUTF8, 6), 9);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 6));

    W_TEST_INT(WStringUtils::CopyN(szDest, 256, szUTF8, 5), 7);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 5));

    W_TEST_INT(WStringUtils::CopyN(szDest, 256, szUTF8, 4), 4);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 4));

    W_TEST_INT(WStringUtils::CopyN(szDest, 256, szUTF8, 1), 1);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 1));

    W_TEST_INT(WStringUtils::CopyN(szDest, 256, szUTF8, 0), 0);
    W_TEST_BOOL(WStringUtils::IsEqual(szDest, ""));

    // copy only from a subset
    W_TEST_INT(WStringUtils::CopyN(szDest, 256, szUTF8, 6, szUTF8 + 7), 7);
    W_TEST_BOOL(WStringUtils::IsEqualN(szDest, szUTF8, 5));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ToUpperChar")
  {
    // this only tests the ASCII range
    for (WInt32 i = 0; i < 128; ++i)
      W_TEST_INT(WStringUtils::ToUpperChar(i), toupper(i));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ToLowerChar")
  {
    // this only tests the ASCII range
    for (WInt32 i = 0; i < 128; ++i)
      W_TEST_INT(WStringUtils::ToLowerChar(i), tolower(i));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ToUpperString")
  {
    WStringUtf8 sL(L"abc öäü ß €");
    WStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    WStringUtils::Copy(szCopy, 256, sL.GetData());

    WStringUtils::ToUpperString(szCopy);

    W_TEST_BOOL(WStringUtils::IsEqual(szCopy, sU.GetData()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ToLowerString")
  {
    WStringUtf8 sL(L"abc öäü ß €");
    WStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    WStringUtils::Copy(szCopy, 256, sU.GetData());

    WStringUtils::ToLowerString(szCopy);

    W_TEST_BOOL(WStringUtils::IsEqual(szCopy, sL.GetData()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareChars")
  {
    W_TEST_BOOL(WStringUtils::CompareChars('a', 'a') == 0); // make sure the order is right
    W_TEST_BOOL(WStringUtils::CompareChars('a', 'b') < 0);  // a smaller than b -> negative
    W_TEST_BOOL(WStringUtils::CompareChars('b', 'a') > 0);  // b bigger than a  -> positive
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareChars(utf8)")
  {
    W_TEST_BOOL(WStringUtils::CompareChars("a", "a") == 0); // make sure the order is right
    W_TEST_BOOL(WStringUtils::CompareChars("a", "b") < 0);  // a smaller than b -> negative
    W_TEST_BOOL(WStringUtils::CompareChars("b", "a") > 0);  // b bigger than a  -> positive
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareChars_NoCase")
  {
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase('a', 'A') == 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase('a', 'B') < 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase('B', 'a') > 0);

    W_TEST_BOOL(WStringUtils::CompareChars_NoCase('A', 'a') == 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase('A', 'b') < 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase('b', 'A') > 0);

    W_TEST_BOOL(WStringUtils::CompareChars_NoCase(L'ä', L'Ä') == 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase(L'ä', L'Ö') < 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase(L'ö', L'Ä') > 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareChars_NoCase(utf8)")
  {
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase("a", "A") == 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase("a", "B") < 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase("B", "a") > 0);

    W_TEST_BOOL(WStringUtils::CompareChars_NoCase("A", "a") == 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase("A", "b") < 0);
    W_TEST_BOOL(WStringUtils::CompareChars_NoCase("b", "A") > 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    W_TEST_BOOL(WStringUtils::IsEqual(nullptr, nullptr) == true);
    W_TEST_BOOL(WStringUtils::IsEqual(nullptr, "") == true);
    W_TEST_BOOL(WStringUtils::IsEqual("", nullptr) == true);
    W_TEST_BOOL(WStringUtils::IsEqual("", "") == true);

    W_TEST_BOOL(WStringUtils::IsEqual("abc", "abc") == true);
    W_TEST_BOOL(WStringUtils::IsEqual("abc", "abcd") == false);
    W_TEST_BOOL(WStringUtils::IsEqual("abcd", "abc") == false);

    W_TEST_BOOL(WStringUtils::IsEqual("a", nullptr) == false);
    W_TEST_BOOL(WStringUtils::IsEqual(nullptr, "a") == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqualN")
  {
    W_TEST_BOOL(WStringUtils::IsEqualN(nullptr, nullptr, 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN(nullptr, "", 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("", nullptr, 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    W_TEST_BOOL(WStringUtils::IsEqualN("abc", nullptr, 0) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("abc", "", 0) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN(nullptr, "abc", 0) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("", "abc", 0) == true);

    W_TEST_BOOL(WStringUtils::IsEqualN("abc", "abcdef", 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("abc", "abcdef", 2) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("abc", "abcdef", 3) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("abc", "abcdef", 4) == false);

    W_TEST_BOOL(WStringUtils::IsEqualN("abcdef", "abc", 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("abcdef", "abc", 2) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("abcdef", "abc", 3) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN("abcdef", "abc", 4) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual_NoCase")
  {
    W_TEST_BOOL(WStringUtils::IsEqual_NoCase(nullptr, nullptr) == true);
    W_TEST_BOOL(WStringUtils::IsEqual_NoCase(nullptr, "") == true);
    W_TEST_BOOL(WStringUtils::IsEqual_NoCase("", nullptr) == true);
    W_TEST_BOOL(WStringUtils::IsEqual_NoCase("", "") == true);


    WStringUtf8 sL(L"abc öäü ß €");
    WStringUtf8 sU(L"ABC ÖÄÜ ß €");
    WStringUtf8 sU2(L"ABC ÖÄÜ ß € ");

    W_TEST_BOOL(WStringUtils::IsEqual_NoCase(sL.GetData(), sU.GetData()) == true);
    W_TEST_BOOL(WStringUtils::IsEqual_NoCase(sL.GetData(), sU2.GetData()) == false);
    W_TEST_BOOL(WStringUtils::IsEqual_NoCase(sU2.GetData(), sL.GetData()) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqualN_NoCase")
  {
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase(nullptr, nullptr, 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase(nullptr, "", 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase("", nullptr, 1) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase("abc", nullptr, 0) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase("abc", "", 0) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase(nullptr, "abc", 0) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase("", "abc", 0) == true);

    WStringUtf8 sL(L"abc öäü ß €");
    WStringUtf8 sU(L"ABC ÖÄÜ ß € moep");

    for (WInt32 i = 0; i < 12; ++i)
      W_TEST_BOOL(WStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), i) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), 12) == false);

    for (WInt32 i = 0; i < 12; ++i)
      W_TEST_BOOL(WStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), i) == true);
    W_TEST_BOOL(WStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), 12) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compare")
  {
    W_TEST_BOOL(WStringUtils::Compare(nullptr, nullptr) == 0);
    W_TEST_BOOL(WStringUtils::Compare(nullptr, "") == 0);
    W_TEST_BOOL(WStringUtils::Compare("", nullptr) == 0);
    W_TEST_BOOL(WStringUtils::Compare("", "") == 0);

    W_TEST_BOOL(WStringUtils::Compare("abc", "abc") == 0);
    W_TEST_BOOL(WStringUtils::Compare("abc", "abcd") < 0);
    W_TEST_BOOL(WStringUtils::Compare("abcd", "abc") > 0);

    W_TEST_BOOL(WStringUtils::Compare("a", nullptr) > 0);
    W_TEST_BOOL(WStringUtils::Compare(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    W_TEST_BOOL(WStringUtils::Compare(sz, "abc", sz + 3) == 0);
    W_TEST_BOOL(WStringUtils::Compare(sz, "abc def", sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::Compare(sz, sz, sz + 7, sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::Compare(sz, sz, sz + 7, sz + 6) > 0);
    W_TEST_BOOL(WStringUtils::Compare(sz, sz, sz + 7, sz + 8) < 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareN")
  {
    W_TEST_BOOL(WStringUtils::CompareN(nullptr, nullptr, 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN(nullptr, "", 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("", nullptr, 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    W_TEST_BOOL(WStringUtils::CompareN("abc", nullptr, 0) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("abc", "", 0) == 0);
    W_TEST_BOOL(WStringUtils::CompareN(nullptr, "abc", 0) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("", "abc", 0) == 0);

    W_TEST_BOOL(WStringUtils::CompareN("abc", "abcdef", 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("abc", "abcdef", 2) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("abc", "abcdef", 3) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("abc", "abcdef", 4) < 0);

    W_TEST_BOOL(WStringUtils::CompareN("abcdef", "abc", 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("abcdef", "abc", 2) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("abcdef", "abc", 3) == 0);
    W_TEST_BOOL(WStringUtils::CompareN("abcdef", "abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    W_TEST_BOOL(WStringUtils::CompareN(sz, "abc", 10, sz + 3) == 0);
    W_TEST_BOOL(WStringUtils::CompareN(sz, "abc def", 10, sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 6) > 0);
    W_TEST_BOOL(WStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compare_NoCase")
  {
    W_TEST_BOOL(WStringUtils::Compare_NoCase(nullptr, nullptr) == 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase(nullptr, "") == 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase("", nullptr) == 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase("", "") == 0);

    W_TEST_BOOL(WStringUtils::Compare_NoCase("abc", "aBc") == 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase("ABC", "abcd") < 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase("abcd", "ABC") > 0);

    W_TEST_BOOL(WStringUtils::Compare_NoCase("a", nullptr) > 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    W_TEST_BOOL(WStringUtils::Compare_NoCase(sz, "ABC", sz + 3) == 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase(sz, "ABC def", sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 6) > 0);
    W_TEST_BOOL(WStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 8) < 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareN_NoCase")
  {
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(nullptr, nullptr, 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(nullptr, "", 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("", nullptr, 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("abc", nullptr, 0) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("abc", "", 0) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(nullptr, "abc", 0) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("", "abc", 0) == 0);

    W_TEST_BOOL(WStringUtils::CompareN_NoCase("aBc", "abcdef", 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("aBc", "abcdef", 2) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("aBc", "abcdef", 3) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("aBc", "abcdef", 4) < 0);

    W_TEST_BOOL(WStringUtils::CompareN_NoCase("abcdef", "Abc", 1) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("abcdef", "Abc", 2) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("abcdef", "Abc", 3) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase("abcdef", "Abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(sz, "ABC", 10, sz + 3) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(sz, "ABC def", 10, sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 7) == 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 6) > 0);
    W_TEST_BOOL(WStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "snprintf")
  {
    // This function has been tested to death during its implementation.
    // That test-code would require several pages, if one would try to test it properly.
    // I am not going to do that here, I am quite confident the function works as expected with pure ASCII strings.
    // So I'm only testing a bit of Utf8 stuff.

    WStringUtf8 s(L"Abc %s äöü ß %i %s %.4f");
    WStringUtf8 s2(L"ÄÖÜ");

    char sz[256];
    WStringUtils::snprintf(sz, 256, s.GetData(), "ASCII", 42, s2.GetData(), 23.31415);

    WStringUtf8 sC(L"Abc ASCII äöü ß 42 ÄÖÜ 23.3142"); // notice the correct float rounding ;-)

    W_TEST_STRING(sz, sC.GetData());


    // NaN and Infinity
    WStringUtils::snprintf(sz, 256, "NaN Value: %.2f", WMath::NaN<float>());
    W_TEST_STRING(sz, "NaN Value: NaN");

    WStringUtils::snprintf(sz, 256, "Inf Value: %.2f", +WMath::Infinity<float>());
    W_TEST_STRING(sz, "Inf Value: Infinity");

    WStringUtils::snprintf(sz, 256, "Inf Value: %.2f", -WMath::Infinity<float>());
    W_TEST_STRING(sz, "Inf Value: -Infinity");

    WStringUtils::snprintf(sz, 256, "NaN Value: %.2e", WMath::NaN<float>());
    W_TEST_STRING(sz, "NaN Value: NaN");

    WStringUtils::snprintf(sz, 256, "Inf Value: %.2e", +WMath::Infinity<float>());
    W_TEST_STRING(sz, "Inf Value: Infinity");

    WStringUtils::snprintf(sz, 256, "Inf Value: %.2e", -WMath::Infinity<float>());
    W_TEST_STRING(sz, "Inf Value: -Infinity");

    WStringUtils::snprintf(sz, 256, "NaN Value: %+10.2f", WMath::NaN<float>());
    W_TEST_STRING(sz, "NaN Value:       +NaN");

    WStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", +WMath::Infinity<float>());
    W_TEST_STRING(sz, "Inf Value:  +Infinity");

    WStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", -WMath::Infinity<float>());
    W_TEST_STRING(sz, "Inf Value:  -Infinity");

    // extended stuff
    WStringUtils::snprintf(sz, 256, "size: %zu", (size_t)12345678);
    W_TEST_STRING(sz, "size: 12345678");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StartsWith")
  {
    W_TEST_BOOL(WStringUtils::StartsWith(nullptr, nullptr) == true);
    W_TEST_BOOL(WStringUtils::StartsWith(nullptr, "") == true);
    W_TEST_BOOL(WStringUtils::StartsWith("", nullptr) == true);
    W_TEST_BOOL(WStringUtils::StartsWith("", "") == true);

    W_TEST_BOOL(WStringUtils::StartsWith("abc", nullptr) == true);
    W_TEST_BOOL(WStringUtils::StartsWith("abc", "") == true);
    W_TEST_BOOL(WStringUtils::StartsWith(nullptr, "abc") == false);
    W_TEST_BOOL(WStringUtils::StartsWith("", "abc") == false);

    W_TEST_BOOL(WStringUtils::StartsWith("abc", "abc") == true);
    W_TEST_BOOL(WStringUtils::StartsWith("abcdef", "abc") == true);
    W_TEST_BOOL(WStringUtils::StartsWith("abcdef", "Abc") == false);

    // substring test
    const char* sz = (const char*)u8"äbc def ghi";
    const WUInt32 uiByteCount = WStringUtils::GetStringElementCount(u8"äbc");

    W_TEST_BOOL(WStringUtils::StartsWith(sz, (const char*)u8"äbc", sz + uiByteCount) == true);
    W_TEST_BOOL(WStringUtils::StartsWith(sz, (const char*)u8"äbc", sz + uiByteCount - 1) == false);
    W_TEST_BOOL(WStringUtils::StartsWith(sz, (const char*)u8"äbc", sz + 0) == false);

    const char* sz2 = (const char*)u8"äbc def";
    W_TEST_BOOL(WStringUtils::StartsWith(sz, sz2, sz + uiByteCount, sz2 + uiByteCount) == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StartsWith_NoCase")
  {
    WStringUtf8 sL(L"äöü");
    WStringUtf8 sU(L"ÄÖÜ");

    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(nullptr, nullptr) == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(nullptr, "") == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("", nullptr) == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("", "") == true);

    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("abc", nullptr) == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("abc", "") == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(nullptr, "abc") == false);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("", "abc") == false);

    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("abc", "ABC") == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("aBCdef", "abc") == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase("aBCdef", "bc") == false);

    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char* sz = (const char*)u8"äbc def ghi";
    const WUInt32 uiByteCount = WStringUtils::GetStringElementCount(u8"äbc");
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(sz, (const char*)u8"ÄBC", sz + uiByteCount) == true);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(sz, (const char*)u8"ÄBC", sz + uiByteCount - 1) == false);
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(sz, (const char*)u8"ÄBC", sz + 0) == false);

    const char* sz2 = (const char*)u8"Äbc def";
    W_TEST_BOOL(WStringUtils::StartsWith_NoCase(sz, sz2, sz + uiByteCount, sz2 + uiByteCount) == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EndsWith")
  {
    W_TEST_BOOL(WStringUtils::EndsWith(nullptr, nullptr) == true);
    W_TEST_BOOL(WStringUtils::EndsWith(nullptr, "") == true);
    W_TEST_BOOL(WStringUtils::EndsWith("", nullptr) == true);
    W_TEST_BOOL(WStringUtils::EndsWith("", "") == true);

    W_TEST_BOOL(WStringUtils::EndsWith("abc", nullptr) == true);
    W_TEST_BOOL(WStringUtils::EndsWith("abc", "") == true);
    W_TEST_BOOL(WStringUtils::EndsWith(nullptr, "abc") == false);
    W_TEST_BOOL(WStringUtils::EndsWith("", "abc") == false);

    W_TEST_BOOL(WStringUtils::EndsWith("abc", "abc") == true);
    W_TEST_BOOL(WStringUtils::EndsWith("abcdef", "def") == true);
    W_TEST_BOOL(WStringUtils::EndsWith("abcdef", "Def") == false);
    W_TEST_BOOL(WStringUtils::EndsWith("def", "abcdef") == false);

    // substring test
    const char* sz = "abc def ghi";
    W_TEST_BOOL(WStringUtils::EndsWith(sz, "abc", sz + 3) == true);
    W_TEST_BOOL(WStringUtils::EndsWith(sz, "def", sz + 7) == true);
    W_TEST_BOOL(WStringUtils::EndsWith(sz, "def", sz + 8) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EndsWith_NoCase")
  {
    WStringUtf8 sL(L"äöü");
    WStringUtf8 sU(L"ÄÖÜ");

    W_TEST_BOOL(WStringUtils::EndsWith_NoCase(nullptr, nullptr) == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase(nullptr, "") == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("", nullptr) == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("", "") == true);

    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("abc", nullptr) == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("abc", "") == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase(nullptr, "abc") == false);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("", "abc") == false);

    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("abc", "abc") == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("abcdef", "def") == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("abcdef", "Def") == true);

    W_TEST_BOOL(WStringUtils::EndsWith_NoCase("def", "abcdef") == false);

    W_TEST_BOOL(WStringUtils::EndsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char* sz = "abc def ghi";
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase(sz, "ABC", sz + 3) == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase(sz, "DEF", sz + 7) == true);
    W_TEST_BOOL(WStringUtils::EndsWith_NoCase(sz, "DEF", sz + 8) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindSubString")
  {
    WStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    WStringUtf8 s2(L"äöü");
    WStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    W_TEST_BOOL(WStringUtils::FindSubString(szABC, szABC) == szABC);
    W_TEST_BOOL(WStringUtils::FindSubString("abc", "") == nullptr);
    W_TEST_BOOL(WStringUtils::FindSubString("abc", nullptr) == nullptr);
    W_TEST_BOOL(WStringUtils::FindSubString(nullptr, "abc") == nullptr);
    W_TEST_BOOL(WStringUtils::FindSubString("", "abc") == nullptr);

    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "abc") == s.GetData());
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "def") == &s.GetData()[4]);
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "ghi") == &s.GetData()[8]);
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "abc2") == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "def2") == &s.GetData()[35]);
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "ghi2") == &s.GetData()[40]);
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 34) == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 33) == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindSubString_NoCase")
  {
    WStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    WStringUtf8 s2(L"äÖü");
    WStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(szABC, "aBc") == szABC);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase("abc", "") == nullptr);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase("abc", nullptr) == nullptr);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(nullptr, "abc") == nullptr);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase("", "abc") == nullptr);

    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "Abc") == s.GetData());
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[4]);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[8]);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "abC2") == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "dEf2") == &s.GetData()[35]);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "Ghi2") == &s.GetData()[40]);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "aBc2", s.GetData() + 34) == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindSubString_NoCase(s.GetData(), "abC2", s.GetData() + 33) == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindLastSubString")
  {
    WStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    WStringUtf8 s2(L"äöü");
    WStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    W_TEST_BOOL(WStringUtils::FindLastSubString(szABC, szABC) == szABC);
    W_TEST_BOOL(WStringUtils::FindLastSubString("abc", "") == nullptr);
    W_TEST_BOOL(WStringUtils::FindLastSubString("abc", nullptr) == nullptr);
    W_TEST_BOOL(WStringUtils::FindLastSubString(nullptr, "abc") == nullptr);
    W_TEST_BOOL(WStringUtils::FindLastSubString("", "abc") == nullptr);

    W_TEST_BOOL(WStringUtils::FindLastSubString(s.GetData(), "abc") == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindLastSubString(s.GetData(), "def") == &s.GetData()[35]);
    W_TEST_BOOL(WStringUtils::FindLastSubString(s.GetData(), "ghi") == &s.GetData()[40]);
    W_TEST_BOOL(WStringUtils::FindLastSubString(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    W_TEST_BOOL(WStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    WStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    WStringUtf8 s2(L"äÖü");
    WStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(szABC, "aBC") == szABC);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase("abc", "") == nullptr);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase("abc", nullptr) == nullptr);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(nullptr, "abc") == nullptr);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase("", "abc") == nullptr);

    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(s.GetData(), "Abc") == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[35]);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[40]);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    W_TEST_BOOL(WStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindWholeWord")
  {
    WStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    W_TEST_BOOL(WStringUtils::FindWholeWord(s.GetData(), "abc", WStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    W_TEST_BOOL(WStringUtils::FindWholeWord(s.GetData(), "def", WStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    W_TEST_BOOL(WStringUtils::FindWholeWord(s.GetData(), "mompfh", WStringUtils::IsWordDelimiter_English) == &s.GetData()[0]); // ü is not english

    // substring test
    W_TEST_BOOL(WStringUtils::FindWholeWord(s.GetData(), "abc", WStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    W_TEST_BOOL(WStringUtils::FindWholeWord(s.GetData(), "abc", WStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
    W_TEST_BOOL(WStringUtils::FindWholeWord(s.GetData(), "abc", WStringUtils::IsWordDelimiter_English, s.GetData() + 30) == s.GetData() + 27);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    WStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    W_TEST_BOOL(WStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", WStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    W_TEST_BOOL(WStringUtils::FindWholeWord_NoCase(s.GetData(), "DEF", WStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    W_TEST_BOOL(WStringUtils::FindWholeWord_NoCase(s.GetData(), "momPFH", WStringUtils::IsWordDelimiter_English) == &s.GetData()[0]);

    // substring test
    W_TEST_BOOL(
      WStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", WStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    W_TEST_BOOL(WStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", WStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindUIntAtTheEnd")
  {
    WUInt32 uiTestValue = 0;
    WUInt32 uiCharactersFromStart = 0;

    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(nullptr, uiTestValue, &uiCharactersFromStart).Failed());

    WStringUtf8 noNumberAtTheEnd(L"ThisStringContainsNoNumberAtTheEnd");
    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(noNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Failed());

    WStringUtf8 noNumberAtTheEnd2(L"ThisStringContainsNoNumberAtTheEndBut42InBetween");
    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(noNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Failed());

    WStringUtf8 aNumberAtTheEnd(L"ThisStringContainsANumberAtTheEnd1");
    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(aNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    W_TEST_INT(uiTestValue, 1);
    W_TEST_INT(uiCharactersFromStart, aNumberAtTheEnd.GetElementCount() - 1);

    WStringUtf8 aZeroLeadingNumberAtTheEnd(L"ThisStringContainsANumberAtTheEnd011129");
    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(aZeroLeadingNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    W_TEST_INT(uiTestValue, 11129);
    W_TEST_INT(uiCharactersFromStart, aZeroLeadingNumberAtTheEnd.GetElementCount() - 6);

    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(aNumberAtTheEnd.GetData(), uiTestValue, nullptr).Succeeded());
    W_TEST_INT(uiTestValue, 1);

    WStringUtf8 twoNumbersInOneString(L"FirstANumber23AndThen42");
    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(twoNumbersInOneString.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    W_TEST_INT(uiTestValue, 42);

    WStringUtf8 onlyANumber(L"55566553");
    W_TEST_BOOL(WStringUtils::FindUIntAtTheEnd(onlyANumber.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    W_TEST_INT(uiTestValue, 55566553);
    W_TEST_INT(uiCharactersFromStart, 0);
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "SkipCharacters")
  {
    WStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char* szEmpty = "";

    W_TEST_BOOL(WStringUtils::SkipCharacters(s.GetData(), WStringUtils::IsWhiteSpace, false) == &s.GetData()[0]);
    W_TEST_BOOL(WStringUtils::SkipCharacters(s.GetData(), WStringUtils::IsWhiteSpace, true) == &s.GetData()[1]);
    W_TEST_BOOL(WStringUtils::SkipCharacters(&s.GetData()[5], WStringUtils::IsWhiteSpace, false) == &s.GetData()[8]);
    W_TEST_BOOL(WStringUtils::SkipCharacters(&s.GetData()[5], WStringUtils::IsWhiteSpace, true) == &s.GetData()[8]);
    W_TEST_BOOL(WStringUtils::SkipCharacters(szEmpty, WStringUtils::IsWhiteSpace, false) == szEmpty);
    W_TEST_BOOL(WStringUtils::SkipCharacters(szEmpty, WStringUtils::IsWhiteSpace, true) == szEmpty);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindWordEnd")
  {
    WStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char* szEmpty = "";

    W_TEST_BOOL(WStringUtils::FindWordEnd(s.GetData(), WStringUtils::IsWhiteSpace, true) == &s.GetData()[5]);
    W_TEST_BOOL(WStringUtils::FindWordEnd(s.GetData(), WStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    W_TEST_BOOL(WStringUtils::FindWordEnd(&s.GetData()[5], WStringUtils::IsWhiteSpace, true) == &s.GetData()[6]);
    W_TEST_BOOL(WStringUtils::FindWordEnd(&s.GetData()[5], WStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    W_TEST_BOOL(WStringUtils::FindWordEnd(szEmpty, WStringUtils::IsWhiteSpace, true) == szEmpty);
    W_TEST_BOOL(WStringUtils::FindWordEnd(szEmpty, WStringUtils::IsWhiteSpace, false) == szEmpty);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsWhitespace")
  {
    W_TEST_BOOL(WStringUtils::IsWhiteSpace(' '));
    W_TEST_BOOL(WStringUtils::IsWhiteSpace('\t'));
    W_TEST_BOOL(WStringUtils::IsWhiteSpace('\n'));
    W_TEST_BOOL(WStringUtils::IsWhiteSpace('\r'));
    W_TEST_BOOL(WStringUtils::IsWhiteSpace('\v'));

    W_TEST_BOOL(WStringUtils::IsWhiteSpace('\0') == false);

    for (WUInt32 i = 33; i < 256; ++i)
    {
      W_TEST_BOOL(WStringUtils::IsWhiteSpace(i) == false);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsDecimalDigit / IsHexDigit")
  {
    W_TEST_BOOL(WStringUtils::IsDecimalDigit('0'));
    W_TEST_BOOL(WStringUtils::IsDecimalDigit('4'));
    W_TEST_BOOL(WStringUtils::IsDecimalDigit('9'));
    W_TEST_BOOL(!WStringUtils::IsDecimalDigit('/'));
    W_TEST_BOOL(!WStringUtils::IsDecimalDigit('A'));

    W_TEST_BOOL(WStringUtils::IsHexDigit('0'));
    W_TEST_BOOL(WStringUtils::IsHexDigit('4'));
    W_TEST_BOOL(WStringUtils::IsHexDigit('9'));
    W_TEST_BOOL(WStringUtils::IsHexDigit('A'));
    W_TEST_BOOL(WStringUtils::IsHexDigit('E'));
    W_TEST_BOOL(WStringUtils::IsHexDigit('a'));
    W_TEST_BOOL(WStringUtils::IsHexDigit('f'));
    W_TEST_BOOL(!WStringUtils::IsHexDigit('g'));
    W_TEST_BOOL(!WStringUtils::IsHexDigit('/'));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsWordDelimiter_English / IsIdentifierDelimiter_C_Code")
  {
    for (WUInt32 i = 0; i < 256; ++i)
    {
      const bool alpha = (i >= 'a' && i <= 'z');
      const bool alpha2 = (i >= 'A' && i <= 'Z');
      const bool num = (i >= '0' && i <= '9');
      const bool dash = i == '-';
      const bool underscore = i == '_';

      const bool bCode = alpha || alpha2 || num || underscore;
      const bool bWord = bCode || dash;


      W_TEST_BOOL(WStringUtils::IsWordDelimiter_English(i) == !bWord);
      W_TEST_BOOL(WStringUtils::IsIdentifierDelimiter_C_Code(i) == !bCode);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValidIdentifierName")
  {
    W_TEST_BOOL(!WStringUtils::IsValidIdentifierName(""));
    W_TEST_BOOL(!WStringUtils::IsValidIdentifierName("1asdf"));
    W_TEST_BOOL(!WStringUtils::IsValidIdentifierName("as df"));
    W_TEST_BOOL(!WStringUtils::IsValidIdentifierName("asdf!"));

    W_TEST_BOOL(WStringUtils::IsValidIdentifierName("asdf1"));
    W_TEST_BOOL(WStringUtils::IsValidIdentifierName("_asdf"));
  }
}
