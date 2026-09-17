#pragma once

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = vmovq_n_f32(WMath::NaN<float>());
#endif
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(float xyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_f32(xyzw);
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(const WSimdFloat& xyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = xyzw.m_v;
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(float x, float y, float z, float w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) float values[4] = {x, y, z, w};
  m_v = vld1q_f32(values);
}

W_ALWAYS_INLINE void WSimdVec4f::Set(float xyzw)
{
  m_v = vmovq_n_f32(xyzw);
}

W_ALWAYS_INLINE void WSimdVec4f::Set(float x, float y, float z, float w)
{
  alignas(16) float values[4] = {x, y, z, w};
  m_v = vld1q_f32(values);
}

W_ALWAYS_INLINE void WSimdVec4f::SetX(const WSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 0);
}

W_ALWAYS_INLINE void WSimdVec4f::SetY(const WSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 1);
}

W_ALWAYS_INLINE void WSimdVec4f::SetZ(const WSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 2);
}

W_ALWAYS_INLINE void WSimdVec4f::SetW(const WSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 3);
}

W_ALWAYS_INLINE void WSimdVec4f::SetZero()
{
  m_v = vmovq_n_f32(0.0f);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<1>(const float* pFloat)
{
  m_v = vld1q_lane_f32(pFloat, vmovq_n_f32(0.0f), 0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<2>(const float* pFloat)
{
  m_v = vreinterpretq_f32_f64(vld1q_lane_f64(reinterpret_cast<const float64_t*>(pFloat), vmovq_n_f64(0.0), 0));
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<3>(const float* pFloat)
{
  m_v = vcombine_f32(vld1_f32(pFloat), vld1_lane_f32(pFloat + 2, vmov_n_f32(0.0f), 0));
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<4>(const float* pFloat)
{
  m_v = vld1q_f32(pFloat);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<1>(float* pFloat) const
{
  vst1q_lane_f32(pFloat, m_v, 0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<2>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<3>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
  vst1q_lane_f32(pFloat + 2, m_v, 2);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<4>(float* pFloat) const
{
  vst1q_f32(pFloat, m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetReciprocal<WMathAcc::BITS_12>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // One iteration of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);

  return x1;
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetReciprocal<WMathAcc::BITS_23>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // Two iterations of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);
  float32x4_t x2 = vmulq_f32(vrecpsq_f32(m_v, x1), x1);

  return x2;
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetReciprocal<WMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetInvSqrt<WMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), vsqrtq_f32(m_v));
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetInvSqrt<WMathAcc::BITS_23>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // Two iterations of Newton-Raphson
  const float32x4_t x1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, m_v), x1), x1);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetInvSqrt<WMathAcc::BITS_12>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // One iteration of Newton-Raphson
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetSqrt<WMathAcc::BITS_12>() const
{
  return CompMul(GetInvSqrt<WMathAcc::BITS_12>());
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetSqrt<WMathAcc::BITS_23>() const
{
  return CompMul(GetInvSqrt<WMathAcc::BITS_23>());
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetSqrt<WMathAcc::FULL>() const
{
  return vsqrtq_f32(m_v);
}

template <int N, WMathAcc::Enum acc>
void WSimdVec4f::NormalizeIfNotZero(const WSimdFloat& fEpsilon)
{
  WSimdFloat sqLength = GetLengthSquared<N>();
  uint32x4_t isNotZero = vcgtq_f32(sqLength.m_v, fEpsilon.m_v);
  m_v = vmulq_f32(m_v, sqLength.GetInvSqrt<acc>().m_v);
  m_v = vreinterpretq_f32_u32(vandq_u32(isNotZero, vreinterpretq_u32_f32(m_v)));
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsZero() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMask(vceqzq_f32(m_v)) & mask) == mask;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsZero(const WSimdFloat& fEpsilon) const
{
  const int mask = W_BIT(N) - 1;
  float32x4_t absVal = Abs().m_v;
  return (WInternal::NeonMoveMask(vcltq_f32(absVal, fEpsilon.m_v)) & mask) == mask;
}

template <int N>
inline bool WSimdVec4f::IsNaN() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMask(vceqq_f32(m_v, m_v)) & mask) != mask;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsValid() const
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  uint32x4_t exponentMask = vmovq_n_u32(0x7f800000);

  uint32x4_t exponentIs1 = vceqq_u32(vandq_u32(vreinterpretq_u32_f32(m_v), exponentMask), exponentMask);

  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMask(exponentIs1) & mask) == 0;
}

template <int N>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::GetComponent() const
{
  return vdupq_laneq_f32(m_v, N);
}

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::x() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::y() const
{
  return GetComponent<1>();
}

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::z() const
{
  return GetComponent<2>();
}

W_ALWAYS_INLINE WSimdFloat WSimdVec4f::w() const
{
  return GetComponent<3>();
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Get() const
{
  return __builtin_shufflevector(m_v, m_v, W_TO_SHUFFLE(s));
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetCombined(const WSimdVec4f& other) const
{
  return __builtin_shufflevector(m_v, other.m_v, W_TO_SHUFFLE(s));
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator-() const
{
  return vnegq_f32(m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator+(const WSimdVec4f& v) const
{
  return vaddq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator-(const WSimdVec4f& v) const
{
  return vsubq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator*(const WSimdFloat& f) const
{
  return vmulq_f32(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator/(const WSimdFloat& f) const
{
  return vdivq_f32(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMul(const WSimdVec4f& v) const
{
  return vmulq_f32(m_v, v.m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompDiv<WMathAcc::FULL>(const WSimdVec4f& v) const
{
  return vdivq_f32(m_v, v.m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompDiv<WMathAcc::BITS_23>(const WSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<WMathAcc::BITS_23>());
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompDiv<WMathAcc::BITS_12>(const WSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<WMathAcc::BITS_12>());
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMin(const WSimdVec4f& v) const
{
  return vminq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMax(const WSimdVec4f& v) const
{
  return vmaxq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Abs() const
{
  return vabsq_f32(m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Round() const
{
  return vrndnq_f32(m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Floor() const
{
  return vrndmq_f32(m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Ceil() const
{
  return vrndpq_f32(m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Trunc() const
{
  return vrndq_f32(m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::FlipSign(const WSimdVec4b& cmp) const
{
  return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(m_v), vshlq_n_u32(cmp.m_v, 31)));
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Select(const WSimdVec4b& cmp, const WSimdVec4f& ifTrue, const WSimdVec4f& ifFalse)
{
  return vbslq_f32(cmp.m_v, ifTrue.m_v, ifFalse.m_v);
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator+=(const WSimdVec4f& v)
{
  m_v = vaddq_f32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator-=(const WSimdVec4f& v)
{
  m_v = vsubq_f32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator*=(const WSimdFloat& f)
{
  m_v = vmulq_f32(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator/=(const WSimdFloat& f)
{
  m_v = vdivq_f32(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator==(const WSimdVec4f& v) const
{
  return vceqq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator!=(const WSimdVec4f& v) const
{
  return vmvnq_u32(vceqq_f32(m_v, v.m_v));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator<=(const WSimdVec4f& v) const
{
  return vcleq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator<(const WSimdVec4f& v) const
{
  return vcltq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator>=(const WSimdVec4f& v) const
{
  return vcgeq_f32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator>(const WSimdVec4f& v) const
{
  return vcgtq_f32(m_v, v.m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<2>() const
{
  return vpadds_f32(vget_low_f32(m_v));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<4>() const
{
  float32x2_t x0 = vpadd_f32(vget_low_f32(m_v), vget_high_f32(m_v));
  return vpadds_f32(x0);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<2>() const
{
  return vpmins_f32(vget_low_f32(m_v));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<3>() const
{
  return vminq_f32(vmovq_n_f32(vpmins_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<4>() const
{
  return vpmins_f32(vpmin_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<2>() const
{
  return vpmaxs_f32(vget_low_f32(m_v));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<3>() const
{
  return vmaxq_f32(vmovq_n_f32(vpmaxs_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<4>() const
{
  return vpmaxs_f32(vpmax_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<1>(const WSimdVec4f& v) const
{
  return vdupq_laneq_f32(vmulq_f32(m_v, v.m_v), 0);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<2>(const WSimdVec4f& v) const
{
  return vpadds_f32(vmul_f32(vget_low_f32(m_v), vget_low_f32(v.m_v)));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<3>(const WSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<3>();
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<4>(const WSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<4>();
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CrossRH(const WSimdVec4f& v) const
{
  float32x4_t a = vmulq_f32(m_v, __builtin_shufflevector(v.m_v, v.m_v, W_TO_SHUFFLE(WSwizzle::YZXW)));
  float32x4_t b = vmulq_f32(v.m_v, __builtin_shufflevector(m_v, m_v, W_TO_SHUFFLE(WSwizzle::YZXW)));
  float32x4_t c = vsubq_f32(a, b);

  return __builtin_shufflevector(c, c, W_TO_SHUFFLE(WSwizzle::YZXW));
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulAdd(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulAdd(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulSub(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulSub(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CopySign(const WSimdVec4f& magnitude, const WSimdVec4f& sign)
{
  return vbslq_f32(vmovq_n_u32(0x80000000), sign.m_v, magnitude.m_v);
}
