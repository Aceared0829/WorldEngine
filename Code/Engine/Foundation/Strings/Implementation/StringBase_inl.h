#pragma once

template <typename Derived>
W_ALWAYS_INLINE const char* WStringBase<Derived>::InternalGetData() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData();
}

template <typename Derived>
W_ALWAYS_INLINE const char* WStringBase<Derived>::InternalGetDataEnd() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData() + pDerived->GetElementCount();
}

template <typename Derived>
W_ALWAYS_INLINE WUInt32 WStringBase<Derived>::InternalGetElementCount() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetElementCount();
}

template <typename Derived>
W_ALWAYS_INLINE bool WStringBase<Derived>::IsEmpty() const
{
  return WStringUtils::IsNullOrEmpty(InternalGetData()) || (InternalGetData() == InternalGetDataEnd());
}

template <typename Derived>
bool WStringBase<Derived>::StartsWith(WStringView sStartsWith) const
{
  return WStringUtils::StartsWith(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool WStringBase<Derived>::StartsWith_NoCase(WStringView sStartsWith) const
{
  return WStringUtils::StartsWith_NoCase(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool WStringBase<Derived>::EndsWith(WStringView sEndsWith) const
{
  return WStringUtils::EndsWith(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
bool WStringBase<Derived>::EndsWith_NoCase(WStringView sEndsWith) const
{
  return WStringUtils::EndsWith_NoCase(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
const char* WStringBase<Derived>::FindSubString(WStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  W_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
const char* WStringBase<Derived>::FindSubString_NoCase(WStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  W_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* WStringBase<Derived>::FindLastSubString(WStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  W_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()),
    "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindLastSubString(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* WStringBase<Derived>::FindLastSubString_NoCase(WStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  W_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()),
    "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindLastSubString_NoCase(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* WStringBase<Derived>::FindWholeWord(const char* szSearchFor, WStringUtils::W_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  W_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
inline const char* WStringBase<Derived>::FindWholeWord_NoCase(const char* szSearchFor, WStringUtils::W_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  W_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return WStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
WInt32 WStringBase<Derived>::Compare(WStringView sOther) const
{
  return WStringUtils::Compare(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
WInt32 WStringBase<Derived>::CompareN(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::CompareN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
WInt32 WStringBase<Derived>::Compare_NoCase(WStringView sOther) const
{
  return WStringUtils::Compare_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
WInt32 WStringBase<Derived>::CompareN_NoCase(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::CompareN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool WStringBase<Derived>::IsEqual(WStringView sOther) const
{
  return WStringUtils::IsEqual(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool WStringBase<Derived>::IsEqualN(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::IsEqualN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool WStringBase<Derived>::IsEqual_NoCase(WStringView sOther) const
{
  return WStringUtils::IsEqual_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool WStringBase<Derived>::IsEqualN_NoCase(WStringView sOther, WUInt32 uiCharsToCompare) const
{
  return WStringUtils::IsEqualN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
const char* WStringBase<Derived>::ComputeCharacterPosition(WUInt32 uiCharacterIndex) const
{
  const char* pos = InternalGetData();
  if (WUnicodeUtils::MoveToNextUtf8(pos, InternalGetDataEnd(), uiCharacterIndex).Failed())
    return nullptr;

  return pos;
}

template <typename Derived>
typename WStringBase<Derived>::iterator WStringBase<Derived>::GetIteratorFront() const
{
  return begin(*this);
}

template <typename Derived>
typename WStringBase<Derived>::reverse_iterator WStringBase<Derived>::GetIteratorBack() const
{
  return rbegin(*this);
}

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE bool operator==(const WStringBase<DerivedLhs>& lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.IsEqual(rhs.GetView());
}

template <typename DerivedRhs>
W_ALWAYS_INLINE bool operator==(const char* lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
W_ALWAYS_INLINE bool operator==(const WStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.IsEqual(rhs);
}

#if W_DISABLED(W_USE_CPP20_OPERATORS)

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE bool operator!=(const WStringBase<DerivedLhs>& lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return !lhs.IsEqual(rhs);
}

template <typename DerivedRhs>
W_ALWAYS_INLINE bool operator!=(const char* lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return !rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
W_ALWAYS_INLINE bool operator!=(const WStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return !lhs.IsEqual(rhs);
}

#endif

#if W_ENABLED(W_USE_CPP20_OPERATORS)

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE std::strong_ordering operator<=>(const WStringBase<DerivedLhs>& lhs, const WStringBase<DerivedRhs>& rhs)
{
  return lhs.Compare(rhs) <=> 0;
}

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE std::strong_ordering operator<=>(const WStringBase<DerivedLhs>& lhs, const char* rhs)
{
  return lhs.Compare(rhs) <=> 0;
}

#else

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE bool operator<(const WStringBase<DerivedLhs>& lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.Compare(rhs) < 0;
}

template <typename DerivedRhs>
W_ALWAYS_INLINE bool operator<(const char* lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) > 0;
}

template <typename DerivedLhs>
W_ALWAYS_INLINE bool operator<(const WStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) < 0;
}

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE bool operator>(const WStringBase<DerivedLhs>& lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.Compare(rhs) > 0;
}

template <typename DerivedRhs>
W_ALWAYS_INLINE bool operator>(const char* lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) < 0;
}

template <typename DerivedLhs>
W_ALWAYS_INLINE bool operator>(const WStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) > 0;
}

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE bool operator<=(const WStringBase<DerivedLhs>& lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return WStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) <= 0;
}

template <typename DerivedRhs>
W_ALWAYS_INLINE bool operator<=(const char* lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) >= 0;
}

template <typename DerivedLhs>
W_ALWAYS_INLINE bool operator<=(const WStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) <= 0;
}

template <typename DerivedLhs, typename DerivedRhs>
W_ALWAYS_INLINE bool operator>=(const WStringBase<DerivedLhs>& lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return WStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) >= 0;
}

template <typename DerivedRhs>
W_ALWAYS_INLINE bool operator>=(const char* lhs, const WStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) <= 0;
}

template <typename DerivedLhs>
W_ALWAYS_INLINE bool operator>=(const WStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) >= 0;
}

#endif

template <typename DerivedLhs>
W_ALWAYS_INLINE WStringBase<DerivedLhs>::operator WStringView() const
{
  return WStringView(InternalGetData(), InternalGetElementCount());
}

template <typename Derived>
W_ALWAYS_INLINE WStringView WStringBase<Derived>::GetView() const
{
  return WStringView(InternalGetData(), InternalGetElementCount());
}

template <typename Derived>
template <typename Container>
void WStringBase<Derived>::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  GetView().Split(bReturnEmptyStrings, ref_output, szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6);
}

template <typename Derived>
WStringView WStringBase<Derived>::GetRootedPathRootName() const
{
  return GetView().GetRootedPathRootName();
}

template <typename Derived>
bool WStringBase<Derived>::IsRootedPath() const
{
  return GetView().IsRootedPath();
}

template <typename Derived>
bool WStringBase<Derived>::IsRelativePath() const
{
  return GetView().IsRelativePath();
}

template <typename Derived>
bool WStringBase<Derived>::IsAbsolutePath() const
{
  return GetView().IsAbsolutePath();
}

template <typename Derived>
WStringView WStringBase<Derived>::GetFileDirectory() const
{
  return GetView().GetFileDirectory();
}

template <typename Derived>
WStringView WStringBase<Derived>::GetFileNameAndExtension() const
{
  return GetView().GetFileNameAndExtension();
}

template <typename Derived>
WStringView WStringBase<Derived>::GetFileName() const
{
  return GetView().GetFileName();
}

template <typename Derived>
WStringView WStringBase<Derived>::GetFileExtension(bool bFullExtension) const
{
  return GetView().GetFileExtension(bFullExtension);
}

template <typename Derived>
bool WStringBase<Derived>::HasExtension(WStringView sExtension) const
{
  return GetView().HasExtension(sExtension);
}

template <typename Derived>
bool WStringBase<Derived>::HasAnyExtension() const
{
  return GetView().HasAnyExtension();
}
