#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>

#include <stdarg.h>

WStringBuilder::WStringBuilder(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4, WStringView sData5, WStringView sData6)
{
  AppendTerminator();

  Append(sData1, sData2, sData3, sData4, sData5, sData6);
}

void WStringBuilder::Set(WStringView sData1)
{
  Clear();
  Append(sData1);
}

void WStringBuilder::Set(WStringView sData1, WStringView sData2)
{
  Clear();
  Append(sData1, sData2);
}

void WStringBuilder::Set(WStringView sData1, WStringView sData2, WStringView sData3)
{
  Clear();
  Append(sData1, sData2, sData3);
}

void WStringBuilder::Set(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4)
{
  Clear();
  Append(sData1, sData2, sData3, sData4);
}

void WStringBuilder::Set(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4, WStringView sData5, WStringView sData6)
{
  Clear();
  Append(sData1, sData2, sData3, sData4, sData5, sData6);
}

void WStringBuilder::SetPath(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4)
{
  Clear();
  AppendPath(sData1, sData2, sData3, sData4);
}

void WStringBuilder::SetSubString_FromTo(const char* pStart, const char* pEnd)
{
  W_ASSERT_DEBUG(WUnicodeUtils::IsValidUtf8(pStart, pEnd), "Invalid substring, the start does not point to a valid Utf-8 character");

  WStringView view(pStart, pEnd);
  *this = view;
}

void WStringBuilder::SetSubString_ElementCount(const char* pStart, WUInt32 uiElementCount)
{
  W_ASSERT_DEBUG(
    WUnicodeUtils::IsValidUtf8(pStart, pStart + uiElementCount), "Invalid substring, the start does not point to a valid Utf-8 character");

  WStringView view(pStart, pStart + uiElementCount);
  *this = view;
}

void WStringBuilder::SetSubString_CharacterCount(const char* pStart, WUInt32 uiCharacterCount)
{
  const char* pEnd = pStart;
  WUnicodeUtils::MoveToNextUtf8(pEnd, uiCharacterCount).IgnoreResult(); // fine to fail, will just copy as much as possible

  WStringView view(pStart, pEnd);
  *this = view;
}

void WStringBuilder::Append(WStringView sData1)
{
  WUInt32 uiMoreBytes = 0;
  uiMoreBytes += sData1.GetElementCount();

  WUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  {
    const char* szStartPtr = sData1.GetStartPointer();
    const WUInt32 uiStrLen = sData1.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }
}

void WStringBuilder::Append(WStringView sData1, WStringView sData2)
{
  WUInt32 uiMoreBytes = 0;
  uiMoreBytes += sData1.GetElementCount();
  uiMoreBytes += sData2.GetElementCount();

  WUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  {
    const char* szStartPtr = sData1.GetStartPointer();
    const WUInt32 uiStrLen = sData1.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData2.GetStartPointer();
    const WUInt32 uiStrLen = sData2.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }
}

void WStringBuilder::Append(WStringView sData1, WStringView sData2, WStringView sData3)
{
  WUInt32 uiMoreBytes = 0;
  uiMoreBytes += sData1.GetElementCount();
  uiMoreBytes += sData2.GetElementCount();
  uiMoreBytes += sData3.GetElementCount();

  WUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  {
    const char* szStartPtr = sData1.GetStartPointer();
    const WUInt32 uiStrLen = sData1.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData2.GetStartPointer();
    const WUInt32 uiStrLen = sData2.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData3.GetStartPointer();
    const WUInt32 uiStrLen = sData3.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }
}

void WStringBuilder::Append(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4)
{
  WUInt32 uiMoreBytes = 0;
  uiMoreBytes += sData1.GetElementCount();
  uiMoreBytes += sData2.GetElementCount();
  uiMoreBytes += sData3.GetElementCount();
  uiMoreBytes += sData4.GetElementCount();

  WUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  {
    const char* szStartPtr = sData1.GetStartPointer();
    const WUInt32 uiStrLen = sData1.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData2.GetStartPointer();
    const WUInt32 uiStrLen = sData2.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData3.GetStartPointer();
    const WUInt32 uiStrLen = sData3.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData4.GetStartPointer();
    const WUInt32 uiStrLen = sData4.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }
}

void WStringBuilder::Append(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4, WStringView sData5, WStringView sData6)
{
  WUInt32 uiMoreBytes = 0;
  uiMoreBytes += sData1.GetElementCount();
  uiMoreBytes += sData2.GetElementCount();
  uiMoreBytes += sData3.GetElementCount();
  uiMoreBytes += sData4.GetElementCount();
  uiMoreBytes += sData5.GetElementCount();
  uiMoreBytes += sData6.GetElementCount();

  WUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  {
    const char* szStartPtr = sData1.GetStartPointer();
    const WUInt32 uiStrLen = sData1.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData2.GetStartPointer();
    const WUInt32 uiStrLen = sData2.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData3.GetStartPointer();
    const WUInt32 uiStrLen = sData3.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData4.GetStartPointer();
    const WUInt32 uiStrLen = sData4.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData5.GetStartPointer();
    const WUInt32 uiStrLen = sData5.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }

  {
    const char* szStartPtr = sData6.GetStartPointer();
    const WUInt32 uiStrLen = sData6.GetElementCount();
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen + 1, szStartPtr, szStartPtr + uiStrLen);
    uiPrevCount += uiStrLen;
  }
}

void WStringBuilder::Prepend(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4, WStringView sData5, WStringView sData6)
{
  // it is not possible to find out how many parameters were passed to a vararg function
  // with a fixed size of parameters we do not need to have a parameter that tells us how many strings will come

  const WUInt32 uiMaxParams = 6;

  const WStringView pStrings[uiMaxParams] = {sData1, sData2, sData3, sData4, sData5, sData6};
  WUInt32 uiStrLen[uiMaxParams] = {0};

  WUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (WUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    uiStrLen[i] = pStrings[i].GetElementCount();
    uiMoreBytes += uiStrLen[i];

    W_ASSERT_DEBUG(WUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  WUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  W_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // move the previous string data at the end
  WMemoryUtils::CopyOverlapped(&m_Data[0] + uiMoreBytes, GetData(), uiPrevCount);

  WUInt32 uiWritePos = 0;

  // and then prepend all the strings
  for (WUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    WMemoryUtils::Copy(&m_Data[uiWritePos], pStrings[i].GetStartPointer(), uiStrLen[i]);

    uiWritePos += uiStrLen[i];
  }
}

void WStringBuilder::SetPrintfArgs(const char* szUtf8Format, va_list szArgs0)
{
  va_list args;
  va_copy(args, szArgs0);

  Clear();

  const WUInt32 TempBuffer = 4096;

  char szTemp[TempBuffer];
  const WInt32 iCount = WStringUtils::vsnprintf(szTemp, TempBuffer - 1, szUtf8Format, args);

  W_ASSERT_DEV(iCount != -1, "There was an error while formatting the string. Probably and unescaped usage of the %% sign.");

  if (iCount == -1)
  {
    va_end(args);
    return;
  }

  if (iCount > TempBuffer - 1)
  {
    WDynamicArray<char> Temp;
    Temp.SetCountUninitialized(iCount + 1);

    WStringUtils::vsnprintf(&Temp[0], iCount + 1, szUtf8Format, args);

    Append(&Temp[0]);
  }
  else
  {
    Append(&szTemp[0]);
  }

  va_end(args);
}

void WStringBuilder::ChangeCharacterNonASCII(iterator& it, WUInt32 uiCharacter)
{
  char* pPos = const_cast<char*>(it.GetData()); // yes, I know...

  const WUInt32 uiOldCharLength = WUnicodeUtils::GetUtf8SequenceLength(*pPos);
  const WUInt32 uiNewCharLength = WUnicodeUtils::GetSizeForCharacterInUtf8(uiCharacter);

  // if the old character and the new one are encoded with the same length, we can replace the character in-place
  if (uiNewCharLength == uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    WUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // if the encoding length is identical, this will also handle all ASCII strings
    // if the string was pure ASCII before, this won't change, so no need to update that state
    return;
  }

  // in this case we can still update the string without reallocation, but the tail of the string has to be moved forwards
  if (uiNewCharLength < uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    WUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // pPos will be changed (moved forwards) to the next character position

    // how much has changed
    const WUInt32 uiDifference = uiOldCharLength - uiNewCharLength;
    const WUInt32 uiTrailStringBytes = (WUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1); // ???

    // move the trailing characters forwards
    WMemoryUtils::CopyOverlapped(pPos, pPos + uiDifference, uiTrailStringBytes);

    // update the data array
    m_Data.PopBack(uiDifference);

    // 'It' references this already, no need to change anything.
  }
  else
  {
    // in this case we insert a character that is longer int Utf8 encoding than the character that already exists there *sigh*
    // so we must first move the trailing string backwards to make room, then we can write the new char in there

    // how much has changed
    const WUInt32 uiDifference = uiNewCharLength - uiOldCharLength;
    const WUInt32 uiTrailStringBytes = (WUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1);
    auto iCurrentPos = (it.GetData() - GetData());
    // resize the array
    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // these might have changed (array realloc)
    pPos = &m_Data[0] + iCurrentPos;
    it.SetCurrentPosition(pPos);

    // move the trailing string backwards
    WMemoryUtils::CopyOverlapped(pPos + uiNewCharLength, pPos + uiOldCharLength, uiTrailStringBytes);

    // just overwrite all characters at the given position with the new Utf8 string
    WUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);
  }
}

void WStringBuilder::Shrink(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack)
{
  if (uiShrinkCharsBack > 0)
  {
    const char* szEnd = GetData() + GetElementCount();
    const char* szNewEnd = szEnd;
    if (WUnicodeUtils::MoveToPriorUtf8(szNewEnd, GetData(), uiShrinkCharsBack).Failed())
    {
      Clear();
      return;
    }

    const WUInt32 uiLessBytes = (WUInt32)(szEnd - szNewEnd);

    m_Data.PopBack(uiLessBytes + 1);
    AppendTerminator();
  }

  const char* szNewStart = &m_Data[0];
  if (WUnicodeUtils::MoveToNextUtf8(szNewStart, uiShrinkCharsFront).Failed())
  {
    Clear();
    return;
  }

  if (szNewStart > &m_Data[0])
  {
    const WUInt32 uiLessBytes = (WUInt32)(szNewStart - &m_Data[0]);

    WMemoryUtils::CopyOverlapped(&m_Data[0], szNewStart, m_Data.GetCount() - uiLessBytes);
    m_Data.PopBack(uiLessBytes);
  }
}

void WStringBuilder::ReplaceSubString(const char* szStartPos, const char* szEndPos, WStringView sReplaceWith)
{
  W_ASSERT_DEV(WMath::IsInRange(szStartPos, GetData(), GetData() + m_Data.GetCount()), "szStartPos is not inside this string.");
  W_ASSERT_DEV(WMath::IsInRange(szEndPos, GetData(), GetData() + m_Data.GetCount()), "szEndPos is not inside this string.");
  W_ASSERT_DEV(szStartPos <= szEndPos, "WStartPos must be before WEndPos");

  const WUInt32 uiWordBytes = sReplaceWith.GetElementCount();

  const WUInt32 uiSubStringBytes = (WUInt32)(szEndPos - szStartPos);

  char* szWritePos = const_cast<char*>(szStartPos); // szStartPos points into our own data anyway
  const char* szReadPos = sReplaceWith.GetStartPointer();

  // most simple case, just replace characters
  if (uiSubStringBytes == uiWordBytes)
  {
    while (szWritePos < szEndPos)
    {
      *szWritePos = *szReadPos;
      ++szWritePos;
      ++szReadPos;
    }

    return;
  }

  // the replacement is shorter than the existing stuff -> move characters to the left, no reallocation needed
  if (uiWordBytes < uiSubStringBytes)
  {
    // first copy the replacement to the correct position
    WMemoryUtils::Copy(szWritePos, sReplaceWith.GetStartPointer(), uiWordBytes);

    const WUInt32 uiDifference = uiSubStringBytes - uiWordBytes;

    const char* szStringEnd = GetData() + m_Data.GetCount();

    // now move all the characters from behind the replaced string to the correct position
    WMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    m_Data.PopBack(uiDifference);

    return;
  }

  // else the replacement is longer than the existing word
  {
    const WUInt32 uiDifference = uiWordBytes - uiSubStringBytes;
    const WUInt64 uiRelativeWritePosition = szWritePos - GetData();
    const WUInt64 uiDataByteCountBefore = m_Data.GetCount();

    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // all pointer are now possibly invalid since the data may be reallocated!
    szWritePos = const_cast<char*>(GetData()) + uiRelativeWritePosition;
    const char* szStringEnd = GetData() + uiDataByteCountBefore;

    // first move the characters to the proper position from back to front
    WMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    // now copy the replacement to the correct position
    WMemoryUtils::Copy(szWritePos, sReplaceWith.GetStartPointer(), uiWordBytes);
  }
}

const char* WStringBuilder::ReplaceFirst(WStringView sSearchFor, WStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    W_ASSERT_DEV(WMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = WStringUtils::FindSubString(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const WUInt32 uiOffset = (WUInt32)(szFoundAt - GetData());

  const WUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* WStringBuilder::ReplaceLast(WStringView sSearchFor, WStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    W_ASSERT_DEV(WMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = WStringUtils::FindLastSubString(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const WUInt32 uiOffset = (WUInt32)(szFoundAt - GetData());

  const WUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

WUInt32 WStringBuilder::ReplaceAll(WStringView sSearchFor, WStringView sReplacement)
{
  const WUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const WUInt32 uiWordBytes = sReplacement.GetElementCount();

  WUInt32 uiReplacements = 0;
  WUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = WStringUtils::FindSubString(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<WUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}


const char* WStringBuilder::ReplaceFirst_NoCase(WStringView sSearchFor, WStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    W_ASSERT_DEV(WMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = WStringUtils::FindSubString_NoCase(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const WUInt32 uiOffset = (WUInt32)(szFoundAt - GetData());

  const WUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* WStringBuilder::ReplaceLast_NoCase(WStringView sSearchFor, WStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    W_ASSERT_DEV(WMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = WStringUtils::FindLastSubString_NoCase(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const WUInt32 uiOffset = (WUInt32)(szFoundAt - GetData());

  const WUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

WUInt32 WStringBuilder::ReplaceAll_NoCase(WStringView sSearchFor, WStringView sReplacement)
{
  const WUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const WUInt32 uiWordBytes = sReplacement.GetElementCount();

  WUInt32 uiReplacements = 0;
  WUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = WStringUtils::FindSubString_NoCase(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<WUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}

const char* WStringBuilder::ReplaceWholeWord(const char* szSearchFor, WStringView sReplaceWith, WStringUtils::W_CHARACTER_FILTER isDelimiterCB)
{
  const char* szPos = FindWholeWord(szSearchFor, isDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const WUInt32 uiOffset = static_cast<WUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + WStringUtils::GetStringElementCount(szSearchFor), sReplaceWith);
  return GetData() + uiOffset;
}

const char* WStringBuilder::ReplaceWholeWord_NoCase(const char* szSearchFor, WStringView sReplaceWith, WStringUtils::W_CHARACTER_FILTER isDelimiterCB)
{
  const char* szPos = FindWholeWord_NoCase(szSearchFor, isDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const WUInt32 uiOffset = static_cast<WUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + WStringUtils::GetStringElementCount(szSearchFor), sReplaceWith);
  return GetData() + uiOffset;
}


WUInt32 WStringBuilder::ReplaceWholeWordAll(const char* szSearchFor, WStringView sReplaceWith, WStringUtils::W_CHARACTER_FILTER isDelimiterCB)
{
  const WUInt32 uiSearchBytes = WStringUtils::GetStringElementCount(szSearchFor);
  const WUInt32 uiWordBytes = WStringUtils::GetStringElementCount(sReplaceWith.GetStartPointer(), sReplaceWith.GetEndPointer());

  WUInt32 uiReplacements = 0;
  WUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = WStringUtils::FindWholeWord(GetData() + uiOffset, szSearchFor, isDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<WUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

WUInt32 WStringBuilder::ReplaceWholeWordAll_NoCase(const char* szSearchFor, WStringView sReplaceWith, WStringUtils::W_CHARACTER_FILTER isDelimiterCB)
{
  const WUInt32 uiSearchBytes = WStringUtils::GetStringElementCount(szSearchFor);
  const WUInt32 uiWordBytes = WStringUtils::GetStringElementCount(sReplaceWith.GetStartPointer(), sReplaceWith.GetEndPointer());

  WUInt32 uiReplacements = 0;
  WUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = WStringUtils::FindWholeWord_NoCase(GetData() + uiOffset, szSearchFor, isDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<WUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

void WStringBuilder::operator=(WStringView rhs)
{
  WUInt32 uiBytes = rhs.GetElementCount();

  // if we need more room, allocate up front (rhs cannot use our own data in this case)
  if (uiBytes + 1 > m_Data.GetCount())
    m_Data.SetCountUninitialized(uiBytes + 1);

  // the data might actually come from our very own string, so we 'move' the memory in there, just to be safe
  // if it comes from our own array, the data will always be a sub-set -> smaller than this array
  // in this case we defer the SetCount till later, to ensure that the data is not corrupted (destructed) before we copy it
  // however, when the new data is larger than the old, it cannot be from our own data, so we can (and must) reallocate before copying
  WMemoryUtils::CopyOverlapped(&m_Data[0], rhs.GetStartPointer(), uiBytes);

  m_Data.SetCountUninitialized(uiBytes + 1);
  m_Data[uiBytes] = '\0';
}

enum PathUpState
{
  NotStarted,
  OneDot,
  TwoDots,
  FoundDotSlash,
  FoundDotDotSlash,
  Invalid,
};

void WStringBuilder::MakeCleanPath()
{
  if (IsEmpty())
    return;

  Trim(" \t\r\n");

  RemoveDoubleSlashesInPath();

  // remove Windows specific DOS device path indicators from the start
  TrimWordStart("//?/");
  TrimWordStart("//./");

  const char* const szEndPos = &m_Data[m_Data.GetCount() - 1];
  const char* szCurReadPos = &m_Data[0];
  char* const szCurWritePos = &m_Data[0];
  int writeOffset = 0;

  WInt32 iLevelsDown = 0;
  PathUpState FoundPathUp = NotStarted;

  while (szCurReadPos < szEndPos)
  {
    char CurChar = *szCurReadPos;

    if (CurChar == '.')
    {
      if (FoundPathUp == NotStarted)
        FoundPathUp = OneDot;
      else if (FoundPathUp == OneDot)
        FoundPathUp = TwoDots;
      else
        FoundPathUp = Invalid;
    }
    else if (WPathUtils::IsPathSeparator(CurChar))
    {
      CurChar = '/';

      if (FoundPathUp == OneDot)
      {
        FoundPathUp = FoundDotSlash;
      }
      else if (FoundPathUp == TwoDots)
      {
        FoundPathUp = FoundDotDotSlash;
      }
      else
      {
        ++iLevelsDown;
        FoundPathUp = NotStarted;
      }
    }
    else
      FoundPathUp = NotStarted;

    if (FoundPathUp == FoundDotDotSlash)
    {
      if (iLevelsDown > 0)
      {
        --iLevelsDown;
        W_ASSERT_DEBUG(writeOffset >= 3, "invalid write offset");
        writeOffset -= 3; // go back, skip two dots, one slash

        while ((writeOffset > 0) && (szCurWritePos[writeOffset - 1] != '/'))
        {
          W_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
          --writeOffset;
        }
      }
      else
      {
        szCurWritePos[writeOffset] = '/';
        ++writeOffset;
      }

      FoundPathUp = NotStarted;
    }
    else if (FoundPathUp == FoundDotSlash)
    {
      W_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
      writeOffset -= 1; // go back to where we wrote the dot

      FoundPathUp = NotStarted;
    }
    else
    {
      szCurWritePos[writeOffset] = CurChar;
      ++writeOffset;
    }

    ++szCurReadPos;
  }

  const WUInt32 uiPrevByteCount = m_Data.GetCount();
  const WUInt32 uiNewByteCount = (WUInt32)(writeOffset) + 1;

  W_IGNORE_UNUSED(uiPrevByteCount);
  W_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
    uiPrevByteCount, uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  szCurWritePos[writeOffset] = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}

void WStringBuilder::PathParentDirectory(WUInt32 uiLevelsUp)
{
  W_ASSERT_DEV(uiLevelsUp > 0, "We have to do something!");

  for (WUInt32 i = 0; i < uiLevelsUp; ++i)
    AppendPath("../");

  MakeCleanPath();
}

void WStringBuilder::AppendPath(WStringView sPath1, WStringView sPath2, WStringView sPath3, WStringView sPath4)
{
  const WStringView sPaths[4] = {sPath1, sPath2, sPath3, sPath4};

  for (WUInt32 i = 0; i < 4; ++i)
  {
    WStringView sThisPath = sPaths[i];

    if (!sThisPath.IsEmpty())
    {
      if ((IsEmpty() && WPathUtils::IsAbsolutePath(sPaths[i])))
      {
        // this is for Linux systems where absolute paths start with a slash, wouldn't want to remove that
      }
      else
      {
        // prevent creating multiple path separators through concatenation
        while (WPathUtils::IsPathSeparator(*sThisPath.GetStartPointer()))
          sThisPath.ChopAwayFirstCharacterAscii();
      }

      if (IsEmpty() || WPathUtils::IsPathSeparator(GetIteratorBack().GetCharacter()))
        Append(sThisPath);
      else
        Append("/", sThisPath);
    }
  }
}

void WStringBuilder::AppendWithSeparator(WStringView sOptional, WStringView sText1, WStringView sText2 /*= WStringView()*/,
  WStringView sText3 /*= WStringView()*/, WStringView sText4 /*= WStringView()*/, WStringView sText5 /*= WStringView()*/,
  WStringView sText6 /*= WStringView()*/)
{
  // if this string already ends with the optional string, reset it to be empty
  if (IsEmpty() || WStringUtils::EndsWith(GetData(), sOptional.GetStartPointer(), GetData() + GetElementCount(), sOptional.GetEndPointer()))
  {
    sOptional = WStringView();
  }

  const WUInt32 uiMaxParams = 7;

  const WStringView pStrings[uiMaxParams] = {sOptional, sText1, sText2, sText3, sText4, sText5, sText6};
  WUInt32 uiStrLen[uiMaxParams] = {0};
  WUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (WUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    W_ASSERT_DEBUG(pStrings[i].GetStartPointer() < m_Data.GetData() || pStrings[i].GetStartPointer() >= m_Data.GetData() + m_Data.GetCapacity(),
      "Parameter {0} comes from the string builders own storage. This type assignment is not allowed.", i);

    uiStrLen[i] = pStrings[i].GetElementCount();
    uiMoreBytes += uiStrLen[i];

    W_ASSERT_DEV(WUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  if (uiMoreBytes == uiStrLen[0])
  {
    // if all other strings (than the separator) are empty, don't append anything
    return;
  }

  WUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  W_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // and then append all the strings
  for (WUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    WStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen[i] + 1, pStrings[i].GetStartPointer(), pStrings[i].GetStartPointer() + uiStrLen[i]);

    uiPrevCount += uiStrLen[i];
  }
}

void WStringBuilder::ChangeFileName(WStringView sNewFileName)
{
  WStringView it = WPathUtils::GetFileName(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileName);
}

void WStringBuilder::ChangeFileNameAndExtension(WStringView sNewFileNameWithExtension)
{
  WStringView it = WPathUtils::GetFileNameAndExtension(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileNameWithExtension);
}

void WStringBuilder::ChangeFileExtension(WStringView sNewExtension, bool bFullExtension /*= false*/)
{
  while (sNewExtension.StartsWith("."))
  {
    sNewExtension.ChopAwayFirstCharacterAscii();
  }

  const WStringView it = WPathUtils::GetFileExtension(GetView(), bFullExtension);

  if (it.IsEmpty())
  {
    if (!EndsWith("."))
    {
      Append(".", sNewExtension);
    }
    else
    {
      Append(sNewExtension);
    }
  }
  else
  {
    ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewExtension);
  }
}

void WStringBuilder::RemoveFileExtension(bool bFullExtension /*= false*/)
{
  if (HasAnyExtension())
  {
    ChangeFileExtension("", bFullExtension);
    Shrink(0, 1); // remove the dot
  }
}

WResult WStringBuilder::MakeRelativeTo(WStringView sAbsolutePathToMakeThisRelativeTo)
{
  WStringBuilder sAbsBase = sAbsolutePathToMakeThisRelativeTo;
  sAbsBase.MakeCleanPath();
  WStringBuilder sAbsThis = *this;
  sAbsThis.MakeCleanPath();

  if (sAbsBase.IsEqual_NoCase(sAbsThis.GetData()))
  {
    Clear();
    return W_SUCCESS;
  }

  if (!sAbsBase.EndsWith("/"))
    sAbsBase.Append("/");

  if (!sAbsThis.EndsWith("/"))
  {
    sAbsThis.Append("/");

    if (sAbsBase.StartsWith(sAbsThis.GetData()))
    {
      Clear();
      const char* szStart = &sAbsBase.GetData()[sAbsThis.GetElementCount()];

      while (*szStart != '\0')
      {
        if (*szStart == '/')
          Append("../");

        ++szStart;
      }

      return W_SUCCESS;
    }
    else
      sAbsThis.Shrink(0, 1);
  }

  const WUInt32 uiMinLen = WMath::Min(sAbsBase.GetElementCount(), sAbsThis.GetElementCount());

  WInt32 iSame = uiMinLen - 1;
  for (; iSame >= 0; --iSame)
  {
    if (sAbsBase.GetData()[iSame] != '/')
      continue;

    // We need to check here if sAbsThis starts with sAbsBase in the range[0, iSame + 1]. However, we can't compare the first N bytes because those might not be a valid utf8 substring in absBase.
    // Thus we can't use IsEqualN_NoCase as N would need to be the number of characters, not bytes. Computing the number of characters in absBase would mean iterating the string twice.
    // As an alternative, as we know [0, iSame + 1] is a valid utf8 string in sAbsBase we can ask whether absThis starts with that substring.
    if (WStringUtils::StartsWith_NoCase(sAbsThis.GetData(), sAbsBase.GetData(), sAbsThis.GetData() + sAbsThis.GetElementCount(), sAbsBase.GetData() + iSame + 1))
      break;
  }

  if (iSame < 0)
  {
    return W_FAILURE;
  }

  Clear();

  for (WUInt32 ui = iSame + 1; ui < sAbsBase.GetElementCount(); ++ui)
  {
    if (sAbsBase.GetData()[ui] == '/')
      Append("../");
  }

  if (sAbsThis.GetData()[iSame] == '/')
    ++iSame;

  Append(&(sAbsThis.GetData()[iSame]));

  return W_SUCCESS;
}

/// An empty folder (zero length) does not contain ANY files.\n
/// A non-existing file-name (zero length) is never in any folder.\n
/// Example:\n
/// IsFileBelowFolder ("", "XYZ") -> always false\n
/// IsFileBelowFolder ("XYZ", "") -> always false\n
/// IsFileBelowFolder ("", "") -> always false\n
bool WStringBuilder::IsPathBelowFolder(const char* szPathToFolder)
{
  W_ASSERT_DEV(!WStringUtils::IsNullOrEmpty(szPathToFolder), "The given path must not be empty. Because is 'nothing' under the empty path, or 'everything' ?");

  // a non-existing file is never in any folder
  if (IsEmpty())
    return false;

  MakeCleanPath();

  WStringBuilder sBasePath(szPathToFolder);
  sBasePath.MakeCleanPath();

  if (IsEqual_NoCase(sBasePath.GetData()))
    return true;

  if (!sBasePath.EndsWith("/"))
    sBasePath.Append("/");

  return StartsWith_NoCase(sBasePath.GetData());
}

void WStringBuilder::MakePathSeparatorsNative()
{
  const char sep = WPathUtils::OsSpecificPathSeparator;

  MakeCleanPath();
  ReplaceAll("/", WStringView(&sep, 1));
}

void WStringBuilder::RemoveDoubleSlashesInPath()
{
  if (IsEmpty())
    return;

  const char* szReadPos = &m_Data[0];
  char* szCurWritePos = &m_Data[0];

  WInt32 iAllowedSlashes = 2;

  while (*szReadPos != '\0')
  {
    char CurChar = *szReadPos;
    ++szReadPos;

    if (CurChar == '\\')
      CurChar = '/';

    if (CurChar != '/')
      iAllowedSlashes = 1;
    else
    {
      if (iAllowedSlashes > 0)
        --iAllowedSlashes;
      else
        continue;
    }

    *szCurWritePos = CurChar;
    ++szCurWritePos;
  }


  const WUInt32 uiPrevByteCount = m_Data.GetCount();
  const WUInt32 uiNewByteCount = (WUInt32)(szCurWritePos - &m_Data[0]) + 1;

  W_IGNORE_UNUSED(uiPrevByteCount);
  W_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
    uiPrevByteCount, uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  *szCurWritePos = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}


void WStringBuilder::ReadAll(WStreamReader& inout_stream)
{
  Clear();

  WHybridArray<WUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  WUInt8 Temp[1024];

  while (true)
  {
    const WUInt32 uiRead = (WUInt32)inout_stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(WArrayPtr<WUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  // A BOM is an encoding marker, not part of the text. Many editors write one silently, so it
  // would otherwise end up in the first token of whatever parses the result.
  const char* szData = (const char*)&Bytes[0];
  WUnicodeUtils::SkipUtf8Bom(szData);

  *this = szData;
}

void WStringBuilder::Trim(const char* szTrimChars)
{
  Trim(szTrimChars, szTrimChars);
}

void WStringBuilder::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  const char* szNewStart = GetData();
  const char* szNewEnd = GetData() + GetElementCount();
  WStringUtils::Trim(szNewStart, szNewEnd, szTrimCharsStart, szTrimCharsEnd);
  Shrink(WStringUtils::GetCharacterCount(GetData(), szNewStart), WStringUtils::GetCharacterCount(szNewEnd, GetData() + GetElementCount()));
}

void WStringBuilder::TrimLeft(const char* szTrimChars /*= " \f\n\r\t\v"*/)
{
  Trim(szTrimChars, "");
}

void WStringBuilder::TrimRight(const char* szTrimChars /*= " \f\n\r\t\v"*/)
{
  Trim("", szTrimChars);
}

bool WStringBuilder::TrimWordStart(WStringView sWord)
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

bool WStringBuilder::TrimWordEnd(WStringView sWord)
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

void WStringBuilder::RemoveCStyleComments()
{
  const char* p = GetData();
  const char* pEnd = p + GetElementCount();

  WStringBuilder result;
  result.Reserve(GetElementCount());

  const char* pChunkStart = p;

  while (p < pEnd)
  {
    if (p[0] == '/' && p + 1 < pEnd && p[1] == '/')
    {
      result.Append(WStringView(pChunkStart, p));
      while (p < pEnd && *p != '\n')
        ++p;
      pChunkStart = p;
    }
    else if (p[0] == '/' && p + 1 < pEnd && p[1] == '*')
    {
      result.Append(WStringView(pChunkStart, p));
      p += 2;
      while (p < pEnd)
      {
        if (*p == '*' && p + 1 < pEnd && p[1] == '/')
        {
          p += 2;
          break;
        }
        if (*p == '\n')
          result.Append('\n');
        ++p;
      }
      pChunkStart = p;
    }
    else
    {
      ++p;
    }
  }

  result.Append(WStringView(pChunkStart, p));
  *this = result;
}

void WStringBuilder::SetFormat(const WFormatString& string)
{
  Clear();
  WStringView sText = string.GetText(*this);

  // this is for the case that GetText does not use the WStringBuilder as temp storage
  if (sText.GetStartPointer() != GetData())
    *this = sText;
}

void WStringBuilder::AppendFormat(const WFormatString& string)
{
  WStringBuilder tmp;
  WStringView view = string.GetText(tmp);

  Append(view);
}

void WStringBuilder::PrependFormat(const WFormatString& string)
{
  WStringBuilder tmp;

  Prepend(string.GetText(tmp));
}

void WStringBuilder::SetPrintf(const char* szUtf8Format, ...)
{
  va_list args;
  va_start(args, szUtf8Format);

  SetPrintfArgs(szUtf8Format, args);

  va_end(args);
}

#if W_ENABLED(W_INTEROP_STL_STRINGS)
WStringBuilder::WStringBuilder(const std::string_view& rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();

  *this = rhs;
}

WStringBuilder::WStringBuilder(const std::string& rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();

  *this = rhs;
}

void WStringBuilder::operator=(const std::string_view& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    *this = WStringView(rhs.data(), rhs.data() + rhs.size());
  }
}

void WStringBuilder::operator=(const std::string& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    *this = WStringView(rhs.data(), rhs.data() + rhs.size());
  }
}

#endif
