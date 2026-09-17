#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

#include <string_view>

using namespace std;

const WStringView gConstant1 = "gConstant1"_wsv;
const WStringView gConstant2("gConstant2");
const std::string_view gConstant3 = "gConstant3"sv;

W_CREATE_SIMPLE_TEST(Strings, StringView)
{
  WStringBuilder tmp;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (simple)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    WStringView it(sz);

    W_TEST_BOOL(it.GetStartPointer() == sz);
    W_TEST_STRING(it.GetData(tmp), sz);
    W_TEST_BOOL(it.GetEndPointer() == sz + 26);
    W_TEST_INT(it.GetElementCount(), 26);

    WStringView it2(sz + 15);

    W_TEST_BOOL(it2.GetStartPointer() == &sz[15]);
    W_TEST_STRING(it2.GetData(tmp), &sz[15]);
    W_TEST_BOOL(it2.GetEndPointer() == sz + 26);
    W_TEST_INT(it2.GetElementCount(), 11);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (complex, YARLY!)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    WStringView it(sz + 3, sz + 17);
    it.SetStartPosition(sz + 5);

    W_TEST_BOOL(it.GetStartPointer() == sz + 5);
    W_TEST_STRING(it.GetData(tmp), "fghijklmnopq");
    W_TEST_BOOL(it.GetEndPointer() == sz + 17);
    W_TEST_INT(it.GetElementCount(), 12);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor constexpr")
  {
    constexpr WStringView b = WStringView("Hello World", 10);
    W_TEST_INT(b.GetElementCount(), 10);
    W_TEST_STRING(b.GetData(tmp), "Hello Worl");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "String literal")
  {
    constexpr WStringView a = "Hello World"_wsv;
    W_TEST_INT(a.GetElementCount(), 11);
    W_TEST_STRING(a.GetData(tmp), "Hello World");

    WStringView b = "Hello Worl"_wsv;
    W_TEST_INT(b.GetElementCount(), 10);
    W_TEST_STRING(b.GetData(tmp), "Hello Worl");

    // tests a special case in which the MSVC compiler would run into trouble
    W_TEST_INT(gConstant1.GetElementCount(), 10);
    W_TEST_STRING(gConstant1.GetData(tmp), "gConstant1");

    W_TEST_INT(gConstant2.GetElementCount(), 10);
    W_TEST_STRING(gConstant2.GetData(tmp), "gConstant2");

    W_TEST_INT(gConstant3.size(), 10);
    W_TEST_BOOL(gConstant3 == "gConstant3");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator++")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    WStringView it(sz);

    for (WInt32 i = 0; i < 26; ++i)
    {
      W_TEST_INT(it.GetCharacter(), sz[i]);
      W_TEST_BOOL(it.IsValid());
      it.Shrink(1, 0);
    }

    W_TEST_BOOL(!it.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== / operator!=")
  {
    WString s1(L"abcdefghiäöüß€");
    WString s2(L"ghiäöüß€abdef");

    WStringView it1 = s1.GetSubString(8, 4);
    WStringView it2 = s2.GetSubString(2, 4);
    WStringView it3 = s2.GetSubString(2, 5);

    W_TEST_BOOL(it1 == it2);
    W_TEST_BOOL(it1 != it3);

    W_TEST_BOOL(it1 == WString(L"iäöü").GetData());
    W_TEST_BOOL(it2 == WString(L"iäöü").GetData());
    W_TEST_BOOL(it3 == WString(L"iäöüß").GetData());

    s1 = "abcdefghijkl";
    s2 = "oghijklm";

    it1 = s1.GetSubString(6, 4);
    it2 = s2.GetSubString(1, 4);
    it3 = s2.GetSubString(1, 5);

    W_TEST_BOOL(it1 == it2);
    W_TEST_BOOL(it1 != it3);

    W_TEST_BOOL(it1 == "ghij");
    W_TEST_BOOL(it1 != "ghijk");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    const char* sz = "abcdef";
    WStringView it(sz);

    W_TEST_BOOL(it.IsEqual(WStringView("abcdef")));
    W_TEST_BOOL(!it.IsEqual(WStringView("abcde")));
    W_TEST_BOOL(!it.IsEqual(WStringView("abcdefg")));

    WStringView it2(sz + 2, sz + 5);

    const char* szRhs = "Abcdef";
    WStringView it3(szRhs + 2, szRhs + 5);
    W_TEST_BOOL(it2.IsEqual(it3));
    it3 = WStringView(szRhs + 1, szRhs + 5);
    W_TEST_BOOL(!it2.IsEqual(it3));
    it3 = WStringView(szRhs + 2, szRhs + 6);
    W_TEST_BOOL(!it2.IsEqual(it3));
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

  W_TEST_BLOCK(WTestBlock::Enabled, "operator+=")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    WStringView it(sz);

    for (WInt32 i = 0; i < 26; i += 2)
    {
      W_TEST_INT(it.GetCharacter(), sz[i]);
      W_TEST_BOOL(it.IsValid());
      it.Shrink(2, 0);
    }

    W_TEST_BOOL(!it.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCharacter")
  {
    WStringUtf8 s(L"abcäöü€");
    WStringView it = WStringView(s.GetData());

    W_TEST_INT(it.GetCharacter(), WUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[0]));
    it.Shrink(1, 0);
    W_TEST_INT(it.GetCharacter(), WUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[1]));
    it.Shrink(1, 0);
    W_TEST_INT(it.GetCharacter(), WUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[2]));
    it.Shrink(1, 0);
    W_TEST_INT(it.GetCharacter(), WUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[3]));
    it.Shrink(1, 0);
    W_TEST_INT(it.GetCharacter(), WUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[5]));
    it.Shrink(1, 0);
    W_TEST_INT(it.GetCharacter(), WUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[7]));
    it.Shrink(1, 0);
    W_TEST_INT(it.GetCharacter(), WUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[9]));
    it.Shrink(1, 0);
    W_TEST_BOOL(!it.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetElementCount")
  {
    WStringUtf8 s(L"abcäöü€");
    WStringView it = WStringView(s.GetData());

    W_TEST_INT(it.GetElementCount(), 12);
    it.Shrink(1, 0);
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(it.GetElementCount(), 11);
    it.Shrink(1, 0);
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(it.GetElementCount(), 10);
    it.Shrink(1, 0);
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(it.GetElementCount(), 9);
    it.Shrink(1, 0);
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(it.GetElementCount(), 7);
    it.Shrink(1, 0);
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(it.GetElementCount(), 5);
    it.Shrink(1, 0);
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(it.GetElementCount(), 3);
    it.Shrink(1, 0);
    W_TEST_BOOL(!it.IsValid());
    W_TEST_INT(it.GetElementCount(), 0);
    it.Shrink(1, 0);
    W_TEST_BOOL(!it.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetStartPosition")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    WStringView it(sz);

    for (WInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      W_TEST_BOOL(it.IsValid());
      W_TEST_BOOL(it.StartsWith(&sz[i]));
    }

    W_TEST_BOOL(it.IsValid());
    it.Shrink(1, 0);
    W_TEST_BOOL(!it.IsValid());

    it = WStringView(sz);
    for (WInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      W_TEST_BOOL(it.IsValid());
      W_TEST_BOOL(it.StartsWith(&sz[i]));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetStartPosition / GetEndPosition / GetData")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    WStringView it(sz + 7, sz + 19);

    W_TEST_BOOL(it.GetStartPointer() == sz + 7);
    W_TEST_BOOL(it.GetEndPointer() == sz + 19);
    W_TEST_STRING(it.GetData(tmp), "hijklmnopqrs");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Shrink")
  {
    WStringUtf8 s(L"abcäöü€def");
    WStringView it(s.GetData());

    W_TEST_BOOL(it.GetStartPointer() == &s.GetData()[0]);
    W_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    W_TEST_STRING(it.GetData(tmp), &s.GetData()[0]);
    W_TEST_BOOL(it.IsValid());

    it.Shrink(1, 0);

    W_TEST_BOOL(it.GetStartPointer() == &s.GetData()[1]);
    W_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    W_TEST_STRING(it.GetData(tmp), &s.GetData()[1]);
    W_TEST_BOOL(it.IsValid());

    it.Shrink(3, 0);

    W_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    W_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    W_TEST_STRING(it.GetData(tmp), &s.GetData()[5]);
    W_TEST_BOOL(it.IsValid());

    it.Shrink(0, 4);

    W_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    W_TEST_BOOL(it.GetEndPointer() == &s.GetData()[9]);
    W_TEST_STRING(it.GetData(tmp), (const char*)u8"öü");
    W_TEST_BOOL(it.IsValid());

    it.Shrink(1, 1);

    W_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    W_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    W_TEST_STRING(it.GetData(tmp), "");
    W_TEST_BOOL(!it.IsValid());

    it.Shrink(10, 10);

    W_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    W_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    W_TEST_STRING(it.GetData(tmp), "");
    W_TEST_BOOL(!it.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ChopAwayFirstCharacterUtf8")
  {
    WStringUtf8 utf8(L"О, Господи!");
    WStringView s(utf8.GetData());

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const WUInt32 uiNumCharsBefore = WStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());
      s.ChopAwayFirstCharacterUtf8();
      const WUInt32 uiNumCharsAfter = WStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());

      W_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // this needs to be true, some code relies on the fact that the start pointer always moves forwards
    W_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ChopAwayFirstCharacterAscii")
  {
    WStringUtf8 utf8(L"Wosn Schmarrn");
    WStringView s("");

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const WUInt32 uiNumCharsBefore = s.GetElementCount();
      s.ChopAwayFirstCharacterAscii();
      const WUInt32 uiNumCharsAfter = s.GetElementCount();

      W_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // this needs to be true, some code relies on the fact that the start pointer always moves forwards
    W_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Trim")
  {
    // Empty input
    WStringUtf8 utf8(L"");
    WStringView view(utf8.GetData());
    view.Trim(" \t");
    W_TEST_BOOL(view.IsEqual(WStringUtf8(L"").GetData()));
    view.Trim(nullptr, " \t");
    W_TEST_BOOL(view.IsEqual(WStringUtf8(L"").GetData()));
    view.Trim(" \t", nullptr);
    W_TEST_BOOL(view.IsEqual(WStringUtf8(L"").GetData()));

    // Clear all from one side
    WStringUtf8 sUnicode(L"私はクリストハさんです");
    view = sUnicode.GetData();
    view.Trim(nullptr, sUnicode.GetData());
    W_TEST_BOOL(view.IsEqual(""));
    view = sUnicode.GetData();
    view.Trim(sUnicode.GetData(), nullptr);
    W_TEST_BOOL(view.IsEqual(""));

    // Clear partial side
    sUnicode = L"ですですですAにぱにぱにぱ";
    view = sUnicode.GetData();
    view.Trim(nullptr, WStringUtf8(L"にぱ").GetData());
    sUnicode = L"ですですですA";
    W_TEST_BOOL(view.IsEqual(sUnicode.GetData()));
    view.Trim(WStringUtf8(L"です").GetData(), nullptr);
    W_TEST_BOOL(view.IsEqual(WStringUtf8(L"A").GetData()));

    sUnicode = L"ですですですAにぱにぱにぱ";
    view = sUnicode.GetData();
    view.Trim(WStringUtf8(L"ですにぱ").GetData());
    W_TEST_BOOL(view.IsEqual(WStringUtf8(L"A").GetData()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TrimWordStart")
  {
    WStringView sb;

    {
      sb = "<test>abc<test>";
      W_TEST_BOOL(sb.TrimWordStart("<test>"));
      W_TEST_STRING(sb, "abc<test>");
      W_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      W_TEST_STRING(sb, "abc<test>");
    }

    {
      sb = "<test><tut><test><test><tut>abc<tut><test>";
      W_TEST_BOOL(!sb.TrimWordStart("<tut>"));
      W_TEST_BOOL(sb.TrimWordStart("<test>"));
      W_TEST_BOOL(sb.TrimWordStart("<tut>"));
      W_TEST_BOOL(sb.TrimWordStart("<test>"));
      W_TEST_BOOL(sb.TrimWordStart("<test>"));
      W_TEST_BOOL(sb.TrimWordStart("<tut>"));
      W_TEST_STRING(sb, "abc<tut><test>");
      W_TEST_BOOL(sb.TrimWordStart("<tut>") == false);
      W_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      W_TEST_STRING(sb, "abc<tut><test>");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>abc";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      W_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      W_TEST_STRING(sb, "");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TrimWordEnd")
  {
    WStringView sb;

    {
      sb = "<test>abc<test>";
      W_TEST_BOOL(sb.TrimWordEnd("<test>"));
      W_TEST_STRING(sb, "<test>abc");
      W_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      W_TEST_STRING(sb, "<test>abc");
    }

    {
      sb = "<tut><test>abc<test><tut><test><test><tut>";
      W_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      W_TEST_BOOL(sb.TrimWordEnd("<test>"));
      W_TEST_BOOL(sb.TrimWordEnd("<test>"));
      W_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      W_TEST_BOOL(sb.TrimWordEnd("<test>"));
      W_TEST_STRING(sb, "<tut><test>abc");
      W_TEST_BOOL(sb.TrimWordEnd("<tut>") == false);
      W_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      W_TEST_STRING(sb, "<tut><test>abc");
    }

    {
      sb = "abc<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      W_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      W_TEST_STRING(sb, "");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Split")
  {
    WStringView s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    WDeque<WStringView> SubStrings;

    s.Split(false, SubStrings, ",", "|", "<>");

    W_TEST_INT(SubStrings.GetCount(), 7);
    W_TEST_BOOL(SubStrings[0] == "abc");
    W_TEST_BOOL(SubStrings[1] == "def");
    W_TEST_BOOL(SubStrings[2] == "ghi");
    W_TEST_BOOL(SubStrings[3] == "jkl");
    W_TEST_BOOL(SubStrings[4] == "mno");
    W_TEST_BOOL(SubStrings[5] == "pqr");
    W_TEST_BOOL(SubStrings[6] == "stu");

    s.Split(true, SubStrings, ",", "|", "<>");

    W_TEST_INT(SubStrings.GetCount(), 10);
    W_TEST_BOOL(SubStrings[0] == "");
    W_TEST_BOOL(SubStrings[1] == "abc");
    W_TEST_BOOL(SubStrings[2] == "def");
    W_TEST_BOOL(SubStrings[3] == "ghi");
    W_TEST_BOOL(SubStrings[4] == "");
    W_TEST_BOOL(SubStrings[5] == "");
    W_TEST_BOOL(SubStrings[6] == "jkl");
    W_TEST_BOOL(SubStrings[7] == "mno");
    W_TEST_BOOL(SubStrings[8] == "pqr");
    W_TEST_BOOL(SubStrings[9] == "stu");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HasAnyExtension")
  {
    WStringView p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    W_TEST_BOOL(!p.HasAnyExtension());
    W_TEST_BOOL(!p.HasAnyExtension());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HasExtension")
  {
    WStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.HasExtension(".Extension"));

    p = "This/Is\\My//Path.dot\\file.ext";
    W_TEST_BOOL(p.HasExtension("EXT"));

    p = "This/Is\\My//Path.dot\\file.ext";
    W_TEST_BOOL(!p.HasExtension("NEXT"));

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(!p.HasExtension(".Ext"));

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(!p.HasExtension("sion"));

    p = "";
    W_TEST_BOOL(!p.HasExtension("ext"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileExtension")
  {
    WStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    W_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    W_TEST_BOOL(p.GetFileExtension() == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileNameAndExtension")
  {
    WStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.GetFileNameAndExtension() == "file.extension");

    p = "This/Is\\My//Path.dot\\.extension";
    W_TEST_BOOL(p.GetFileNameAndExtension() == ".extension");

    p = "This/Is\\My//Path.dot\\file";
    W_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "\\file";
    W_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "";
    W_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "/";
    W_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "This/Is\\My//Path.dot\\";
    W_TEST_BOOL(p.GetFileNameAndExtension() == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileName")
  {
    WStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.GetFileName() == "file");

    p = "This/Is\\My//Path.dot\\file";
    W_TEST_BOOL(p.GetFileName() == "file");

    p = "\\file";
    W_TEST_BOOL(p.GetFileName() == "file");

    p = "";
    W_TEST_BOOL(p.GetFileName() == "");

    p = "/";
    W_TEST_BOOL(p.GetFileName() == "");

    p = "This/Is\\My//Path.dot\\";
    W_TEST_BOOL(p.GetFileName() == "");

    p = "This/Is\\My//Path.dot\\.stupidfile";
    W_TEST_BOOL(p.GetFileName() == ".stupidfile");

    p = "This/Is\\My//Path.dot\\.stupidfile.ext";
    W_TEST_BOOL(p.GetFileName() == ".stupidfile");

    p = "This/Is\\My//Path.dot\\.stupidfile.ext.";
    W_TEST_BOOL(p.GetFileName() == ".stupidfile.ext.");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileDirectory")
  {
    WStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\.extension";
    W_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\file";
    W_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "\\file";
    W_TEST_BOOL(p.GetFileDirectory() == "\\");

    p = "";
    W_TEST_BOOL(p.GetFileDirectory() == "");

    p = "/";
    W_TEST_BOOL(p.GetFileDirectory() == "/");

    p = "This/Is\\My//Path.dot\\";
    W_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This";
    W_TEST_BOOL(p.GetFileDirectory() == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsAbsolutePath / IsRelativePath / IsRootedPath")
  {
    WStringView p;

    p = "";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

#if W_ENABLED(W_PLATFORM_WINDOWS)
    p = "C:\\temp.stuff";
    W_TEST_BOOL(p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = "C:/temp.stuff";
    W_TEST_BOOL(p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = "\\\\myserver\\temp.stuff";
    W_TEST_BOOL(p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = "\\myserver\\temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath()); // neither absolute nor relativ, just stupid
    W_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath()); // bloed
    W_TEST_BOOL(!p.IsRootedPath());

    p = "\\temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath()); // bloed
    W_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = ":MyDataDir\bla";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath());
    W_TEST_BOOL(p.IsRootedPath());

    p = ":\\MyDataDir\bla";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath());
    W_TEST_BOOL(p.IsRootedPath());

    p = ":/MyDataDir/bla";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath());
    W_TEST_BOOL(p.IsRootedPath());

#else

    p = "C:\\temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    W_TEST_BOOL(p.IsAbsolutePath());
    W_TEST_BOOL(!p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    W_TEST_BOOL(!p.IsAbsolutePath());
    W_TEST_BOOL(p.IsRelativePath());
    W_TEST_BOOL(!p.IsRootedPath());

#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRootedPathRootName")
  {
    WStringView p;

    p = ":root\\bla";
    W_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":root/bla";
    W_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://root/bla";
    W_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":/\\/root\\/bla";
    W_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://\\root";
    W_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":";
    W_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "";
    W_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "noroot\\bla";
    W_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "C:\\noroot/bla";
    W_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "/noroot/bla";
    W_TEST_BOOL(p.GetRootedPathRootName() == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetSubString")
  {
    WStringView s = u8"Пожалуйста, дай мне очень длинные Unicode-стринги!";

    W_TEST_BOOL(s.GetElementCount() > WStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer()));

    WStringView w1 = s.GetSubString(0, 10);
    WStringView w2 = s.GetSubString(12, 3);
    WStringView w3 = s.GetSubString(20, 5);
    WStringView w4 = s.GetSubString(34, 15);
    WStringView w5 = s.GetSubString(34, 20);
    WStringView w6 = s.GetSubString(100, 10);

    W_TEST_BOOL(w1 == WStringView(u8"Пожалуйста"));
    W_TEST_BOOL(w2 == WStringView(u8"дай"));
    W_TEST_BOOL(w3 == WStringView(u8"очень"));
    W_TEST_BOOL(w4 == WStringView(u8"Unicode-стринги"));
    W_TEST_BOOL(w5 == WStringView(u8"Unicode-стринги!"));
    W_TEST_BOOL(!w6.IsValid());
    W_TEST_BOOL(w6 == WStringView(""));
  }
}
