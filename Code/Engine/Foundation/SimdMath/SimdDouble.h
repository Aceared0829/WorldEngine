#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class W_FOUNDATION_DLL WSimdDouble
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor, leaves the data uninitialized.
  WSimdDouble();

  /// Constructs from a given float.
  WSimdDouble(float f);

  /// Constructs from a given double.
  WSimdDouble(double f);

  /// Constructs from a given integer.
  WSimdDouble(WInt32 i);

  /// Constructs from a given integer.
  WSimdDouble(WUInt32 i);

  /// Constructs from given angle.
  WSimdDouble(WAngle a);

  /// Constructs from smaller SIMD
  WSimdDouble(WInternal::QuadFloat v);

  /// Constructs from the internal implementation type.
  WSimdDouble(WInternal::QuadDouble v);

  // /// Returns the stored number as a standard float.
  // operator float() const;

  /// Returns the stored number as a standard double.
  operator double() const;

  /// Creates an WSimdDouble that is initialized to zero.
  [[nodiscard]] static WSimdDouble MakeZero();

  /// Creates an WSimdDouble that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static WSimdDouble MakeNaN();

public:
  WSimdDouble operator+(const WSimdDouble& f) const;
  WSimdDouble operator-(const WSimdDouble& f) const;
  WSimdDouble operator*(const WSimdDouble& f) const;
  WSimdDouble operator/(const WSimdDouble& f) const;

  WSimdDouble& operator+=(const WSimdDouble& f);
  WSimdDouble& operator-=(const WSimdDouble& f);
  WSimdDouble& operator*=(const WSimdDouble& f);
  WSimdDouble& operator/=(const WSimdDouble& f);

  bool IsEqual(const WSimdDouble& rhs, const WSimdDouble& fEpsilon) const;

  bool operator==(const WSimdDouble& f) const;
  bool operator!=(const WSimdDouble& f) const;
  bool operator>(const WSimdDouble& f) const;
  bool operator>=(const WSimdDouble& f) const;
  bool operator<(const WSimdDouble& f) const;
  bool operator<=(const WSimdDouble& f) const;

  bool operator==(double f) const;
  bool operator!=(double f) const;
  bool operator>(double f) const;
  bool operator>=(double f) const;
  bool operator<(double f) const;
  bool operator<=(double f) const;

  bool operator==(float f) const;
  bool operator!=(float f) const;
  bool operator>(float f) const;
  bool operator>=(float f) const;
  bool operator<(float f) const;
  bool operator<=(float f) const;

  WSimdDouble GetReciprocal() const;

  WSimdDouble GetSqrt() const;

  WSimdDouble GetInvSqrt() const;

  [[nodiscard]] WSimdDouble Max(const WSimdDouble& d) const;
  [[nodiscard]] WSimdDouble Min(const WSimdDouble& d) const;
  [[nodiscard]] WSimdDouble Abs() const;

public:
  WInternal::QuadDouble m_v;
};

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  if W_SSE_LEVEL >= W_SSE_AVX
#    include <Foundation/SimdMath/Implementation/SSE/SSEDouble_AVX_inl.h>
#  else
#    include <Foundation/SimdMath/Implementation/SSE/SSEDouble_inl.h>
#  endif
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUDouble_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONDouble_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
