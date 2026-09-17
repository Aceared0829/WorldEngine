#pragma once

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  m_v.Set(0xCDCDCDCD);
#endif
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInt32 xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInt32 x, WInt32 y, WInt32 z, WInt32 w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInternal::QuadInt v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::MakeZero()
{
  return WSimdVec4i(0);
}

W_ALWAYS_INLINE void WSimdVec4i::Set(WInt32 xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE void WSimdVec4i::Set(WInt32 x, WInt32 y, WInt32 z, WInt32 w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4i::SetZero()
{
  m_v.SetZero();
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4i::Load(const WInt32* pInts)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pInts[i];
  }
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4i::Store(WInt32* pInts) const
{
  for (int i = 0; i < N; ++i)
  {
    pInts[i] = (&m_v.x)[i];
  }
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4i::ToFloat() const
{
  WSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Truncate(const WSimdVec4f& f)
{
  WSimdVec4i result;
  result.m_v.x = (WInt32)f.m_v.x;
  result.m_v.y = (WInt32)f.m_v.y;
  result.m_v.z = (WInt32)f.m_v.z;
  result.m_v.w = (WInt32)f.m_v.w;

  return result;
}

template <int N>
W_ALWAYS_INLINE WInt32 WSimdVec4i::GetComponent() const
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

W_ALWAYS_INLINE WInt32 WSimdVec4i::x() const
{
  return m_v.x;
}

W_ALWAYS_INLINE WInt32 WSimdVec4i::y() const
{
  return m_v.y;
}

W_ALWAYS_INLINE WInt32 WSimdVec4i::z() const
{
  return m_v.z;
}

W_ALWAYS_INLINE WInt32 WSimdVec4i::w() const
{
  return m_v.w;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Get() const
{
  WSimdVec4i result;

  const WInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::GetCombined(const WSimdVec4i& other) const
{
  WSimdVec4i result;

  const WInt32* v = &m_v.x;
  const WInt32* o = &other.m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = o[(s & 0x0030) >> 4];
  result.m_v.w = o[(s & 0x0003)];

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator-() const
{
  return -m_v;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator+(const WSimdVec4i& v) const
{
  return m_v + v.m_v;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator-(const WSimdVec4i& v) const
{
  return m_v - v.m_v;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMul(const WSimdVec4i& v) const
{
  return m_v.CompMul(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompDiv(const WSimdVec4i& v) const
{
  return m_v.CompDiv(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator|(const WSimdVec4i& v) const
{
  WSimdVec4i result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator&(const WSimdVec4i& v) const
{
  WSimdVec4i result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator^(const WSimdVec4i& v) const
{
  WSimdVec4i result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator~() const
{
  WSimdVec4i result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator<<(WUInt32 uiShift) const
{
  WSimdVec4i result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator>>(WUInt32 uiShift) const
{
  WSimdVec4i result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator<<(const WSimdVec4i& v) const
{
  WSimdVec4i result;
  result.m_v.x = m_v.x << v.m_v.x;
  result.m_v.y = m_v.y << v.m_v.y;
  result.m_v.z = m_v.z << v.m_v.z;
  result.m_v.w = m_v.w << v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator>>(const WSimdVec4i& v) const
{
  WSimdVec4i result;
  result.m_v.x = m_v.x >> v.m_v.x;
  result.m_v.y = m_v.y >> v.m_v.y;
  result.m_v.z = m_v.z >> v.m_v.z;
  result.m_v.w = m_v.w >> v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator+=(const WSimdVec4i& v)
{
  m_v += v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator-=(const WSimdVec4i& v)
{
  m_v -= v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator|=(const WSimdVec4i& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator&=(const WSimdVec4i& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator^=(const WSimdVec4i& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator<<=(WUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator>>=(WUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMin(const WSimdVec4i& v) const
{
  return m_v.CompMin(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMax(const WSimdVec4i& v) const
{
  return m_v.CompMax(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Abs() const
{
  WSimdVec4i result;
  result.m_v.x = WMath::Abs(m_v.x);
  result.m_v.y = WMath::Abs(m_v.y);
  result.m_v.z = WMath::Abs(m_v.z);
  result.m_v.w = WMath::Abs(m_v.w);

  return result;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator==(const WSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator!=(const WSimdVec4i& v) const
{
  return !(*this == v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator<=(const WSimdVec4i& v) const
{
  return !(*this > v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator<(const WSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator>=(const WSimdVec4i& v) const
{
  return !(*this < v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator>(const WSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

// static
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Select(const WSimdVec4b& cmp, const WSimdVec4i& ifTrue, const WSimdVec4i& ifFalse)
{
  WSimdVec4i result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}
