#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class W_FOUNDATION_DLL WSimdFloat
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor, leaves the data uninitialized.
  WSimdFloat(); // [tested]

  /// Constructs from a given float.
  WSimdFloat(float f); // [tested]

  /// Constructs from a given integer.
  WSimdFloat(WInt32 i); // [tested]

  /// Constructs from a given integer.
  WSimdFloat(WUInt32 i); // [tested]

  /// Constructs from given angle.
  WSimdFloat(WAngle a); // [tested]

  /// Constructs from the internal implementation type.
  WSimdFloat(WInternal::QuadFloat v); // [tested]

  /// Returns the stored number as a standard float.
  operator float() const; // [tested]

  /// Creates an WSimdFloat that is initialized to zero.
  [[nodiscard]] static WSimdFloat MakeZero(); // [tested]

  /// Creates an WSimdFloat that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static WSimdFloat MakeNaN();        // [tested]

public:
  WSimdFloat operator+(const WSimdFloat& f) const; // [tested]
  WSimdFloat operator-(const WSimdFloat& f) const; // [tested]
  WSimdFloat operator*(const WSimdFloat& f) const; // [tested]
  WSimdFloat operator/(const WSimdFloat& f) const; // [tested]

  WSimdFloat& operator+=(const WSimdFloat& f);     // [tested]
  WSimdFloat& operator-=(const WSimdFloat& f);     // [tested]
  WSimdFloat& operator*=(const WSimdFloat& f);     // [tested]
  WSimdFloat& operator/=(const WSimdFloat& f);     // [tested]

  bool IsEqual(const WSimdFloat& rhs, const WSimdFloat& fEpsilon) const;

  bool operator==(const WSimdFloat& f) const;               // [tested]
  bool operator!=(const WSimdFloat& f) const;               // [tested]
  bool operator>(const WSimdFloat& f) const;                // [tested]
  bool operator>=(const WSimdFloat& f) const;               // [tested]
  bool operator<(const WSimdFloat& f) const;                // [tested]
  bool operator<=(const WSimdFloat& f) const;               // [tested]

  bool operator==(float f) const;                            // [tested]
  bool operator!=(float f) const;                            // [tested]
  bool operator>(float f) const;                             // [tested]
  bool operator>=(float f) const;                            // [tested]
  bool operator<(float f) const;                             // [tested]
  bool operator<=(float f) const;                            // [tested]

  template <WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdFloat GetReciprocal() const;                         // [tested]

  template <WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdFloat GetSqrt() const;                               // [tested]

  template <WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdFloat GetInvSqrt() const;                            // [tested]

  [[nodiscard]] WSimdFloat Max(const WSimdFloat& f) const; // [tested]
  [[nodiscard]] WSimdFloat Min(const WSimdFloat& f) const; // [tested]
  [[nodiscard]] WSimdFloat Abs() const;                     // [tested]

public:
  WInternal::QuadFloat m_v;
};

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEFloat_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUFloat_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONFloat_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
