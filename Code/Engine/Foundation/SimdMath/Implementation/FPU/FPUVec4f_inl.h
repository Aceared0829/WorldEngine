#pragma once

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f() = default;

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(float xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(const WSimdFloat& xyzw)
{
  m_v = xyzw.m_v;
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4f::Set(float xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE void WSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4f::SetX(const WSimdFloat& f)
{
  m_v.x = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4f::SetY(const WSimdFloat& f)
{
  m_v.y = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4f::SetZ(const WSimdFloat& f)
{
  m_v.z = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4f::SetW(const WSimdFloat& f)
{
  m_v.w = f.m_v.x;
}

W_ALWAYS_INLINE void WSimdVec4f::SetZero()
{
  m_v.SetZero();
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4f::Load(const float* pFloats)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pFloats[i];
  }
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4f::Store(float* pFloats) const
{
  for (int i = 0; i < N; ++i)
  {
    pFloats[i] = (&m_v.x)[i];
  }
}

template <WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetReciprocal() const
{
  return WVec4(1.0f).CompDiv(m_v);
}

template <WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetSqrt() const
{
  WSimdVec4f result;
  result.m_v.x = WMath::Sqrt(m_v.x);
  result.m_v.y = WMath::Sqrt(m_v.y);
  result.m_v.z = WMath::Sqrt(m_v.z);
  result.m_v.w = WMath::Sqrt(m_v.w);

  return result;
}

template <WMathAcc::Enum acc>
WSimdVec4f WSimdVec4f::GetInvSqrt() const
{
  WSimdVec4f result;
  result.m_v.x = 1.0f / WMath::Sqrt(m_v.x);
  result.m_v.y = 1.0f / WMath::Sqrt(m_v.y);
  result.m_v.z = 1.0f / WMath::Sqrt(m_v.z);
  result.m_v.w = 1.0f / WMath::Sqrt(m_v.w);

  return result;
}

template <int N, WMathAcc::Enum acc>
void WSimdVec4f::NormalizeIfNotZero(const WSimdFloat& fEpsilon)
{
  WSimdFloat sqLength = GetLengthSquared<N>();
  m_v *= sqLength.GetInvSqrt<acc>();
  m_v = sqLength > fEpsilon.m_v ? m_v : WVec4::MakeZero();
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsZero() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i] != 0.0f)
      return false;
  }

  return true;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsZero(const WSimdFloat& fEpsilon) const
{
  for (int i = 0; i < N; ++i)
  {
    if (!WMath::IsZero((&m_v.x)[i], (float)fEpsilon))
      return false;
  }

  return true;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsNaN() const
{
  for (int i = 0; i < N; ++i)
  {
    if (WMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsValid() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!WMath::IsFinite((&m_v.x)[i]))
      return false;
  }

  return true;
}

template <int N>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::GetComponent() const
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

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::x() const
{
  return m_v.x;
}

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::y() const
{
  return m_v.y;
}

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::z() const
{
  return m_v.z;
}

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::w() const
{
  return m_v.w;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Get() const
{
  WSimdVec4f result;

  const float* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetCombined(const WSimdVec4f& other) const
{
  WSimdVec4f result;

  const float* v = &m_v.x;
  const float* o = &other.m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = o[(s & 0x0030) >> 4];
  result.m_v.w = o[(s & 0x0003)];

  return result;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator-() const
{
  return -m_v;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator+(const WSimdVec4f& v) const
{
  return m_v + v.m_v;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator-(const WSimdVec4f& v) const
{
  return m_v - v.m_v;
}


W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator*(const WSimdFloat& f) const
{
  return m_v * f.m_v.x;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator/(const WSimdFloat& f) const
{
  return m_v / f.m_v.x;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMul(const WSimdVec4f& v) const
{
  return m_v.CompMul(v.m_v);
}

template <WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompDiv(const WSimdVec4f& v) const
{
  return m_v.CompDiv(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMin(const WSimdVec4f& v) const
{
  return m_v.CompMin(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMax(const WSimdVec4f& v) const
{
  return m_v.CompMax(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Abs() const
{
  return m_v.Abs();
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Round() const
{
  WSimdVec4f result;
  result.m_v.x = WMath::Round(m_v.x);
  result.m_v.y = WMath::Round(m_v.y);
  result.m_v.z = WMath::Round(m_v.z);
  result.m_v.w = WMath::Round(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Floor() const
{
  WSimdVec4f result;
  result.m_v.x = WMath::Floor(m_v.x);
  result.m_v.y = WMath::Floor(m_v.y);
  result.m_v.z = WMath::Floor(m_v.z);
  result.m_v.w = WMath::Floor(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Ceil() const
{
  WSimdVec4f result;
  result.m_v.x = WMath::Ceil(m_v.x);
  result.m_v.y = WMath::Ceil(m_v.y);
  result.m_v.z = WMath::Ceil(m_v.z);
  result.m_v.w = WMath::Ceil(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Trunc() const
{
  WSimdVec4f result;
  result.m_v.x = WMath::Trunc(m_v.x);
  result.m_v.y = WMath::Trunc(m_v.y);
  result.m_v.z = WMath::Trunc(m_v.z);
  result.m_v.w = WMath::Trunc(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::FlipSign(const WSimdVec4b& cmp) const
{
  WSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? -m_v.x : m_v.x;
  result.m_v.y = cmp.m_v.y ? -m_v.y : m_v.y;
  result.m_v.z = cmp.m_v.z ? -m_v.z : m_v.z;
  result.m_v.w = cmp.m_v.w ? -m_v.w : m_v.w;

  return result;
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Select(const WSimdVec4b& cmp, const WSimdVec4f& ifTrue, const WSimdVec4f& ifFalse)
{
  WSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator+=(const WSimdVec4f& v)
{
  m_v += v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator-=(const WSimdVec4f& v)
{
  m_v -= v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator*=(const WSimdFloat& f)
{
  m_v *= f.m_v.x;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator/=(const WSimdFloat& f)
{
  m_v /= f.m_v.x;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator==(const WSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator!=(const WSimdVec4f& v) const
{
  return !(*this == v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator<=(const WSimdVec4f& v) const
{
  return !(*this > v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator<(const WSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator>=(const WSimdVec4f& v) const
{
  return !(*this < v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator>(const WSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<2>() const
{
  return m_v.x + m_v.y;
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<3>() const
{
  return (float)HorizontalSum<2>() + m_v.z;
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<4>() const
{
  return (float)HorizontalSum<3>() + m_v.w;
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<2>() const
{
  return WMath::Min(m_v.x, m_v.y);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<3>() const
{
  return WMath::Min((float)HorizontalMin<2>(), m_v.z);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<4>() const
{
  return WMath::Min((float)HorizontalMin<3>(), m_v.w);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<2>() const
{
  return WMath::Max(m_v.x, m_v.y);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<3>() const
{
  return WMath::Max((float)HorizontalMax<2>(), m_v.z);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<4>() const
{
  return WMath::Max((float)HorizontalMax<3>(), m_v.w);
}

template <int N>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot(const WSimdVec4f& v) const
{
  float result = 0.0f;

  for (int i = 0; i < N; ++i)
  {
    result += (&m_v.x)[i] * (&v.m_v.x)[i];
  }

  return result;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CrossRH(const WSimdVec4f& v) const
{
  return m_v.GetAsVec3().CrossRH(v.m_v.GetAsVec3()).GetAsVec4(0.0f);
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulAdd(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c)
{
  return a.CompMul(b) + c;
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulAdd(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c)
{
  return a * b + c;
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulSub(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c)
{
  return a.CompMul(b) - c;
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulSub(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c)
{
  return a * b - c;
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CopySign(const WSimdVec4f& magnitude, const WSimdVec4f& sign)
{
  WSimdVec4f result;
  result.m_v.x = sign.m_v.x < 0.0f ? -magnitude.m_v.x : magnitude.m_v.x;
  result.m_v.y = sign.m_v.y < 0.0f ? -magnitude.m_v.y : magnitude.m_v.y;
  result.m_v.z = sign.m_v.z < 0.0f ? -magnitude.m_v.z : magnitude.m_v.z;
  result.m_v.w = sign.m_v.w < 0.0f ? -magnitude.m_v.w : magnitude.m_v.w;

  return result;
}
