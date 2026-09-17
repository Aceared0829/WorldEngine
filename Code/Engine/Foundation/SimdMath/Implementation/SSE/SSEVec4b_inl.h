#pragma once

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b()
{
  W_CHECK_SIMD_ALIGNMENT(this);
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(bool b)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  WUInt32 mask = b ? 0xFFFFFFFF : 0;
  __m128 tmp = _mm_load_ss((float*)&mask);
  m_v = _mm_shuffle_ps(tmp, tmp, W_TO_SHUFFLE(WSwizzle::XXXX));
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(bool x, bool y, bool z, bool w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) WUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = _mm_load_ps((float*)mask);
}

W_ALWAYS_INLINE WSimdVec4b::WSimdVec4b(WInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::GetComponent() const
{
  return _mm_movemask_ps(_mm_shuffle_ps(m_v, m_v, W_SHUFFLE(N, N, N, N))) != 0;
}

W_ALWAYS_INLINE bool WSimdVec4b::x() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE bool WSimdVec4b::y() const
{
  return GetComponent<1>();
}

W_ALWAYS_INLINE bool WSimdVec4b::z() const
{
  return GetComponent<2>();
}

W_ALWAYS_INLINE bool WSimdVec4b::w() const
{
  return GetComponent<3>();
}

template <WSwizzle::Enum s>
W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::Get() const
{
  return _mm_shuffle_ps(m_v, m_v, W_TO_SHUFFLE(s));
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator&&(const WSimdVec4b& rhs) const
{
  return _mm_and_ps(m_v, rhs.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator||(const WSimdVec4b& rhs) const
{
  return _mm_or_ps(m_v, rhs.m_v);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator!() const
{
  __m128 allTrue = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_setzero_ps());
  return _mm_xor_ps(m_v, allTrue);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator==(const WSimdVec4b& rhs) const
{
  return !(*this != rhs);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::operator!=(const WSimdVec4b& rhs) const
{
  return _mm_xor_ps(m_v, rhs.m_v);
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::AllSet() const
{
  const int mask = W_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == mask;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::AnySet() const
{
  const int mask = W_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) != 0;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4b::NoneSet() const
{
  const int mask = W_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == 0;
}

// static
W_ALWAYS_INLINE WSimdVec4b WSimdVec4b::Select(const WSimdVec4b& vCmp, const WSimdVec4b& vTrue, const WSimdVec4b& vFalse)
{
#if W_SSE_LEVEL >= W_SSE_41
  return _mm_blendv_ps(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  return _mm_or_ps(_mm_andnot_ps(vCmp.m_v, vFalse.m_v), _mm_and_ps(vCmp.m_v, vTrue.m_v));
#endif
}
