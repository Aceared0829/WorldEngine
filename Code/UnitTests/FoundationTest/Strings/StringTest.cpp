#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/String.h>

static WString GetString(const char* szSz)
{
  WString s;
  s = szSz;
  return s;
}

static WStringBuilder GetStringBuilder(const char* szSz)
{
  WStringBuilder s;

  for (WUInt32 i = 0; i < 10; ++i)
    s.Append(szSz);

  return s;
}

W_CREATE_SIMPLE_TEST(Strings, String)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WString s1;
    W_TEST_BOOL(s1 == "");

    WString s2("abc");
    W_TEST_BOOL(s2 == "abc");

    WString s3(s2);
    W_TEST_BOOL(s2 == s3);
    W_TEST_BOOL(s3 == "abc");

    WString s4(L"abc");
    W_TEST_BOOL(s4 == "abc");

    WStringView it = s4.GetFirst(2);
    WString s5(it);
    W_TEST_BOOL(s5 == "ab");

    WStringBuilder strB("wobwob");
    WString s6(strB);
    W_TEST_BOOL(s6 == "wobwob");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator=")
  {
    WString s2;
    s2 = "abc";
    W_TEST_BOOL(s2 == "abc");

    WString s3;
    s3 = s2;
    W_TEST_BOOL(s2 == s3);
    W_TEST_BOOL(s3 == "abc");

    WString s4;
    s4 = L"abc";
    W_TEST_BOOL(s4 == "abc");

    WString s5(L"abcdefghijklm");
    WStringView it(s5.GetData() + 2, s5.GetData() + 10);
    WString s5b = it;
    W_TEST_STRING(s5b, "cdefghij");

    WString s6(L"aölsdföasld");
    WStringBuilder strB("wobwob");
    s6 = strB;
    W_TEST_BOOL(s6 == "wobwob");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "convert to WStringView")
  {
    WString s(L"aölsdföasld");
    WStringBuilder tmp;

    WStringView sv = s;

    W_TEST_STRING(sv.GetData(tmp), WStringUtf8(L"aölsdföasld").GetData());
    W_TEST_BOOL(sv == WStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    W_TEST_STRING(sv.GetStartPointer(), "abcdef");
    W_TEST_BOOL(sv == "abcdef");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move constructor / operator")
  {
    WString s1(GetString("move me"));
    W_TEST_STRING(s1.GetData(), "move me");

    s1 = GetString("move move move move move move move move ");
    W_TEST_STRING(s1.GetData(), "move move move move move move move move ");

    WString s2(GetString("move move move move move move move move "));
    W_TEST_STRING(s2.GetData(), "move move move move move move move move ");

    s2 = GetString("move me");
    W_TEST_STRING(s2.GetData(), "move me");

    s1 = s2;
    W_TEST_STRING(s1.GetData(), "move me");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move constructor / operator (StringBuilder)")
  {
    const WString s1(GetStringBuilder("move me"));
    const WString s2(GetStringBuilder("move move move move move move move move "));

    WString s3(GetStringBuilder("move me"));
    W_TEST_BOOL(s3 == s1);

    s3 = GetStringBuilder("move move move move move move move move ");
    W_TEST_BOOL(s3 == s2);

    WString s4(GetStringBuilder("move move move move move move move move "));
    W_TEST_BOOL(s4 == s2);

    s4 = GetStringBuilder("move me");
    W_TEST_BOOL(s4 == s1);

    s3 = s4;
    W_TEST_BOOL(s3 == s1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    WString s("abcdef");
    W_TEST_BOOL(s == "abcdef");

    s.Clear();
    W_TEST_BOOL(s.IsEmpty());
    W_TEST_BOOL(s == "");
    W_TEST_BOOL(s == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetData")
  {
    const char* sz = "abcdef";

    WString s(sz);
    W_TEST_BOOL(s.GetData() != sz); // it should NOT be the exact same string
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetElementCount / GetCharacterCount")
  {
    WString s(L"abcäöü€");

    W_TEST_INT(s.GetElementCount(), 12);
    W_TEST_INT(s.GetCharacterCount(), 7);

    s = "testtest";
    W_TEST_INT(s.GetElementCount(), 8);
    W_TEST_INT(s.GetCharacterCount(), 8);

    s.Clear();

    W_TEST_INT(s.GetElementCount(), 0);
    W_TEST_INT(s.GetCharacterCount(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Convert to WStringView")
  {
    WString s(L"abcäöü€def");

    WStringView view = s;
    W_TEST_BOOL(view.StartsWith("abc"));
    W_TEST_BOOL(view.EndsWith("def"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetSubString")
  {
    WString s(L"abcäöü€def");
    WStringUtf8 s8(L"äöü€");

    WStringView it = s.GetSubString(3, 4);
    W_TEST_BOOL(it == s8.GetData());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFirst")
  {
    WString s(L"abcäöü€def");

    W_TEST_BOOL(s.GetFirst(3) == "abc");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLast")
  {
    WString s(L"abcäöü€def");

    W_TEST_BOOL(s.GetLast(3) == "def");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadAll")
  {
    WDefaultMemoryStreamStorage StreamStorage;

    WMemoryStreamWriter MemoryWriter(&StreamStorage);
    WMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText =
      "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, WStringUtils::GetStringElementCount(szText)).IgnoreResult();

    WString s;
    s.ReadAll(MemoryReader);

    W_TEST_BOOL(s == szText);
  }
}
