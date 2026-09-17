#pragma once

W_ALWAYS_INLINE WSimdDouble::WSimdDouble()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v.xy = _mm_set1_pd(WMath::NaN<double>());
  m_v.zw = m_v.xy;
#endif


}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(float f)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(double(f));
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(double d)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(d);
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  __m128i packedInt32 = _mm_set1_epi32(i);
  m_v.xy = _mm_cvtepi32_pd(packedInt32);
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WUInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(static_cast<double>(i));
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WAngle a)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(a.GetRadian());
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInternal::QuadFloat v)
{
  m_v.xy = _mm_cvtps_pd(v);
  m_v.zw = _mm_cvtps_pd(_mm_movehl_ps(v, v));
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInternal::QuadDouble v)
{
  m_v = v;
}



// W_ALWAYS_INLINE WSimdDouble::operator float() const
// {
//   double d;
//   _mm256_store_pd(&d, m_v);
//   return float(d);
// }

W_ALWAYS_INLINE WSimdDouble::operator double() const
{
  return _mm_cvtsd_f64(m_v.xy);
}


// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeZero()
{
  WSimdDouble result;
  result.m_v.xy = _mm_setzero_pd();
  result.m_v.zw = result.m_v.xy;
  return result;
}

// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeNaN()
{
  WSimdDouble result;
  result.m_v.xy = _mm_set1_pd(WMath::NaN<double>());
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator+(const WSimdDouble& d) const
{
  WSimdDouble result;
  result.m_v.xy = _mm_add_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator-(const WSimdDouble& d) const
{
  WSimdDouble result;
  result.m_v.xy = _mm_sub_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator*(const WSimdDouble& d) const
{
  WSimdDouble result;
  result.m_v.xy = _mm_mul_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator/(const WSimdDouble& d) const
{
  WSimdDouble result;
  result.m_v.xy = _mm_div_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator+=(const WSimdDouble& d)
{
  m_v.xy = _mm_add_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator-=(const WSimdDouble& d)
{
  m_v.xy = _mm_sub_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator*=(const WSimdDouble& d)
{
  m_v.xy = _mm_mul_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator/=(const WSimdDouble& d)
{
  m_v.xy = _mm_div_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE bool WSimdDouble::IsEqual(const WSimdDouble& rhs, const WSimdDouble& fEpsilon) const
{
  WSimdDouble minusEps = rhs - fEpsilon;
  WSimdDouble plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(const WSimdDouble& d) const
{
  return _mm_comieq_sd(m_v.xy, d.m_v.xy) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(const WSimdDouble& d) const
{
  return _mm_comineq_sd(m_v.xy, d.m_v.xy) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(const WSimdDouble& d) const
{
  return _mm_comige_sd(m_v.xy, d.m_v.xy) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(const WSimdDouble& d) const
{
  return _mm_comigt_sd(m_v.xy, d.m_v.xy) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(const WSimdDouble& d) const
{
  return _mm_comile_sd(m_v.xy, d.m_v.xy) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(const WSimdDouble& d) const
{
  return _mm_comilt_sd(m_v.xy, d.m_v.xy) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(double d) const
{
  return (*this) == WSimdDouble(d);
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(double d) const
{
  return (*this) != WSimdDouble(d);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(double d) const
{
  return (*this) > WSimdDouble(d);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(double d) const
{
  return (*this) >= WSimdDouble(d);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(double d) const
{
  return (*this) < WSimdDouble(d);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(double d) const
{
  return (*this) <= WSimdDouble(d);
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(float f) const
{
  return (*this) == WSimdDouble(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(float f) const
{
  return (*this) != WSimdDouble(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(float f) const
{
  return (*this) > WSimdDouble(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(float f) const
{
  return (*this) >= WSimdDouble(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(float f) const
{
  return (*this) < WSimdDouble(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(float f) const
{
  return (*this) <= WSimdDouble(f);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetReciprocal() const
{
  WSimdDouble result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetInvSqrt() const
{
  WSimdDouble result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, _mm_sqrt_pd(m_v.xy));
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetSqrt() const
{
  WSimdDouble result;
  result.m_v.xy = _mm_sqrt_pd(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Max(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = _mm_max_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Min(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = _mm_min_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Abs() const
{
  WSimdDouble result;
  __m128d sign_mask = _mm_set1_pd(-0.0);
  result.m_v.xy = _mm_andnot_pd(sign_mask, m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}
