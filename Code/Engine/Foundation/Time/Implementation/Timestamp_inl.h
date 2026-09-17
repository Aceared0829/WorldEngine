#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>

inline WTimestamp::WTimestamp() = default;

inline bool WTimestamp::IsValid() const
{
  return m_iTimestamp != W_INVALID_TIME_STAMP;
}

inline void WTimestamp::operator+=(const WTime& timeSpan)
{
  W_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp += (WInt64)timeSpan.GetMicroseconds();
}

inline void WTimestamp::operator-=(const WTime& timeSpan)
{
  W_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp -= (WInt64)timeSpan.GetMicroseconds();
}

inline const WTime WTimestamp::operator-(const WTimestamp& other) const
{
  W_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  W_ASSERT_DEBUG(other.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return WTime::MakeFromMicroseconds((double)(m_iTimestamp - other.m_iTimestamp));
}

inline const WTimestamp WTimestamp::operator+(const WTime& timeSpan) const
{
  W_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return WTimestamp::MakeFromInt(m_iTimestamp + (WInt64)timeSpan.GetMicroseconds(), WSIUnitOfTime::Microsecond);
}

inline const WTimestamp WTimestamp::operator-(const WTime& timeSpan) const
{
  W_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return WTimestamp::MakeFromInt(m_iTimestamp - (WInt64)timeSpan.GetMicroseconds(), WSIUnitOfTime::Microsecond);
}

inline const WTimestamp operator+(const WTime& timeSpan, const WTimestamp& timestamp)
{
  W_ASSERT_DEBUG(timestamp.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return WTimestamp::MakeFromInt(timestamp.GetInt64(WSIUnitOfTime::Microsecond) + (WInt64)timeSpan.GetMicroseconds(), WSIUnitOfTime::Microsecond);
}



inline WUInt32 WDateTime::GetYear() const
{
  return m_iYear;
}

inline void WDateTime::SetYear(WInt16 iYear)
{
  m_iYear = iYear;
}

inline WUInt8 WDateTime::GetMonth() const
{
  return m_uiMonth;
}

inline void WDateTime::SetMonth(WUInt8 uiMonth)
{
  W_ASSERT_DEBUG(uiMonth >= 1 && uiMonth <= 12, "Invalid month value");
  m_uiMonth = uiMonth;
}

inline WUInt8 WDateTime::GetDay() const
{
  return m_uiDay;
}

inline void WDateTime::SetDay(WUInt8 uiDay)
{
  W_ASSERT_DEBUG(uiDay >= 1 && uiDay <= 31, "Invalid day value");
  m_uiDay = uiDay;
}

inline WUInt8 WDateTime::GetDayOfWeek() const
{
  return m_uiDayOfWeek;
}

inline void WDateTime::SetDayOfWeek(WUInt8 uiDayOfWeek)
{
  W_ASSERT_DEBUG(uiDayOfWeek <= 6, "Invalid day of week value");
  m_uiDayOfWeek = uiDayOfWeek;
}

inline WUInt8 WDateTime::GetHour() const
{
  return m_uiHour;
}

inline void WDateTime::SetHour(WUInt8 uiHour)
{
  W_ASSERT_DEBUG(uiHour <= 23, "Invalid hour value");
  m_uiHour = uiHour;
}

inline WUInt8 WDateTime::GetMinute() const
{
  return m_uiMinute;
}

inline void WDateTime::SetMinute(WUInt8 uiMinute)
{
  W_ASSERT_DEBUG(uiMinute <= 59, "Invalid minute value");
  m_uiMinute = uiMinute;
}

inline WUInt8 WDateTime::GetSecond() const
{
  return m_uiSecond;
}

inline void WDateTime::SetSecond(WUInt8 uiSecond)
{
  W_ASSERT_DEBUG(uiSecond <= 59, "Invalid second value");
  m_uiSecond = uiSecond;
}

inline WUInt32 WDateTime::GetMicroseconds() const
{
  return m_uiMicroseconds;
}

inline void WDateTime::SetMicroseconds(WUInt32 uiMicroSeconds)
{
  W_ASSERT_DEBUG(uiMicroSeconds <= 999999u, "Invalid micro-second value");
  m_uiMicroseconds = uiMicroSeconds;
}
