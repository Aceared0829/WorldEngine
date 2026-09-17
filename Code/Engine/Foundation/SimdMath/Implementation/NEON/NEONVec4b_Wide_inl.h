#pragma once

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide()
{
  W_CHECK_SIMD_ALIGNMENT(this);
}

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide(bool b)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vmovq_n_u64(b ? 0xFFFFFFFFFFFFFFFFULL : 0);
  m_v.zw = m_v.xy;
}

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide(bool x, bool y, bool z, bool w)
{
  W_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) WUInt64 mask[4] = {x ? 0xFFFFFFFFFFFFFFFFULL : 0, y ? 0xFFFFFFFFFFFFFFFFULL : 0, z ? 0xFFFFFFFFFFFFFFFFULL : 0, w ? 0xFFFFFFFFFFFFFFFFULL : 0};
  m_v.xy = vld1q_u64(reinterpret_cast<unsigned long *>(mask));
  m_v.zw = vld1q_u64(reinterpret_cast<unsigned long *>(mask + 2));
}

W_ALWAYS_INLINE WSimdVec4bWide::WSimdVec4bWide(WInternal::QuadBoolWide v)
{
  m_v = v;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::GetComponent() const
{
  if constexpr (N < 2)
  {
    return vgetq_lane_u64(m_v.xy, N) != 0;
  }
  else
  {
    return vgetq_lane_u64(m_v.zw, N - 2) != 0;
  }
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
  WSimdVec4bWide result;

  // For simplicity, implement basic shuffle
  // This might need adjustment for full swizzle support
  const uint64_t* src = (const uint64_t*)&m_v;
  uint64_t* dst = (uint64_t*)&result.m_v;

  dst[0] = src[(s >> 12) & 3];
  dst[1] = src[(s >> 8) & 3];
  dst[2] = src[(s >> 4) & 3];
  dst[3] = src[s & 3];

  return result;
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator&&(const WSimdVec4bWide& rhs) const
{
  WSimdVec4bWide result;
  result.m_v.xy = vandq_u64(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = vandq_u64(m_v.zw, rhs.m_v.zw);
  return result;
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator||(const WSimdVec4bWide& rhs) const
{
  WSimdVec4bWide result;
  result.m_v.xy = vorrq_u64(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = vorrq_u64(m_v.zw, rhs.m_v.zw);
  return result;
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator!() const
{
  WSimdVec4bWide result;
  result.m_v.xy = veorq_u64(m_v.xy, vmovq_n_u64(~0ULL));
  result.m_v.zw = veorq_u64(m_v.zw, vmovq_n_u64(~0ULL));
  return result;
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator==(const WSimdVec4bWide& rhs) const
{
  return !(*this != rhs);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::operator!=(const WSimdVec4bWide& rhs) const
{
  WSimdVec4bWide result;
  result.m_v.xy = veorq_u64(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = veorq_u64(m_v.zw, rhs.m_v.zw);
  return result;
}

namespace WInternal
{
  W_ALWAYS_INLINE uint32_t NeonMoveMaskWide(const QuadBoolWide& v)
  {
    uint32_t mask = 0;
    mask |= (vgetq_lane_u64(v.xy, 0) != 0) ? 1 : 0;
    mask |= (vgetq_lane_u64(v.xy, 1) != 0) ? 2 : 0;
    mask |= (vgetq_lane_u64(v.zw, 0) != 0) ? 4 : 0;
    mask |= (vgetq_lane_u64(v.zw, 1) != 0) ? 8 : 0;
    return mask;
  }
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::AllSet() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMaskWide(m_v) & mask) == mask;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::AnySet() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMaskWide(m_v) & mask) != 0;
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4bWide::NoneSet() const
{
  const int mask = W_BIT(N) - 1;
  return (WInternal::NeonMoveMaskWide(m_v) & mask) == 0;
}

// static
W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4bWide::Select(const WSimdVec4bWide& vCmp, const WSimdVec4bWide& vTrue, const WSimdVec4bWide& vFalse)
{
  WSimdVec4bWide result;
  result.m_v.xy = vbslq_u64(vCmp.m_v.xy, vTrue.m_v.xy, vFalse.m_v.xy);
  result.m_v.zw = vbslq_u64(vCmp.m_v.zw, vTrue.m_v.zw, vFalse.m_v.zw);
  return result;
}
