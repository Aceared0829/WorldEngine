#pragma once

W_ALWAYS_INLINE WInt32 WStringUtils::CompareChars(WUInt32 uiCharacter1, WUInt32 uiCharacter2)
{
  return (WInt32)uiCharacter1 - (WInt32)uiCharacter2;
}

inline WInt32 WStringUtils::CompareChars_NoCase(WUInt32 uiCharacter1, WUInt32 uiCharacter2)
{
  return (WInt32)ToUpperChar(uiCharacter1) - (WInt32)ToUpperChar(uiCharacter2);
}

inline WInt32 WStringUtils::CompareChars(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars(WUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), WUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

inline WInt32 WStringUtils::CompareChars_NoCase(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars_NoCase(WUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), WUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

template <typename T>
W_ALWAYS_INLINE constexpr bool WStringUtils::IsNullOrEmpty(const T* pString)
{
  return (pString == nullptr) || (pString[0] == '\0');
}

template <typename T>
W_ALWAYS_INLINE bool WStringUtils::IsNullOrEmpty(const T* pString, const T* pStringEnd)
{
  return (pString == nullptr) || pString == pStringEnd || (pString[0] == '\0');
}

template <typename T>
W_ALWAYS_INLINE void WStringUtils::UpdateStringEnd(const T* pStringStart, const T*& ref_pStringEnd)
{
  if (ref_pStringEnd != WUnicodeUtils::GetMaxStringEnd<T>())
    return;

  ref_pStringEnd = pStringStart + GetStringElementCount(pStringStart, WUnicodeUtils::GetMaxStringEnd<T>());
}

template <typename T>
constexpr WUInt32 WStringUtils::GetStringElementCount(const T* pString)
{
  // can't use strlen here as long as it's not constexpr (C++ 23)

  if (pString == nullptr)
    return 0;

  WUInt32 uiCount = 0;
  while (*pString != '\0')
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

template <typename T>
WUInt32 WStringUtils::GetStringElementCount(const T* pString, const T* pStringEnd)
{
  if (IsNullOrEmpty(pString))
    return 0;

  if (pStringEnd != WUnicodeUtils::GetMaxStringEnd<T>())
    return (WUInt32)(pStringEnd - pString);

  WUInt32 uiCount = 0;
  while ((pString < pStringEnd) && (*pString != '\0'))
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

inline WUInt32 WStringUtils::GetCharacterCount(const char* szUtf8, const char* pStringEnd)
{
  if (IsNullOrEmpty(szUtf8))
    return 0;

  WUInt32 uiCharacters = 0;

  while ((szUtf8 < pStringEnd) && (*szUtf8 != '\0'))
  {
    // skip all the Utf8 continuation bytes
    if (!WUnicodeUtils::IsUtf8ContinuationByte(*szUtf8))
      ++uiCharacters;

    ++szUtf8;
  }

  return uiCharacters;
}

inline void WStringUtils::GetCharacterAndElementCount(const char* szUtf8, WUInt32& ref_uiCharacterCount, WUInt32& ref_uiElementCount, const char* pStringEnd)
{
  ref_uiCharacterCount = 0;
  ref_uiElementCount = 0;

  if (IsNullOrEmpty(szUtf8))
    return;

  while (szUtf8 < pStringEnd)
  {
    char uiByte = *szUtf8;
    if (uiByte == '\0')
    {
      break;
    }

    // skip all the Utf8 continuation bytes
    if (!WUnicodeUtils::IsUtf8ContinuationByte(uiByte))
      ++ref_uiCharacterCount;

    ++szUtf8;
    ++ref_uiElementCount;
  }
}

W_ALWAYS_INLINE bool WStringUtils::IsEqual(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return WStringUtils::Compare(pString1, pString2, pString1End, pString2End) == 0;
}

W_ALWAYS_INLINE bool WStringUtils::IsEqualN(
  const char* pString1, const char* pString2, WUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return WStringUtils::CompareN(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

W_ALWAYS_INLINE bool WStringUtils::IsEqual_NoCase(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return WStringUtils::Compare_NoCase(pString1, pString2, pString1End, pString2End) == 0;
}

W_ALWAYS_INLINE bool WStringUtils::IsEqualN_NoCase(
  const char* pString1, const char* pString2, WUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return WStringUtils::CompareN_NoCase(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

W_ALWAYS_INLINE bool WStringUtils::IsDecimalDigit(WUInt32 uiChar)
{
  return (uiChar >= '0' && uiChar <= '9');
}

W_ALWAYS_INLINE bool WStringUtils::IsHexDigit(WUInt32 uiChar)
{
  return IsDecimalDigit(uiChar) || (uiChar >= 'A' && uiChar <= 'F') || (uiChar >= 'a' && uiChar <= 'f');
}
