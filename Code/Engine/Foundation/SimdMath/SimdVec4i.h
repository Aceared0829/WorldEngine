#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

class WSimdVec4u;

/// A SIMD 4-component vector class of signed 32b integers
class W_FOUNDATION_DLL WSimdVec4i
{
public:
  W_DECLARE_POD_TYPE();

  WSimdVec4i();                                               // [tested]

  explicit WSimdVec4i(WInt32 iXyzw);                         // [tested]

  WSimdVec4i(WInt32 x, WInt32 y, WInt32 z, WInt32 w = 1); // [tested]

  WSimdVec4i(WInternal::QuadInt v);                          // [tested]

  /// Creates an WSimdVec4i that is initialized to zero.
  [[nodiscard]] static WSimdVec4i MakeZero();                     // [tested]

  void Set(WInt32 iXyzw);                                         // [tested]

  void Set(WInt32 x, WInt32 y, WInt32 z, WInt32 w);            // [tested]

  void SetZero();                                                  // [tested]

  template <int N>
  void Load(const WInt32* pInts);                                 // [tested]

  template <int N>
  void Store(WInt32* pInts) const;                                // [tested]

public:
  explicit WSimdVec4i(const WSimdVec4u& u);                      // [tested]

public:
  WSimdVec4f ToFloat() const;                                     // [tested]

  [[nodiscard]] static WSimdVec4i Truncate(const WSimdVec4f& f); // [tested]

public:
  template <int N>
  WInt32 GetComponent() const; // [tested]

  WInt32 x() const;            // [tested]
  WInt32 y() const;            // [tested]
  WInt32 z() const;            // [tested]
  WInt32 w() const;            // [tested]

  template <WSwizzle::Enum s>
  WSimdVec4i Get() const;      // [tested]

  ///x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <WSwizzle::Enum s>
  [[nodiscard]] WSimdVec4i GetCombined(const WSimdVec4i& other) const;                                                 // [tested]

public:
  [[nodiscard]] WSimdVec4i operator-() const;                                                                           // [tested]
  [[nodiscard]] WSimdVec4i operator+(const WSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4i operator-(const WSimdVec4i& v) const;                                                       // [tested]

  [[nodiscard]] WSimdVec4i CompMul(const WSimdVec4i& v) const;                                                         // [tested]
  [[nodiscard]] WSimdVec4i CompDiv(const WSimdVec4i& v) const;                                                         // [tested]

  [[nodiscard]] WSimdVec4i operator|(const WSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4i operator&(const WSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4i operator^(const WSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4i operator~() const;                                                                           // [tested]

  [[nodiscard]] WSimdVec4i operator<<(WUInt32 uiShift) const;                                                          // [tested]
  [[nodiscard]] WSimdVec4i operator>>(WUInt32 uiShift) const;                                                          // [tested]
  [[nodiscard]] WSimdVec4i operator<<(const WSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] WSimdVec4i operator>>(const WSimdVec4i& v) const;                                                      // [tested]

  WSimdVec4i& operator+=(const WSimdVec4i& v);                                                                         // [tested]
  WSimdVec4i& operator-=(const WSimdVec4i& v);                                                                         // [tested]

  WSimdVec4i& operator|=(const WSimdVec4i& v);                                                                         // [tested]
  WSimdVec4i& operator&=(const WSimdVec4i& v);                                                                         // [tested]
  WSimdVec4i& operator^=(const WSimdVec4i& v);                                                                         // [tested]

  WSimdVec4i& operator<<=(WUInt32 uiShift);                                                                            // [tested]
  WSimdVec4i& operator>>=(WUInt32 uiShift);                                                                            // [tested]

  [[nodiscard]] WSimdVec4i CompMin(const WSimdVec4i& v) const;                                                         // [tested]
  [[nodiscard]] WSimdVec4i CompMax(const WSimdVec4i& v) const;                                                         // [tested]
  [[nodiscard]] WSimdVec4i Abs() const;                                                                                 // [tested]

  [[nodiscard]] WSimdVec4b operator==(const WSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] WSimdVec4b operator!=(const WSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] WSimdVec4b operator<=(const WSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] WSimdVec4b operator<(const WSimdVec4i& v) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4b operator>=(const WSimdVec4i& v) const;                                                      // [tested]
  [[nodiscard]] WSimdVec4b operator>(const WSimdVec4i& v) const;                                                       // [tested]

  [[nodiscard]] static WSimdVec4i Select(const WSimdVec4b& vCmp, const WSimdVec4i& vTrue, const WSimdVec4i& vFalse); // [tested]

public:
  WInternal::QuadInt m_v;
};

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4i_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4i_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4i_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
