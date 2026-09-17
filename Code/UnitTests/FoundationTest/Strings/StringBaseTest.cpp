#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

W_CREATE_SIMPLE_TEST(Strings, StringBase)
{
  // These tests need not be very through, as WStringBase only passes through to WStringUtil
  // which has been tested elsewhere already.
  // Here it is only assured that WStringBases passes its own pointers properly through,
  // such that the WStringUtil functions are called correctly.

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEmpty")
  {
    WStringView it(nullptr);
    W_TEST_BOOL(it.IsEmpty());

    WStringView it2("");
    W_TEST_BOOL(it2.IsEmpty());

    WStringView it3(nullptr, nullptr);
    W_TEST_BOOL(it3.IsEmpty());

    const char* sz = "abcdef";

    WStringView it4(sz, sz);
    W_TEST_BOOL(it4.IsEmpty());

    WStringView it5(sz, sz + 1);
    W_TEST_BOOL(!it5.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StartsWith")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.StartsWith("abc"));
    W_TEST_BOOL(it.StartsWith("abcdef"));
    W_TEST_BOOL(it.StartsWith("")); // empty strings always return true

    WStringView it2(sz + 3);

    W_TEST_BOOL(it2.StartsWith("def"));
    W_TEST_BOOL(it2.StartsWith(""));

    WStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    W_TEST_BOOL(it3.StartsWith("d"));
    W_TEST_BOOL(!it3.StartsWith("de"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StartsWith_NoCase")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.StartsWith_NoCase("ABC"));
    W_TEST_BOOL(it.StartsWith_NoCase("abcDEF"));
    W_TEST_BOOL(it.StartsWith_NoCase("")); // empty strings always return true

    WStringView it2(sz + 3);

    W_TEST_BOOL(it2.StartsWith_NoCase("DEF"));
    W_TEST_BOOL(it2.StartsWith_NoCase(""));

    WStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    W_TEST_BOOL(it3.StartsWith_NoCase("D"));
    W_TEST_BOOL(!it3.StartsWith_NoCase("DE"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EndsWith")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.EndsWith("def"));
    W_TEST_BOOL(it.EndsWith("abcdef"));
    W_TEST_BOOL(it.EndsWith("")); // empty strings always return true

    WStringView it2(sz + 3);

    W_TEST_BOOL(it2.EndsWith("def"));
    W_TEST_BOOL(it2.EndsWith(""));

    WStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    W_TEST_BOOL(it3.EndsWith("d"));
    W_TEST_BOOL(!it3.EndsWith("cd"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EndsWith_NoCase")
  {
    const char* sz = "ABCDEF";
    WStringView it(sz);

    W_TEST_BOOL(it.EndsWith_NoCase("def"));
    W_TEST_BOOL(it.EndsWith_NoCase("abcdef"));
    W_TEST_BOOL(it.EndsWith_NoCase("")); // empty strings always return true

    WStringView it2(sz + 3);

    W_TEST_BOOL(it2.EndsWith_NoCase("def"));
    W_TEST_BOOL(it2.EndsWith_NoCase(""));

    WStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    W_TEST_BOOL(it3.EndsWith_NoCase("d"));
    W_TEST_BOOL(!it3.EndsWith_NoCase("cd"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindSubString")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.FindSubString("abcdef") == sz);
    W_TEST_BOOL(it.FindSubString("abc") == sz);
    W_TEST_BOOL(it.FindSubString("def") == sz + 3);
    W_TEST_BOOL(it.FindSubString("cd") == sz + 2);
    W_TEST_BOOL(it.FindSubString("") == nullptr);
    W_TEST_BOOL(it.FindSubString(nullptr) == nullptr);
    W_TEST_BOOL(it.FindSubString("g") == nullptr);

    W_TEST_BOOL(it.FindSubString("abcdef", sz) == sz);
    W_TEST_BOOL(it.FindSubString("abcdef", sz + 1) == nullptr);
    W_TEST_BOOL(it.FindSubString("def", sz + 2) == sz + 3);
    W_TEST_BOOL(it.FindSubString("def", sz + 3) == sz + 3);
    W_TEST_BOOL(it.FindSubString("def", sz + 4) == nullptr);
    W_TEST_BOOL(it.FindSubString("", sz + 3) == nullptr);

    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.FindSubString("abcdef") == nullptr);
    W_TEST_BOOL(it2.FindSubString("abc") == nullptr);
    W_TEST_BOOL(it2.FindSubString("de") == sz + 3);
    W_TEST_BOOL(it2.FindSubString("cd") == sz + 2);
    W_TEST_BOOL(it2.FindSubString("") == nullptr);
    W_TEST_BOOL(it2.FindSubString(nullptr) == nullptr);
    W_TEST_BOOL(it2.FindSubString("g") == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    WStringView it(sz);

    W_TEST_BOOL(it.FindSubString_NoCase("abcdef") == sz);
    W_TEST_BOOL(it.FindSubString_NoCase("abc") == sz);
    W_TEST_BOOL(it.FindSubString_NoCase("def") == sz + 3);
    W_TEST_BOOL(it.FindSubString_NoCase("cd") == sz + 2);
    W_TEST_BOOL(it.FindSubString_NoCase("") == nullptr);
    W_TEST_BOOL(it.FindSubString_NoCase(nullptr) == nullptr);
    W_TEST_BOOL(it.FindSubString_NoCase("g") == nullptr);

    W_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz) == sz);
    W_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz + 1) == nullptr);
    W_TEST_BOOL(it.FindSubString_NoCase("def", sz + 2) == sz + 3);
    W_TEST_BOOL(it.FindSubString_NoCase("def", sz + 3) == sz + 3);
    W_TEST_BOOL(it.FindSubString_NoCase("def", sz + 4) == nullptr);
    W_TEST_BOOL(it.FindSubString_NoCase("", sz + 3) == nullptr);


    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.FindSubString_NoCase("abcdef") == nullptr);
    W_TEST_BOOL(it2.FindSubString_NoCase("abc") == nullptr);
    W_TEST_BOOL(it2.FindSubString_NoCase("de") == sz + 3);
    W_TEST_BOOL(it2.FindSubString_NoCase("cd") == sz + 2);
    W_TEST_BOOL(it2.FindSubString_NoCase("") == nullptr);
    W_TEST_BOOL(it2.FindSubString_NoCase(nullptr) == nullptr);
    W_TEST_BOOL(it2.FindSubString_NoCase("g") == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindLastSubString")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.FindLastSubString("abcdef") == sz);
    W_TEST_BOOL(it.FindLastSubString("abc") == sz);
    W_TEST_BOOL(it.FindLastSubString("def") == sz + 3);
    W_TEST_BOOL(it.FindLastSubString("cd") == sz + 2);
    W_TEST_BOOL(it.FindLastSubString("") == nullptr);
    W_TEST_BOOL(it.FindLastSubString(nullptr) == nullptr);
    W_TEST_BOOL(it.FindLastSubString("g") == nullptr);

    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.FindLastSubString("abcdef") == nullptr);
    W_TEST_BOOL(it2.FindLastSubString("abc") == nullptr);
    W_TEST_BOOL(it2.FindLastSubString("de") == sz + 3);
    W_TEST_BOOL(it2.FindLastSubString("cd") == sz + 2);
    W_TEST_BOOL(it2.FindLastSubString("") == nullptr);
    W_TEST_BOOL(it2.FindLastSubString(nullptr) == nullptr);
    W_TEST_BOOL(it2.FindLastSubString("g") == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    WStringView it(sz);

    W_TEST_BOOL(it.FindLastSubString_NoCase("abcdef") == sz);
    W_TEST_BOOL(it.FindLastSubString_NoCase("abc") == sz);
    W_TEST_BOOL(it.FindLastSubString_NoCase("def") == sz + 3);
    W_TEST_BOOL(it.FindLastSubString_NoCase("cd") == sz + 2);
    W_TEST_BOOL(it.FindLastSubString_NoCase("") == nullptr);
    W_TEST_BOOL(it.FindLastSubString_NoCase(nullptr) == nullptr);
    W_TEST_BOOL(it.FindLastSubString_NoCase("g") == nullptr);

    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.FindLastSubString_NoCase("abcdef") == nullptr);
    W_TEST_BOOL(it2.FindLastSubString_NoCase("abc") == nullptr);
    W_TEST_BOOL(it2.FindLastSubString_NoCase("de") == sz + 3);
    W_TEST_BOOL(it2.FindLastSubString_NoCase("cd") == sz + 2);
    W_TEST_BOOL(it2.FindLastSubString_NoCase("") == nullptr);
    W_TEST_BOOL(it2.FindLastSubString_NoCase(nullptr) == nullptr);
    W_TEST_BOOL(it2.FindLastSubString_NoCase("g") == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compare")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.Compare("abcdef") == 0);
    W_TEST_BOOL(it.Compare("abcde") > 0);
    W_TEST_BOOL(it.Compare("abcdefg") < 0);

    WStringView it2(sz + 2, sz + 5);
    it2.SetStartPosition(sz + 3);

    W_TEST_BOOL(it2.Compare("de") == 0);
    W_TEST_BOOL(it2.Compare("def") < 0);
    W_TEST_BOOL(it2.Compare("d") > 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compare_NoCase")
  {
    const char* sz = "ABCDEF";
    WStringView it(sz);

    W_TEST_BOOL(it.Compare_NoCase("abcdef") == 0);
    W_TEST_BOOL(it.Compare_NoCase("abcde") > 0);
    W_TEST_BOOL(it.Compare_NoCase("abcdefg") < 0);

    WStringView it2(sz + 2, sz + 5);
    it2.SetStartPosition(sz + 3);

    W_TEST_BOOL(it2.Compare_NoCase("de") == 0);
    W_TEST_BOOL(it2.Compare_NoCase("def") < 0);
    W_TEST_BOOL(it2.Compare_NoCase("d") > 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareN")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.CompareN("abc", 3) == 0);
    W_TEST_BOOL(it.CompareN("abcde", 6) > 0);
    W_TEST_BOOL(it.CompareN("abcg", 3) == 0);

    WStringView it2(sz + 2, sz + 5);

    W_TEST_BOOL(it2.CompareN("cd", 2) == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompareN_NoCase")
  {
    const char* sz = "ABCDEF";
    WStringView it(sz);

    W_TEST_BOOL(it.CompareN_NoCase("abc", 3) == 0);
    W_TEST_BOOL(it.CompareN_NoCase("abcde", 6) > 0);
    W_TEST_BOOL(it.CompareN_NoCase("abcg", 3) == 0);

    WStringView it2(sz + 2, sz + 5);

    W_TEST_BOOL(it2.CompareN_NoCase("cd", 2) == 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.IsEqual("abcdef"));
    W_TEST_BOOL(!it.IsEqual("abcde"));
    W_TEST_BOOL(!it.IsEqual("abcdefg"));

    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.IsEqual("cde"));
    W_TEST_BOOL(!it2.IsEqual("bcde"));
    W_TEST_BOOL(!it2.IsEqual("cdef"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual_NoCase")
  {
    const char* sz = "ABCDEF";
    WStringView it(sz);

    W_TEST_BOOL(it.IsEqual_NoCase("abcdef"));
    W_TEST_BOOL(!it.IsEqual_NoCase("abcde"));
    W_TEST_BOOL(!it.IsEqual_NoCase("abcdefg"));

    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.IsEqual_NoCase("cde"));
    W_TEST_BOOL(!it2.IsEqual_NoCase("bcde"));
    W_TEST_BOOL(!it2.IsEqual_NoCase("cdef"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqualN")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.IsEqualN("abcGHI", 3));
    W_TEST_BOOL(!it.IsEqualN("abcGHI", 4));

    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.IsEqualN("cdeZX", 3));
    W_TEST_BOOL(!it2.IsEqualN("cdeZX", 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqualN_NoCase")
  {
    const char* sz = "ABCDEF";
    WStringView it(sz);

    W_TEST_BOOL(it.IsEqualN_NoCase("abcGHI", 3));
    W_TEST_BOOL(!it.IsEqualN_NoCase("abcGHI", 4));

    WStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    W_TEST_BOOL(it2.IsEqualN_NoCase("cdeZX", 3));
    W_TEST_BOOL(!it2.IsEqualN_NoCase("cdeZX", 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator==/!=")
  {
    const char* sz = "abcdef";
    const char* sz2 = "blabla";
    WStringView it(sz);
    WStringView it2(sz);
    WStringView it3(sz2);

    W_TEST_BOOL(it == sz);
    W_TEST_BOOL(sz == it);
    W_TEST_BOOL(it == "abcdef");
    W_TEST_BOOL("abcdef" == it);
    W_TEST_BOOL(it == it);
    W_TEST_BOOL(it == it2);

    W_TEST_BOOL(it != sz2);
    W_TEST_BOOL(sz2 != it);
    W_TEST_BOOL(it != "blabla");
    W_TEST_BOOL("blabla" != it);
    W_TEST_BOOL(it != it3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "substring operator ==/!=/</>/<=/>=")
  {

    const char* sz1 = "aaabbbcccddd";
    const char* sz2 = "aaabbbdddeee";

    WStringView it1(sz1 + 3, sz1 + 6);
    WStringView it2(sz2 + 3, sz2 + 6);

    W_TEST_BOOL(it1 == it1);
    W_TEST_BOOL(it2 == it2);

    W_TEST_BOOL(it1 == it2);
    W_TEST_BOOL(!(it1 != it2));
    W_TEST_BOOL(!(it1 < it2));
    W_TEST_BOOL(!(it1 > it2));
    W_TEST_BOOL(it1 <= it2);
    W_TEST_BOOL(it1 >= it2);

    it1 = WStringView(sz1 + 3, sz1 + 7);
    it2 = WStringView(sz2 + 3, sz2 + 7);

    W_TEST_BOOL(it1 == it1);
    W_TEST_BOOL(it2 == it2);

    W_TEST_BOOL(it1 != it2);
    W_TEST_BOOL(!(it1 == it2));

    W_TEST_BOOL(it1 < it2);
    W_TEST_BOOL(!(it1 > it2));
    W_TEST_BOOL(it1 <= it2);
    W_TEST_BOOL(!(it1 >= it2));

    W_TEST_BOOL(it2 > it1);
    W_TEST_BOOL(!(it2 < it1));
    W_TEST_BOOL(it2 >= it1);
    W_TEST_BOOL(!(it2 <= it1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator</>")
  {
    const char* sz = "abcdef";
    const char* sz2 = "abcdefg";
    WStringView it(sz);
    WStringView it2(sz2);

    W_TEST_BOOL(it < sz2);
    W_TEST_BOOL(sz < it2);
    W_TEST_BOOL(it < it2);

    W_TEST_BOOL(sz2 > it);
    W_TEST_BOOL(it2 > sz);
    W_TEST_BOOL(it2 > it);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator<=/>=")
  {
    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdefg";
      WStringView it(sz);
      WStringView it2(sz2);

      W_TEST_BOOL(it <= sz2);
      W_TEST_BOOL(sz <= it2);
      W_TEST_BOOL(it <= it2);

      W_TEST_BOOL(sz2 >= it);
      W_TEST_BOOL(it2 >= sz);
      W_TEST_BOOL(it2 >= it);
    }

    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdef";
      WStringView it(sz);
      WStringView it2(sz2);

      W_TEST_BOOL(it <= sz2);
      W_TEST_BOOL(sz <= it2);
      W_TEST_BOOL(it <= it2);

      W_TEST_BOOL(sz2 >= it);
      W_TEST_BOOL(it2 >= sz);
      W_TEST_BOOL(it2 >= it);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindWholeWord")
  {
    WStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    WStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    WStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    W_TEST_BOOL(it.FindWholeWord("abc", WStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[34]);
    W_TEST_BOOL(it.FindWholeWord("def", WStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[38]);
    W_TEST_BOOL(
      it.FindWholeWord("mompfh", WStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[0]); // ü is not English (thus a delimiter)

    W_TEST_BOOL(it.FindWholeWord("abc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    W_TEST_BOOL(it.FindWholeWord("abc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == nullptr);

    W_TEST_BOOL(it2.FindWholeWord("abc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    W_TEST_BOOL(it2.FindWholeWord("abc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == &it.GetStartPointer()[42]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    WStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    WStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    WStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    W_TEST_BOOL(it.FindWholeWord_NoCase("ABC", WStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[34]);
    W_TEST_BOOL(it.FindWholeWord_NoCase("DEF", WStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[38]);
    W_TEST_BOOL(it.FindWholeWord_NoCase("momPFH", WStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[0]);

    W_TEST_BOOL(it.FindWholeWord_NoCase("ABc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    W_TEST_BOOL(it.FindWholeWord_NoCase("ABc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == nullptr);

    W_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    W_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", WStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == &it.GetStartPointer()[42]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ComputeCharacterPosition")
  {
    const wchar_t* sz = L"mompfhüßß ßßß öäü abcdef abc def abc def";
    WStringBuilder s(sz);

    W_TEST_STRING(s.ComputeCharacterPosition(14), WStringUtf8(L"öäü abcdef abc def abc def").GetData());
  }
}
