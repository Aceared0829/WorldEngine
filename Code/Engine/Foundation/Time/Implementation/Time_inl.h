#pragma once

#include <Foundation/Basics.h>

constexpr W_ALWAYS_INLINE WTime::WTime(double fTime)
  : m_fTime(fTime)
{
}

constexpr W_ALWAYS_INLINE float WTime::AsFloatInSeconds() const
{
  return static_cast<float>(m_fTime);
}

constexpr W_ALWAYS_INLINE double WTime::GetNanoseconds() const
{
  return m_fTime * 1000000000.0;
}

constexpr W_ALWAYS_INLINE double WTime::GetMicroseconds() const
{
  return m_fTime * 1000000.0;
}

constexpr W_ALWAYS_INLINE double WTime::GetMilliseconds() const
{
  return m_fTime * 1000.0;
}

constexpr W_ALWAYS_INLINE double WTime::GetSeconds() const
{
  return m_fTime;
}

constexpr W_ALWAYS_INLINE double WTime::GetMinutes() const
{
  return m_fTime / 60.0;
}

constexpr W_ALWAYS_INLINE double WTime::GetHours() const
{
  return m_fTime / (60.0 * 60.0);
}

constexpr W_ALWAYS_INLINE void WTime::operator-=(const WTime& other)
{
  m_fTime -= other.m_fTime;
}

constexpr W_ALWAYS_INLINE void WTime::operator+=(const WTime& other)
{
  m_fTime += other.m_fTime;
}

constexpr W_ALWAYS_INLINE void WTime::operator*=(double fFactor)
{
  m_fTime *= fFactor;
}

constexpr W_ALWAYS_INLINE void WTime::operator/=(double fFactor)
{
  m_fTime /= fFactor;
}

constexpr W_ALWAYS_INLINE WTime WTime::operator-() const
{
  return WTime(-m_fTime);
}

constexpr W_ALWAYS_INLINE WTime WTime::operator-(const WTime& other) const
{
  return WTime(m_fTime - other.m_fTime);
}

constexpr W_ALWAYS_INLINE WTime WTime::operator+(const WTime& other) const
{
  return WTime(m_fTime + other.m_fTime);
}

constexpr W_ALWAYS_INLINE WTime operator*(const WTime& t, double f)
{
  return WTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr W_ALWAYS_INLINE WTime operator*(double f, const WTime& t)
{
  return WTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr W_ALWAYS_INLINE WTime operator*(const WTime& f, const WTime& t)
{
  return WTime::MakeFromSeconds(t.GetSeconds() * f.GetSeconds());
}

constexpr W_ALWAYS_INLINE WTime operator/(const WTime& t, double f)
{
  return WTime::MakeFromSeconds(t.GetSeconds() / f);
}

constexpr W_ALWAYS_INLINE WTime operator/(double f, const WTime& t)
{
  return WTime::MakeFromSeconds(f / t.GetSeconds());
}

constexpr W_ALWAYS_INLINE WTime operator/(const WTime& f, const WTime& t)
{
  return WTime::MakeFromSeconds(f.GetSeconds() / t.GetSeconds());
}
