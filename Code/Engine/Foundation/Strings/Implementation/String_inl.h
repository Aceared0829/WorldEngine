#pragma once

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  Clear();
}

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(const WHybridStringBase& rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(WHybridStringBase&& rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  operator=(std::move(rhs));
}

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(const char* rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(const wchar_t* rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(const WStringView& rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <WUInt16 Size>
WHybridStringBase<Size>::~WHybridStringBase() = default;

template <WUInt16 Size>
void WHybridStringBase<Size>::Clear()
{
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

template <WUInt16 Size>
W_ALWAYS_INLINE const char* WHybridStringBase<Size>::GetData() const
{
  W_ASSERT_DEBUG(!m_Data.IsEmpty(), "WHybridString has been corrupted, the array can never be empty. This can happen when you access a "
                                     "string that was previously std::move'd into another string.");

  return &m_Data[0];
}

template <WUInt16 Size>
W_ALWAYS_INLINE WUInt32 WHybridStringBase<Size>::GetElementCount() const
{
  return m_Data.GetCount() - 1;
}

template <WUInt16 Size>
W_ALWAYS_INLINE WUInt32 WHybridStringBase<Size>::GetCharacterCount() const
{
  return WStringUtils::GetCharacterCount(GetData());
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(const char* szString)
{
  WUInt32 uiElementCount = WStringUtils::GetStringElementCount(szString);

  if (szString + uiElementCount < m_Data.GetData() || szString >= m_Data.GetData() + m_Data.GetCount())
  {
    // source string is outside our own memory, so no overlapped copy
  }
  else
  {
    // source string overlaps with our own memory -> we can't increase the size of our memory, as that might invalidate the source data
    W_ASSERT_DEBUG(uiElementCount < m_Data.GetCount(), "Invalid copy of overlapping string data.");
  }

  m_Data.SetCountUninitialized(uiElementCount + 1);
  WStringUtils::Copy(&m_Data[0], uiElementCount + 1, szString);
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(const WHybridStringBase& rhs)
{
  if (this == &rhs)
    return;

  m_Data = rhs.m_Data;
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(WHybridStringBase&& rhs)
{
  if (this == &rhs)
    return;

  m_Data = std::move(rhs.m_Data);
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(const wchar_t* szString)
{
  WStringUtf8 sConversion(szString, m_Data.GetAllocator());
  *this = sConversion.GetData();
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(const WStringView& rhs)
{
  W_ASSERT_DEBUG(rhs.GetStartPointer() < m_Data.GetData() || rhs.GetStartPointer() >= m_Data.GetData() + m_Data.GetCount(),
    "Can't assign string a value that points to ourself!");

  m_Data.SetCountUninitialized(rhs.GetElementCount() + 1);
  WStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.GetStartPointer(), rhs.GetEndPointer());
}

template <WUInt16 Size>
WStringView WHybridStringBase<Size>::GetSubString(WUInt32 uiFirstCharacter, WUInt32 uiNumCharacters) const
{
  const char* szStart = GetData();
  if (WUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter).Failed())
    return {};                                                           // szStart was moved too far, the result is just an empty string

  const char* szEnd = szStart;
  WUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters).IgnoreResult(); // if it fails, szEnd just points to the end of this string

  return WStringView(szStart, szEnd);
}

template <WUInt16 Size>
WStringView WHybridStringBase<Size>::GetFirst(WUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

template <WUInt16 Size>
WStringView WHybridStringBase<Size>::GetLast(WUInt32 uiNumCharacters) const
{
  const WUInt32 uiMaxCharacterCount = GetCharacterCount();
  W_ASSERT_DEV(uiNumCharacters < uiMaxCharacterCount, "The string only contains {0} characters, cannot return the last {1} characters.",
    uiMaxCharacterCount, uiNumCharacters);
  return GetSubString(uiMaxCharacterCount - uiNumCharacters, uiNumCharacters);
}


template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString()
  : WHybridStringBase<Size>(A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(WAllocator* pAllocator)
  : WHybridStringBase<Size>(pAllocator)
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const WHybridString<Size, A>& other)
  : WHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const WHybridStringBase<Size>& other)
  : WHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(WHybridString<Size, A>&& other)
  : WHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(WHybridStringBase<Size>&& other)
  : WHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const char* rhs)
  : WHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const wchar_t* rhs)
  : WHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const WStringView& rhs)
  : WHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const WHybridString<Size, A>& rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const WHybridStringBase<Size>& rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(WHybridString<Size, A>&& rhs)
{
  WHybridStringBase<Size>::operator=(std::move(rhs));
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(WHybridStringBase<Size>&& rhs)
{
  WHybridStringBase<Size>::operator=(std::move(rhs));
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const char* rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const wchar_t* rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const WStringView& rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

#if W_ENABLED(W_INTEROP_STL_STRINGS)

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(const std::string_view& rhs, WAllocator* pAllocator)
{
  *this = rhs;
}

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(const std::string& rhs, WAllocator* pAllocator)
{
  *this = rhs;
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(const std::string_view& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    m_Data.SetCountUninitialized(((WUInt32)rhs.size() + 1));
    WStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.data(), rhs.data() + rhs.size());
  }
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(const std::string& rhs)
{
  *this = std::string_view(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const std::string_view& rhs)
  : WHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const std::string& rhs)
  : WHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const std::string_view& rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const std::string& rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

#endif

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
