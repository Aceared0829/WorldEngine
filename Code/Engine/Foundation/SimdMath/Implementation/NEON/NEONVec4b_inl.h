#pragma once

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b()
{
  W_CHECK_SIMD_ALIGNMENT(this);
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(bool b)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_u32(b ? 0xFFFFFFFF : 0);
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(bool x, bool y, bool z, bool w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) WUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = vld1q_u32(mask);
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(WInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::GetComponent() const
{
  return vgetq_lane_u32(m_v, N) & 1;
}

W_ALWAYS_INLINE bool WSimdVec4b::x() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE bool WSimdVec4b::y() const
{
  return GetComponent<1>();
}

W_ALWAYS_INLINE bool WSimdVec4b::z() const
{
  return GetComponent<2>();
}

W_ALWAYS_INLINE bool WSimdVec4b::w() const
{
  return GetComponent<3>();
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::Get() const
{
  return __builtin_shufflevector(m_v, m_v, W_TO_SHUFFLE(s));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator&&(const WSimdVec4b& rhs) const
{
  return vandq_u32(m_v, rhs.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator||(const WSimdVec4b& rhs) const
{
  return vorrq_u32(m_v, rhs.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator!() const
{
  return vmvnq_u32(m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator==(const WSimdVec4b& rhs) const
{
  return vceqq_u32(m_v, rhs.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator!=(const WSimdVec4b& rhs) const
{
  return veorq_u32(m_v, rhs.m_v);
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::AllSet() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMask(m_v) & mask) == mask;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::AnySet() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMask(m_v) & mask) != 0;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::NoneSet() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMask(m_v) & mask) == 0;
}

// static
W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::Select(const WSimdVec4b& vCmp, const WSimdVec4b& vTrue, const WSimdVec4b& vFalse)
{
  return vbslq_u32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}
