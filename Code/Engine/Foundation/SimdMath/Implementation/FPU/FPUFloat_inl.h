#pragma once

W_ALWAYS_INLINE WSimdFloat::WSimdFloat() {}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(float f)
{
  m_v.Set(f);
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WInt32 i)
{
  m_v.Set((float)i);
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WUInt32 i)
{
  m_v.Set((float)i);
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WAngle a)
{
  m_v.Set(a.GetRadian());
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WInternal::QuadFloat v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdFloat::operator float() const
{
  return m_v.x;
}

// static
W_ALWAYS_INLINE WSimdFloat WSimdFloat::MakeZero()
{
  return WSimdFloat(0.0f);
}

// static
W_ALWAYS_INLINE WSimdFloat WSimdFloat::MakeNaN()
{
  return WSimdFloat(WMath::NaN<float>());
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator+(const WSimdFloat& f) const
{
  return m_v + f.m_v;
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator-(const WSimdFloat& f) const
{
  return m_v - f.m_v;
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator*(const WSimdFloat& f) const
{
  return m_v.CompMul(f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator/(const WSimdFloat& f) const
{
  return m_v.CompDiv(f.m_v);
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator+=(const WSimdFloat& f)
{
  m_v += f.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator-=(const WSimdFloat& f)
{
  m_v -= f.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator*=(const WSimdFloat& f)
{
  m_v = m_v.CompMul(f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator/=(const WSimdFloat& f)
{
  m_v = m_v.CompDiv(f.m_v);
  return *this;
}

W_ALWAYS_INLINE bool WSimdFloat::IsEqual(const WSimdFloat& rhs, const WSimdFloat& fEpsilon) const
{
  return m_v.IsEqual(rhs.m_v, fEpsilon);
}

W_ALWAYS_INLINE bool WSimdFloat::operator==(const WSimdFloat& f) const
{
  return m_v.x == f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdFloat::operator!=(const WSimdFloat& f) const
{
  return m_v.x != f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdFloat::operator>=(const WSimdFloat& f) const
{
  return m_v.x >= f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdFloat::operator>(const WSimdFloat& f) const
{
  return m_v.x > f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdFloat::operator<=(const WSimdFloat& f) const
{
  return m_v.x <= f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdFloat::operator<(const WSimdFloat& f) const
{
  return m_v.x < f.m_v.x;
}

W_ALWAYS_INLINE bool WSimdFloat::operator==(float f) const
{
  return m_v.x == f;
}

W_ALWAYS_INLINE bool WSimdFloat::operator!=(float f) const
{
  return m_v.x != f;
}

W_ALWAYS_INLINE bool WSimdFloat::operator>(float f) const
{
  return m_v.x > f;
}

W_ALWAYS_INLINE bool WSimdFloat::operator>=(float f) const
{
  return m_v.x >= f;
}

W_ALWAYS_INLINE bool WSimdFloat::operator<(float f) const
{
  return m_v.x < f;
}

W_ALWAYS_INLINE bool WSimdFloat::operator<=(float f) const
{
  return m_v.x <= f;
}

template <WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetReciprocal() const
{
  return WSimdFloat(1.0f / m_v.x);
}

template <WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetSqrt() const
{
  return WSimdFloat(WMath::Sqrt(m_v.x));
}

template <WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetInvSqrt() const
{
  return WSimdFloat(1.0f / WMath::Sqrt(m_v.x));
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::Max(const WSimdFloat& f) const
{
  return m_v.CompMax(f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::Min(const WSimdFloat& f) const
{
  return m_v.CompMin(f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::Abs() const
{
  return WSimdFloat(WMath::Abs(m_v.x));
}
