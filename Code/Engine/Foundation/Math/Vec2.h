#pragma once

#include <Foundation/Math/Math.h>
// inclusion order matters to avoid circular dependencies
#include <Foundation/Math/Angle.h>

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
#  define W_VEC2_CHECK_FOR_NAN(obj) (obj)->AssertNotNaN();
#else
#  define W_VEC2_CHECK_FOR_NAN(obj)
#endif

/// A 2-component vector class.
template <typename Type>
class WVec2Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  using ComponentType = Type;


  // *** Data ***
public:
  Type x;
  Type y;

  // *** Constructors ***
public:
  /// default-constructed vector is uninitialized (for speed)
  WVec2Template(); // [tested]

  /// Initializes the vector with x,y
  WVec2Template(Type x, Type y); // [tested]

  /// Initializes all components with xy
  explicit WVec2Template(Type v); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// Returns a vector with all components set to Not-a-Number (NaN).
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static const WVec2Template<Type> MakeNaN() { return WVec2Template<Type>(WMath::NaN<Type>()); }

  /// Static function that returns a zero-vector.
  [[nodiscard]] static constexpr WVec2Template<Type> MakeZero() { return WVec2Template(0); } // [tested]

  /// Returns a vector initialized to x,y
  [[nodiscard]] static WVec2Template<Type> Make(Type x, Type y) { return WVec2Template<Type>(x, y); } // [tested]

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    W_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                               "check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversions ***
public:
  /// Returns an WVec3Template with x,y from this vector and z set by the parameter.
  const WVec3Template<Type> GetAsVec3(Type z) const; // [tested]

  /// Returns an WVec4Template with x,y from this vector and z and w set by the parameters.
  const WVec4Template<Type> GetAsVec4(Type z, Type w) const; // [tested]

  /// Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// Sets all components to this value.
  void Set(Type xy); // [tested]

  /// Sets the vector to these values.
  void Set(Type x, Type y); // [tested]

  /// Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// Returns the length of the vector.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetLength() const; // [tested]

  /// Returns the length between this position and rhs.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetDistanceTo(const WVec2Template<Type>& rhs) const;

  /// Returns the squared length between this position and rhs.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetSquaredDistanceTo(const WVec2Template<Type>& rhs) const;

  /// Tries to rescale the vector to the given length. If the vector is too close to zero, W_FAILURE is returned and the vector is
  /// set to zero.
  W_DECLARE_IF_FLOAT_TYPE
  WResult SetLength(Type fNewLength, Type fEpsilon = WMath::DefaultEpsilon<Type>()); // [tested]

  /// Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetLengthAndNormalize(); // [tested]

  /// Returns a normalized version of this vector, leaves the vector itself unchanged.
  W_DECLARE_IF_FLOAT_TYPE
  const WVec2Template<Type> GetNormalized() const; // [tested]

  /// Normalizes this vector.
  W_DECLARE_IF_FLOAT_TYPE
  void Normalize(); // [tested]

  /// Tries to normalize this vector. If the vector is too close to zero, W_FAILURE is returned and the vector is set to the given
  /// fallback value.
  W_DECLARE_IF_FLOAT_TYPE
  WResult NormalizeIfNotZero(const WVec2Template<Type>& vFallback = WVec2Template<Type>(1, 0), Type fEpsilon = WMath::DefaultEpsilon<Type>()); // [tested]

  /// Returns, whether this vector is (0, 0).
  bool IsZero() const; // [tested]

  /// Returns, whether this vector is (0, 0) within a certain threshold.
  bool IsZero(Type fEpsilon) const; // [tested]

  /// Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  W_DECLARE_IF_FLOAT_TYPE
  bool IsNormalized(Type fEpsilon = WMath::HugeEpsilon<Type>()) const; // [tested]

  /// Returns true, if any of x or y is NaN
  bool IsNaN() const; // [tested]

  /// Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


  // *** Operators ***
public:
  /// Returns the negation of this vector.
  const WVec2Template<Type> operator-() const; // [tested]

  /// Adds cc component-wise to this vector
  void operator+=(const WVec2Template<Type>& vCc); // [tested]

  /// Subtracts cc component-wise from this vector
  void operator-=(const WVec2Template<Type>& vCc); // [tested]

  /// Multiplies all components of this vector with f
  void operator*=(Type f); // [tested]

  /// Divides all components of this vector by f
  void operator/=(Type f); // [tested]

  /// Equality Check (bitwise)
  bool IsIdentical(const WVec2Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon
  bool IsEqual(const WVec2Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// Returns the positive angle between *this and rhs.
  WAngleTemplate<Type> GetAngleBetween(const WVec2Template<Type>& rhs) const; // [tested]

  /// Returns the Dot-product of the two vectors (commutative, order does not matter)
  Type Dot(const WVec2Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise minimum of *this and rhs
  const WVec2Template<Type> CompMin(const WVec2Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise maximum of *this and rhs
  const WVec2Template<Type> CompMax(const WVec2Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise clamped value of *this between low and high.
  const WVec2Template<Type> CompClamp(const WVec2Template<Type>& vLow, const WVec2Template<Type>& vHigh) const; // [tested]

  /// Returns the component-wise multiplication of *this and rhs
  const WVec2Template<Type> CompMul(const WVec2Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise division of *this and rhs
  const WVec2Template<Type> CompDiv(const WVec2Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  const WVec2Template<Type> Abs() const; // [tested]


  // *** Other common operations ***
public:
  /// Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  W_DECLARE_IF_FLOAT_TYPE
  void MakeOrthogonalTo(const WVec2Template<Type>& vNormal); // [tested]

  /// Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  const WVec2Template<Type> GetOrthogonalVector() const; // [tested]

  /// Returns this vector reflected at vNormal.
  W_DECLARE_IF_FLOAT_TYPE
  const WVec2Template<Type> GetReflectedVector(const WVec2Template<Type>& vNormal) const; // [tested]
};

// *** Operators ***

/// Component-wise addition.
template <typename Type>
const WVec2Template<Type> operator+(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2); // [tested]

/// Component-wise subtraction.
template <typename Type>
const WVec2Template<Type> operator-(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2); // [tested]

/// Returns a scaled vector.
template <typename Type>
const WVec2Template<Type> operator*(Type f, const WVec2Template<Type>& v); // [tested]

/// Returns a scaled vector.
template <typename Type>
const WVec2Template<Type> operator*(const WVec2Template<Type>& v, Type f); // [tested]

/// Returns a scaled vector.
template <typename Type>
const WVec2Template<Type> operator/(const WVec2Template<Type>& v, Type f); // [tested]

/// Returns true, if both vectors are identical.
template <typename Type>
bool operator==(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2); // [tested]

/// Returns true, if both vectors are not identical.
template <typename Type>
bool operator!=(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2); // [tested]

/// Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2);

#include <Foundation/Math/Implementation/Vec2_inl.h>
