#pragma once

#include <Foundation/SimdMath/SimdDouble.h>
#include <Foundation/SimdMath/SimdVec4b_Wide.h>

/// A 4-component SIMD vector class using doubles
class W_FOUNDATION_DLL WSimdVec4d
{
public:
  W_DECLARE_POD_TYPE();

  WSimdVec4d();

  explicit WSimdVec4d(float fXyzw);
  explicit WSimdVec4d(double fXyzw);

  explicit WSimdVec4d(const WSimdDouble& fXyzw);

  WSimdVec4d(int x, int y, int z, int w = 1);
  WSimdVec4d(float x, float y, float z, float w = 1.0f);
  WSimdVec4d(double x, double y, double z, double w = 1.0);

  WSimdVec4d(WInternal::QuadDouble v);

  /// Creates an WSimdVec4d that is initialized to zero.
  [[nodiscard]] static WSimdVec4d MakeZero();

  /// Creates an WSimdVec4d that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static WSimdVec4d MakeNaN();

  void Set(float fXyzw);
  void Set(double fXyzw);

  void Set(int x, int y, int z, int w);
  void Set(float x, float y, float z, float w);
  void Set(double x, double y, double z, double w);

  void SetX(const WSimdDouble& f);
  void SetY(const WSimdDouble& f);
  void SetZ(const WSimdDouble& f);
  void SetW(const WSimdDouble& f);

  void SetZero();

  /// Loads N floats from pFloats, converts them to double, and stores in the vector.
  /// N must be between 1 and 4. Unused components are set to zero.
  template <int N>
  void Load(const float* pFloats);

  /// Loads N doubles from pDoubles into the vector.
  /// N must be between 1 and 4. Unused components are set to zero.
  template <int N>
  void Load(const double* pDoubles);

  /// Converts the first N components to float and stores them to pFloats.
  /// N must be between 1 and 4.
  template <int N>
  void Store(float* pFloats) const;

  /// Stores the first N components to pDoubles.
  /// N must be between 1 and 4.
  template <int N>
  void Store(double* pDoubles) const;

public:
  WSimdVec4d GetReciprocal() const;

  WSimdVec4d GetSqrt() const;

  WSimdVec4d GetInvSqrt() const;

  template <int N>
  WSimdDouble GetLength() const;

  template <int N>
  WSimdDouble GetInvLength() const;

  template <int N>
  WSimdDouble GetLengthSquared() const;

  template <int N>
  WSimdDouble GetLengthAndNormalize();

  template <int N>
  WSimdVec4d GetNormalized() const;

  template <int N>
  void Normalize();

  /// Normalizes the first N components if the squared length is greater than fEpsilon, otherwise sets the vector to zero.
  template <int N>
  void NormalizeIfNotZero(const WSimdDouble& fEpsilon = WMath::SmallEpsilon<double>());

  /// Normalizes the first N components if the squared length is greater than fEpsilon, otherwise sets the vector to vFallback.
  template <int N>
  void NormalizeIfNotZero(const WSimdVec4d& vFallback, const WSimdDouble& fEpsilon = WMath::SmallEpsilon<double>());

  template <int N>
  bool IsZero() const;

  template <int N>
  bool IsZero(const WSimdDouble& fEpsilon) const;

  template <int N>
  bool IsNormalized(const WSimdDouble& fEpsilon = WMath::HugeEpsilon<double>()) const;

  template <int N>
  bool IsNaN() const;

  template <int N>
  bool IsValid() const;

public:
  template <int N>
  WSimdDouble GetComponent() const;

  WSimdDouble GetComponent(int i) const;

  WSimdDouble x() const;
  WSimdDouble y() const;
  WSimdDouble z() const;
  WSimdDouble w() const;

  template <WSwizzle::Enum s>
  WSimdVec4d Get() const;

  ///x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <WSwizzle::Enum s>
  [[nodiscard]] WSimdVec4d GetCombined(const WSimdVec4d& other) const;

public:
  [[nodiscard]] WSimdVec4d operator-() const;
  [[nodiscard]] WSimdVec4d operator+(const WSimdVec4d& v) const;
  [[nodiscard]] WSimdVec4d operator-(const WSimdVec4d& v) const;

  [[nodiscard]] WSimdVec4d operator*(const WSimdDouble& f) const;
  [[nodiscard]] WSimdVec4d operator/(const WSimdDouble& f) const;

  [[nodiscard]] WSimdVec4d CompMul(const WSimdVec4d& v) const;

  [[nodiscard]] WSimdVec4d CompDiv(const WSimdVec4d& v) const;

  [[nodiscard]] WSimdVec4d CompMin(const WSimdVec4d& rhs) const;
  [[nodiscard]] WSimdVec4d CompMax(const WSimdVec4d& rhs) const;

  [[nodiscard]] WSimdVec4d Abs() const;
  [[nodiscard]] WSimdVec4d Round() const;
  [[nodiscard]] WSimdVec4d Floor() const;
  [[nodiscard]] WSimdVec4d Ceil() const;
  [[nodiscard]] WSimdVec4d Trunc() const;
  [[nodiscard]] WSimdVec4d Fraction() const;

  [[nodiscard]] WSimdVec4d FlipSign(const WSimdVec4bWide& vCmp) const;

  [[nodiscard]] static WSimdVec4d Select(const WSimdVec4bWide& vCmp, const WSimdVec4d& vTrue, const WSimdVec4d& vFalse);

  [[nodiscard]] static WSimdVec4d Lerp(const WSimdVec4d& a, const WSimdVec4d& b, const WSimdVec4d& t);

  WSimdVec4d& operator+=(const WSimdVec4d& v);
  WSimdVec4d& operator-=(const WSimdVec4d& v);

  WSimdVec4d& operator*=(const WSimdDouble& f);
  WSimdVec4d& operator/=(const WSimdDouble& f);

  WSimdVec4bWide IsEqual(const WSimdVec4d& rhs, const WSimdDouble& fEpsilon) const;

  [[nodiscard]] WSimdVec4bWide operator==(const WSimdVec4d& v) const;
  [[nodiscard]] WSimdVec4bWide operator!=(const WSimdVec4d& v) const;
  [[nodiscard]] WSimdVec4bWide operator<=(const WSimdVec4d& v) const;
  [[nodiscard]] WSimdVec4bWide operator<(const WSimdVec4d& v) const;
  [[nodiscard]] WSimdVec4bWide operator>=(const WSimdVec4d& v) const;
  [[nodiscard]] WSimdVec4bWide operator>(const WSimdVec4d& v) const;

  /// Returns the sum of the first N components.
  template <int N>
  [[nodiscard]] WSimdDouble HorizontalSum() const;

  /// Returns the minimum of the first N components.
  template <int N>
  [[nodiscard]] WSimdDouble HorizontalMin() const;

  /// Returns the maximum of the first N components.
  template <int N>
  [[nodiscard]] WSimdDouble HorizontalMax() const;

  /// Returns the dot product of the first N components of this vector and v.
  template <int N>
  [[nodiscard]] WSimdDouble Dot(const WSimdVec4d& v) const;

  ///3D cross product, w is ignored.
  [[nodiscard]] WSimdVec4d CrossRH(const WSimdVec4d& v) const;

  ///Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  [[nodiscard]] WSimdVec4d GetOrthogonalVector() const;

  /// Returns a * b + c
  [[nodiscard]] static WSimdVec4d MulAdd(const WSimdVec4d& a, const WSimdVec4d& b, const WSimdVec4d& c);
  [[nodiscard]] static WSimdVec4d MulAdd(const WSimdVec4d& a, const WSimdDouble& b, const WSimdVec4d& c);

  /// Returns a * b - c
  [[nodiscard]] static WSimdVec4d MulSub(const WSimdVec4d& a, const WSimdVec4d& b, const WSimdVec4d& c);
  [[nodiscard]] static WSimdVec4d MulSub(const WSimdVec4d& a, const WSimdDouble& b, const WSimdVec4d& c);

  /// Returns a vector with the magnitude from vMagnitude and the sign from vSign.
  [[nodiscard]] static WSimdVec4d CopySign(const WSimdVec4d& vMagnitude, const WSimdVec4d& vSign);

public:
  WInternal::QuadDouble m_v;
};

const WSimdVec4d operator*(const WSimdDouble& f, const WSimdVec4d& v);

#include <Foundation/SimdMath/Implementation/SimdVec4d_inl.h>

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4d_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4d_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4d_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif