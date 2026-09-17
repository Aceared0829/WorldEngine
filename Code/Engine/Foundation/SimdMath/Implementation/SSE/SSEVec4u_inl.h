#pragma once

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WUInt32 uiXyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_epi32(uiXyzw);
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

W_ALWAYS_INLINE WSimdVec4u::WSimdVec4u(WInternal::QuadInt v)
{
  m_v = v;
}

W_ALWAYS_INLINE void WSimdVec4u::Set(WUInt32 uiXyzw)
{
  m_v = _mm_set1_epi32(uiXyzw);
}

W_ALWAYS_INLINE void WSimdVec4u::Set(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4u::SetZero()
{
  m_v = _mm_setzero_si128();
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
  __m128 two16 = _mm_set1_ps((float)0x10000); // 2^16
  __m128i high = _mm_srli_epi32(m_v, 16);
  __m128i low = _mm_srli_epi32(_mm_slli_epi32(m_v, 16), 16);
  __m128 fHigh = _mm_mul_ps(_mm_cvtepi32_ps(high), two16);
  __m128 fLow = _mm_cvtepi32_ps(low);

  return _mm_add_ps(fHigh, fLow);
}

// static
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::Truncate(const WSimdVec4f& f)
{
  alignas(16) const float fmax[4] = {2.14748364e+009f, 2.14748364e+009f, 2.14748364e+009f, 2.14748364e+009f};
  alignas(16) const float fmax_unsigned[4] = {4.29496729e+009f, 4.29496729e+009f, 4.29496729e+009f, 4.29496729e+009f};
  __m128i zero = _mm_setzero_si128();
  __m128i mask = _mm_cmpgt_epi32(_mm_castps_si128(f.m_v), zero);
  __m128 min = _mm_and_ps(_mm_castsi128_ps(mask), f.m_v);
  __m128 max = _mm_min_ps(min, _mm_load_ps(fmax_unsigned)); // clamped in 0 - 4.29496729+009

  __m128 diff = _mm_sub_ps(max, _mm_load_ps(fmax));
  mask = _mm_cmpgt_epi32(_mm_castps_si128(diff), zero);
  diff = _mm_and_ps(_mm_castsi128_ps(mask), diff);

  __m128i res1 = _mm_cvttps_epi32(diff);
  __m128i res2 = _mm_cvttps_epi32(max);
  return _mm_add_epi32(res1, res2);
}

template <int N>
W_ALWAYS_INLINE WUInt32 WSimdVec4u::GetComponent() const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return ((WUInt32*)&m_v)[N];
  // return m_v.m128i_i32[N];
#endif
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
  return _mm_shuffle_epi32(m_v, W_TO_SHUFFLE(s));
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator+(const WSimdVec4u& v) const
{
  return _mm_add_epi32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator-(const WSimdVec4u& v) const
{
  return _mm_sub_epi32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMul(const WSimdVec4u& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_mullo_epi32(m_v, v.m_v);
#else
  W_ASSERT_NOT_IMPLEMENTED; // not sure whether this code works so better assert
  __m128i tmp1 = _mm_mul_epu32(m_v, v.m_v);
  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(m_v, 4), _mm_srli_si128(v.m_v, 4));
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, W_SHUFFLE(0, 2, 0, 0)), _mm_shuffle_epi32(tmp2, W_SHUFFLE(0, 2, 0, 0)));
#endif
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator|(const WSimdVec4u& v) const
{
  return _mm_or_si128(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator&(const WSimdVec4u& v) const
{
  return _mm_and_si128(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator^(const WSimdVec4u& v) const
{
  return _mm_xor_si128(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator~() const
{
  __m128i ones = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_xor_si128(ones, m_v);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator<<(WUInt32 uiShift) const
{
  return _mm_slli_epi32(m_v, uiShift);
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::operator>>(WUInt32 uiShift) const
{
  return _mm_srli_epi32(m_v, uiShift);
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator+=(const WSimdVec4u& v)
{
  m_v = _mm_add_epi32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator-=(const WSimdVec4u& v)
{
  m_v = _mm_sub_epi32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator|=(const WSimdVec4u& v)
{
  m_v = _mm_or_si128(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator&=(const WSimdVec4u& v)
{
  m_v = _mm_and_si128(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator^=(const WSimdVec4u& v)
{
  m_v = _mm_xor_si128(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator<<=(WUInt32 uiShift)
{
  m_v = _mm_slli_epi32(m_v, uiShift);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u& WSimdVec4u::operator>>=(WUInt32 uiShift)
{
  m_v = _mm_srli_epi32(m_v, uiShift);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMin(const WSimdVec4u& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_min_epu32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmplt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::CompMax(const WSimdVec4u& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_max_epu32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmpgt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator==(const WSimdVec4u& v) const
{
  return _mm_castsi128_ps(_mm_cmpeq_epi32(m_v, v.m_v));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator!=(const WSimdVec4u& v) const
{
  return !(*this == v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator<=(const WSimdVec4u& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  __m128i minValue = _mm_min_epu32(m_v, v.m_v);
  return _mm_castsi128_ps(_mm_cmpeq_epi32(minValue, m_v));
#else
  return !(*this > v);
#endif
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator<(const WSimdVec4u& v) const
{
  __m128i signBit = _mm_set1_epi32(0x80000000);
  __m128i a = _mm_sub_epi32(m_v, signBit);
  __m128i b = _mm_sub_epi32(v.m_v, signBit);
  return _mm_castsi128_ps(_mm_cmplt_epi32(a, b));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator>=(const WSimdVec4u& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  __m128i maxValue = _mm_max_epu32(m_v, v.m_v);
  return _mm_castsi128_ps(_mm_cmpeq_epi32(maxValue, m_v));
#else
  return !(*this < v);
#endif
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4u::operator>(const WSimdVec4u& v) const
{
  __m128i signBit = _mm_set1_epi32(0x80000000);
  __m128i a = _mm_sub_epi32(m_v, signBit);
  __m128i b = _mm_sub_epi32(v.m_v, signBit);
  return _mm_castsi128_ps(_mm_cmpgt_epi32(a, b));
}

// static
W_ALWAYS_INLINE WSimdVec4u WSimdVec4u::MakeZero()
{
  return _mm_setzero_si128();
}

// not needed atm
#if 0
void WSimdVec4u::Transpose(WSimdVec4u& v0, WSimdVec4u& v1, WSimdVec4u& v2, WSimdVec4u& v3)
{
  __m128i T0 = _mm_unpacklo_epi32(v0.m_v, v1.m_v);
  __m128i T1 = _mm_unpacklo_epi32(v2.m_v, v3.m_v);
  __m128i T2 = _mm_unpackhi_epi32(v0.m_v, v1.m_v);
  __m128i T3 = _mm_unpackhi_epi32(v2.m_v, v3.m_v);

  v0.m_v = _mm_unpacklo_epi64(T0, T1);
  v1.m_v = _mm_unpackhi_epi64(T0, T1);
  v2.m_v = _mm_unpacklo_epi64(T2, T3);
  v3.m_v = _mm_unpackhi_epi64(T2, T3);
}
#endif
