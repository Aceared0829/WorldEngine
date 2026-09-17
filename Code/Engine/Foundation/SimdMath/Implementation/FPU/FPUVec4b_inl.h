#pragma once

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b() {}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(bool b)
{
  m_v.x = b ? 0xFFFFFFFF : 0;
  m_v.y = b ? 0xFFFFFFFF : 0;
  m_v.z = b ? 0xFFFFFFFF : 0;
  m_v.w = b ? 0xFFFFFFFF : 0;
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(bool x, bool y, bool z, bool w)
{
  m_v.x = x ? 0xFFFFFFFF : 0;
  m_v.y = y ? 0xFFFFFFFF : 0;
  m_v.z = z ? 0xFFFFFFFF : 0;
  m_v.w = w ? 0xFFFFFFFF : 0;
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(WInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::GetComponent() const
{
  if constexpr (N == 0)
  {
    return m_v.x != 0;
  }
  else if constexpr (N == 1)
  {
    return m_v.y != 0;
  }
  else if constexpr (N == 2)
  {
    return m_v.z != 0;
  }
  else if constexpr (N == 3)
  {
    return m_v.w != 0;
  }
  else
  {
    return m_v.w != 0;
  }
}

W_ALWAYS_INLINE bool WSimdVec4b::x() const
{
  return m_v.x != 0;
}

W_ALWAYS_INLINE bool WSimdVec4b::y() const
{
  return m_v.y != 0;
}

W_ALWAYS_INLINE bool WSimdVec4b::z() const
{
  return m_v.z != 0;
}

W_ALWAYS_INLINE bool WSimdVec4b::w() const
{
  return m_v.w != 0;
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::Get() const
{
  WSimdVec4b result;

  const WUInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator&&(const WSimdVec4b& rhs) const
{
  WSimdVec4b result;
  result.m_v.x = m_v.x & rhs.m_v.x;
  result.m_v.y = m_v.y & rhs.m_v.y;
  result.m_v.z = m_v.z & rhs.m_v.z;
  result.m_v.w = m_v.w & rhs.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator||(const WSimdVec4b& rhs) const
{
  WSimdVec4b result;
  result.m_v.x = m_v.x | rhs.m_v.x;
  result.m_v.y = m_v.y | rhs.m_v.y;
  result.m_v.z = m_v.z | rhs.m_v.z;
  result.m_v.w = m_v.w | rhs.m_v.w;

  return result;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator!() const
{
  WSimdVec4b result;
  result.m_v.x = m_v.x ^ 0xFFFFFFFF;
  result.m_v.y = m_v.y ^ 0xFFFFFFFF;
  result.m_v.z = m_v.z ^ 0xFFFFFFFF;
  result.m_v.w = m_v.w ^ 0xFFFFFFFF;

  return result;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator==(const WSimdVec4b& rhs) const
{
  return !(*this != rhs);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator!=(const WSimdVec4b& rhs) const
{
  WSimdVec4b result;
  result.m_v.x = m_v.x ^ rhs.m_v.x;
  result.m_v.y = m_v.y ^ rhs.m_v.y;
  result.m_v.z = m_v.z ^ rhs.m_v.z;
  result.m_v.w = m_v.w ^ rhs.m_v.w;

  return result;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::AllSet() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!(&m_v.x)[i])
      return false;
  }

  return true;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::AnySet() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i])
      return true;
  }

  return false;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::NoneSet() const
{
  return !AnySet<N>();
}

// static
W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::Select(const WSimdVec4b& cmp, const WSimdVec4b& ifTrue, const WSimdVec4b& ifFalse)
{
  WSimdVec4b result;
  result.m_v.x = (cmp.m_v.x != 0) ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = (cmp.m_v.y != 0) ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = (cmp.m_v.z != 0) ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = (cmp.m_v.w != 0) ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}
