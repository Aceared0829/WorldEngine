#pragma once

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInt32 xyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_s32(xyzw);
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInt32 x, WInt32 y, WInt32 z, WInt32 w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) WInt32 values[4] = {x, y, z, w};
  m_v = vld1q_s32(values);
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInternal::QuadInt v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::MakeZero()
{
  return vmovq_n_s32(0);
}

W_ALWAYS_INLINE void WSimdVec4i::Set(WInt32 xyzw)
{
  m_v = vmovq_n_s32(xyzw);
}

W_ALWAYS_INLINE void WSimdVec4i::Set(WInt32 x, WInt32 y, WInt32 z, WInt32 w)
{
  alignas(16) WInt32 values[4] = {x, y, z, w};
  m_v = vld1q_s32(values);
}

W_ALWAYS_INLINE void WSimdVec4i::SetZero()
{
  m_v = vmovq_n_s32(0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<1>(const WInt32* pInts)
{
  m_v = vld1q_lane_s32(pInts, vmovq_n_s32(0), 0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<2>(const WInt32* pInts)
{
  m_v = vreinterpretq_s32_s64(vld1q_lane_s64(reinterpret_cast<const int64_t*>(pInts), vmovq_n_s64(0), 0));
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<3>(const WInt32* pInts)
{
  m_v = vcombine_s32(vld1_s32(pInts), vld1_lane_s32(pInts + 2, vmov_n_s32(0), 0));
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<4>(const WInt32* pInts)
{
  m_v = vld1q_s32(pInts);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<1>(WInt32* pInts) const
{
  vst1q_lane_s32(pInts, m_v, 0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<2>(WInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<3>(WInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
  vst1q_lane_s32(pInts + 2, m_v, 2);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<4>(WInt32* pInts) const
{
  vst1q_s32(pInts, m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4i::ToFloat() const
{
  return vcvtq_f32_s32(m_v);
}

// static
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Truncate(const WSimdVec4f& f)
{
  return vcvtq_s32_f32(f.m_v);
}

template <int N>
W_ALWAYS_INLINE WInt32 WSimdVec4i::GetComponent() const
{
  return vgetq_lane_s32(m_v, N);
}

W_ALWAYS_INLINE WInt32 WSimdVec4i::x() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE WInt32 WSimdVec4i::y() const
{
  return GetComponent<1>();
}

W_ALWAYS_INLINE WInt32 WSimdVec4i::z() const
{
  return GetComponent<2>();
}

W_ALWAYS_INLINE WInt32 WSimdVec4i::w() const
{
  return GetComponent<3>();
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Get() const
{
  return __builtin_shufflevector(m_v, m_v, W_TO_SHUFFLE(s));
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::GetCombined(const WSimdVec4i& other) const
{
  return __builtin_shufflevector(m_v, other.m_v, W_TO_SHUFFLE(s));
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator-() const
{
  return vnegq_s32(m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator+(const WSimdVec4i& v) const
{
  return vaddq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator-(const WSimdVec4i& v) const
{
  return vsubq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMul(const WSimdVec4i& v) const
{
  return vmulq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompDiv(const WSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (WUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] / b[i];
  }

  WSimdVec4i r;
  r.Load<4>(a);
  return r;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator|(const WSimdVec4i& v) const
{
  return vorrq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator&(const WSimdVec4i& v) const
{
  return vandq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator^(const WSimdVec4i& v) const
{
  return veorq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator~() const
{
  return vmvnq_s32(m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator<<(WUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(uiShift));
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator>>(WUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(-uiShift));
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator<<(const WSimdVec4i& v) const
{
  return vshlq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator>>(const WSimdVec4i& v) const
{
  return vshlq_s32(m_v, vnegq_s32(v.m_v));
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator+=(const WSimdVec4i& v)
{
  m_v = vaddq_s32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator-=(const WSimdVec4i& v)
{
  m_v = vsubq_s32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator|=(const WSimdVec4i& v)
{
  m_v = vorrq_s32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator&=(const WSimdVec4i& v)
{
  m_v = vandq_s32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator^=(const WSimdVec4i& v)
{
  m_v = veorq_s32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator<<=(WUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(uiShift));
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator>>=(WUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(-uiShift));
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMin(const WSimdVec4i& v) const
{
  return vminq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMax(const WSimdVec4i& v) const
{
  return vmaxq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Abs() const
{
  return vabsq_s32(m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator==(const WSimdVec4i& v) const
{
  return vceqq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator!=(const WSimdVec4i& v) const
{
  return vmvnq_u32(vceqq_s32(m_v, v.m_v));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator<=(const WSimdVec4i& v) const
{
  return vcleq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator<(const WSimdVec4i& v) const
{
  return vcltq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator>=(const WSimdVec4i& v) const
{
  return vcgeq_s32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator>(const WSimdVec4i& v) const
{
  return vcgtq_s32(m_v, v.m_v);
}

// static
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Select(const WSimdVec4b& vCmp, const WSimdVec4i& vTrue, const WSimdVec4i& vFalse)
{
  return vbslq_s32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}

// not needed atm
#if 0
void WSimdVec4i::Transpose(WSimdVec4i& v0, WSimdVec4i& v1, WSimdVec4i& v2, WSimdVec4i& v3)
{
  int32x4x2_t P0 = vzipq_s32(v0.m_v, v2.m_v);
  int32x4x2_t P1 = vzipq_s32(v1.m_v, v3.m_v);

  int32x4x2_t T0 = vzipq_s32(P0.val[0], P1.val[0]);
  int32x4x2_t T1 = vzipq_s32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif
