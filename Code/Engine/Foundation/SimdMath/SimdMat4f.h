#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

/// A 4x4 matrix class
class W_FOUNDATION_DLL WSimdMat4f
{
public:
  W_DECLARE_POD_TYPE();

  WSimdMat4f();

  /// Returns a zero matrix.
  [[nodiscard]] static WSimdMat4f MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static WSimdMat4f MakeIdentity();

  /// Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static WSimdMat4f MakeFromRowMajorArray(const float* const pData);

  /// Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static WSimdMat4f MakeFromColumnMajorArray(const float* const pData);

  /// Creates a matrix from 4 column vectors.
  [[nodiscard]] static WSimdMat4f MakeFromColumns(const WSimdVec4f& vCol0, const WSimdVec4f& vCol1, const WSimdVec4f& vCol2, const WSimdVec4f& vCol3);

  /// Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static WSimdMat4f MakeFromValues(float f1r1, float f2r1, float f3r1, float f4r1, float f1r2, float f2r2, float f3r2, float f4r2, float f1r3, float f2r3, float f3r3, float f4r3, float f1r4, float f2r4, float f3r4, float f4r4);

  void GetAsArray(float* out_pData, WMatrixLayout::Enum layout) const; // [tested]

public:
  /// Transposes this matrix.
  void Transpose(); // [tested]

  /// Returns the transpose of this matrix.
  WSimdMat4f GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be inverted.
  WResult Invert(const WSimdFloat& fEpsilon = WMath::SmallEpsilon<float>()); // [tested]

  /// Returns the inverse of this matrix.
  WSimdMat4f GetInverse(const WSimdFloat& fEpsilon = WMath::SmallEpsilon<float>()) const; // [tested]

public:
  /// Equality Check with epsilon
  bool IsEqual(const WSimdMat4f& rhs, const WSimdFloat& fEpsilon) const; // [tested]

  /// Checks whether this is an identity matrix.
  bool IsIdentity(const WSimdFloat& fEpsilon = WMath::DefaultEpsilon<float>()) const; // [tested]

  /// Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const;                                                                                                   // [tested]

public:
  void SetRows(const WSimdVec4f& vRow0, const WSimdVec4f& vRow1, const WSimdVec4f& vRow2, const WSimdVec4f& vRow3); // [tested]
  void GetRows(WSimdVec4f& ref_vRow0, WSimdVec4f& ref_vRow1, WSimdVec4f& ref_vRow2, WSimdVec4f& ref_vRow3) const;   // [tested]

public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  [[nodiscard]] WSimdVec4f TransformPosition(const WSimdVec4f& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  [[nodiscard]] WSimdVec4f TransformDirection(const WSimdVec4f& v) const; // [tested]

  [[nodiscard]] WSimdMat4f operator*(const WSimdMat4f& rhs) const;        // [tested]
  void operator*=(const WSimdMat4f& rhs);

  [[nodiscard]] bool operator==(const WSimdMat4f& rhs) const;              // [tested]
  [[nodiscard]] bool operator!=(const WSimdMat4f& rhs) const;              // [tested]

public:
  WSimdVec4f m_col0;
  WSimdVec4f m_col1;
  WSimdVec4f m_col2;
  WSimdVec4f m_col3;
};

/// Multiply two affine matrices, where the 4th row of each is 0,0,0,1.
[[nodiscard]] WSimdMat4f MultiplyAffine(const WSimdMat4f& lhs, const WSimdMat4f& rhs);

#include <Foundation/SimdMath/Implementation/SimdMat4f_inl.h>

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEMat4f_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4f_inl.h>
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONMat4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
