#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/Timestamp.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WTimestamp, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("time", m_iTimestamp),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WInt64 WTimestamp::GetInt64(WSIUnitOfTime::Enum unitOfTime) const
{
  W_ASSERT_DEV(IsValid(), "Can't retrieve timestamp of invalid values!");
  W_ASSERT_DEV(unitOfTime >= WSIUnitOfTime::Nanosecond && unitOfTime <= WSIUnitOfTime::Second, "Invalid WSIUnitOfTime value ({0})", unitOfTime);

  switch (unitOfTime)
  {
    case WSIUnitOfTime::Nanosecond:
      return m_iTimestamp * 1000LL;
    case WSIUnitOfTime::Microsecond:
      return m_iTimestamp;
    case WSIUnitOfTime::Millisecond:
      return m_iTimestamp / 1000LL;
    case WSIUnitOfTime::Second:
      return m_iTimestamp / 1000000LL;
  }
  return W_INVALID_TIME_STAMP;
}

WTimestamp WTimestamp::MakeFromInt(WInt64 iTimeValue, WSIUnitOfTime::Enum unitOfTime)
{
  W_ASSERT_DEV(unitOfTime >= WSIUnitOfTime::Nanosecond && unitOfTime <= WSIUnitOfTime::Second, "Invalid WSIUnitOfTime value ({0})", unitOfTime);

  WTimestamp ts;

  switch (unitOfTime)
  {
    case WSIUnitOfTime::Nanosecond:
      ts.m_iTimestamp = iTimeValue / 1000LL;
      break;
    case WSIUnitOfTime::Microsecond:
      ts.m_iTimestamp = iTimeValue;
      break;
    case WSIUnitOfTime::Millisecond:
      ts.m_iTimestamp = iTimeValue * 1000LL;
      break;
    case WSIUnitOfTime::Second:
      ts.m_iTimestamp = iTimeValue * 1000000LL;
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ts;
}

bool WTimestamp::Compare(const WTimestamp& rhs, CompareMode::Enum mode) const
{
  switch (mode)
  {
    case CompareMode::FileTimeEqual:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) == (rhs.m_iTimestamp / 1000000LL);

    case CompareMode::Identical:
      return m_iTimestamp == rhs.m_iTimestamp;

    case CompareMode::Newer:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) > (rhs.m_iTimestamp / 1000000LL);
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return false;
}

WDateTime::WDateTime() = default;
WDateTime::~WDateTime() = default;

WDateTime WDateTime::MakeFromTimestamp(WTimestamp timestamp)
{
  WDateTime res;
  res.SetFromTimestamp(timestamp).AssertSuccess("Invalid timestamp");
  return res;
}

bool WDateTime::IsValid() const
{
  if (m_uiMonth <= 0 || m_uiMonth > 12)
    return false;

  if (m_uiDay <= 0 || m_uiDay > 31)
    return false;

  if (m_uiDayOfWeek > 6)
    return false;

  if (m_uiHour > 23)
    return false;

  if (m_uiMinute > 59)
    return false;

  if (m_uiSecond > 59)
    return false;

  return true;
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WDateTime& arg)
{
  WStringUtils::snprintf(szTmp, uiLength, "%04u-%02u-%02u_%02u-%02u-%02u-%03u", arg.GetYear(), arg.GetMonth(), arg.GetDay(), arg.GetHour(),
    arg.GetMinute(), arg.GetSecond(), arg.GetMicroseconds() / 1000);

  return szTmp;
}

namespace
{
  // This implementation chooses a 3-character-long short name for each of the twelve months
  // for consistency reasons. Mind, that other, potentially more widely-spread stylist
  // alternatives may exist.
  const char* GetMonthShortName(const WDateTime& dateTime)
  {
    switch (dateTime.GetMonth())
    {
      case 1:
        return "Jan";
      case 2:
        return "Feb";
      case 3:
        return "Mar";
      case 4:
        return "Apr";
      case 5:
        return "May";
      case 6:
        return "Jun";
      case 7:
        return "Jul";
      case 8:
        return "Aug";
      case 9:
        return "Sep";
      case 10:
        return "Oct";
      case 11:
        return "Nov";
      case 12:
        return "Dec";
      default:
        W_ASSERT_DEV(false, "Unknown month.");
        return "Unknown Month";
    }
  }

  // This implementation chooses a 3-character-long short name for each of the seven days
  // of the week for consistency reasons. Mind, that other, potentially more widely-spread
  // stylistic alternatives may exist.
  const char* GetDayOfWeekShortName(const WDateTime& dateTime)
  {
    switch (dateTime.GetDayOfWeek())
    {
      case 0:
        return "Sun";
      case 1:
        return "Mon";
      case 2:
        return "Tue";
      case 3:
        return "Wed";
      case 4:
        return "Thu";
      case 5:
        return "Fri";
      case 6:
        return "Sat";
      default:
        W_ASSERT_DEV(false, "Unknown day of week.");
        return "Unknown Day of Week";
    }
  }
} // namespace

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgDateTime& arg)
{
  const WDateTime& dateTime = arg.m_Value;

  WUInt32 offset = 0;

  if ((arg.m_uiFormattingFlags & WArgDateTime::ShowDate) == WArgDateTime::ShowDate)
  {
    if ((arg.m_uiFormattingFlags & WArgDateTime::TextualDate) == WArgDateTime::TextualDate)
    {
      offset += WStringUtils::snprintf(
        szTmp + offset, uiLength - offset, "%04u %s %02u", dateTime.GetYear(), ::GetMonthShortName(dateTime), dateTime.GetDay());
    }
    else
    {
      offset +=
        WStringUtils::snprintf(szTmp + offset, uiLength - offset, "%04u-%02u-%02u", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay());
    }
  }

  if ((arg.m_uiFormattingFlags & WArgDateTime::ShowWeekday) == WArgDateTime::ShowWeekday)
  {
    // add a space
    if (offset != 0)
    {
      szTmp[offset] = ' ';
      ++offset;
      szTmp[offset] = '\0';
    }

    offset += WStringUtils::snprintf(szTmp + offset, uiLength - offset, "(%s)", ::GetDayOfWeekShortName(dateTime));
  }

  if ((arg.m_uiFormattingFlags & WArgDateTime::ShowTime) == WArgDateTime::ShowTime)
  {
    // add a space
    if (offset != 0)
    {
      szTmp[offset] = ' ';
      szTmp[offset + 1] = '-';
      szTmp[offset + 2] = ' ';
      szTmp[offset + 3] = '\0';
      offset += 3;
    }

    offset += WStringUtils::snprintf(szTmp + offset, uiLength - offset, "%02u:%02u", dateTime.GetHour(), dateTime.GetMinute());

    if ((arg.m_uiFormattingFlags & WArgDateTime::ShowSeconds) == WArgDateTime::ShowSeconds)
    {
      offset += WStringUtils::snprintf(szTmp + offset, uiLength - offset, ":%02u", dateTime.GetSecond());
    }

    if ((arg.m_uiFormattingFlags & WArgDateTime::ShowMilliseconds) == WArgDateTime::ShowMilliseconds)
    {
      offset += WStringUtils::snprintf(szTmp + offset, uiLength - offset, ".%03u", dateTime.GetMicroseconds() / 1000);
    }

    if ((arg.m_uiFormattingFlags & WArgDateTime::ShowTimeZone) == WArgDateTime::ShowTimeZone)
    {
      WStringUtils::snprintf(szTmp + offset, uiLength - offset, " (UTC)");
    }
  }

  return szTmp;
}

W_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Timestamp);
