#pragma once

W_ALWAYS_INLINE WSimdDouble::WSimdDouble() {}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(float f)
{
  m_v.Set((double)f);
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(double f)
{
  m_v.Set(f);
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInt32 i)
{
  m_v.Set((double)i);
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WUInt32 i)
{
  m_v.Set((double)i);
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WAngle a)
{
  m_v.Set(a.GetRadian());
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInternal::QuadFloat v)
{
  m_v.Set(v.x);
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInternal::QuadDouble v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdDouble::operator double() const
{
  return m_v.x;
}

// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeZero()
{
  return WSimdDouble(0.0);
}

// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeNaN()
{
  return WSimdDouble(WMath::NaN<double>());
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator+(const WSimdDouble& f) const
{
  return m_v + f.m_v;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator-(const WSimdDouble& f) const
{
  return m_v - f.m_v;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator*(const WSimdDouble& f) const
{
  return m_v.CompMul(f.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator/(const WSimdDouble& f) const
{
  return m_v.CompDiv(f.m_v);
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator+=(const WSimdDouble& f)
{
  m_v += f.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator-=(const WSimdDouble& f)
{
  m_v -= f.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator*=(const WSimdDouble& f)
{
  m_v = m_v.CompMul(f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator/=(const WSimdDouble& f)
{
  m_v = m_v.CompDiv(f.m_v);
  return *this;
}

W_ALWAYS_INLINE bool WSimdDouble::IsEqual(const WSimdDouble& rhs, const WSimdDouble& dEpsilon) const
{
  return m_v.IsEqual(rhs.m_v, dEpsilon);
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(const WSimdDouble& f) const
{
  return m_v.x == f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(const WSimdDouble& f) const
{
  return m_v.x != f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(const WSimdDouble& f) const
{
  return m_v.x >= f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(const WSimdDouble& f) const
{
  return m_v.x > f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(const WSimdDouble& f) const
{
  return m_v.x <= f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(const WSimdDouble& f) const
{
  return m_v.x < f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(double f) const
{
  return m_v.x == f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(double f) const
{
  return m_v.x != f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(double f) const
{
  return m_v.x > f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(double f) const
{
  return m_v.x >= f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(double f) const
{
  return m_v.x < f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(double f) const
{
  return m_v.x <= f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(float f) const
{
  return m_v.x == (double)f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(float f) const
{
  return m_v.x != (double)f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(float f) const
{
  return m_v.x > (double)f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(float f) const
{
  return m_v.x >= (double)f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(float f) const
{
  return m_v.x < (double)f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(float f) const
{
  return m_v.x <= (double)f;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetReciprocal() const
{
  return WSimdDouble(1.0 / m_v.x);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetSqrt() const
{
  return WSimdDouble(WMath::Sqrt(m_v.x));
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetInvSqrt() const
{
  return WSimdDouble(1.0 / WMath::Sqrt(m_v.x));
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Max(const WSimdDouble& f) const
{
  return m_v.CompMax(f.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Min(const WSimdDouble& f) const
{
  return m_v.CompMin(f.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Abs() const
{
  return WSimdDouble(WMath::Abs(m_v.x));
}
