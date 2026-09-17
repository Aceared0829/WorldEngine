#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_OSX)

#  include <Foundation/Time/Timestamp.h>

#  include <Foundation/Platform/OSX/Utils/ScopedCFRef.h>

#  include <CoreFoundation/CFCalendar.h>
#  include <CoreFoundation/CoreFoundation.h>

const WTimestamp WTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);

  return WTimestamp::MakeFromInt(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, WSIUnitOfTime::Microsecond);
}

const WTimestamp WDateTime::GetTimestamp() const
{
  WScopedCFRef<CFTimeZoneRef> timezone(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  WScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timezone);

  int year = m_iYear, month = m_uiMonth, day = m_uiDay, hour = m_uiHour, minute = m_uiMinute, second = m_uiSecond;

  // Validate the year against the valid range of the calendar
  {
    auto yearMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitYear), yearMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitYear);

    if (year < yearMin.location || year > yearMax.length)
    {
      return WTimestamp::MakeInvalid();
    }
  }

  // Validate the month against the valid range of the calendar
  {
    auto monthMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitMonth), monthMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitMonth);

    if (month < monthMin.location || month > monthMax.length)
    {
      return WTimestamp::MakeInvalid();
    }
  }

  // Validate the day against the valid range of the calendar
  {
    auto dayMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitDay), dayMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitDay);

    if (day < dayMin.location || day > dayMax.length)
    {
      return WTimestamp::MakeInvalid();
    }
  }

  CFAbsoluteTime absTime;
  if (CFCalendarComposeAbsoluteTime(calendar, &absTime, "yMdHms", year, month, day, hour, minute, second) == FALSE)
  {
    return WTimestamp::MakeInvalid();
  }

  return WTimestamp::MakeFromInt(static_cast<WInt64>((absTime + kCFAbsoluteTimeIntervalSince1970) * 1000000.0), WSIUnitOfTime::Microsecond);
}

WResult WDateTime::SetFromTimestamp(WTimestamp timestamp)
{
  // Round the microseconds to the full second so that we can reconstruct the right date / time afterwards
  WInt64 us = timestamp.GetInt64(WSIUnitOfTime::Microsecond);
  WInt64 microseconds = us % (1000 * 1000);

  CFAbsoluteTime at = (static_cast<CFAbsoluteTime>((us - microseconds) / 1000000.0)) - kCFAbsoluteTimeIntervalSince1970;

  WScopedCFRef<CFTimeZoneRef> timezone(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  WScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timezone);

  int year, month, day, dayOfWeek, hour, minute, second;

  if (CFCalendarDecomposeAbsoluteTime(calendar, at, "yMdHmsE", &year, &month, &day, &hour, &minute, &second, &dayOfWeek) == FALSE)
  {
    return W_FAILURE;
  }

  m_iYear = (WInt16)year;
  m_uiMonth = (WUInt8)month;
  m_uiDay = (WUInt8)day;
  m_uiDayOfWeek = (WUInt8)(dayOfWeek - 1);
  m_uiHour = (WUInt8)hour;
  m_uiMinute = (WUInt8)minute;
  m_uiSecond = (WUInt8)second;
  m_uiMicroseconds = (WUInt32)microseconds;
  return W_SUCCESS;
}

#endif
