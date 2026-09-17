#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

// this file takes ages to compile in a Release build
// since we don't care for runtime performance, just disable all optimizations
#pragma optimize("", off)

W_CREATE_SIMPLE_TEST(Strings, StringBuilder)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(empty)")
  {
    WStringBuilder s;

    W_TEST_BOOL(s.IsEmpty());
    W_TEST_INT(s.GetCharacterCount(), 0);
    W_TEST_INT(s.GetElementCount(), 0);
    W_TEST_BOOL(s == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(Utf8)")
  {
    WStringUtf8 sUtf8(L"abc äöü € def");
    WStringBuilder s(sUtf8.GetData());

    W_TEST_BOOL(s.GetData() != sUtf8.GetData());
    W_TEST_BOOL(s == sUtf8.GetData());
    W_TEST_INT(s.GetElementCount(), 18);
    W_TEST_INT(s.GetCharacterCount(), 13);

    WStringBuilder s2("test test");

    W_TEST_BOOL(s2 == "test test");
    W_TEST_INT(s2.GetElementCount(), 9);
    W_TEST_INT(s2.GetCharacterCount(), 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(wchar_t)")
  {
    WStringUtf8 sUtf8(L"abc äöü € def");
    WStringBuilder s(L"abc äöü € def");

    W_TEST_BOOL(s == sUtf8.GetData());
    W_TEST_INT(s.GetElementCount(), 18);
    W_TEST_INT(s.GetCharacterCount(), 13);

    WStringBuilder s2(L"test test");

    W_TEST_BOOL(s2 == "test test");
    W_TEST_INT(s2.GetElementCount(), 9);
    W_TEST_INT(s2.GetCharacterCount(), 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(copy)")
  {
    WStringUtf8 sUtf8(L"abc äöü € def");
    WStringBuilder s(L"abc äöü € def");
    WStringBuilder s2(s);

    W_TEST_BOOL(s2 == sUtf8.GetData());
    W_TEST_INT(s2.GetElementCount(), 18);
    W_TEST_INT(s2.GetCharacterCount(), 13);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(StringView)")
  {
    WStringUtf8 sUtf8(L"abc äöü € def");

    WStringView it(sUtf8.GetData() + 2, sUtf8.GetData() + 8);

    WStringBuilder s(it);

    W_TEST_INT(s.GetElementCount(), 6);
    W_TEST_INT(s.GetCharacterCount(), 4);
    W_TEST_BOOL(s == WStringUtf8(L"c äö").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(multiple)")
  {
    WStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    WStringUtf8 sUtf2(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");

    WStringBuilder sb(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData());

    W_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=(Utf8)")
  {
    WStringUtf8 sUtf8(L"abc äöü € def");
    WStringBuilder s("bla");
    s = sUtf8.GetData();

    W_TEST_BOOL(s.GetData() != sUtf8.GetData());
    W_TEST_BOOL(s == sUtf8.GetData());
    W_TEST_INT(s.GetElementCount(), 18);
    W_TEST_INT(s.GetCharacterCount(), 13);

    WStringBuilder s2("bla");
    s2 = "test test";

    W_TEST_BOOL(s2 == "test test");
    W_TEST_INT(s2.GetElementCount(), 9);
    W_TEST_INT(s2.GetCharacterCount(), 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=(wchar_t)")
  {
    WStringUtf8 sUtf8(L"abc äöü € def");
    WStringBuilder s("bla");
    s = L"abc äöü € def";

    W_TEST_BOOL(s == sUtf8.GetData());
    W_TEST_INT(s.GetElementCount(), 18);
    W_TEST_INT(s.GetCharacterCount(), 13);

    WStringBuilder s2("bla");
    s2 = L"test test";

    W_TEST_BOOL(s2 == "test test");
    W_TEST_INT(s2.GetElementCount(), 9);
    W_TEST_INT(s2.GetCharacterCount(), 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=(copy)")
  {
    WStringUtf8 sUtf8(L"abc äöü € def");
    WStringBuilder s(L"abc äöü € def");
    WStringBuilder s2;
    s2 = s;

    W_TEST_BOOL(s2 == sUtf8.GetData());
    W_TEST_INT(s2.GetElementCount(), 18);
    W_TEST_INT(s2.GetCharacterCount(), 13);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=(StringView)")
  {
    WStringBuilder s("abcdefghi");
    WStringView it(s.GetData() + 2, s.GetData() + 8);
    it.SetStartPosition(s.GetData() + 3);

    s = it;

    W_TEST_BOOL(s == "defgh");
    W_TEST_INT(s.GetElementCount(), 5);
    W_TEST_INT(s.GetCharacterCount(), 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "convert to WStringView")
  {
    WStringBuilder s(L"aölsdföasld");
    WStringBuilder tmp;

    WStringView sv = s;

    W_TEST_STRING(sv.GetData(tmp), WStringUtf8(L"aölsdföasld").GetData());
    W_TEST_BOOL(sv == WStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    W_TEST_STRING(sv.GetStartPointer(), "abcdef");
    W_TEST_BOOL(sv == "abcdef");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    WStringBuilder s(L"abc äöü € def");

    W_TEST_BOOL(!s.IsEmpty());

    s.Clear();
    W_TEST_BOOL(s.IsEmpty());
    W_TEST_INT(s.GetElementCount(), 0);
    W_TEST_INT(s.GetCharacterCount(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetElementCount / GetCharacterCount")
  {
    WStringBuilder s(L"abc äöü € def");

    W_TEST_INT(s.GetElementCount(), 18);
    W_TEST_INT(s.GetCharacterCount(), 13);

    s = "abc";

    W_TEST_INT(s.GetElementCount(), 3);
    W_TEST_INT(s.GetCharacterCount(), 3);

    s = L"Hällo! I love €";

    W_TEST_INT(s.GetElementCount(), 18);
    W_TEST_INT(s.GetCharacterCount(), 15);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Append(single unicode char)")
  {
    WStringUtf32 u32(L"äöüß");

    WStringBuilder s("abc");
    W_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(u32.GetData()[0]);
    W_TEST_INT(s.GetCharacterCount(), 4);

    W_TEST_BOOL(s == WStringUtf8(L"abcä").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Prepend(single unicode char)")
  {
    WStringUtf32 u32(L"äöüß");

    WStringBuilder s("abc");
    W_TEST_INT(s.GetCharacterCount(), 3);
    s.Prepend(u32.GetData()[0]);
    W_TEST_INT(s.GetCharacterCount(), 4);

    W_TEST_BOOL(s == WStringUtf8(L"äabc").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Append(char)")
  {
    WStringBuilder s("abc");
    W_TEST_INT(s.GetCharacterCount(), 3);
    s.Append("de", "fg", "hi", WStringUtf8(L"öä").GetData(), "jk", WStringUtf8(L"ü€").GetData());
    W_TEST_INT(s.GetCharacterCount(), 15);

    W_TEST_BOOL(s == WStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, "b", nullptr, "d", nullptr, WStringUtf8(L"ü€").GetData());
    W_TEST_BOOL(s == WStringUtf8(L"pupsbdü€").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Append(wchar_t)")
  {
    WStringBuilder s("abc");
    W_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");
    W_TEST_INT(s.GetCharacterCount(), 15);

    W_TEST_BOOL(s == WStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    W_TEST_BOOL(s == WStringUtf8(L"pupsbdü€").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Append(multiple)")
  {
    WStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    WStringUtf8 sUtf2(L"Test⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    WStringBuilder sb("Test");
    sb.Append(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    W_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Set(multiple)")
  {
    WStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    WStringUtf8 sUtf2(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    WStringBuilder sb("Test");
    sb.Set(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    W_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AppendFormat")
  {
    WStringBuilder s("abc");
    s.AppendFormat("Test{0}{1}{2}", 42, "foo", WStringUtf8(L"bär").GetData());

    W_TEST_BOOL(s == WStringUtf8(L"abcTest42foobär").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Prepend(char)")
  {
    WStringBuilder s("abc");
    s.Prepend("de", "fg", "hi", WStringUtf8(L"öä").GetData(), "jk", WStringUtf8(L"ü€").GetData());

    W_TEST_BOOL(s == WStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, "b", nullptr, "d", nullptr, WStringUtf8(L"ü€").GetData());
    W_TEST_BOOL(s == WStringUtf8(L"bdü€pups").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Prepend(wchar_t)")
  {
    WStringBuilder s("abc");
    s.Prepend(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");

    W_TEST_BOOL(s == WStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    W_TEST_BOOL(s == WStringUtf8(L"bdü€pups").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PrependFormat")
  {
    WStringBuilder s("abc");
    s.PrependFormat("Test{0}{1}{2}", 42, "foo", WStringUtf8(L"bär").GetData());

    W_TEST_BOOL(s == WStringUtf8(L"Test42foobärabc").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Printf")
  {
    WStringBuilder s("abc");
    s.SetPrintf("Test%i%s%s", 42, "foo", WStringUtf8(L"bär").GetData());

    W_TEST_BOOL(s == WStringUtf8(L"Test42foobär").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFormat")
  {
    WStringBuilder s("abc");
    s.SetFormat("Test{0}{1}{2}", 42, "foo", WStringUtf8(L"bär").GetData());
    W_TEST_BOOL(s == WStringUtf8(L"Test42foobär").GetData());

    s.SetFormat("%%процент{}%%", 100);
    W_TEST_BOOL(s == WStringUtf8(L"%процент100%").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ToUpper")
  {
    WStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.ToUpper();
    W_TEST_BOOL(s == WStringUtf8(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ToLower")
  {
    WStringBuilder s(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß");
    s.ToLower();
    W_TEST_BOOL(s == WStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Shrink")
  {
    WStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.Shrink(5, 3);

    W_TEST_BOOL(s == WStringUtf8(L"fghijklmnopqrstuvwxyzäö").GetData());

    s.Shrink(9, 7);
    W_TEST_BOOL(s == WStringUtf8(L"opqrstu").GetData());

    s.Shrink(3, 2);
    W_TEST_BOOL(s == WStringUtf8(L"rs").GetData());

    s.Shrink(1, 0);
    W_TEST_BOOL(s == WStringUtf8(L"s").GetData());

    s.Shrink(0, 0);
    W_TEST_BOOL(s == WStringUtf8(L"s").GetData());

    s.Shrink(0, 1);
    W_TEST_BOOL(s == WStringUtf8(L"").GetData());

    s.Shrink(10, 0);
    W_TEST_BOOL(s == WStringUtf8(L"").GetData());

    s.Shrink(0, 10);
    W_TEST_BOOL(s == WStringUtf8(L"").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Reserve")
  {
    WHeapAllocator allocator("reserve test allocator");
    WStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß", &allocator);
    WUInt32 characterCountBefore = s.GetCharacterCount();

    s.Reserve(2048);

    W_TEST_BOOL(s.GetCharacterCount() == characterCountBefore);

    WUInt64 iNumAllocs = allocator.GetStats().m_uiNumAllocations;
    s.Append("blablablablablablablablablablablablablablablablablablablablablablablablablablablablablabla");
    W_TEST_BOOL(iNumAllocs == allocator.GetStats().m_uiNumAllocations);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Convert to StringView")
  {
    WStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    WStringView it = s;

    W_TEST_BOOL(it.StartsWith(WStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
    W_TEST_BOOL(it.EndsWith(WStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ChangeCharacter")
  {
    WStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    WStringUtf8 upr(L"ÄÖÜ€ßABCDEFGHIJKLMNOPQRSTUVWXYZ");
    WStringView view(upr.GetData());

    for (auto it = begin(s); it.IsValid(); ++it, view.Shrink(1, 0))
    {
      s.ChangeCharacter(it, view.GetCharacter());

      W_TEST_BOOL(it.GetCharacter() == view.GetCharacter()); // iterator reflects the changes
    }

    W_TEST_BOOL(s == upr.GetData());
    W_TEST_INT(s.GetCharacterCount(), 31);
    W_TEST_INT(s.GetElementCount(), 37);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceSubString")
  {
    WStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    s.ReplaceSubString(s.GetData() + 3, s.GetData() + 7, "DEFG"); // equal length, equal num characters
    W_TEST_BOOL(s == WStringUtf8(L"abcDEFGhijklmnopqrstuvwxyzäöü€ß").GetData());
    W_TEST_INT(s.GetCharacterCount(), 31);
    W_TEST_INT(s.GetElementCount(), 37);

    s.ReplaceSubString(s.GetData() + 7, s.GetData() + 15, ""); // remove
    W_TEST_BOOL(s == WStringUtf8(L"abcDEFGpqrstuvwxyzäöü€ß").GetData());
    W_TEST_INT(s.GetCharacterCount(), 23);
    W_TEST_INT(s.GetElementCount(), 29);

    s.ReplaceSubString(s.GetData() + 17, s.GetData() + 22, "blablub"); // make longer
    W_TEST_BOOL(s == WStringUtf8(L"abcDEFGpqrstuvwxyblablubü€ß").GetData());
    W_TEST_INT(s.GetCharacterCount(), 27);
    W_TEST_INT(s.GetElementCount(), 31);

    s.ReplaceSubString(s.GetData() + 22, s.GetData() + 22, WStringUtf8(L"määh!").GetData()); // insert
    W_TEST_BOOL(s == WStringUtf8(L"abcDEFGpqrstuvwxyblablmääh!ubü€ß").GetData());
    W_TEST_INT(s.GetCharacterCount(), 32);
    W_TEST_INT(s.GetElementCount(), 38);

    s.ReplaceSubString(s.GetData(), s.GetData() + 10, nullptr); // remove at front
    W_TEST_BOOL(s == WStringUtf8(L"stuvwxyblablmääh!ubü€ß").GetData());
    W_TEST_INT(s.GetCharacterCount(), 22);
    W_TEST_INT(s.GetElementCount(), 28);

    s.ReplaceSubString(s.GetData() + 18, s.GetData() + 28, nullptr); // remove at back
    W_TEST_BOOL(s == WStringUtf8(L"stuvwxyblablmääh").GetData());
    W_TEST_INT(s.GetCharacterCount(), 16);
    W_TEST_INT(s.GetElementCount(), 18);

    s.ReplaceSubString(s.GetData(), s.GetData() + 18, nullptr); // clear
    W_TEST_BOOL(s == WStringUtf8(L"").GetData());
    W_TEST_INT(s.GetCharacterCount(), 0);
    W_TEST_INT(s.GetElementCount(), 0);

    const char* szInsert = "abc def ghi";

    s.ReplaceSubString(s.GetData(), s.GetData(), WStringView(szInsert, szInsert + 7)); // partial insert into empty
    W_TEST_BOOL(s == WStringUtf8(L"abc def").GetData());
    W_TEST_INT(s.GetCharacterCount(), 7);
    W_TEST_INT(s.GetElementCount(), 7);

    // insert very large block
    s = WStringBuilder("a"); // hard reset to keep buffer small
    WString insertString("omfg this string is so long it possibly won't never ever ever ever fit into the current buffer - this will "
                          "hopefully lead to a buffer resize :)"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................");
    s.ReplaceSubString(s.GetData(), s.GetData() + s.GetElementCount(), insertString.GetData());
    W_TEST_BOOL(s == insertString.GetData());
    W_TEST_INT(s.GetCharacterCount(), insertString.GetCharacterCount());
    W_TEST_INT(s.GetElementCount(), insertString.GetElementCount());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Insert")
  {
    WStringBuilder s;

    s.Insert(s.GetData(), "test");
    W_TEST_BOOL(s == "test");

    s.Insert(s.GetData() + 2, "TUT");
    W_TEST_BOOL(s == "teTUTst");

    s.Insert(s.GetData(), "MOEP");
    W_TEST_BOOL(s == "MOEPteTUTst");

    s.Insert(s.GetData() + s.GetElementCount(), "hompf");
    W_TEST_BOOL(s == "MOEPteTUTsthompf");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Remove")
  {
    WStringBuilder s("MOEPteTUTsthompf");

    s.Remove(s.GetData() + 11, s.GetData() + s.GetElementCount());
    W_TEST_BOOL(s == "MOEPteTUTst");

    s.Remove(s.GetData(), s.GetData() + 4);
    W_TEST_BOOL(s == "teTUTst");

    s.Remove(s.GetData() + 2, s.GetData() + 5);
    W_TEST_BOOL(s == "test");

    s.Remove(s.GetData(), s.GetData() + s.GetElementCount());
    W_TEST_BOOL(s == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceFirst")
  {
    WStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst("def", "BLOED");
    W_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED");
    W_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED", s.GetData() + 15);
    W_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    W_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    W_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("def", "OEDE");
    W_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("abc", "BLOEDE");
    W_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    W_TEST_BOOL(s == "weg");

    s.ReplaceFirst("weg", nullptr);
    W_TEST_BOOL(s == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceLast")
  {
    WStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast("abc", "ABC");
    W_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    W_TEST_BOOL(s == "abc def ABC def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    W_TEST_BOOL(s == "ABC def ABC def ghi ABC ghi");

    s.ReplaceLast("ghi", "GHI", s.GetData() + 24);
    W_TEST_BOOL(s == "ABC def ABC def GHI ABC ghi");

    s.ReplaceLast("i", "I");
    W_TEST_BOOL(s == "ABC def ABC def GHI ABC ghI");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceAll")
  {
    WStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll("abc", "TEST");
    W_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "def");
    W_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    W_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    W_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll("def", " ");
    W_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll(" ", "");
    W_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll("TEST", "a");
    W_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll("hi", "hihi");
    W_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll("ag", " ");
    W_TEST_BOOL(s == "a hihi hihi");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceFirst_NoCase")
  {
    WStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst_NoCase("DEF", "BLOED");
    W_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED");
    W_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED", s.GetData() + 15);
    W_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    W_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    W_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("DEF", "OEDE");
    W_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("ABC", "BLOEDE");
    W_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    W_TEST_BOOL(s == "weg");

    s.ReplaceFirst_NoCase("WEG", nullptr);
    W_TEST_BOOL(s == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceLast_NoCase")
  {
    WStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast_NoCase("abc", "ABC");
    W_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("aBc", "ABC");
    W_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("ABC", "ABC");
    W_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("GHI", "GHI", s.GetData() + 24);
    W_TEST_BOOL(s == "abc def abc def GHI ABC ghi");

    s.ReplaceLast_NoCase("I", "I");
    W_TEST_BOOL(s == "abc def abc def GHI ABC ghI");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceAll_NoCase")
  {
    WStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll_NoCase("ABC", "TEST");
    W_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "def");
    W_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    W_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    W_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", " ");
    W_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll_NoCase(" ", "");
    W_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll_NoCase("teST", "a");
    W_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll_NoCase("hI", "hihi");
    W_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll_NoCase("Ag", " ");
    W_TEST_BOOL(s == "a hihi hihi");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceWholeWord")
  {
    WStringBuilder s = "abcd abc abcd abc dabc abc";

    W_TEST_BOOL(s.ReplaceWholeWord("abc", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    W_TEST_BOOL(s.ReplaceWholeWord("abc", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "abcd def abcd def dabc abc");

    W_TEST_BOOL(s.ReplaceWholeWord("abc", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord("abc", "def", WStringUtils::IsWordDelimiter_English) == nullptr);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "def def abcd def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "def def def def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", WStringUtils::IsWordDelimiter_English) == nullptr);
    W_TEST_BOOL(s == "def def def def dabc def");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceWholeWord_NoCase")
  {
    WStringBuilder s = "abcd abc abcd abc dabc abc";

    W_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    W_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "abcd def abcd def dabc abc");

    W_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", WStringUtils::IsWordDelimiter_English) == nullptr);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABCd", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "def def abcd def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord_NoCase("aBCD", "def", WStringUtils::IsWordDelimiter_English) != nullptr);
    W_TEST_BOOL(s == "def def def def dabc def");

    W_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABcd", "def", WStringUtils::IsWordDelimiter_English) == nullptr);
    W_TEST_BOOL(s == "def def def def dabc def");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceWholeWordAll")
  {
    WStringBuilder s = "abcd abc abcd abc dabc abc";

    W_TEST_INT(s.ReplaceWholeWordAll("abc", "def", WStringUtils::IsWordDelimiter_English), 3);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_INT(s.ReplaceWholeWordAll("abc", "def", WStringUtils::IsWordDelimiter_English), 0);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", WStringUtils::IsWordDelimiter_English), 2);
    W_TEST_BOOL(s == "def def def def dabc def");

    W_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", WStringUtils::IsWordDelimiter_English), 0);
    W_TEST_BOOL(s == "def def def def dabc def");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceWholeWordAll_NoCase")
  {
    WStringBuilder s = "abcd abc abcd abc dabc abc";

    W_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", WStringUtils::IsWordDelimiter_English), 3);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", WStringUtils::IsWordDelimiter_English), 0);
    W_TEST_BOOL(s == "abcd def abcd def dabc def");

    W_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", WStringUtils::IsWordDelimiter_English), 2);
    W_TEST_BOOL(s == "def def def def dabc def");

    W_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", WStringUtils::IsWordDelimiter_English), 0);
    W_TEST_BOOL(s == "def def def def dabc def");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "teset")
  {
    const char* sz = "abc def";
    WStringView it(sz);

    WStringBuilder s = it;
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Split")
  {
    WStringBuilder s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    WTempHybridArray<WStringView, 32> SubStrings;

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


  W_TEST_BLOCK(WTestBlock::Enabled, "MakeCleanPath")
  {
    WStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "C:/temp/temp/tut");

    p = "\\temp/temp//tut\\\\";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "/temp/temp/tut/");

    p = "\\";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "/");

    p = "file";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "file");

    p = "C:\\temp/..//tut";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "C:/tut");

    p = "C:\\temp/..";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "C:/temp/..");

    p = "C:\\temp/..\\";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "C:/");

    p = "\\//temp/../bla\\\\blub///..\\temp//tut/tat/..\\\\..\\//ploep";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "//bla/temp/ploep");

    p = "a/b/c/../../../../e/f";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "../e/f");

    p = "/../../a/../../e/f";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "../../e/f");

    p = "/../../a/../../e/f/../";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "../../e/");

    p = "/../../a/../../e/f/..";
    p.MakeCleanPath();
    W_TEST_BOOL(p == "../../e/f/..");

    p = "\\//temp/./bla\\\\blub///.\\temp//tut/tat/..\\.\\.\\//ploep";
    p.MakeCleanPath();
    W_TEST_STRING(p.GetData(), "//temp/bla/blub/temp/tut/ploep");

    p = "./";
    p.MakeCleanPath();
    W_TEST_STRING(p.GetData(), "");

    p = "/./././";
    p.MakeCleanPath();
    W_TEST_STRING(p.GetData(), "/");

    p = "./.././";
    p.MakeCleanPath();
    W_TEST_STRING(p.GetData(), "../");

    // more than two dots are invalid, so the should be kept as is
    p = "./..././abc/...\\def";
    p.MakeCleanPath();
    W_TEST_STRING(p.GetData(), ".../abc/.../def");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PathParentDirectory")
  {
    WStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.PathParentDirectory();
    W_TEST_BOOL(p == "C:/temp/temp/");

    p = "C:\\temp/temp//tut\\\\";
    p.PathParentDirectory();
    W_TEST_BOOL(p == "C:/temp/temp/");

    p = "file";
    p.PathParentDirectory();
    W_TEST_BOOL(p == "");

    p = "/file";
    p.PathParentDirectory();
    W_TEST_BOOL(p == "/");

    p = "C:\\temp/..//tut";
    p.PathParentDirectory();
    W_TEST_BOOL(p == "C:/");

    p = "file";
    p.PathParentDirectory(3);
    W_TEST_BOOL(p == "../../");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AppendPath")
  {
    WStringBuilder p;

    p = "this/is\\my//path";
    p.AppendPath("orly/nowai");
    W_TEST_BOOL(p == "this/is\\my//path/orly/nowai");

    p = "this/is\\my//path///";
    p.AppendPath("orly/nowai");
    W_TEST_BOOL(p == "this/is\\my//path///orly/nowai");

    p = "";
    p.AppendPath("orly/nowai");
    W_TEST_BOOL(p == "orly/nowai");

    // It should be valid to append an absolute path to an empty string.
    {
#if W_ENABLED(W_PLATFORM_WINDOWS)
      const char* szAbsPath = "C:\\folder";
      const char* szAbsPathAppendResult = "C:\\folder/File.ext";
#else
      const char* szAbsPath = "/folder";
      const char* szAbsPathAppendResult = "/folder/File.ext";
#endif

      p = "";
      p.AppendPath(szAbsPath, "File.ext");
      W_TEST_BOOL(p == szAbsPathAppendResult);
    }

    p = "bla";
    p.AppendPath("");
    W_TEST_BOOL(p == "bla");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ChangeFileName")
  {
    WStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileName("bla");
    W_TEST_BOOL(p == "C:/test/test/bla.ext");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileName("toeff");
    W_TEST_BOOL(p == "test/test/tut/toeff.toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileName("toeff");
    W_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf/";
    p.ChangeFileName("toeff");
    W_TEST_BOOL(p == "test/test/tut/murpf/toeff"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.file";                // files that start with a dot are considered to be filenames with no extension
    p.ChangeFileName("toeff");
    W_TEST_BOOL(p == "test/test/tut/murpf/toeff");

    p = "test/test/tut/murpf/.file.extension";
    p.ChangeFileName("toeff");
    W_TEST_BOOL(p == "test/test/tut/murpf/toeff.extension");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileName("toeff");
    W_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ChangeFileNameAndExtension")
  {
    WStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileNameAndExtension("bla.pups");
    W_TEST_BOOL(p == "C:/test/test/bla.pups");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileNameAndExtension("toeff");
    W_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileNameAndExtension("toeff.tut");
    W_TEST_BOOL(p == "test/test/tut/toeff.tut");

    p = "test/test/tut/murpf/";
    p.ChangeFileNameAndExtension("toeff.blo");
    W_TEST_BOOL(p == "test/test/tut/murpf/toeff.blo"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.extension";               // folders that start with a dot must be considered to be empty filenames with an extension
    p.ChangeFileNameAndExtension("toeff.ext");
    W_TEST_BOOL(p == "test/test/tut/murpf/toeff.ext");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileNameAndExtension("toeff");
    W_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ChangeFileExtension")
  {
    WStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("pups");
    W_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("pups");
    W_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("");
    W_TEST_BOOL(p == "C:/test/test/tut.");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("");
    W_TEST_BOOL(p == "C:/test/test/tut.");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HasAnyExtension")
  {
    WStringBuilder p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    W_TEST_BOOL(!p.HasAnyExtension());
    W_TEST_BOOL(!p.HasAnyExtension());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HasExtension")
  {
    WStringBuilder p;

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
    WStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    W_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    W_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    W_TEST_BOOL(p.GetFileExtension() == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileNameAndExtension")
  {
    WStringBuilder p;

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
    WStringBuilder p;

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
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileDirectory")
  {
    WStringBuilder p;

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
    WStringBuilder p;

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
    WStringBuilder p;

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

  W_TEST_BLOCK(WTestBlock::Enabled, "IsPathBelowFolder")
  {
    WStringBuilder p;

    p = "a/b\\c//d\\\\e/f";
    W_TEST_BOOL(!p.IsPathBelowFolder("/a/b\\c"));
    W_TEST_BOOL(p.IsPathBelowFolder("a/b\\c"));
    W_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//"));
    W_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//d/\\e\\f")); // equal paths are considered 'below'
    W_TEST_BOOL(!p.IsPathBelowFolder("a/b\\c//d/\\e\\f/g"));
    W_TEST_BOOL(p.IsPathBelowFolder("a"));
    W_TEST_BOOL(!p.IsPathBelowFolder("b"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRelativeTo")
  {
    WStringBuilder p;

    p = (const char*)u8"ä/b\\c/d\\\\e/f/g";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c").Succeeded());
    W_TEST_BOOL(p == "d/e/f/g");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c").Failed());
    W_TEST_BOOL(p == "d/e/f/g");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/g";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c").Succeeded());
    W_TEST_BOOL(p == "d/e/f/g");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c").Failed());
    W_TEST_BOOL(p == "d/e/f/g");

    p = (const char*)u8"ä/b\\c/d\\\\e/f/g";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c/").Succeeded());
    W_TEST_BOOL(p == "d/e/f/g");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c/").Failed());
    W_TEST_BOOL(p == "d/e/f/g");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/g";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c/").Succeeded());
    W_TEST_BOOL(p == "d/e/f/g");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c/").Failed());
    W_TEST_BOOL(p == "d/e/f/g");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/g";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c\\/d/\\e\\f/g").Succeeded());
    W_TEST_BOOL(p == "");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c\\/d/\\e\\f/g").Failed());
    W_TEST_BOOL(p == "");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/g/";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c\\/d//e\\f/g\\h/i").Succeeded());
    W_TEST_BOOL(p == "../../");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c\\/d//e\\f/g\\h/i").Failed());
    W_TEST_BOOL(p == "../../");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/g/j/k";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c\\/d//e\\f/g\\h/i").Succeeded());
    W_TEST_BOOL(p == "../../j/k");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c\\/d//e\\f/g\\h/i").Failed());
    W_TEST_BOOL(p == "../../j/k");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/ge";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c//d/\\e\\f/g\\h/i").Succeeded());
    W_TEST_BOOL(p == "../../../ge");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c//d/\\e\\f/g\\h/i").Failed());
    W_TEST_BOOL(p == "../../../ge");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/g.txt";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c//d//e\\f/g\\h/i").Succeeded());
    W_TEST_BOOL(p == "../../../g.txt");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c//d//e\\f/g\\h/i").Failed());
    W_TEST_BOOL(p == "../../../g.txt");

    p = (const char*)u8"ä/b\\c//d\\\\e/f/g";
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c//d//e\\f/g\\h/i").Succeeded());
    W_TEST_BOOL(p == "../../");
    W_TEST_BOOL(p.MakeRelativeTo((const char*)u8"ä\\b/c//d//e\\f/g\\h/i").Failed());
    W_TEST_BOOL(p == "../../");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakePathSeparatorsNative")
  {
    WStringBuilder p;
    p = "This/is\\a/temp\\\\path//to/my///file";

    p.MakePathSeparatorsNative();

#if W_ENABLED(W_PLATFORM_WINDOWS)
    W_TEST_STRING(p.GetData(), "This\\is\\a\\temp\\path\\to\\my\\file");
#else
    W_TEST_STRING(p.GetData(), "This/is/a/temp/path/to/my/file");
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadAll")
  {
    WDefaultMemoryStreamStorage StreamStorage;

    WMemoryStreamWriter MemoryWriter(&StreamStorage);
    WMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText =
      "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, WStringUtils::GetStringElementCount(szText)).IgnoreResult();

    WStringBuilder s;
    s.ReadAll(MemoryReader);

    W_TEST_BOOL(s == szText);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadAll (Utf8 BOM)")
  {
    WDefaultMemoryStreamStorage StreamStorage;

    WMemoryStreamWriter MemoryWriter(&StreamStorage);
    WMemoryStreamReader MemoryReader(&StreamStorage);

    // written as raw bytes, so that no editor can turn the BOM into something else
    const WUInt8 textWithBom[] = {0xEF, 0xBB, 0xBF, '#', ' ', 'S', 'o', 'm', 'e', ' ', 'T', 'e', 'x', 't'};

    MemoryWriter.WriteBytes(textWithBom, W_ARRAY_SIZE(textWithBom)).IgnoreResult();

    WStringBuilder s;
    s.ReadAll(MemoryReader);

    // the BOM is an encoding marker and must not show up in the content
    W_TEST_BOOL(s == "# Some Text");
    W_TEST_BOOL(s.StartsWith("#"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetSubString_FromTo")
  {
    WStringBuilder sb = "basf";

    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    sb.SetSubString_FromTo(sz + 5, sz + 13);
    W_TEST_BOOL(sb == "fghijklm");

    sb.SetSubString_FromTo(sz + 17, sz + 30);
    W_TEST_BOOL(sb == "rstuvwxyz");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetSubString_ElementCount")
  {
    WStringBuilder sb = "basf";

    WStringUtf8 sz(L"aäbcödefügh");

    sb.SetSubString_ElementCount(sz.GetData() + 5, 5);
    W_TEST_BOOL(sb == WStringUtf8(L"ödef").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetSubString_CharacterCount")
  {
    WStringBuilder sb = "basf";

    WStringUtf8 sz(L"aäbcödefgh");

    sb.SetSubString_CharacterCount(sz.GetData() + 5, 5);
    W_TEST_BOOL(sb == WStringUtf8(L"ödefg").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveFileExtension")
  {
    WStringBuilder sb = L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺.";

    sb.RemoveFileExtension();
    W_TEST_STRING_UNICODE(sb.GetData(), WStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺").GetData());

    sb.RemoveFileExtension();
    W_TEST_STRING_UNICODE(sb.GetData(), WStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴").GetData());

    sb.RemoveFileExtension();
    W_TEST_STRING_UNICODE(sb.GetData(), WStringUtf8(L"⺅⻩⽇⿕").GetData());

    sb.RemoveFileExtension();
    W_TEST_STRING_UNICODE(sb.GetData(), WStringUtf8(L"⺅⻩⽇⿕").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Trim")
  {
    // Empty input
    WStringBuilder sb = L"";
    sb.Trim(" \t");
    W_TEST_STRING(sb.GetData(), WStringUtf8(L"").GetData());
    sb.Trim(nullptr, " \t");
    W_TEST_STRING(sb.GetData(), WStringUtf8(L"").GetData());
    sb.Trim(" \t", nullptr);
    W_TEST_STRING(sb.GetData(), WStringUtf8(L"").GetData());

    // Clear all from one side
    auto sUnicode = L"私はクリストハさんです";
    sb = sUnicode;
    sb.Trim(nullptr, WStringUtf8(sUnicode).GetData());
    W_TEST_STRING(sb.GetData(), "");
    sb = sUnicode;
    sb.Trim(WStringUtf8(sUnicode).GetData(), nullptr);
    W_TEST_STRING(sb.GetData(), "");

    // Clear partial side
    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(nullptr, WStringUtf8(L"にぱ").GetData());
    W_TEST_STRING_UNICODE(sb.GetData(), WStringUtf8(L"ですですですA").GetData());
    sb.Trim(WStringUtf8(L"です").GetData(), nullptr);
    W_TEST_STRING_UNICODE(sb.GetData(), WStringUtf8(L"A").GetData());

    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(WStringUtf8(L"ですにぱ").GetData());
    W_TEST_STRING(sb.GetData(), WStringUtf8(L"A").GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TrimWordStart")
  {
    WStringBuilder sb;

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
    WStringBuilder sb;

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

  W_TEST_BLOCK(WTestBlock::Enabled, "RemoveCStyleComments")
  {
    WStringBuilder sb;

    // no comments
    sb = "outPos = inPos;";
    sb.RemoveCStyleComments();
    W_TEST_STRING(sb, "outPos = inPos;");

    // line comment at end
    sb = "outPos = inPos; // move position";
    sb.RemoveCStyleComments();
    W_TEST_STRING(sb, "outPos = inPos; ");

    // line comment covers a stream name
    sb = "// outDiscard = 1;";
    sb.RemoveCStyleComments();
    W_TEST_BOOL(sb.FindSubString("outDiscard") == nullptr);

    // block comment covers a stream name
    sb = "/* outDiscard = 1; */";
    sb.RemoveCStyleComments();
    W_TEST_BOOL(sb.FindSubString("outDiscard") == nullptr);

    // block comment preserves newlines
    sb = "a\n/* b\nc */\nd";
    sb.RemoveCStyleComments();
    W_TEST_STRING(sb, "a\n\n\nd");

    // multiple comments
    sb = "a // comment1\nb /* comment2 */ c";
    sb.RemoveCStyleComments();
    W_TEST_STRING(sb, "a \nb  c");

    // unterminated block comment
    sb = "a /* unterminated";
    sb.RemoveCStyleComments();
    W_TEST_STRING(sb, "a ");

    // empty string
    sb = "";
    sb.RemoveCStyleComments();
    W_TEST_STRING(sb, "");
  }
}

#pragma optimize("", on)
