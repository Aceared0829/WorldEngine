#pragma once

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm_set1_ps(WMath::NaN<float>());
#endif
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(float fXyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(fXyzw);
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(const WSimdFloat& fXyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = fXyzw.m_v;
}

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(float x, float y, float z, float w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_setr_ps(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4f::Set(float fXyzw)
{
  m_v = _mm_set1_ps(fXyzw);
}

W_ALWAYS_INLINE void WSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v = _mm_setr_ps(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4f::SetX(const WSimdFloat& f)
{
  m_v = _mm_move_ss(m_v, f.m_v);
}

W_ALWAYS_INLINE void WSimdVec4f::SetY(const WSimdFloat& f)
{
  m_v = _mm_shuffle_ps(_mm_unpacklo_ps(m_v, f.m_v), m_v, W_TO_SHUFFLE(WSwizzle::XYZW));
}

W_ALWAYS_INLINE void WSimdVec4f::SetZ(const WSimdFloat& f)
{
  m_v = _mm_shuffle_ps(m_v, _mm_unpackhi_ps(f.m_v, m_v), W_TO_SHUFFLE(WSwizzle::XYZW));
}

W_ALWAYS_INLINE void WSimdVec4f::SetW(const WSimdFloat& f)
{
  m_v = _mm_shuffle_ps(m_v, _mm_unpackhi_ps(m_v, f.m_v), W_TO_SHUFFLE(WSwizzle::XYXY));
}

W_ALWAYS_INLINE void WSimdVec4f::SetZero()
{
  m_v = _mm_setzero_ps();
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<1>(const float* pFloat)
{
  m_v = _mm_load_ss(pFloat);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<2>(const float* pFloat)
{
  m_v = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pFloat)));
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<3>(const float* pFloat)
{
// There is a compiler bug in GCC where GCC will incorrectly optimize the alternative faster implementation.
#if W_ENABLED(W_COMPILER_GCC)
  m_v = _mm_set_ps(0.0f, pFloat[2], pFloat[1], pFloat[0]);
#else
  m_v = _mm_movelh_ps(_mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pFloat))), _mm_load_ss(pFloat + 2));
#endif
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Load<4>(const float* pFloat)
{
  m_v = _mm_loadu_ps(pFloat);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<1>(float* pFloat) const
{
  _mm_store_ss(pFloat, m_v);
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<2>(float* pFloat) const
{
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(m_v));
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<3>(float* pFloat) const
{
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(m_v));
  _mm_store_ss(pFloat + 2, _mm_movehl_ps(m_v, m_v));
}

template <>
W_ALWAYS_INLINE void WSimdVec4f::Store<4>(float* pFloat) const
{
  _mm_storeu_ps(pFloat, m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetReciprocal<WMathAcc::BITS_12>() const
{
  return _mm_rcp_ps(m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetReciprocal<WMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rcp_ps(m_v);

  // One Newton-Raphson iteration
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(m_v, x0)));

  return x1;
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetReciprocal<WMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetSqrt<WMathAcc::BITS_12>() const
{
  return _mm_mul_ps(m_v, _mm_rsqrt_ps(m_v));
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetSqrt<WMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));

  return _mm_mul_ps(m_v, x1);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetSqrt<WMathAcc::FULL>() const
{
  return _mm_sqrt_ps(m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetInvSqrt<WMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(m_v));
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetInvSqrt<WMathAcc::BITS_23>() const
{
  const __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  return _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetInvSqrt<WMathAcc::BITS_12>() const
{
  return _mm_rsqrt_ps(m_v);
}

template <int N, WMathAcc::Enum acc>
void WSimdVec4f::NormalizeIfNotZero(const WSimdFloat& fEpsilon)
{
  WSimdFloat sqLength = GetLengthSquared<N>();
  __m128 isNotZero = _mm_cmpgt_ps(sqLength.m_v, fEpsilon.m_v);
  m_v = _mm_mul_ps(m_v, sqLength.GetInvSqrt<acc>().m_v);
  m_v = _mm_and_ps(isNotZero, m_v);
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsZero() const
{
  const int mask = W_BIT(N) - 1;
  return (_mm_movemask_ps(_mm_cmpeq_ps(m_v, _mm_setzero_ps())) & mask) == mask;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsZero(const WSimdFloat& fEpsilon) const
{
  const int mask = W_BIT(N) - 1;
  __m128 absVal = Abs().m_v;
  return (_mm_movemask_ps(_mm_cmplt_ps(absVal, fEpsilon.m_v)) & mask) == mask;
}

template <int N>
inline bool WSimdVec4f::IsNaN() const
{
  // NAN -> (exponent = all 1, mantissa = non-zero)

  alignas(16) const WUInt32 s_exponentMask[4] = {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};
  alignas(16) const WUInt32 s_mantissaMask[4] = {0x7FFFFF, 0x7FFFFF, 0x7FFFFF, 0x7FFFFF};

  __m128 exponentMask = _mm_load_ps(reinterpret_cast<const float*>(s_exponentMask));
  __m128 mantissaMask = _mm_load_ps(reinterpret_cast<const float*>(s_mantissaMask));

  __m128 exponentAll1 = _mm_cmpeq_ps(_mm_and_ps(m_v, exponentMask), exponentMask);
  __m128 mantissaNon0 = _mm_cmpneq_ps(_mm_and_ps(m_v, mantissaMask), _mm_setzero_ps());

  const int mask = W_BIT(N) - 1;
  return (_mm_movemask_ps(_mm_and_ps(exponentAll1, mantissaNon0)) & mask) != 0;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsValid() const
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  alignas(16) const WUInt32 s_exponentMask[4] = {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};

  __m128 exponentMask = _mm_load_ps(reinterpret_cast<const float*>(s_exponentMask));

  __m128 exponentNot1 = _mm_cmpneq_ps(_mm_and_ps(m_v, exponentMask), exponentMask);

  const int mask = W_BIT(N) - 1;
  return (_mm_movemask_ps(exponentNot1) & mask) == mask;
}

template <int N>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::GetComponent() const
{
  return _mm_shuffle_ps(m_v, m_v, W_SHUFFLE(N, N, N, N));
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
  return _mm_shuffle_ps(m_v, m_v, W_TO_SHUFFLE(s));
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetCombined(const WSimdVec4f& other) const
{
  return _mm_shuffle_ps(m_v, other.m_v, W_TO_SHUFFLE(s));
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator-() const
{
  return _mm_sub_ps(_mm_setzero_ps(), m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator+(const WSimdVec4f& v) const
{
  return _mm_add_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator-(const WSimdVec4f& v) const
{
  return _mm_sub_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator*(const WSimdFloat& f) const
{
  return _mm_mul_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::operator/(const WSimdFloat& f) const
{
  return _mm_div_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMul(const WSimdVec4f& v) const
{
  return _mm_mul_ps(m_v, v.m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompDiv<WMathAcc::FULL>(const WSimdVec4f& v) const
{
  return _mm_div_ps(m_v, v.m_v);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompDiv<WMathAcc::BITS_23>(const WSimdVec4f& v) const
{
  __m128 x0 = _mm_rcp_ps(v.m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(v.m_v, x0)));

  return _mm_mul_ps(m_v, x1);
}

template <>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompDiv<WMathAcc::BITS_12>(const WSimdVec4f& v) const
{
  return _mm_mul_ps(m_v, _mm_rcp_ps(v.m_v));
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMin(const WSimdVec4f& v) const
{
  return _mm_min_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CompMax(const WSimdVec4f& v) const
{
  return _mm_max_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Abs() const
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Round() const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_NINT);
#else
  return WSimdVec4f(floorf(static_cast<float>(x()) + 0.5f), floorf(static_cast<float>(y()) + 0.5f), floorf(static_cast<float>(z()) + 0.5f), floorf(static_cast<float>(w()) + 0.5f));
#endif
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Floor() const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_FLOOR);
#else
  return WSimdVec4f(floorf(static_cast<float>(x())), floorf(static_cast<float>(y())), floorf(static_cast<float>(z())), floorf(static_cast<float>(w())));
#endif
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Ceil() const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_CEIL);
#else
  return WSimdVec4f(ceilf(static_cast<float>(x())), ceilf(static_cast<float>(y())), ceilf(static_cast<float>(z())), ceilf(static_cast<float>(w())));
#endif
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Trunc() const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_TRUNC);
#else
  return WSimdVec4f(int(x()), int(y()), int(z()), int(w()));
  return {};
#endif
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::FlipSign(const WSimdVec4b& vCmp) const
{
  return _mm_xor_ps(m_v, _mm_and_ps(vCmp.m_v, _mm_set1_ps(-0.0f)));
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Select(const WSimdVec4b& vCmp, const WSimdVec4f& vTrue, const WSimdVec4f& vFalse)
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_blendv_ps(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  return _mm_or_ps(_mm_andnot_ps(vCmp.m_v, vFalse.m_v), _mm_and_ps(vCmp.m_v, vTrue.m_v));
#endif
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator+=(const WSimdVec4f& v)
{
  m_v = _mm_add_ps(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator-=(const WSimdVec4f& v)
{
  m_v = _mm_sub_ps(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator*=(const WSimdFloat& f)
{
  m_v = _mm_mul_ps(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4f& WSimdVec4f::operator/=(const WSimdFloat& f)
{
  m_v = _mm_div_ps(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator==(const WSimdVec4f& v) const
{
  return _mm_cmpeq_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator!=(const WSimdVec4f& v) const
{
  return _mm_cmpneq_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator<=(const WSimdVec4f& v) const
{
  return _mm_cmple_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator<(const WSimdVec4f& v) const
{
  return _mm_cmplt_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator>=(const WSimdVec4f& v) const
{
  return _mm_cmpge_ps(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::operator>(const WSimdVec4f& v) const
{
  return _mm_cmpgt_ps(m_v, v.m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<2>() const
{
#if W_SSE_LEVEL >= W_SSE_31
  __m128 a = _mm_hadd_ps(m_v, m_v);
  return _mm_shuffle_ps(a, a, W_TO_SHUFFLE(WSwizzle::XXXX));
#else
  return GetComponent<0>() + GetComponent<1>();
#endif
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<4>() const
{
#if W_SSE_LEVEL >= W_SSE_31
  __m128 a = _mm_hadd_ps(m_v, m_v);
  return _mm_hadd_ps(a, a);
#else
  return (GetComponent<0>() + GetComponent<1>()) + (GetComponent<2>() + GetComponent<3>());
#endif
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<2>() const
{
  return _mm_min_ps(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<3>() const
{
  return _mm_min_ps(_mm_min_ps(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<4>() const
{
  __m128 xyxyzwzw = _mm_min_ps(_mm_shuffle_ps(m_v, m_v, W_TO_SHUFFLE(WSwizzle::ZWXY)), m_v);
  __m128 zwzwxyxy = _mm_shuffle_ps(xyxyzwzw, xyxyzwzw, W_TO_SHUFFLE(WSwizzle::YXWZ));
  return _mm_min_ps(xyxyzwzw, zwzwxyxy);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<2>() const
{
  return _mm_max_ps(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<3>() const
{
  return _mm_max_ps(_mm_max_ps(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<4>() const
{
  __m128 xyxyzwzw = _mm_max_ps(_mm_shuffle_ps(m_v, m_v, W_TO_SHUFFLE(WSwizzle::ZWXY)), m_v);
  __m128 zwzwxyxy = _mm_shuffle_ps(xyxyzwzw, xyxyzwzw, W_TO_SHUFFLE(WSwizzle::YXWZ));
  return _mm_max_ps(xyxyzwzw, zwzwxyxy);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<1>(const WSimdVec4f& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x1f);
#else
  return CompMul(v).HorizontalSum<1>();
#endif
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<2>(const WSimdVec4f& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x3f);
#else
  return CompMul(v).HorizontalSum<2>();
#endif
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<3>(const WSimdVec4f& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x7f);
#else
  return CompMul(v).HorizontalSum<3>();
#endif
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::Dot<4>(const WSimdVec4f& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0xff);
#else
  return CompMul(v).HorizontalSum<4>();
#endif
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CrossRH(const WSimdVec4f& v) const
{
  __m128 a = _mm_mul_ps(m_v, _mm_shuffle_ps(v.m_v, v.m_v, W_TO_SHUFFLE(WSwizzle::YZXW)));
  __m128 b = _mm_mul_ps(v.m_v, _mm_shuffle_ps(m_v, m_v, W_TO_SHUFFLE(WSwizzle::YZXW)));
  __m128 c = _mm_sub_ps(a, b);

  return _mm_shuffle_ps(c, c, W_TO_SHUFFLE(WSwizzle::YZXW));
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulAdd(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c)
{
#if W_SSE_LEVEL >= W_SSE_AVX2
  return _mm_fmadd_ps(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) + c;
#endif
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulAdd(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c)
{
#if W_SSE_LEVEL >= W_SSE_AVX2
  return _mm_fmadd_ps(a.m_v, b.m_v, c.m_v);
#else
  return a * b + c;
#endif
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulSub(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c)
{
#if W_SSE_LEVEL >= W_SSE_AVX2
  return _mm_fmsub_ps(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) - c;
#endif
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MulSub(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c)
{
#if W_SSE_LEVEL >= W_SSE_AVX2
  return _mm_fmsub_ps(a.m_v, b.m_v, c.m_v);
#else
  return a * b - c;
#endif
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::CopySign(const WSimdVec4f& vMagnitude, const WSimdVec4f& vSign)
{
  __m128 minusZero = _mm_set1_ps(-0.0f);
  return _mm_or_ps(_mm_andnot_ps(minusZero, vMagnitude.m_v), _mm_and_ps(minusZero, vSign.m_v));
}
