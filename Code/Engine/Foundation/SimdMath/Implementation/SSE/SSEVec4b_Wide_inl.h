#pragma once

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide()
{
  W_CHECK_SIMD_ALIGNMENT(this);
}

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide(bool b)
{
  W_CHECK_SIMD_ALIGNMENT(this);
#if W_SSE_LEVEL >= W_SSE_AVX

  alignas(32) WUInt64 mask[4] = {b ? 0xFFFFFFFFFFFFFFFFULL : 0, b ? 0xFFFFFFFFFFFFFFFFULL : 0, b ? 0xFFFFFFFFFFFFFFFFULL : 0, b ? 0xFFFFFFFFFFFFFFFFULL : 0};
  m_v = _mm256_load_pd((double*)mask);


#else

  __m128d val;
  if(b)
  {
    alignas(16) WUInt64 trueVal[2] = {0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL};
    val = _mm_load_pd(((double*)(&trueVal)));
  }
  else
  {
    val = _mm_set1_pd(0.0);
  }

  m_v.xy = val;
  m_v.zw = val;
#endif
}

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide(bool x, bool y, bool z, bool w)
{
  W_CHECK_SIMD_ALIGNMENT(this);
#if W_SSE_LEVEL >= W_SSE_AVX
  alignas(32) WUInt64 mask[4] = {x ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, y ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, z ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, w ? 0xFFFFFFFFFFFFFFFFULL : 0ULL};
  m_v = _mm256_load_pd((double*)mask);
#else
  alignas(16) WUInt64 mask[4] = {x ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, y ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, z ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, w ? 0xFFFFFFFFFFFFFFFFULL : 0ULL};
  m_v.xy = _mm_load_pd((double*)&mask[0]);
  m_v.zw = _mm_load_pd((double*)&mask[2]);
#endif
}

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide(WInternal::QuadBoolWide v)
{
  m_v = v;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::GetComponent() const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  return (_mm256_movemask_pd(m_v) & (1 << N)) != 0;
#else
  if constexpr (N < 2)
  {
    __m128d shuffled = _mm_shuffle_pd(m_v.xy, m_v.xy, N == 0 ? 0 : 3);
    return (_mm_movemask_pd(shuffled) & 1) != 0;
  }
  else
  {
    __m128d shuffled = _mm_shuffle_pd(m_v.zw, m_v.zw, N == 2 ? 0 : 3);
    return (_mm_movemask_pd(shuffled) & 1) != 0;
  }
#endif
}

W_ALWAYS_INLINE bool WSimdVec4bWide::x() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE bool WSimdVec4bWide::y() const
{
  return GetComponent<1>();
}

W_ALWAYS_INLINE bool WSimdVec4bWide::z() const
{
  return GetComponent<2>();
}

W_ALWAYS_INLINE bool WSimdVec4bWide::w() const
{
  return GetComponent<3>();
}


template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::Get() const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  WSimdVec4bWide result;
  W_WIDE_SHUFFLE_AVX1(m_v, m_v, W_TO_SHUFFLE(s), result.m_v);
  return result;
#else
  WSimdVec4bWide result;
  W_WIDE_SHUFFLE_SSE(m_v.xy, m_v.zw, m_v.xy, m_v.zw, W_TO_SHUFFLE(s), result.m_v.xy, result.m_v.zw);
  return result;
#endif
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator&&(const WSimdVec4bWide& rhs) const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  return _mm256_and_pd(m_v, rhs.m_v);
#else
  WSimdVec4bWide result;
  result.m_v.xy = _mm_and_pd(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = _mm_and_pd(m_v.zw, rhs.m_v.zw);
  return result;
#endif
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator||(const WSimdVec4bWide& rhs) const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  return _mm256_or_pd(m_v, rhs.m_v);
#else
  WSimdVec4bWide result;
  result.m_v.xy = _mm_or_pd(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = _mm_or_pd(m_v.zw, rhs.m_v.zw);
  return result;
#endif
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator!() const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  __m256d allTrue = _mm256_cmp_pd(_mm256_setzero_pd(), _mm256_setzero_pd(), _CMP_EQ_OQ);
  return _mm256_xor_pd(m_v, allTrue);
#else
  __m128d allTrue = _mm_cmpeq_pd(_mm_setzero_pd(), _mm_setzero_pd());
  WSimdVec4bWide result;
  result.m_v.xy = _mm_xor_pd(m_v.xy, allTrue);
  result.m_v.zw = _mm_xor_pd(m_v.zw, allTrue);
  return result;
#endif
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator==(const WSimdVec4bWide& rhs) const
{
  return !(*this != rhs);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator!=(const WSimdVec4bWide& rhs) const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  return _mm256_xor_pd(m_v, rhs.m_v);
#else
  WSimdVec4bWide result;
  result.m_v.xy = _mm_xor_pd(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = _mm_xor_pd(m_v.zw, rhs.m_v.zw);
  return result;
#endif
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::AllSet() const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  const int mask = W_BIT(N) - 1;
  return (_mm256_movemask_pd(m_v) & mask) == mask;
#else
  int xy_bits = _mm_movemask_pd(m_v.xy);
  int zw_bits = _mm_movemask_pd(m_v.zw);
  int combined = (zw_bits << 2) | xy_bits;
  const int mask = W_BIT(N) - 1;
  return (combined & mask) == mask;
#endif
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::AnySet() const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  const int mask = W_BIT(N) - 1;
  return (_mm256_movemask_pd(m_v) & mask) != 0;
#else
  int xy_bits = _mm_movemask_pd(m_v.xy);
  int zw_bits = _mm_movemask_pd(m_v.zw);
  int combined = (zw_bits << 2) | xy_bits;
  const int mask = W_BIT(N) - 1;
  return (combined & mask) != 0;
#endif
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::NoneSet() const
{
#if W_SSE_LEVEL >= W_SSE_AVX
  const int mask = W_BIT(N) - 1;
  return (_mm256_movemask_pd(m_v) & mask) == 0;
#else
  int xy_bits = _mm_movemask_pd(m_v.xy);
  int zw_bits = _mm_movemask_pd(m_v.zw);
  int combined = (zw_bits << 2) | xy_bits;
  const int mask = W_BIT(N) - 1;
  return (combined & mask) == 0;
#endif
}

// static
W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::Select(const WSimdVec4bWide& vCmp, const WSimdVec4bWide& vTrue, const WSimdVec4bWide& vFalse)
{
#if W_SSE_LEVEL >= W_SSE_AVX
  return _mm256_blendv_pd(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  WSimdVec4bWide result;
#if W_SSE_LEVEL >= W_SSE_41
  result.m_v.xy = _mm_blendv_pd(vFalse.m_v.xy, vTrue.m_v.xy, vCmp.m_v.xy);
  result.m_v.zw = _mm_blendv_pd(vFalse.m_v.zw, vTrue.m_v.zw, vCmp.m_v.zw);
#else
  result.m_v.xy = _mm_or_pd(_mm_andnot_pd(vCmp.m_v.xy, vFalse.m_v.xy), _mm_and_pd(vCmp.m_v.xy, vTrue.m_v.xy));
  result.m_v.zw = _mm_or_pd(_mm_andnot_pd(vCmp.m_v.zw, vFalse.m_v.zw), _mm_and_pd(vCmp.m_v.zw, vTrue.m_v.zw));
#endif
  return result;
#endif
}
