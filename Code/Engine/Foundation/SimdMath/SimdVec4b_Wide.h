#pragma once

#include <Foundation/SimdMath/SimdSwizzle.h>
#include <Foundation/SimdMath/SimdTypes.h>


class W_FOUNDATION_DLL WSimdVec4bWide
{
public:
  W_DECLARE_POD_TYPE();

  WSimdVec4bWide();                               // [tested]
  WSimdVec4bWide(bool b);                         // [tested]
  WSimdVec4bWide(bool x, bool y, bool z, bool w); // [tested]
  WSimdVec4bWide(WInternal::QuadBoolWide b);         // [tested]

public:
  template <int N>
  bool GetComponent() const;                                                                               // [tested]

  bool x() const;                                                                                          // [tested]
  bool y() const;                                                                                          // [tested]
  bool z() const;                                                                                          // [tested]
  bool w() const;                                                                                          // [tested]

  template <WSwizzle::Enum s>
  WSimdVec4bWide Get() const;                                                                                 // [tested]

public:
  WSimdVec4bWide operator&&(const WSimdVec4bWide& rhs) const;                                                    // [tested]
  WSimdVec4bWide operator||(const WSimdVec4bWide& rhs) const;                                                    // [tested]
  WSimdVec4bWide operator!() const;                                                                           // [tested]

  WSimdVec4bWide operator==(const WSimdVec4bWide& rhs) const;                                                    // [tested]
  WSimdVec4bWide operator!=(const WSimdVec4bWide& rhs) const;                                                    // [tested]

  template <int N = 4>
  bool AllSet() const;                                                                                     // [tested]

  template <int N = 4>
  bool AnySet() const;                                                                                     // [tested]

  template <int N = 4>
  bool NoneSet() const;                                                                                    // [tested]

  static WSimdVec4bWide Select(const WSimdVec4bWide& vCmp, const WSimdVec4bWide& vTrue, const WSimdVec4bWide& vFalse); // [tested]

public:
  WInternal::QuadBoolWide m_v;
};









#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4b_Wide_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4b_Wide_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4b_Wide_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
