#pragma once

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WUInt32 xyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_u32(xyzw);
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) WUInt32 values[4] = {x, y, z, w};
  m_v = vld1q_u32(values);
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WInternal::QuadUInt v)
{
  m_v = v;
}

W_ALWAYS_INLINE void WSimdVec4u::Set(WUInt32 xyzw)
{
  m_v = vmovq_n_u32(xyzw);
}

W_ALWAYS_INLINE void WSimdVec4u::Set(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w)
{
  alignas(16) WUInt32 values[4] = {x, y, z, w};
  m_v = vld1q_u32(values);
}

W_ALWAYS_INLINE void WSimdVec4u::SetZero()
{
  m_v = vmovq_n_u32(0);
}

// needs to be implemented here because of include dependencies
W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(const WSimdVec4u& u)
  : m_v(u.m_v)
{
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(const WSimdVec4i& i)
  : m_v(i.m_v)
{
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4u::ToFloat() const
{
  return vcvtq_f32_u32(m_v);
}

// static
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::Truncate(const WSimdVec4f& f)
{
  return vcvtq_u32_f32(f.m_v);
}

template <int N>
W_ALWAYS_INLINE WUInt32 WSimdVec4u::GetComponent() const
{
  return vgetq_lane_u32(m_v, N);
}

W_ALWAYS_INLINE WUInt32 WSimdVec4u::x() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE WUInt32 WSimdVec4u::y() const
{
  return GetComponent<1>();
}

W_ALWAYS_INLINE WUInt32 WSimdVec4u::z() const
{
  return GetComponent<2>();
}

W_ALWAYS_INLINE WUInt32 WSimdVec4u::w() const
{
  return GetComponent<3>();
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::Get() const
{
  return __builtin_shufflevector(m_v, m_v, W_TO_SHUFFLE(s));
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator+(const WSimdVec4u& v) const
{
  return vaddq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator-(const WSimdVec4u& v) const
{
  return vsubq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMul(const WSimdVec4u& v) const
{
  return vmulq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator|(const WSimdVec4u& v) const
{
  return vorrq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator&(const WSimdVec4u& v) const
{
  return vandq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator^(const WSimdVec4u& v) const
{
  return veorq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator~() const
{
  return vmvnq_u32(m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator<<(WUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(uiShift));
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator>>(WUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(-uiShift));
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator+=(const WSimdVec4u& v)
{
  m_v = vaddq_u32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator-=(const WSimdVec4u& v)
{
  m_v = vsubq_u32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator|=(const WSimdVec4u& v)
{
  m_v = vorrq_u32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator&=(const WSimdVec4u& v)
{
  m_v = vandq_u32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator^=(const WSimdVec4u& v)
{
  m_v = veorq_u32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator<<=(WUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(uiShift));
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator>>=(WUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(-uiShift));
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMin(const WSimdVec4u& v) const
{
  return vminq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMax(const WSimdVec4u& v) const
{
  return vmaxq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator==(const WSimdVec4u& v) const
{
  return vceqq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator!=(const WSimdVec4u& v) const
{
  return vmvnq_u32(vceqq_u32(m_v, v.m_v));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator<=(const WSimdVec4u& v) const
{
  return vcleq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator<(const WSimdVec4u& v) const
{
  return vcltq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator>=(const WSimdVec4u& v) const
{
  return vcgeq_u32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator>(const WSimdVec4u& v) const
{
  return vcgtq_u32(m_v, v.m_v);
}

// static
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::MakeZero()
{
  return vmovq_n_u32(0);
}

// not needed atm
#if 0
void WSimdVec4u::Transpose(WSimdVec4u& v0, WSimdVec4u& v1, WSimdVec4u& v2, WSimdVec4u& v3)
{
  uint32x4x2_t P0 = vzipq_u32(v0.m_v, v2.m_v);
  uint32x4x2_t P1 = vzipq_u32(v1.m_v, v3.m_v);

  uint32x4x2_t T0 = vzipq_u32(P0.val[0], P1.val[0]);
  uint32x4x2_t T1 = vzipq_u32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif
