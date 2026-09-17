#pragma once

W_ALWAYS_INLINE constexpr WStringView::WStringView() = default;

W_ALWAYS_INLINE WStringView::WStringView(char* pStart)
  : m_pStart(pStart)
  , m_uiElementCount(WStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr W_ALWAYS_INLINE WStringView::WStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, int>::type*)
  : m_pStart(pStart)
  , m_uiElementCount(WStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr W_ALWAYS_INLINE WStringView::WStringView(const T&& str, typename std::enable_if<std::is_same<T, const char*>::value == false && std::is_convertible<T, const char*>::value, int>::type*)
{
  m_pStart = str;
  m_uiElementCount = WStringUtils::GetStringElementCount(m_pStart);
}

constexpr W_ALWAYS_INLINE WStringView::WStringView(const char* pStart, const char* pEnd)
{
  W_ASSERT_DEBUG(pStart <= pEnd, "Invalid pointers to construct a string view from.");

  m_pStart = pStart;
  m_uiElementCount = static_cast<WUInt32>(pEnd - pStart);
}

constexpr W_ALWAYS_INLINE WStringView::WStringView(const char* pStart, WUInt32 uiLength)
  : m_pStart(pStart)
  , m_uiElementCount(uiLength)
{
}

template <size_t N>
constexpr W_ALWAYS_INLINE WStringView::WStringView(const char (&str)[N])
  : m_pStart(str)
  , m_uiElementCount(N - 1)
{
  static_assert(N > 0, "Not a string literal");
}

template <size_t N>
constexpr W_ALWAYS_INLINE WStringView::WStringView(char (&str)[N])
{
  m_pStart = str;
  m_uiElementCount = WStringUtils::GetStringElementCount(str, str + N);
}

inline void WStringView::operator++()
{
  if (!IsValid())
    return;

  const char* pEnd = m_pStart + m_uiElementCount;
  WUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd).IgnoreResult(); // if it fails, the string is just empty
  m_uiElementCount = static_cast<WUInt32>(pEnd - m_pStart);
}

inline void WStringView::operator+=(WUInt32 d)
{
  const char* pEnd = m_pStart + m_uiElementCount;
  WUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd, d).IgnoreResult(); // if it fails, the string is just empty
  m_uiElementCount = static_cast<WUInt32>(pEnd - m_pStart);
}

W_ALWAYS_INLINE bool WStringView::IsValid() const
{
  return (m_pStart != nullptr) && (m_uiElementCount > 0);
}

W_ALWAYS_INLINE void WStringView::SetStartPosition(const char* szCurPos)
{
  W_ASSERT_DEV((szCurPos >= m_pStart) && (szCurPos <= m_pStart + m_uiElementCount), "New start position must still be inside the view's range.");

  const char* pEnd = m_pStart + m_uiElementCount;
  m_pStart = szCurPos;
  m_uiElementCount = static_cast<WUInt32>(pEnd - m_pStart);
}

W_ALWAYS_INLINE bool WStringView::IsEmpty() const
{
  return m_uiElementCount == 0;
}

W_ALWAYS_INLINE bool WStringView::IsEqual(WStringView sOther) const
{
  return WStringUtils::IsEqual(m_pStart, sOther.GetStartPointer(), m_pStart + m_uiElementCount, sOther.GetEndPointer());
}

W_ALWAYS_INLINE bool WStringView::IsEqual_NoCase(WStringView sOther) const
{
  return WStringUtils::IsEqual_NoCase(m_pStart, sOther.GetStartPointer(), m_pStart + m_uiElementCount, sOther.GetEndPointer());
}

W_ALWAYS_INLINE bool WStringView::StartsWith(WStringView sStartsWith) const
{
  return WStringUtils::StartsWith(m_pStart, sStartsWith.GetStartPointer(), m_pStart + m_uiElementCount, sStartsWith.GetEndPointer());
}

W_ALWAYS_INLINE bool WStringView::StartsWith_NoCase(WStringView sStartsWith) const
{
  return WStringUtils::StartsWith_NoCase(m_pStart, sStartsWith.GetStartPointer(), m_pStart + m_uiElementCount, sStartsWith.GetEndPointer());
}

W_ALWAYS_INLINE bool WStringView::EndsWith(WStringView sEndsWith) const
{
  return WStringUtils::EndsWith(m_pStart, sEndsWith.GetStartPointer(), m_pStart + m_uiElementCount, sEndsWith.GetEndPointer());
}

W_ALWAYS_INLINE bool WStringView::EndsWith_NoCase(WStringView sEndsWith) const
{
  return WStringUtils::EndsWith_NoCase(m_pStart, sEndsWith.GetStartPointer(), m_pStart + m_uiElementCount, sEndsWith.GetEndPointer());
}

W_ALWAYS_INLINE void WStringView::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

W_ALWAYS_INLINE void WStringView::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  if (IsValid())
  {
    const char* pEnd = m_pStart + m_uiElementCount;
    WStringUtils::Trim(m_pStart, pEnd, szTrimCharsStart, szTrimCharsEnd);
    m_uiElementCount = static_cast<WUInt32>(pEnd - m_pStart);
  }
}

constexpr W_ALWAYS_INLINE WStringView operator"" _wsv(const char* pString, size_t uiLen)
{
  return WStringView(pString, static_cast<WUInt32>(uiLen));
}

template <typename Container>
void WStringView::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  ref_output.Clear();

  if (IsEmpty())
    return;

  const WUInt32 uiParams = 6;

  const WStringView seps[uiParams] = {szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6};

  const char* szReadPos = GetStartPointer();

  while (true)
  {
    const char* szFoundPos = WUnicodeUtils::GetMaxStringEnd<char>();
    WUInt32 uiFoundSeparator = 0;

    for (WUInt32 i = 0; i < uiParams; ++i)
    {
      const char* szFound = WStringUtils::FindSubString(szReadPos, seps[i].GetStartPointer(), GetEndPointer(), seps[i].GetEndPointer());

      if ((szFound != nullptr) && (szFound < szFoundPos))
      {
        szFoundPos = szFound;
        uiFoundSeparator = i;
      }
    }

    // nothing found
    if (szFoundPos == WUnicodeUtils::GetMaxStringEnd<char>())
    {
      const WUInt32 uiLen = WStringUtils::GetStringElementCount(szReadPos, GetEndPointer());

      if (bReturnEmptyStrings || (uiLen > 0))
        ref_output.PushBack(WStringView(szReadPos, szReadPos + uiLen));

      return;
    }

    if (bReturnEmptyStrings || (szFoundPos > szReadPos))
      ref_output.PushBack(WStringView(szReadPos, szFoundPos));

    szReadPos = szFoundPos + seps[uiFoundSeparator].GetElementCount();
  }
}

W_ALWAYS_INLINE bool operator==(WStringView lhs, WStringView rhs)
{
  return lhs.IsEqual(rhs);
}

#if W_DISABLED(W_USE_CPP20_OPERATORS)

W_ALWAYS_INLINE bool operator!=(WStringView lhs, WStringView rhs)
{
  return !lhs.IsEqual(rhs);
}

#endif

#if W_ENABLED(W_USE_CPP20_OPERATORS)

W_ALWAYS_INLINE std::strong_ordering operator<=>(WStringView lhs, WStringView rhs)
{
  return lhs.Compare(rhs) <=> 0;
}

#else

W_ALWAYS_INLINE bool operator<(WStringView lhs, WStringView rhs)
{
  return lhs.Compare(rhs) < 0;
}

W_ALWAYS_INLINE bool operator<=(WStringView lhs, WStringView rhs)
{
  return lhs.Compare(rhs) <= 0;
}

W_ALWAYS_INLINE bool operator>(WStringView lhs, WStringView rhs)
{
  return lhs.Compare(rhs) > 0;
}

W_ALWAYS_INLINE bool operator>=(WStringView lhs, WStringView rhs)
{
  return lhs.Compare(rhs) >= 0;
}

#endif
