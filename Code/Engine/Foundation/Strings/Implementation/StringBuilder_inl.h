#pragma once

#include <Foundation/Strings/StringConversion.h>

inline WStringBuilder::WStringBuilder(WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();
}

inline WStringBuilder::WStringBuilder(const WStringBuilder& rhs)
  : m_Data(rhs.GetAllocator())
{
  AppendTerminator();

  *this = rhs;
}

inline WStringBuilder::WStringBuilder(WStringBuilder&& rhs) noexcept
  : m_Data(rhs.GetAllocator())
{
  AppendTerminator();

  *this = std::move(rhs);
}

inline WStringBuilder::WStringBuilder(const char* szUTF8, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();

  *this = szUTF8;
}

inline WStringBuilder::WStringBuilder(const wchar_t* pWChar, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();

  *this = pWChar;
}

inline WStringBuilder::WStringBuilder(WStringView rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();

  *this = rhs;
}

W_ALWAYS_INLINE WAllocator* WStringBuilder::GetAllocator() const
{
  return m_Data.GetAllocator();
}

W_ALWAYS_INLINE void WStringBuilder::operator=(const char* szUTF8)
{
  Set(szUTF8);
}

W_FORCE_INLINE void WStringBuilder::operator=(const wchar_t* pWChar)
{
  // fine to do this, szWChar can never come from the stringbuilder's own data array
  Clear();
  Append(pWChar);
}

W_ALWAYS_INLINE void WStringBuilder::operator=(const WStringBuilder& rhs)
{
  m_Data = rhs.m_Data;
}

W_ALWAYS_INLINE void WStringBuilder::operator=(WStringBuilder&& rhs) noexcept
{
  m_Data = std::move(rhs.m_Data);
}

W_ALWAYS_INLINE WUInt32 WStringBuilder::GetElementCount() const
{
  return m_Data.GetCount() - 1; // exclude the '\0' terminator
}

W_ALWAYS_INLINE WUInt32 WStringBuilder::GetCharacterCount() const
{
  return WStringUtils::GetCharacterCount(m_Data.GetData());
}

W_FORCE_INLINE void WStringBuilder::Clear()
{
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

inline void WStringBuilder::Append(WUInt32 uiChar)
{
  char szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar = &szChar[0];

  WUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  WUInt32 uiCharLen = (WUInt32)(pChar - szChar);
  WUInt32 uiOldCount = m_Data.GetCount();
  m_Data.SetCountUninitialized(uiOldCount + uiCharLen);
  uiOldCount--;
  for (WUInt32 i = 0; i < uiCharLen; i++)
  {
    m_Data[uiOldCount + i] = szChar[i];
  }
  m_Data[uiOldCount + uiCharLen] = '\0';
}

inline void WStringBuilder::Prepend(WUInt32 uiChar)
{
  char szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar = &szChar[0];

  WUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  Prepend(szChar);
}

inline void WStringBuilder::Append(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  WStringUtf8 s1(pData1, m_Data.GetAllocator());
  WStringUtf8 s2(pData2, m_Data.GetAllocator());
  WStringUtf8 s3(pData3, m_Data.GetAllocator());
  WStringUtf8 s4(pData4, m_Data.GetAllocator());
  WStringUtf8 s5(pData5, m_Data.GetAllocator());
  WStringUtf8 s6(pData6, m_Data.GetAllocator());

  Append(s1.GetView(), s2.GetView(), s3.GetView(), s4.GetView(), s5.GetView(), s6.GetView());
}

inline void WStringBuilder::Prepend(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  WStringUtf8 s1(pData1, m_Data.GetAllocator());
  WStringUtf8 s2(pData2, m_Data.GetAllocator());
  WStringUtf8 s3(pData3, m_Data.GetAllocator());
  WStringUtf8 s4(pData4, m_Data.GetAllocator());
  WStringUtf8 s5(pData5, m_Data.GetAllocator());
  WStringUtf8 s6(pData6, m_Data.GetAllocator());

  Prepend(s1.GetView(), s2.GetView(), s3.GetView(), s4.GetView(), s5.GetView(), s6.GetView());
}

W_ALWAYS_INLINE const char* WStringBuilder::GetData() const
{
  W_ASSERT_DEBUG(!m_Data.IsEmpty(), "WStringBuilder has been corrupted, the array can never be empty.");

  return &m_Data[0];
}

inline void WStringBuilder::AppendTerminator()
{
  // make sure the string terminates with a zero.
  if (m_Data.IsEmpty() || (m_Data.PeekBack() != '\0'))
    m_Data.PushBack('\0');
}

inline void WStringBuilder::ToUpper()
{
  const WUInt32 uiNewStringLength = WStringUtils::ToUpperString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void WStringBuilder::ToLower()
{
  const WUInt32 uiNewStringLength = WStringUtils::ToLowerString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void WStringBuilder::ChangeCharacter(iterator& ref_it, WUInt32 uiCharacter)
{
  W_ASSERT_DEV(ref_it.IsValid(), "The given character iterator does not point to a valid character.");
  W_ASSERT_DEV(ref_it.GetData() >= GetData() && ref_it.GetData() < GetData() + GetElementCount(),
    "The given character iterator does not point into this string. It was either created from another string, or this string "
    "has been reallocated in the mean time.");

  // this is only an optimization for pure ASCII strings
  // without it, the code below would still work
  if (WUnicodeUtils::IsASCII(*ref_it) && WUnicodeUtils::IsASCII(uiCharacter))
  {
    char* pPos = const_cast<char*>(ref_it.GetData()); // yes, I know...
    *pPos = uiCharacter & 0xFF;
    return;
  }

  ChangeCharacterNonASCII(ref_it, uiCharacter);
}

W_ALWAYS_INLINE void WStringBuilder::Reserve(WUInt32 uiNumElements)
{
  m_Data.Reserve(uiNumElements);
}

W_ALWAYS_INLINE void WStringBuilder::Insert(const char* szInsertAtPos, WStringView sTextToInsert)
{
  ReplaceSubString(szInsertAtPos, szInsertAtPos, sTextToInsert);
}

W_ALWAYS_INLINE void WStringBuilder::Remove(const char* szRemoveFromPos, const char* szRemoveToPos)
{
  ReplaceSubString(szRemoveFromPos, szRemoveToPos, WStringView());
}

template <typename Container>
bool WUnicodeUtils::RepairNonUtf8Text(const char* pStartData, const char* pEndData, Container& out_result)
{
  if (WUnicodeUtils::IsValidUtf8(pStartData, pEndData))
  {
    out_result = WStringView(pStartData, pEndData);
    return false;
  }

  out_result.Clear();

  WHybridArray<char, 1024> fixedText;
  WUnicodeUtils::UtfInserter<char, decltype(fixedText)> inserter(&fixedText);

  while (pStartData < pEndData)
  {
    const WUInt32 uiChar = WUnicodeUtils::DecodeUtf8ToUtf32(pStartData);
    WUnicodeUtils::EncodeUtf32ToUtf8(uiChar, inserter);
  }

  W_ASSERT_DEV(WUnicodeUtils::IsValidUtf8(fixedText.GetData(), fixedText.GetData() + fixedText.GetCount()), "Repaired text is still not a valid Utf8 string.");

  out_result = WStringView(fixedText.GetData(), fixedText.GetCount());
  return true;
}

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
