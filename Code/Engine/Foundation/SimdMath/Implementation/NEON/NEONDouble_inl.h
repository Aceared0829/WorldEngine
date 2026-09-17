#pragma once

W_ALWAYS_INLINE WSimdDouble::WSimdDouble()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v.xy = vdupq_n_f64(WMath::NaN<double>());
  m_v.zw = m_v.xy;
#endif
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(float f)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(static_cast<double>(f));
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(double f)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(f);
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(static_cast<double>(i));
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WUInt32 i)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(static_cast<double>(i));
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WAngle a)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(a.GetRadian());
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInternal::QuadFloat v)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  // Convert first float to double and broadcast
  float x = vgetq_lane_f32(v, 0);
  m_v.xy = vdupq_n_f64(static_cast<double>(x));
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdDouble::WSimdDouble(WInternal::QuadDouble v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdDouble::operator double() const
{
  return vgetq_lane_f64(m_v.xy, 0);
}

// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeZero()
{
  WSimdDouble result;
  result.m_v.xy = vdupq_n_f64(0.0);
  result.m_v.zw = result.m_v.xy;
  return result;
}

// static
W_ALWAYS_INLINE WSimdDouble WSimdDouble::MakeNaN()
{
  WSimdDouble result;
  result.m_v.xy = vdupq_n_f64(WMath::NaN<double>());
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator+(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = vaddq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator-(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = vsubq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator*(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = vmulq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::operator/(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = vdivq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator+=(const WSimdDouble& f)
{
  m_v.xy = vaddq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator-=(const WSimdDouble& f)
{
  m_v.xy = vsubq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator*=(const WSimdDouble& f)
{
  m_v.xy = vmulq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE WSimdDouble& WSimdDouble::operator/=(const WSimdDouble& f)
{
  m_v.xy = vdivq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

W_ALWAYS_INLINE bool WSimdDouble::IsEqual(const WSimdDouble& rhs, const WSimdDouble& fEpsilon) const
{
  // Match SSE implementation: (this >= rhs - eps) && (this <= rhs + eps)
  WSimdDouble minusEps = rhs - fEpsilon;
  WSimdDouble plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(const WSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) == vgetq_lane_f64(f.m_v.xy, 0);
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(const WSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) != vgetq_lane_f64(f.m_v.xy, 0);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(const WSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) >= vgetq_lane_f64(f.m_v.xy, 0);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(const WSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) > vgetq_lane_f64(f.m_v.xy, 0);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(const WSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) <= vgetq_lane_f64(f.m_v.xy, 0);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(const WSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) < vgetq_lane_f64(f.m_v.xy, 0);
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) == f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) != f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) > f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) >= f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) < f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) <= f;
}

W_ALWAYS_INLINE bool WSimdDouble::operator==(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) == static_cast<double>(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator!=(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) != static_cast<double>(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) > static_cast<double>(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator>=(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) >= static_cast<double>(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) < static_cast<double>(f);
}

W_ALWAYS_INLINE bool WSimdDouble::operator<=(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) <= static_cast<double>(f);
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetReciprocal() const
{
  WSimdDouble result;
  result.m_v.xy = vdivq_f64(vdupq_n_f64(1.0), m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetSqrt() const
{
  WSimdDouble result;
  result.m_v.xy = vsqrtq_f64(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::GetInvSqrt() const
{
  WSimdDouble result;
  result.m_v.xy = vdivq_f64(vdupq_n_f64(1.0), vsqrtq_f64(m_v.xy));
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Max(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = vmaxq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Min(const WSimdDouble& f) const
{
  WSimdDouble result;
  result.m_v.xy = vminq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

W_ALWAYS_INLINE WSimdDouble WSimdDouble::Abs() const
{
  WSimdDouble result;
  result.m_v.xy = vabsq_f64(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}
