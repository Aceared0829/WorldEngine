#pragma once

#include <Foundation/SimdMath/SimdSwizzle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class W_FOUNDATION_DLL WSimdVec4b
{
public:
  W_DECLARE_POD_TYPE();

  WSimdVec4b();                               // [tested]
  WSimdVec4b(bool b);                         // [tested]
  WSimdVec4b(bool x, bool y, bool z, bool w); // [tested]
  WSimdVec4b(WInternal::QuadBool b);         // [tested]

public:
  template <int N>
  bool GetComponent() const;                                                                               // [tested]

  bool x() const;                                                                                          // [tested]
  bool y() const;                                                                                          // [tested]
  bool z() const;                                                                                          // [tested]
  bool w() const;                                                                                          // [tested]

  template <WSwizzle::Enum s>
  WSimdVec4b Get() const;                                                                                 // [tested]

public:
  WSimdVec4b operator&&(const WSimdVec4b& rhs) const;                                                    // [tested]
  WSimdVec4b operator||(const WSimdVec4b& rhs) const;                                                    // [tested]
  WSimdVec4b operator!() const;                                                                           // [tested]

  WSimdVec4b operator==(const WSimdVec4b& rhs) const;                                                    // [tested]
  WSimdVec4b operator!=(const WSimdVec4b& rhs) const;                                                    // [tested]

  template <int N = 4>
  bool AllSet() const;                                                                                     // [tested]

  template <int N = 4>
  bool AnySet() const;                                                                                     // [tested]

  template <int N = 4>
  bool NoneSet() const;                                                                                    // [tested]

  static WSimdVec4b Select(const WSimdVec4b& vCmp, const WSimdVec4b& vTrue, const WSimdVec4b& vFalse); // [tested]

public:
  WInternal::QuadBool m_v;
};

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4b_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4b_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4b_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
