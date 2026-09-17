#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

WUInt32 WStringView::GetCharacter() const
{
  if (!IsValid())
    return 0;

  return WUnicodeUtils::ConvertUtf8ToUtf32(m_pStart);
}

const char* WStringView::GetData(WStringBuilder& ref_sTempStorage) const
{
  ref_sTempStorage = *this;
  return ref_sTempStorage.GetData();
}

bool WStringView::IsEqualN(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::IsEqualN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

bool WStringView::IsEqualN_NoCase(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::IsEqualN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

WInt32 WStringView::Compare(WStringView sOther) const
{
  return WStringUtils::Compare(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

WInt32 WStringView::CompareN(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::CompareN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

WInt32 WStringView::Compare_NoCase(WStringView sOther) const
{
  return WStringUtils::Compare_NoCase(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

WInt32 WStringView::CompareN_NoCase(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::CompareN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

const char* WStringView::ComputeCharacterPosition(WUInt32 uiCharacterIndex) const
{
  const char* pos = GetStartPointer();
  if (WUnicodeUtils::MoveToNextUtf8(pos, GetEndPointer(), uiCharacterIndex).Failed())
    return nullptr;

  return pos;
}

const char* WStringView::FindSubString(WStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  W_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* WStringView::FindSubString_NoCase(WStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  W_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* WStringView::FindLastSubString(WStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  W_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindLastSubString(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* WStringView::FindLastSubString_NoCase(WStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  W_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindLastSubString_NoCase(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* WStringView::FindWholeWord(const char* szSearchFor, WStringUtils::W_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  W_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

const char* WStringView::FindWholeWord_NoCase(const char* szSearchFor, WStringUtils::W_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  W_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

void WStringView::Shrink(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack)
{
  const char* pEnd = m_pStart + m_uiElementCount;

  while (IsValid() && (uiShrinkCharsFront > 0))
  {
    if (WUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd, 1).Failed())
    {
      *this = {};
      return;
    }

    --uiShrinkCharsFront;
  }

  while (IsValid() && (uiShrinkCharsBack > 0))
  {
    if (WUnicodeUtils::MoveToPriorUtf8(pEnd, m_pStart, 1).Failed())
    {
      *this = {};
      return;
    }

    --uiShrinkCharsBack;
  }

  m_uiElementCount = static_cast<WUInt32>(pEnd - m_pStart);
}

WStringView WStringView::GetShrunk(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack) const
{
  WStringView tmp = *this;
  tmp.Shrink(uiShrinkCharsFront, uiShrinkCharsBack);
  return tmp;
}

WStringView WStringView::GetSubString(WUInt32 uiFirstCharacter, WUInt32 uiNumCharacters) const
{
  if (!IsValid())
  {
    return {};
  }

  const char* pEnd = m_pStart + m_uiElementCount;

  const char* pSubStart = m_pStart;
  if (WUnicodeUtils::MoveToNextUtf8(pSubStart, pEnd, uiFirstCharacter).Failed() || pSubStart == pEnd)
  {
    return {};
  }

  const char* pSubEnd = pSubStart;
  WUnicodeUtils::MoveToNextUtf8(pSubEnd, pEnd, uiNumCharacters).IgnoreResult(); // if it fails, it just points to the end

  return WStringView(pSubStart, pSubEnd);
}

void WStringView::ChopAwayFirstCharacterUtf8()
{
  if (IsValid())
  {
    const char* pEnd = m_pStart + m_uiElementCount;
    WUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd, 1).AssertSuccess();
    m_uiElementCount = static_cast<WUInt32>(pEnd - m_pStart);
  }
}

void WStringView::ChopAwayFirstCharacterAscii()
{
  if (IsValid())
  {
    W_ASSERT_DEBUG(WUnicodeUtils::IsASCII(*m_pStart), "ChopAwayFirstCharacterAscii() was called on a non-ASCII character.");

    m_pStart += 1;
    m_uiElementCount--;
  }
}

bool WStringView::TrimWordStart(WStringView sWord)
{
  const bool bTrimAll = false;

  bool trimmed = false;

  do
  {
    if (!sWord.IsEmpty() && StartsWith_NoCase(sWord))
    {
      Shrink(WStringUtils::GetCharacterCount(sWord.GetStartPointer(), sWord.GetEndPointer()), 0);
      trimmed = true;
    }

  } while (bTrimAll);

  return trimmed;
}

bool WStringView::TrimWordEnd(WStringView sWord)
{
  const bool bTrimAll = false;

  bool trimmed = false;

  do
  {
    if (!sWord.IsEmpty() && EndsWith_NoCase(sWord))
    {
      Shrink(0, WStringUtils::GetCharacterCount(sWord.GetStartPointer(), sWord.GetEndPointer()));
      trimmed = true;
    }

  } while (bTrimAll);

  return trimmed;
}

WStringView::iterator WStringView::GetIteratorFront() const
{
  return begin(*this);
}

WStringView::reverse_iterator WStringView::GetIteratorBack() const
{
  return rbegin(*this);
}

bool WStringView::HasAnyExtension() const
{
  return WPathUtils::HasAnyExtension(*this);
}

bool WStringView::HasExtension(WStringView sExtension) const
{
  return WPathUtils::HasExtension(*this, sExtension);
}

WStringView WStringView::GetFileExtension(bool bFullExtension /*= false*/) const
{
  return WPathUtils::GetFileExtension(*this, bFullExtension);
}

WStringView WStringView::GetFileName() const
{
  return WPathUtils::GetFileName(*this);
}

WStringView WStringView::GetFileNameAndExtension() const
{
  return WPathUtils::GetFileNameAndExtension(*this);
}

WStringView WStringView::GetFileDirectory() const
{
  return WPathUtils::GetFileDirectory(*this);
}

bool WStringView::IsAbsolutePath() const
{
  return WPathUtils::IsAbsolutePath(*this);
}

bool WStringView::IsRelativePath() const
{
  return WPathUtils::IsRelativePath(*this);
}

bool WStringView::IsRootedPath() const
{
  return WPathUtils::IsRootedPath(*this);
}

WStringView WStringView::GetRootedPathRootName() const
{
  return WPathUtils::GetRootedPathRootName(*this);
}

#if W_ENABLED(W_INTEROP_STL_STRINGS)
WStringView::WStringView(const std::string_view& rhs)
{
  if (!rhs.empty())
  {
    m_pStart = rhs.data();
    m_uiElementCount = static_cast<WUInt32>(rhs.size());
  }
}

WStringView::WStringView(const std::string& rhs)
{
  if (!rhs.empty())
  {
    m_pStart = rhs.data();
    m_uiElementCount = static_cast<WUInt32>(rhs.size());
  }
}

std::string_view WStringView::GetAsStdView() const
{
  return std::string_view(m_pStart, static_cast<size_t>(m_uiElementCount));
}

WStringView::operator std::string_view() const
{
  return GetAsStdView();
}
#endif
