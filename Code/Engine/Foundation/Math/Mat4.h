#pragma once


#include <Foundation/Math/Math.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

/// 4x4 matrix template for 3D transformations and projection operations.
///
/// Matrix layout:
/// ```
/// | m00 m10 m20 m30 |   Column 0: (m00, m01, m02, m03)
/// | m01 m11 m21 m31 |   Column 1: (m10, m11, m12, m13)
/// | m02 m12 m22 m32 |   Column 2: (m20, m21, m22, m23)
/// | m03 m13 m23 m33 |   Column 3: (m30, m31, m32, m33)
/// ```
template <typename Type>
class WMat4Template
{
public:
  W_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is column 0 (with elements of row 0, row 1, row 2, row 3),
  // then column 1, then column 2 and finally column 3

  /// The matrix as a 16-element Type array (column-major)
  Type m_fElementsCM[16];

  W_ALWAYS_INLINE Type& Element(WInt32 iColumn, WInt32 iRow) { return m_fElementsCM[iColumn * 4 + iRow]; }
  W_ALWAYS_INLINE Type Element(WInt32 iColumn, WInt32 iRow) const { return m_fElementsCM[iColumn * 4 + iRow]; }

  // *** Constructors ***
public:
  /// Default Constructor DOES NOT INITIALIZE the matrix, at all.
  WMat4Template(); // [tested]

  /// Copies 16 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ///
  /// \param pData
  ///   The array of Type values from which to set the matrix data.
  /// \param layout
  ///   The layout in which pData stores the matrix. The data will get transposed, if necessary.
  ///   The data should be in column-major format, if you want to prevent unnecessary transposes.
  WMat4Template(const Type* const pData, WMatrixLayout::Enum layout); // [tested]

  /// Sets each element manually: Naming is "column-n row-m"
  WMat4Template(Type c1r1, Type c2r1, Type c3r1, Type c4r1, Type c1r2, Type c2r2, Type c3r2, Type c4r2, Type c1r3, Type c2r3, Type c3r3, Type c4r3,
    Type c1r4, Type c2r4, Type c3r4, Type c4r4); // [tested]

  /// Creates a transformation matrix from a rotation and a translation.
  WMat4Template(const WMat3Template<Type>& mRotation, const WVec3Template<Type>& vTranslation); // [tested]

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    W_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                               "check that all code-paths properly initialize this object.");
  }
#endif

  /// Returns a zero matrix.
  [[nodiscard]] static WMat4Template<Type> MakeZero();

  /// Returns an identity matrix.
  [[nodiscard]] static WMat4Template<Type> MakeIdentity();

  /// Creates a matrix from 16 values that are in row-major layout.
  [[nodiscard]] static WMat4Template<Type> MakeFromRowMajorArray(const Type* const pData);

  /// Creates a matrix from 16 values that are in column-major layout.
  [[nodiscard]] static WMat4Template<Type> MakeFromColumnMajorArray(const Type* const pData);

  /// Creates a matrix from 16 values. Naming is "column-n row-m"
  [[nodiscard]] static WMat4Template<Type> MakeFromValues(Type c1r1, Type c2r1, Type c3r1, Type c4r1, Type c1r2, Type c2r2, Type c3r2, Type c4r2, Type c1r3, Type c2r3, Type c3r3, Type c4r3, Type c1r4, Type c2r4, Type c3r4, Type c4r4);

  /// Creates a matrix with all zero values, except the last column, which is set to x, y, z, 1
  [[nodiscard]] static WMat4Template<Type> MakeTranslation(const WVec3Template<Type>& vTranslation);

  /// Creates a transformation matrix from a rotation and a translation.
  [[nodiscard]] static WMat4Template<Type> MakeTransformation(const WMat3Template<Type>& mRotation, const WVec3Template<Type>& vTranslation);

  /// Creates a matrix with all zero values, except along the diagonal, which is set to x, y, z, 1
  [[nodiscard]] static WMat4Template<Type> MakeScaling(const WVec3Template<Type>& vScale);

  /// Creates a matrix that is a rotation matrix around the X-axis.
  [[nodiscard]] static WMat4Template<Type> MakeRotationX(WAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Y-axis.
  [[nodiscard]] static WMat4Template<Type> MakeRotationY(WAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the Z-axis.
  [[nodiscard]] static WMat4Template<Type> MakeRotationZ(WAngleTemplate<Type> angle);

  /// Creates a matrix that is a rotation matrix around the given axis.
  [[nodiscard]] static WMat4Template<Type> MakeAxisRotation(const WVec3Template<Type>& vAxis, WAngleTemplate<Type> angle);

  /// Copies the 16 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or
  /// row-major format.
  void GetAsArray(Type* out_pData, WMatrixLayout::Enum layout) const; // [tested]

  /// Sets a transformation matrix from a rotation and a translation.
  void SetTransformationMatrix(const WMat3Template<Type>& mRotation, const WVec3Template<Type>& vTranslation); // [tested]

  // *** Special matrix constructors ***
public:
  /// Sets all elements to zero.
  void SetZero(); // [tested]

  /// Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity(); // [tested]

  // *** Common Matrix Operations ***
public:
  /// Transposes this matrix.
  void Transpose(); // [tested]

  /// Returns the transpose of this matrix.
  const WMat4Template<Type> GetTranspose() const; // [tested]

  /// Inverts this matrix. Return value indicates whether the matrix could be inverted.
  WResult Invert(Type fEpsilon = WMath::SmallEpsilon<Type>()); // [tested]

  /// Returns the inverse of this matrix.
  const WMat4Template<Type> GetInverse(Type fEpsilon = WMath::SmallEpsilon<Type>()) const; // [tested]

  // *** Checks ***
public:
  /// Checks whether all elements are zero.
  bool IsZero(Type fEpsilon = WMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Checks whether this is an identity matrix.
  bool IsIdentity(Type fEpsilon = WMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  // *** Special Accessors ***
public:
  /// Returns all 4 components of the i-th row.
  WVec4Template<Type> GetRow(WUInt32 uiRow) const; // [tested]

  /// Sets all 4 components of the i-th row.
  void SetRow(WUInt32 uiRow, const WVec4Template<Type>& vRow); // [tested]

  /// Returns all 4 components of the i-th column.
  WVec4Template<Type> GetColumn(WUInt32 uiColumn) const; // [tested]

  /// Sets all 4 components of the i-th column.
  void SetColumn(WUInt32 uiColumn, const WVec4Template<Type>& vColumn); // [tested]

  /// Returns all 4 components on the diagonal of the matrix.
  WVec4Template<Type> GetDiagonal() const; // [tested]

  /// Sets all 4 components on the diagonal of the matrix.
  void SetDiagonal(const WVec4Template<Type>& vDiag); // [tested]

  /// Returns the first 3 components of the last column.
  const WVec3Template<Type> GetTranslationVector() const; // [tested]

  /// Sets the first 3 components of the last column.
  void SetTranslationVector(const WVec3Template<Type>& v); // [tested]

  /// Sets the 3x3 rotational part of the matrix.
  void SetRotationalPart(const WMat3Template<Type>& mRotation); // [tested]

  /// Returns the 3x3 rotational and scaling part of the matrix.
  const WMat3Template<Type> GetRotationalPart() const; // [tested]

  /// Returns the 3 scaling factors that are encoded in the matrix.
  const WVec3Template<Type> GetScalingFactors() const; // [tested]

  /// Tries to set the three scaling factors in the matrix. Returns W_FAILURE if the matrix columns cannot be normalized and thus no
  /// rescaling is possible.
  WResult SetScalingFactors(const WVec3Template<Type>& vXYZ, Type fEpsilon = WMath::DefaultEpsilon<Type>()); // [tested]

  // *** Operators ***
public:
  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  const WVec3Template<Type> TransformPosition(const WVec3Template<Type>& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  void TransformPosition(WVec3Template<Type>* pV, WUInt32 uiNumVectors, WUInt32 uiStride = sizeof(WVec3Template<Type>)) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an
  /// optimization.
  const WVec3Template<Type> TransformDirection(const WVec3Template<Type>& v) const; // [tested]

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an
  /// optimization.
  void TransformDirection(WVec3Template<Type>* pV, WUInt32 uiNumVectors, WUInt32 uiStride = sizeof(WVec3Template<Type>)) const; // [tested]

  /// Matrix-vector multiplication.
  const WVec4Template<Type> Transform(const WVec4Template<Type>& v) const; // [tested]

  /// Matrix-vector multiplication.
  void Transform(WVec4Template<Type>* pV, WUInt32 uiNumVectors, WUInt32 uiStride = sizeof(WVec4Template<Type>)) const; // [tested]

  /// Component-wise multiplication (commutative)
  void operator*=(Type f); // [tested]

  /// Component-wise division
  void operator/=(Type f); // [tested]

  /// Equality Check
  bool IsIdentical(const WMat4Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon
  bool IsEqual(const WMat4Template<Type>& rhs, Type fEpsilon) const; // [tested]
};

// *** free functions ***

/// Matrix-Matrix multiplication
template <typename Type>
const WMat4Template<Type> operator*(const WMat4Template<Type>& m1, const WMat4Template<Type>& m2); // [tested]

/// Matrix-vector multiplication
template <typename Type>
const WVec3Template<Type> operator*(const WMat4Template<Type>& m, const WVec3Template<Type>& v); // [tested]

/// Matrix-vector multiplication
template <typename Type>
const WVec4Template<Type> operator*(const WMat4Template<Type>& m, const WVec4Template<Type>& v); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const WMat4Template<Type> operator*(const WMat4Template<Type>& m1, Type f); // [tested]

/// Component-wise multiplication (commutative)
template <typename Type>
const WMat4Template<Type> operator*(Type f, const WMat4Template<Type>& m1); // [tested]

/// Component-wise division
template <typename Type>
const WMat4Template<Type> operator/(const WMat4Template<Type>& m1, Type f); // [tested]

/// Adding two matrices (component-wise)
template <typename Type>
const WMat4Template<Type> operator+(const WMat4Template<Type>& m1, const WMat4Template<Type>& m2); // [tested]

/// Subtracting two matrices (component-wise)
template <typename Type>
const WMat4Template<Type> operator-(const WMat4Template<Type>& m1, const WMat4Template<Type>& m2); // [tested]

/// Comparison Operator ==
template <typename Type>
bool operator==(const WMat4Template<Type>& lhs, const WMat4Template<Type>& rhs); // [tested]

/// Comparison Operator !=
template <typename Type>
bool operator!=(const WMat4Template<Type>& lhs, const WMat4Template<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Mat4_inl.h>
