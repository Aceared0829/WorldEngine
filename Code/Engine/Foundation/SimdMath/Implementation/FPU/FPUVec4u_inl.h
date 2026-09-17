#pragma once

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  m_v.Set(0xCDCDCDCD);
#endif
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WUInt32 xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WInternal::QuadUInt v)
{
  m_v = v;
}

W_ALWAYS_INLINE void WSimdVec4u::Set(WUInt32 xyzw)
{
  m_v.Set(xyzw);
}

W_ALWAYS_INLINE void WSimdVec4u::Set(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w)
{
  m_v.Set(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4u::SetZero()
{
  m_v.SetZero();
}

// needs to be implemented here because of include dependencies
W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(const WSimdVec4u& u)
  : m_v(u.m_v.x, u.m_v.y, u.m_v.z, u.m_v.w)
{
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(const WSimdVec4i& i)
  : m_v(i.m_v.x, i.m_v.y, i.m_v.z, i.m_v.w)
{
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4u::ToFloat() const
{
  WSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::Truncate(const WSimdVec4f& f)
{
  WSimdVec4f clampedF = f.CompMax(WSimdVec4f::MakeZero());

  WSimdVec4u result;
  result.m_v.x = (WUInt32)clampedF.m_v.x;
  result.m_v.y = (WUInt32)clampedF.m_v.y;
  result.m_v.z = (WUInt32)clampedF.m_v.z;
  result.m_v.w = (WUInt32)clampedF.m_v.w;

  return result;
}

template <int N>
W_ALWAYS_INLINE WUInt32 WSimdVec4u::GetComponent() const
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

W_ALWAYS_INLINE WUInt32 WSimdVec4u::x() const
{
  return m_v.x;
}

W_ALWAYS_INLINE WUInt32 WSimdVec4u::y() const
{
  return m_v.y;
}

W_ALWAYS_INLINE WUInt32 WSimdVec4u::z() const
{
  return m_v.z;
}

W_ALWAYS_INLINE WUInt32 WSimdVec4u::w() const
{
  return m_v.w;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::Get() const
{
  WSimdVec4u result;

  const WUInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator+(const WSimdVec4u& v) const
{
  return m_v + v.m_v;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator-(const WSimdVec4u& v) const
{
  return m_v - v.m_v;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMul(const WSimdVec4u& v) const
{
  return m_v.CompMul(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator|(const WSimdVec4u& v) const
{
  WSimdVec4u result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator&(const WSimdVec4u& v) const
{
  WSimdVec4u result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator^(const WSimdVec4u& v) const
{
  WSimdVec4u result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator~() const
{
  WSimdVec4u result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator<<(WUInt32 uiShift) const
{
  WSimdVec4u result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator>>(WUInt32 uiShift) const
{
  WSimdVec4u result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator+=(const WSimdVec4u& v)
{
  m_v += v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator-=(const WSimdVec4u& v)
{
  m_v -= v.m_v;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator|=(const WSimdVec4u& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator&=(const WSimdVec4u& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator^=(const WSimdVec4u& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator<<=(WUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator>>=(WUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMin(const WSimdVec4u& v) const
{
  return m_v.CompMin(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMax(const WSimdVec4u& v) const
{
  return m_v.CompMax(v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator==(const WSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator!=(const WSimdVec4u& v) const
{
  return !(*this == v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator<=(const WSimdVec4u& v) const
{
  return !(*this > v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator<(const WSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator>=(const WSimdVec4u& v) const
{
  return !(*this < v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator>(const WSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return WSimdVec4b(result[0], result[1], result[2], result[3]);
}

// static
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::MakeZero()
{
  return WVec4U32::MakeZero();
}
