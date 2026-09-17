#pragma once

#include <Foundation/SimdMath/SimdVec4d.h>

/// A 4x4 matrix class
class W_FOUNDATION_DLL WSimdMat4d
{
public:
  W_DECLARE_POD_TYPE();

  WSimdMat4d();

  /// Returns a zero matrix.
  [[nodiscard]] static WSimdMat4d MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static WSimdMat4d MakeIdentity();

  /// Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static WSimdMat4d MakeFromRowMajorArray(const double* const pData);

  /// Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static WSimdMat4d MakeFromColumnMajorArray(const double* const pData);

  /// Creates a matrix from 4 column vectors.
  [[nodiscard]] static WSimdMat4d MakeFromColumns(const WSimdVec4d& vCol0, const WSimdVec4d& vCol1, const WSimdVec4d& vCol2, const WSimdVec4d& vCol3);

  /// Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static WSimdMat4d MakeFromValues(double f1r1, double f2r1, double f3r1, double f4r1, double f1r2, double f2r2, double f3r2, double f4r2, double f1r3, double f2r3, double f3r3, double f4r3, double f1r4, double f2r4, double f3r4, double f4r4);

  void GetAsArray(double* out_pData, WMatrixLayout::Enum layout) const; // [tested]

public:
  /// Transposes this matrix.
  void Transpose(); // [tested]

  /// Returns the transpose of this matrix.
  WSimdMat4d GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be inverted.
  WResult Invert(const WSimdDouble& fEpsilon = WMath::SmallEpsilon<double>()); // [tested]

  /// Returns the inverse of this matrix.
  WSimdMat4d GetInverse(const WSimdDouble& fEpsilon = WMath::SmallEpsilon<double>()) const; // [tested]

public:
  /// Equality Check with epsilon
  bool IsEqual(const WSimdMat4d& rhs, const WSimdDouble& fEpsilon) const; // [tested]

  /// Checks whether this is an identity matrix.
  bool IsIdentity(const WSimdDouble& fEpsilon = WMath::DefaultEpsilon<double>()) const; // [tested]

  /// Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const;                                                                                                   // [tested]

public:
  void SetRows(const WSimdVec4d& vRow0, const WSimdVec4d& vRow1, const WSimdVec4d& vRow2, const WSimdVec4d& vRow3); // [tested]
  void GetRows(WSimdVec4d& ref_vRow0, WSimdVec4d& ref_vRow1, WSimdVec4d& ref_vRow2, WSimdVec4d& ref_vRow3) const;   // [tested]

public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  [[nodiscard]] WSimdVec4d TransformPosition(const WSimdVec4d& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  [[nodiscard]] WSimdVec4d TransformDirection(const WSimdVec4d& v) const; // [tested]

  [[nodiscard]] WSimdMat4d operator*(const WSimdMat4d& rhs) const;        // [tested]
  void operator*=(const WSimdMat4d& rhs);

  [[nodiscard]] bool operator==(const WSimdMat4d& rhs) const;              // [tested]
  [[nodiscard]] bool operator!=(const WSimdMat4d& rhs) const;              // [tested]

public:
  WSimdVec4d m_col0;
  WSimdVec4d m_col1;
  WSimdVec4d m_col2;
  WSimdVec4d m_col3;
};

/// Multiply two affine matrices, where the 4th row of each is 0,0,0,1.
[[nodiscard]] WSimdMat4d MultiplyAffine(const WSimdMat4d& lhs, const WSimdMat4d& rhs);

#include <Foundation/SimdMath/Implementation/SimdMat4d_inl.h>

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEMat4d_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4d_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONMat4d_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
