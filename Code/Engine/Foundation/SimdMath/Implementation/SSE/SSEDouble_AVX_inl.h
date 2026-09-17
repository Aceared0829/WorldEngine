#pragma once

W_ALWAYS_INLINE WSimdDouble::WSimdDouble()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm256_set1_pd(WMath::NaN<double>());
#endif


}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(float f)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(double(f));
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(double d)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(d);
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  __m128i packedInt32 = _mm_set1_epi32(i);
  m_v = _mm256_cvtepi32_pd(packedInt32);
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WUInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(static_cast<double>(i));
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WAngle a)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(a.GetRadian());
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInternal::QuadFloat v)
{
  m_v = _mm256_cvtps_pd(v);
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
  return _mm_cvtsd_f64(_mm256_castpd256_pd128(m_v));
}


// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeZero()
{
  return _mm256_setzero_pd();
}

// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeNaN()
{
  return _mm256_set1_pd(WMath::NaN<double>());
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator+(const WSimdDouble& d) const
{
  return _mm256_add_pd(m_v, d.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator-(const WSimdDouble& d) const
{
  return _mm256_sub_pd(m_v, d.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator*(const WSimdDouble& d) const
{
  return _mm256_mul_pd(m_v, d.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator/(const WSimdDouble& d) const
{
  return _mm256_div_pd(m_v, d.m_v);
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator+=(const WSimdDouble& d)
{
  m_v = _mm256_add_pd(m_v, d.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator-=(const WSimdDouble& d)
{
  m_v = _mm256_sub_pd(m_v, d.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator*=(const WSimdDouble& d)
{
  m_v = _mm256_mul_pd(m_v, d.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator/=(const WSimdDouble& d)
{
  m_v = _mm256_div_pd(m_v, d.m_v);
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
  return _mm_comieq_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(const WSimdDouble& d) const
{
  return _mm_comineq_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(const WSimdDouble& d) const
{
  return _mm_comige_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(const WSimdDouble& d) const
{
  return _mm_comigt_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(const WSimdDouble& d) const
{
  return _mm_comile_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(const WSimdDouble& d) const
{
  return _mm_comilt_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
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
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetInvSqrt() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetSqrt() const
{
  return _mm256_sqrt_pd(m_v);
}


W_ALWAYS_INLINE WSimdDouble WSimdDouble::Max(const WSimdDouble& f) const
{
  return _mm256_max_pd(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Min(const WSimdDouble& f) const
{
  return _mm256_min_pd(m_v, f.m_v);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Abs() const
{
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0), m_v);
}
