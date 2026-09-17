#pragma once

W_ALWAYS_INLINE WSimdFloat::WSimdFloat()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm_set1_ps(WMath::NaN<float>());
#endif
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(float f)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(f);
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
  m_v = _mm_shuffle_ps(v, v, W_TO_SHUFFLE(WSwizzle::XXXX));
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WUInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_PLATFORM_64BIT)
  __m128 v = _mm_cvtsi64_ss(_mm_setzero_ps(), i);
#else
  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
#endif
  m_v = _mm_shuffle_ps(v, v, W_TO_SHUFFLE(WSwizzle::XXXX));
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WAngle a)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(a.GetRadian());
}

W_ALWAYS_INLINE WSimdFloat::WSimdFloat(WInternal::QuadFloat v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdFloat::operator float() const
{
  float f;
  _mm_store_ss(&f, m_v);
  return f;
}

// static
W_ALWAYS_INLINE WSimdFloat WSimdFloat::MakeZero()
{
  return _mm_setzero_ps();
}

// static
W_ALWAYS_INLINE WSimdFloat WSimdFloat::MakeNaN()
{
  return _mm_set1_ps(WMath::NaN<float>());
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator+(const WSimdFloat& f) const
{
  return _mm_add_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator-(const WSimdFloat& f) const
{
  return _mm_sub_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator*(const WSimdFloat& f) const
{
  return _mm_mul_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::operator/(const WSimdFloat& f) const
{
  return _mm_div_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator+=(const WSimdFloat& f)
{
  m_v = _mm_add_ps(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator-=(const WSimdFloat& f)
{
  m_v = _mm_sub_ps(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator*=(const WSimdFloat& f)
{
  m_v = _mm_mul_ps(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdFloat& WSimdFloat::operator/=(const WSimdFloat& f)
{
  m_v = _mm_div_ps(m_v, f.m_v);
  return *this;
}

W_ALWAYS_INLINE bool WSimdFloat::IsEqual(const WSimdFloat& rhs, const WSimdFloat& fEpsilon) const
{
  WSimdFloat minusEps = rhs - fEpsilon;
  WSimdFloat plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

W_ALWAYS_INLINE bool WSimdFloat::operator==(const WSimdFloat& f) const
{
  return _mm_comieq_ss(m_v, f.m_v) == 1;
}

W_ALWAYS_INLINE bool WSimdFloat::operator!=(const WSimdFloat& f) const
{
  return _mm_comineq_ss(m_v, f.m_v) == 1;
}

W_ALWAYS_INLINE bool WSimdFloat::operator>=(const WSimdFloat& f) const
{
  return _mm_comige_ss(m_v, f.m_v) == 1;
}

W_ALWAYS_INLINE bool WSimdFloat::operator>(const WSimdFloat& f) const
{
  return _mm_comigt_ss(m_v, f.m_v) == 1;
}

W_ALWAYS_INLINE bool WSimdFloat::operator<=(const WSimdFloat& f) const
{
  return _mm_comile_ss(m_v, f.m_v) == 1;
}

W_ALWAYS_INLINE bool WSimdFloat::operator<(const WSimdFloat& f) const
{
  return _mm_comilt_ss(m_v, f.m_v) == 1;
}

W_ALWAYS_INLINE bool WSimdFloat::operator==(float f) const
{
  return (*this) == WSimdFloat(f);
}

W_ALWAYS_INLINE bool WSimdFloat::operator!=(float f) const
{
  return (*this) != WSimdFloat(f);
}

W_ALWAYS_INLINE bool WSimdFloat::operator>(float f) const
{
  return (*this) > WSimdFloat(f);
}

W_ALWAYS_INLINE bool WSimdFloat::operator>=(float f) const
{
  return (*this) >= WSimdFloat(f);
}

W_ALWAYS_INLINE bool WSimdFloat::operator<(float f) const
{
  return (*this) < WSimdFloat(f);
}

W_ALWAYS_INLINE bool WSimdFloat::operator<=(float f) const
{
  return (*this) <= WSimdFloat(f);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetReciprocal<WMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetReciprocal<WMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rcp_ps(m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(m_v, x0)));

  return x1;
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetReciprocal<WMathAcc::BITS_12>() const
{
  return _mm_rcp_ps(m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetInvSqrt<WMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(m_v));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetInvSqrt<WMathAcc::BITS_23>() const
{
  const __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  return _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetInvSqrt<WMathAcc::BITS_12>() const
{
  return _mm_rsqrt_ps(m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetSqrt<WMathAcc::FULL>() const
{
  return _mm_sqrt_ps(m_v);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetSqrt<WMathAcc::BITS_23>() const
{
  return (*this) * GetInvSqrt<WMathAcc::BITS_23>();
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdFloat::GetSqrt<WMathAcc::BITS_12>() const
{
  return (*this) * GetInvSqrt<WMathAcc::BITS_12>();
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::Max(const WSimdFloat& f) const
{
  return _mm_max_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::Min(const WSimdFloat& f) const
{
  return _mm_min_ps(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdFloat WSimdFloat::Abs() const
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), m_v);
}
