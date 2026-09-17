#pragma once

W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d() = default;

W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d(double xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d(float xyzw)
{
  m_v.Set(double(xyzw));
}

W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d(const WSimdDouble& xyzw)
{
  m_v = xyzw.m_v;
}

W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d(double x, double y, double z, double w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d(int x, int y, int z, int w)
{
  m_v.Set(double(x), double(y), double(z), double(w));
}

W_ALWAYS_INLINE void WSimdVec4d::Set(double xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE void WSimdVec4d::Set(float xyzw)
{
  m_v.Set(double(xyzw));
}


W_ALWAYS_INLINE void WSimdVec4d::Set(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4d::Set(double x, double y, double z, double w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4d::Set(int x, int y, int z, int w)
{
  m_v.Set(double(x), double(y), double(z), double(w));
}

W_ALWAYS_INLINE void WSimdVec4d::SetX(const WSimdDouble& f)
{
  m_v.x = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4d::SetY(const WSimdDouble& f)
{
  m_v.y = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4d::SetZ(const WSimdDouble& f)
{
  m_v.z = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4d::SetW(const WSimdDouble& f)
{
  m_v.w = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4d::SetZero()
{
  m_v.SetZero();
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4d::Load(const double* pDoubles)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pDoubles[i];
  }
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4d::Store(double* pDoubles) const
{
  for (int i = 0; i < N; ++i)
  {
    pDoubles[i] = (&m_v.x)[i];
  }
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4d::Load(const float* pFloats)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = static_cast<double>(pFloats[i]);
  }
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4d::Store(float* pFloats) const
{
  for (int i = 0; i < N; ++i)
  {
    pFloats[i] = static_cast<float>((&m_v.x)[i]);
  }
}


W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::GetReciprocal() const
{
  return WVec4d(1.0).CompDiv(m_v);
}


W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::GetSqrt() const
{
  WSimdVec4d result;
  result.m_v.x = WMath::Sqrt(m_v.x);
  result.m_v.y = WMath::Sqrt(m_v.y);
  result.m_v.z = WMath::Sqrt(m_v.z);
  result.m_v.w = WMath::Sqrt(m_v.w);

  return result;
}


W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::GetInvSqrt() const
{
  WSimdVec4d result;
  result.m_v.x = 1.0 / WMath::Sqrt(m_v.x);
  result.m_v.y = 1.0 / WMath::Sqrt(m_v.y);
  result.m_v.z = 1.0 / WMath::Sqrt(m_v.z);
  result.m_v.w = 1.0 / WMath::Sqrt(m_v.w);

  return result;
}

template <int N>
void WSimdVec4d::NormalizeIfNotZero(const WSimdDouble& fEpsilon)
{
  WSimdDouble sqLength = GetLengthSquared<N>();
  m_v *= sqLength.GetInvSqrt();
  m_v = sqLength > fEpsilon.m_v ? m_v : WVec4d::MakeZero();
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4d::IsZero() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i] != 0.0)
      return false;
  }

  return true;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4d::IsZero(const WSimdDouble& fEpsilon) const
{
  for (int i = 0; i < N; ++i)
  {
    if (!WMath::IsZero((&m_v.x)[i], (double)fEpsilon))
      return false;
  }

  return true;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4d::IsNaN() const
{
  for (int i = 0; i < N; ++i)
  {
    if (WMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4d::IsValid() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!WMath::IsFinite((&m_v.x)[i]))
      return false;
  }

  return true;
}

template <int N>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::GetComponent() const
{
  if constexpr (N == 0)
  {
    return m_v.x;
  }
  else if constexpr (N == 1)
  {
    return m_v.y;
  }
  else if constexpr (N == 2)
  {
    return m_v.z;
  }
  else if constexpr (N == 3)
  {
    return m_v.w;
  }
  else
  {
    return m_v.w;
  }
}

W_ALWAYS_INLINE WSimdDouble WSimdVec4d::x() const
{
  return m_v.x;
}

W_ALWAYS_INLINE WSimdDouble WSimdVec4d::y() const
{
  return m_v.y;
}

W_ALWAYS_INLINE WSimdDouble WSimdVec4d::z() const
{
  return m_v.z;
}

W_ALWAYS_INLINE WSimdDouble WSimdVec4d::w() const
{
  return m_v.w;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Get() const
{
  WSimdVec4d result;

  const double* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::GetCombined(const WSimdVec4d& other) const
{
  WSimdVec4d result;

  const double* v = &m_v.x;
  const double* o = &other.m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = o[(s & 0x0030) >> 4];
  result.m_v.w = o[(s & 0x0003)];

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::operator-() const
{
  return -m_v;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::operator+(const WSimdVec4d& v) const
{
  return m_v + v.m_v;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::operator-(const WSimdVec4d& v) const
{
  return m_v - v.m_v;
}


W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::operator*(const WSimdDouble& f) const
{
  return m_v * f.m_v.x;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::operator/(const WSimdDouble& f) const
{
  return m_v / f.m_v.x;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::CompMul(const WSimdVec4d& v) const
{
  return m_v.CompMul(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::CompDiv(const WSimdVec4d& v) const
{
  return m_v.CompDiv(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::CompMin(const WSimdVec4d& v) const
{
  return m_v.CompMin(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::CompMax(const WSimdVec4d& v) const
{
  return m_v.CompMax(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Abs() const
{
  return m_v.Abs();
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Round() const
{
  WSimdVec4d result;
  result.m_v.x = WMath::Round(m_v.x);
  result.m_v.y = WMath::Round(m_v.y);
  result.m_v.z = WMath::Round(m_v.z);
  result.m_v.w = WMath::Round(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Floor() const
{
  WSimdVec4d result;
  result.m_v.x = WMath::Floor(m_v.x);
  result.m_v.y = WMath::Floor(m_v.y);
  result.m_v.z = WMath::Floor(m_v.z);
  result.m_v.w = WMath::Floor(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Ceil() const
{
  WSimdVec4d result;
  result.m_v.x = WMath::Ceil(m_v.x);
  result.m_v.y = WMath::Ceil(m_v.y);
  result.m_v.z = WMath::Ceil(m_v.z);
  result.m_v.w = WMath::Ceil(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Trunc() const
{
  WSimdVec4d result;
  result.m_v.x = WMath::Trunc(m_v.x);
  result.m_v.y = WMath::Trunc(m_v.y);
  result.m_v.z = WMath::Trunc(m_v.z);
  result.m_v.w = WMath::Trunc(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::FlipSign(const WSimdVec4bWide& cmp) const
{
  WSimdVec4d result;
  result.m_v.x = cmp.m_v.x ? -m_v.x : m_v.x;
  result.m_v.y = cmp.m_v.y ? -m_v.y : m_v.y;
  result.m_v.z = cmp.m_v.z ? -m_v.z : m_v.z;
  result.m_v.w = cmp.m_v.w ? -m_v.w : m_v.w;

  return result;
}

// static
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Select(const WSimdVec4bWide& cmp, const WSimdVec4d& ifTrue, const WSimdVec4d& ifFalse)
{
  WSimdVec4d result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4d& WSimdVec4d::operator+=(const WSimdVec4d& v)
{
  m_v += v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4d& WSimdVec4d::operator-=(const WSimdVec4d& v)
{
  m_v -= v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4d& WSimdVec4d::operator*=(const WSimdDouble& f)
{
  m_v *= f.m_v.x;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4d& WSimdVec4d::operator/=(const WSimdDouble& f)
{
  m_v /= f.m_v.x;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4d::operator==(const WSimdVec4d& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return WSimdVec4bWide(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4d::operator!=(const WSimdVec4d& v) const
{
  return !(*this == v);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4d::operator<=(const WSimdVec4d& v) const
{
  return !(*this > v);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4d::operator<(const WSimdVec4d& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return WSimdVec4bWide(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4d::operator>=(const WSimdVec4d& v) const
{
  return !(*this < v);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4d::operator>(const WSimdVec4d& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return WSimdVec4bWide(result[0], result[1], result[2], result[3]);
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalSum<2>() const
{
  return m_v.x + m_v.y;
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalSum<3>() const
{
  return (double)HorizontalSum<2>() + m_v.z;
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalSum<4>() const
{
  return (double)HorizontalSum<3>() + m_v.w;
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMin<2>() const
{
  return WMath::Min(m_v.x, m_v.y);
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMin<3>() const
{
  return WMath::Min((double)HorizontalMin<2>(), m_v.z);
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMin<4>() const
{
  return WMath::Min((double)HorizontalMin<3>(), m_v.w);
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMax<2>() const
{
  return WMath::Max(m_v.x, m_v.y);
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMax<3>() const
{
  return WMath::Max((double)HorizontalMax<2>(), m_v.z);
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMax<4>() const
{
  return WMath::Max((double)HorizontalMax<3>(), m_v.w);
}

template <int N>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::Dot(const WSimdVec4d& v) const
{
  double result = 0.0;

  for (int i = 0; i < N; ++i)
  {
    result += (&m_v.x)[i] * (&v.m_v.x)[i];
  }

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::CrossRH(const WSimdVec4d& v) const
{
  return m_v.GetAsVec3().CrossRH(v.m_v.GetAsVec3()).GetAsVec4(0.0);
}

// static
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::MulAdd(const WSimdVec4d& a, const WSimdVec4d& b, const WSimdVec4d& c)
{
  return a.CompMul(b) + c;
}

// static
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::MulAdd(const WSimdVec4d& a, const WSimdDouble& b, const WSimdVec4d& c)
{
  return a * b + c;
}

// static
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::MulSub(const WSimdVec4d& a, const WSimdVec4d& b, const WSimdVec4d& c)
{
  return a.CompMul(b) - c;
}

// static
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::MulSub(const WSimdVec4d& a, const WSimdDouble& b, const WSimdVec4d& c)
{
  return a * b - c;
}

// static
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::CopySign(const WSimdVec4d& magnitude, const WSimdVec4d& sign)
{
  WSimdVec4d result;
  // Use std::copysign to properly handle -0.0
  result.m_v.x = std::copysign(magnitude.m_v.x, sign.m_v.x);
  result.m_v.y = std::copysign(magnitude.m_v.y, sign.m_v.y);
  result.m_v.z = std::copysign(magnitude.m_v.z, sign.m_v.z);
  result.m_v.w = std::copysign(magnitude.m_v.w, sign.m_v.w);

  return result;
}
