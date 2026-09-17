#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

/// A SIMD 4-component vector class of unsigned 32b integers
class W_FOUNDATION_DLL WSimdVec4u
{
public:
  W_DECLARE_POD_TYPE();

  WSimdVec4u();                                                   // [tested]

  explicit WSimdVec4u(WUInt32 uiXyzw);                           // [tested]

  WSimdVec4u(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w = 1); // [tested]

  WSimdVec4u(WInternal::QuadUInt v);                             // [tested]

  /// Creates an WSimdVec4u that is initialized to zero.
  [[nodiscard]] static WSimdVec4u MakeZero();                     // [tested]

  void Set(WUInt32 uiXyzw);                                       // [tested]

  void Set(WUInt32 x, WUInt32 y, WUInt32 z, WUInt32 w);        // [tested]

  void SetZero();                                                  // [tested]

public:
  explicit WSimdVec4u(const WSimdVec4i& i);                      // [tested]

public:
  WSimdVec4f ToFloat() const;                                     // [tested]

  [[nodiscard]] static WSimdVec4u Truncate(const WSimdVec4f& f); // [tested]

public:
  template <int N>
  WUInt32 GetComponent() const;                                   // [tested]

  WUInt32 x() const;                                              // [tested]
  WUInt32 y() const;                                              // [tested]
  WUInt32 z() const;                                              // [tested]
  WUInt32 w() const;                                              // [tested]

  template <WSwizzle::Enum s>
  WSimdVec4u Get() const;                                         // [tested]

public:
  [[nodiscard]] WSimdVec4u operator+(const WSimdVec4u& v) const; // [tested]
  [[nodiscard]] WSimdVec4u operator-(const WSimdVec4u& v) const; // [tested]

  [[nodiscard]] WSimdVec4u CompMul(const WSimdVec4u& v) const;   // [tested]

  [[nodiscard]] WSimdVec4u operator|(const WSimdVec4u& v) const; // [tested]
  [[nodiscard]] WSimdVec4u operator&(const WSimdVec4u& v) const; // [tested]
  [[nodiscard]] WSimdVec4u operator^(const WSimdVec4u& v) const; // [tested]
  [[nodiscard]] WSimdVec4u operator~() const;                     // [tested]

  [[nodiscard]] WSimdVec4u operator<<(WUInt32 uiShift) const;    // [tested]
  [[nodiscard]] WSimdVec4u operator>>(WUInt32 uiShift) const;    // [tested]

  WSimdVec4u& operator+=(const WSimdVec4u& v);                   // [tested]
  WSimdVec4u& operator-=(const WSimdVec4u& v);                   // [tested]

  WSimdVec4u& operator|=(const WSimdVec4u& v);                   // [tested]
  WSimdVec4u& operator&=(const WSimdVec4u& v);                   // [tested]
  WSimdVec4u& operator^=(const WSimdVec4u& v);                   // [tested]

  WSimdVec4u& operator<<=(WUInt32 uiShift);                      // [tested]
  WSimdVec4u& operator>>=(WUInt32 uiShift);                      // [tested]

  [[nodiscard]] WSimdVec4u CompMin(const WSimdVec4u& v) const;   // [tested]
  [[nodiscard]] WSimdVec4u CompMax(const WSimdVec4u& v) const;   // [tested]

  WSimdVec4b operator==(const WSimdVec4u& v) const;              // [tested]
  WSimdVec4b operator!=(const WSimdVec4u& v) const;              // [tested]
  WSimdVec4b operator<=(const WSimdVec4u& v) const;              // [tested]
  WSimdVec4b operator<(const WSimdVec4u& v) const;               // [tested]
  WSimdVec4b operator>=(const WSimdVec4u& v) const;              // [tested]
  WSimdVec4b operator>(const WSimdVec4u& v) const;               // [tested]

public:
  WInternal::QuadUInt m_v;
};

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4u_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4u_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4u_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
