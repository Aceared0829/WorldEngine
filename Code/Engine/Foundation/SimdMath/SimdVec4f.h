#pragma once

#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4b.h>

/// A 4-component SIMD vector class
class W_FOUNDATION_DLL WSimdVec4f
{
public:
  W_DECLARE_POD_TYPE();

  WSimdVec4f();                                          // [tested]

  explicit WSimdVec4f(float fXyzw);                      // [tested]

  explicit WSimdVec4f(const WSimdFloat& fXyzw);         // [tested]

  WSimdVec4f(float x, float y, float z, float w = 1.0f); // [tested]

  WSimdVec4f(WInternal::QuadFloat v);                   // [tested]

  /// Creates an WSimdVec4f that is initialized to zero.
  [[nodiscard]] static WSimdVec4f MakeZero(); // [tested]

  /// Creates an WSimdVec4f that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static WSimdVec4f MakeNaN();   // [tested]

  void Set(float fXyzw);                        // [tested]

  void Set(float x, float y, float z, float w); // [tested]

  void SetX(const WSimdFloat& f);              // [tested]
  void SetY(const WSimdFloat& f);              // [tested]
  void SetZ(const WSimdFloat& f);              // [tested]
  void SetW(const WSimdFloat& f);              // [tested]

  void SetZero();                               // [tested]

  /// Loads N floats from pFloats into the vector.
  /// N must be between 1 and 4. Unused components are set to zero.
  template <int N>
  void Load(const float* pFloats);              // [tested]

  /// Stores the first N components to pFloats.
  /// N must be between 1 and 4.
  template <int N>
  void Store(float* pFloats) const;             // [tested]

public:
  template <WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdVec4f GetReciprocal() const;                                                                                  // [tested]

  template <WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdVec4f GetSqrt() const;                                                                                        // [tested]

  template <WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdVec4f GetInvSqrt() const;                                                                                     // [tested]

  template <int N, WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdFloat GetLength() const;                                                                                      // [tested]

  template <int N, WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdFloat GetInvLength() const;                                                                                   // [tested]

  template <int N>
  WSimdFloat GetLengthSquared() const;                                                                               // [tested]

  template <int N, WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdFloat GetLengthAndNormalize();                                                                                // [tested]

  template <int N, WMathAcc::Enum acc = WMathAcc::FULL>
  WSimdVec4f GetNormalized() const;                                                                                  // [tested]

  template <int N, WMathAcc::Enum acc = WMathAcc::FULL>
  void Normalize();                                                                                                   // [tested]

  /// Normalizes the first N components if the squared length is greater than fEpsilon, otherwise sets the vector to zero.
  template <int N, WMathAcc::Enum acc = WMathAcc::FULL>
  void NormalizeIfNotZero(const WSimdFloat& fEpsilon = WMath::SmallEpsilon<float>());                               // [tested]

  /// Normalizes the first N components if the squared length is greater than fEpsilon, otherwise sets the vector to vFallback.
  template <int N, WMathAcc::Enum acc = WMathAcc::FULL>
  void NormalizeIfNotZero(const WSimdVec4f& vFallback, const WSimdFloat& fEpsilon = WMath::SmallEpsilon<float>()); // [tested]

  template <int N>
  bool IsZero() const;                                                                                                // [tested]

  template <int N>
  bool IsZero(const WSimdFloat& fEpsilon) const;                                                                     // [tested]

  template <int N>
  bool IsNormalized(const WSimdFloat& fEpsilon = WMath::HugeEpsilon<float>()) const;                                // [tested]

  template <int N>
  bool IsNaN() const;                                                                                                 // [tested]

  template <int N>
  bool IsValid() const;                                                                                               // [tested]

public:
  /// Creates an WSimdFloat with all elements set to the given component
  template <int N>
  WSimdFloat GetComponent() const;      // [tested]

  /// Creates an WSimdFloat with all elements set to the given component
  WSimdFloat GetComponent(int i) const; // [tested]

  WSimdFloat x() const;                 // [tested]
  WSimdFloat y() const;                 // [tested]
  WSimdFloat z() const;                 // [tested]
  WSimdFloat w() const;                 // [tested]

  template <WSwizzle::Enum s>
  WSimdVec4f Get() const;               // [tested]

  ///x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <WSwizzle::Enum s>
  [[nodiscard]] WSimdVec4f GetCombined(const WSimdVec4f& other) const;                                                 // [tested]

public:
  [[nodiscard]] WSimdVec4f operator-() const;                                                                           // [tested]
  [[nodiscard]] WSimdVec4f operator+(const WSimdVec4f& v) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4f operator-(const WSimdVec4f& v) const;                                                       // [tested]

  [[nodiscard]] WSimdVec4f operator*(const WSimdFloat& f) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4f operator/(const WSimdFloat& f) const;                                                       // [tested]

  [[nodiscard]] WSimdVec4f CompMul(const WSimdVec4f& v) const;                                                         // [tested]

  template <WMathAcc::Enum acc = WMathAcc::FULL>
  [[nodiscard]] WSimdVec4f CompDiv(const WSimdVec4f& v) const;                                                         // [tested]

  [[nodiscard]] WSimdVec4f CompMin(const WSimdVec4f& rhs) const;                                                       // [tested]
  [[nodiscard]] WSimdVec4f CompMax(const WSimdVec4f& rhs) const;                                                       // [tested]

  [[nodiscard]] WSimdVec4f Abs() const;                                                                                 // [tested]
  [[nodiscard]] WSimdVec4f Round() const;                                                                               // [tested]
  [[nodiscard]] WSimdVec4f Floor() const;                                                                               // [tested]
  [[nodiscard]] WSimdVec4f Ceil() const;                                                                                // [tested]
  [[nodiscard]] WSimdVec4f Trunc() const;                                                                               // [tested]
  [[nodiscard]] WSimdVec4f Fraction() const;                                                                            // [tested]

  [[nodiscard]] WSimdVec4f FlipSign(const WSimdVec4b& vCmp) const;                                                     // [tested]

  [[nodiscard]] static WSimdVec4f Select(const WSimdVec4b& vCmp, const WSimdVec4f& vTrue, const WSimdVec4f& vFalse); // [tested]

  [[nodiscard]] static WSimdVec4f Lerp(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& t);

  WSimdVec4f& operator+=(const WSimdVec4f& v);                                  // [tested]
  WSimdVec4f& operator-=(const WSimdVec4f& v);                                  // [tested]

  WSimdVec4f& operator*=(const WSimdFloat& f);                                  // [tested]
  WSimdVec4f& operator/=(const WSimdFloat& f);                                  // [tested]

  WSimdVec4b IsEqual(const WSimdVec4f& rhs, const WSimdFloat& fEpsilon) const; // [tested]

  [[nodiscard]] WSimdVec4b operator==(const WSimdVec4f& v) const;               // [tested]
  [[nodiscard]] WSimdVec4b operator!=(const WSimdVec4f& v) const;               // [tested]
  [[nodiscard]] WSimdVec4b operator<=(const WSimdVec4f& v) const;               // [tested]
  [[nodiscard]] WSimdVec4b operator<(const WSimdVec4f& v) const;                // [tested]
  [[nodiscard]] WSimdVec4b operator>=(const WSimdVec4f& v) const;               // [tested]
  [[nodiscard]] WSimdVec4b operator>(const WSimdVec4f& v) const;                // [tested]

  /// Returns the sum of the first N components.
  template <int N>
  [[nodiscard]] WSimdFloat HorizontalSum() const;                                // [tested]

  /// Returns the minimum of the first N components.
  template <int N>
  [[nodiscard]] WSimdFloat HorizontalMin() const;                                // [tested]

  /// Returns the maximum of the first N components.
  template <int N>
  [[nodiscard]] WSimdFloat HorizontalMax() const;                                // [tested]

  /// Returns the dot product of the first N components of this vector and v.
  template <int N>
  [[nodiscard]] WSimdFloat Dot(const WSimdVec4f& v) const;                      // [tested]

  ///3D cross product, w is ignored.
  [[nodiscard]] WSimdVec4f CrossRH(const WSimdVec4f& v) const; // [tested]

  ///Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  [[nodiscard]] WSimdVec4f GetOrthogonalVector() const;                                                     // [tested]

  /// Returns a * b + c
  [[nodiscard]] static WSimdVec4f MulAdd(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c); // [tested]
  [[nodiscard]] static WSimdVec4f MulAdd(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c); // [tested]

  /// Returns a * b - c
  [[nodiscard]] static WSimdVec4f MulSub(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& c); // [tested]
  [[nodiscard]] static WSimdVec4f MulSub(const WSimdVec4f& a, const WSimdFloat& b, const WSimdVec4f& c); // [tested]

  /// Returns a vector with the magnitude from vMagnitude and the sign from vSign.
  [[nodiscard]] static WSimdVec4f CopySign(const WSimdVec4f& vMagnitude, const WSimdVec4f& vSign);        // [tested]

public:
  WInternal::QuadFloat m_v;
};

const WSimdVec4f operator*(const WSimdFloat& f, const WSimdVec4f& v);

#include <Foundation/SimdMath/Implementation/SimdVec4f_inl.h>

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4f_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4f_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
