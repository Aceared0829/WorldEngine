#pragma once

#if W_ENABLED(W_COMPILER_MSVC_PURE)
#  include <intrin.h>
#endif

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i()
{
  W_CHECK_SIMD_ALIGNMENT(this);

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInt32 iXyzw)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_epi32(iXyzw);
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInt32 x, WInt32 y, WInt32 z, WInt32 w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

W_ALWAYS_INLINE WSimdVec4i::WSimdVec4i(WInternal::QuadInt v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::MakeZero()
{
  return _mm_setzero_si128();
}

W_ALWAYS_INLINE void WSimdVec4i::Set(WInt32 iXyzw)
{
  m_v = _mm_set1_epi32(iXyzw);
}

W_ALWAYS_INLINE void WSimdVec4i::Set(WInt32 x, WInt32 y, WInt32 z, WInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

W_ALWAYS_INLINE void WSimdVec4i::SetZero()
{
  m_v = _mm_setzero_si128();
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<1>(const WInt32* pInts)
{
  m_v = _mm_loadu_si32(pInts);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<2>(const WInt32* pInts)
{
  m_v = _mm_loadu_si64(pInts);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<3>(const WInt32* pInts)
{
  m_v = _mm_setr_epi32(pInts[0], pInts[1], pInts[2], 0);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Load<4>(const WInt32* pInts)
{
  m_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pInts));
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<1>(WInt32* pInts) const
{
  _mm_storeu_si32(pInts, m_v);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<2>(WInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<3>(WInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
  _mm_storeu_si32(pInts + 2, _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(m_v), _mm_castsi128_ps(m_v))));
}

template <>
W_ALWAYS_INLINE void WSimdVec4i::Store<4>(WInt32* pInts) const
{
  _mm_storeu_si128(reinterpret_cast<__m128i*>(pInts), m_v);
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4i::ToFloat() const
{
  return _mm_cvtepi32_ps(m_v);
}

// static
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Truncate(const WSimdVec4f& f)
{
  return _mm_cvttps_epi32(f.m_v);
}

template <int N>
W_ALWAYS_INLINE WInt32 WSimdVec4i::GetComponent() const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return ((WInt32*)&m_v)[N];
  // return m_v.m128i_i32[N];
#endif
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
  return _mm_shuffle_epi32(m_v, W_TO_SHUFFLE(s));
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::GetCombined(const WSimdVec4i& other) const
{
  return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(m_v), _mm_castsi128_ps(other.m_v), W_TO_SHUFFLE(s)));
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator-() const
{
  return _mm_sub_epi32(_mm_setzero_si128(), m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator+(const WSimdVec4i& v) const
{
  return _mm_add_epi32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator-(const WSimdVec4i& v) const
{
  return _mm_sub_epi32(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMul(const WSimdVec4i& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_mullo_epi32(m_v, v.m_v);
#else
  __m128i tmp1 = _mm_mul_epu32(m_v, v.m_v);
  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(m_v, 4), _mm_srli_si128(v.m_v, 4));
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, W_SHUFFLE(0, 2, 0, 0)), _mm_shuffle_epi32(tmp2, W_SHUFFLE(0, 2, 0, 0)));
#endif
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompDiv(const WSimdVec4i& v) const
{
#if W_ENABLED(W_COMPILER_MSVC_PURE)
  return _mm_div_epi32(m_v, v.m_v);
#else
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
#endif
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator|(const WSimdVec4i& v) const
{
  return _mm_or_si128(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator&(const WSimdVec4i& v) const
{
  return _mm_and_si128(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator^(const WSimdVec4i& v) const
{
  return _mm_xor_si128(m_v, v.m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator~() const
{
  __m128i ones = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_xor_si128(ones, m_v);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator<<(WUInt32 uiShift) const
{
  return _mm_slli_epi32(m_v, uiShift);
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::operator>>(WUInt32 uiShift) const
{
  return _mm_srai_epi32(m_v, uiShift);
}

W_FORCE_INLINE WSimdVec4i WSimdVec4i::operator<<(const WSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (WUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] << b[i];
  }

  WSimdVec4i r;
  r.Load<4>(a);
  return r;
}

W_FORCE_INLINE WSimdVec4i WSimdVec4i::operator>>(const WSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (WUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] >> b[i];
  }

  WSimdVec4i r;
  r.Load<4>(a);
  return r;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator+=(const WSimdVec4i& v)
{
  m_v = _mm_add_epi32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator-=(const WSimdVec4i& v)
{
  m_v = _mm_sub_epi32(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator|=(const WSimdVec4i& v)
{
  m_v = _mm_or_si128(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator&=(const WSimdVec4i& v)
{
  m_v = _mm_and_si128(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator^=(const WSimdVec4i& v)
{
  m_v = _mm_xor_si128(m_v, v.m_v);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator<<=(WUInt32 uiShift)
{
  m_v = _mm_slli_epi32(m_v, uiShift);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i& WSimdVec4i::operator>>=(WUInt32 uiShift)
{
  m_v = _mm_srai_epi32(m_v, uiShift);
  return *this;
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMin(const WSimdVec4i& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_min_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmplt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::CompMax(const WSimdVec4i& v) const
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_max_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmpgt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Abs() const
{
#if W_SSE_LEVEL >= W_SSE_31
  return _mm_abs_epi32(m_v);
#else
  __m128i negMask = _mm_cmplt_epi32(m_v, _mm_setzero_si128());
  __m128i neg = _mm_sub_epi32(_mm_setzero_si128(), m_v);
  return _mm_or_si128(_mm_and_si128(negMask, neg), _mm_andnot_si128(negMask, m_v));
#endif
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator==(const WSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpeq_epi32(m_v, v.m_v));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator!=(const WSimdVec4i& v) const
{
  return !(*this == v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator<=(const WSimdVec4i& v) const
{
  return !(*this > v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator<(const WSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmplt_epi32(m_v, v.m_v));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator>=(const WSimdVec4i& v) const
{
  return !(*this < v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4i::operator>(const WSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpgt_epi32(m_v, v.m_v));
}

// static
W_ALWAYS_INLINE WSimdVec4i WSimdVec4i::Select(const WSimdVec4b& vCmp, const WSimdVec4i& vTrue, const WSimdVec4i& vFalse)
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(vFalse.m_v), _mm_castsi128_ps(vTrue.m_v), vCmp.m_v));
#else
  return _mm_castps_si128(_mm_or_ps(_mm_andnot_ps(vCmp.m_v, _mm_castsi128_ps(vFalse.m_v)), _mm_and_ps(vCmp.m_v, _mm_castsi128_ps(vTrue.m_v))));
#endif
}

// not needed atm
#if 0
void WSimdVec4i::Transpose(WSimdVec4i& v0, WSimdVec4i& v1, WSimdVec4i& v2, WSimdVec4i& v3)
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
