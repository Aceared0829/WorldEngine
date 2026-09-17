#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Time/Timestamp.h>

// Helper function to shift windows file time into Unix epoch (in microseconds).
WInt64 FileTimeToEpoch(FILETIME fileTime)
{
  ULARGE_INTEGER currentTime;
  currentTime.LowPart = fileTime.dwLowDateTime;
  currentTime.HighPart = fileTime.dwHighDateTime;

  WInt64 iTemp = currentTime.QuadPart / 10;
  iTemp -= 11644473600000000LL;
  return iTemp;
}

// Helper function to shift Unix epoch (in microseconds) into windows file time.
FILETIME EpochToFileTime(WInt64 iFileTime)
{
  WInt64 iTemp = iFileTime + 11644473600000000LL;
  iTemp *= 10;

  FILETIME fileTime;
  ULARGE_INTEGER currentTime;
  currentTime.QuadPart = iTemp;
  fileTime.dwLowDateTime = currentTime.LowPart;
  fileTime.dwHighDateTime = currentTime.HighPart;
  return fileTime;
}

const WTimestamp WTimestamp::CurrentTimestamp()
{
  FILETIME fileTime;
  GetSystemTimeAsFileTime(&fileTime);
  return WTimestamp::MakeFromInt(FileTimeToEpoch(fileTime), WSIUnitOfTime::Microsecond);
}

const WTimestamp WDateTime::GetTimestamp() const
{
  SYSTEMTIME st;
  FILETIME fileTime;
  memset(&st, 0, sizeof(SYSTEMTIME));
  st.wYear = (WORD)m_iYear;
  st.wMonth = m_uiMonth;
  st.wDay = m_uiDay;
  st.wDayOfWeek = m_uiDayOfWeek;
  st.wHour = m_uiHour;
  st.wMinute = m_uiMinute;
  st.wSecond = m_uiSecond;
  st.wMilliseconds = (WORD)(m_uiMicroseconds / 1000);
  BOOL res = SystemTimeToFileTime(&st, &fileTime);
  WTimestamp timestamp;
  if (res != 0)
    timestamp = WTimestamp::MakeFromInt(FileTimeToEpoch(fileTime), WSIUnitOfTime::Microsecond);

  return timestamp;
}

WResult WDateTime::SetFromTimestamp(WTimestamp timestamp)
{
  FILETIME fileTime = EpochToFileTime(timestamp.GetInt64(WSIUnitOfTime::Microsecond));

  SYSTEMTIME st;
  BOOL res = FileTimeToSystemTime(&fileTime, &st);
  if (res == 0)
    return W_FAILURE;

  m_iYear = (WInt16)st.wYear;
  m_uiMonth = (WUInt8)st.wMonth;
  m_uiDay = (WUInt8)st.wDay;
  m_uiDayOfWeek = (WUInt8)st.wDayOfWeek;
  m_uiHour = (WUInt8)st.wHour;
  m_uiMinute = (WUInt8)st.wMinute;
  m_uiSecond = (WUInt8)st.wSecond;
  m_uiMicroseconds = WUInt32(st.wMilliseconds * 1000);
  return W_SUCCESS;
}

#endif
