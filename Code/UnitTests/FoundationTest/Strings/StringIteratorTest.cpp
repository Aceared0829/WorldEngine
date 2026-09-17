#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

template <typename STRING>
void TestConstruction(const STRING& value, const char* szStart, const char* szEnd)
{
  WStringUtf8 sUtf8(L"A単語F");
  W_TEST_BOOL(value.IsEqual(sUtf8.GetData()));
  const bool bEqualForwardItTypes = WConversionTest<typename STRING::iterator, typename STRING::const_iterator>::sameType == 1;
  static_assert(
    bEqualForwardItTypes, "As the string iterator is read-only, both const and non-const versions should be the same type.");
  const bool bEqualReverseItTypes = WConversionTest<typename STRING::reverse_iterator, typename STRING::const_reverse_iterator>::sameType == 1;
  static_assert(
    bEqualReverseItTypes, "As the reverse string iterator is read-only, both const and non-const versions should be the same type.");

  typename STRING::iterator itInvalid;
  W_TEST_BOOL(!itInvalid.IsValid());
  typename STRING::reverse_iterator itInvalidR;
  W_TEST_BOOL(!itInvalidR.IsValid());

  // Begin
  const typename STRING::iterator itBegin = begin(value);
  W_TEST_BOOL(itBegin == value.GetIteratorFront());
  W_TEST_BOOL(itBegin.IsValid());
  W_TEST_BOOL(itBegin == itBegin);
  W_TEST_BOOL(itBegin.GetData() == szStart);
  W_TEST_BOOL(itBegin.GetCharacter() == WUnicodeUtils::ConvertUtf8ToUtf32("A"));
  W_TEST_BOOL(*itBegin == WUnicodeUtils::ConvertUtf8ToUtf32("A"));

  // End
  const typename STRING::iterator itEnd = end(value);
  W_TEST_BOOL(!itEnd.IsValid());
  W_TEST_BOOL(itEnd == itEnd);
  W_TEST_BOOL(itBegin != itEnd);
  W_TEST_BOOL(itEnd.GetData() == szEnd);
  W_TEST_BOOL(itEnd.GetCharacter() == 0);
  W_TEST_BOOL(*itEnd == 0);

  // RBegin
  const typename STRING::reverse_iterator itBeginR = rbegin(value);
  W_TEST_BOOL(itBeginR == value.GetIteratorBack());
  W_TEST_BOOL(itBeginR.IsValid());
  W_TEST_BOOL(itBeginR == itBeginR);
  const char* szEndPrior = szEnd;
  WUnicodeUtils::MoveToPriorUtf8(szEndPrior, szStart).AssertSuccess();
  W_TEST_BOOL(itBeginR.GetData() == szEndPrior);
  W_TEST_BOOL(itBeginR.GetCharacter() == WUnicodeUtils::ConvertUtf8ToUtf32("F"));
  W_TEST_BOOL(*itBeginR == WUnicodeUtils::ConvertUtf8ToUtf32("F"));

  // REnd
  const typename STRING::reverse_iterator itEndR = rend(value);
  W_TEST_BOOL(!itEndR.IsValid());
  W_TEST_BOOL(itEndR == itEndR);
  W_TEST_BOOL(itBeginR != itEndR);
  W_TEST_BOOL(itEndR.GetData() == nullptr); // Position before first character is not a valid ptr, so it is set to nullptr.
  W_TEST_BOOL(itEndR.GetCharacter() == 0);
  W_TEST_BOOL(*itEndR == 0);
}

template <typename STRING, typename IT>
void TestIteratorBegin(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itBegin = it;
  --itBegin;
  itBegin -= 4;
  W_TEST_BOOL(itBegin == it);
  W_TEST_BOOL(itBegin - 2 == it);

  // Prefix / Postfix
  W_TEST_BOOL(itBegin + 2 != it);
  W_TEST_BOOL(itBegin++ == it);
  W_TEST_BOOL(itBegin-- != it);
  itBegin = it;
  W_TEST_BOOL(++itBegin != it);
  W_TEST_BOOL(--itBegin == it);

  // Misc
  itBegin = it;
  W_TEST_BOOL(it + 2 == ++(++itBegin));
  itBegin -= 1;
  W_TEST_BOOL(itBegin == it + 1);
  itBegin -= 0;
  W_TEST_BOOL(itBegin == it + 1);
  itBegin += 0;
  W_TEST_BOOL(itBegin == it + 1);
  itBegin += -1;
  W_TEST_BOOL(itBegin == it);
}

template <typename STRING, typename IT>
void TestIteratorEnd(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itEnd = it;
  ++itEnd;
  itEnd += 4;
  W_TEST_BOOL(itEnd == it);
  W_TEST_BOOL(itEnd + 2 == it);

  // Prefix / Postfix
  W_TEST_BOOL(itEnd - 2 != it);
  W_TEST_BOOL(itEnd-- == it);
  W_TEST_BOOL(itEnd++ != it);
  itEnd = it;
  W_TEST_BOOL(--itEnd != it);
  W_TEST_BOOL(++itEnd == it);

  // Misc
  itEnd = it;
  W_TEST_BOOL(it - 2 == --(--itEnd));
  itEnd += 1;
  W_TEST_BOOL(itEnd == it - 1);
  itEnd += 0;
  W_TEST_BOOL(itEnd == it - 1);
  itEnd -= 0;
  W_TEST_BOOL(itEnd == it - 1);
  itEnd -= -1;
  W_TEST_BOOL(itEnd == it);
}

template <typename STRING>
void TestOperators(const STRING& value, const char* szStart, const char* szEnd)
{
  WStringUtf8 sUtf8(L"A単語F");
  W_TEST_BOOL(value.IsEqual(sUtf8.GetData()));

  // Begin
  typename STRING::iterator itBegin = begin(value);
  TestIteratorBegin(value, itBegin);

  // End
  typename STRING::iterator itEnd = end(value);
  TestIteratorEnd(value, itEnd);

  // RBegin
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  TestIteratorBegin(value, itBeginR);

  // REnd
  typename STRING::reverse_iterator itEndR = rend(value);
  TestIteratorEnd(value, itEndR);
}

template <typename STRING>
void TestLoops(const STRING& value, const char* szStart, const char* szEnd)
{
  WStringUtf8 sUtf8(L"A単語F");
  WUInt32 characters[] = {WUnicodeUtils::ConvertUtf8ToUtf32(WStringUtf8(L"A").GetData()),
    WUnicodeUtils::ConvertUtf8ToUtf32(WStringUtf8(L"単").GetData()), WUnicodeUtils::ConvertUtf8ToUtf32(WStringUtf8(L"語").GetData()),
    WUnicodeUtils::ConvertUtf8ToUtf32(WStringUtf8(L"F").GetData())};

  // Forward
  WInt32 iIndex = 0;
  for (WUInt32 character : value)
  {
    W_TEST_INT(characters[iIndex], character);
    ++iIndex;
  }
  W_TEST_INT(iIndex, 4);

  typename STRING::iterator itBegin = begin(value);
  typename STRING::iterator itEnd = end(value);
  iIndex = 0;
  for (auto it = itBegin; it != itEnd; ++it)
  {
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(characters[iIndex], it.GetCharacter());
    W_TEST_INT(characters[iIndex], *it);
    W_TEST_BOOL(it.GetData() >= szStart);
    W_TEST_BOOL(it.GetData() < szEnd);
    ++iIndex;
  }
  W_TEST_INT(iIndex, 4);

  // Reverse
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  typename STRING::reverse_iterator itEndR = rend(value);
  iIndex = 3;
  for (auto it = itBeginR; it != itEndR; ++it)
  {
    W_TEST_BOOL(it.IsValid());
    W_TEST_INT(characters[iIndex], it.GetCharacter());
    W_TEST_INT(characters[iIndex], *it);
    W_TEST_BOOL(it.GetData() >= szStart);
    W_TEST_BOOL(it.GetData() < szEnd);
    --iIndex;
  }
  W_TEST_INT(iIndex, -1);
}

W_CREATE_SIMPLE_TEST(Strings, StringIterator)
{
  WStringUtf8 sUtf8(L"_A単語F_");
  WStringBuilder sTestStringBuilder = sUtf8.GetData();
  sTestStringBuilder.Shrink(1, 1);
  WString sTextString = sTestStringBuilder.GetData();

  WStringView view(sUtf8.GetData());
  view.Shrink(1, 1);

  W_TEST_BLOCK(WTestBlock::Enabled, "Construction")
  {
    TestConstruction<WString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestConstruction<WStringBuilder>(
      sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestConstruction<WStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    TestOperators<WString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestOperators<WStringBuilder>(
      sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestOperators<WStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Loops")
  {
    TestLoops<WString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestLoops<WStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestLoops<WStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }
}
