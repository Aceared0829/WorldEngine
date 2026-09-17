#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>

/// A 4-component vector class.
template <typename Type>
class WVec4Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  Type x, y, z, w;

  // *** Constructors ***
public:
  /// Default-constructed vector is uninitialized (for speed)
  WVec4Template(); // [tested]

  /// Initializes the vector with x,y,z,w
  WVec4Template(Type x, Type y, Type z, Type w); // [tested]

  /// Initializes the vector from a vec3 and a float.
  WVec4Template(WVec3Template<Type> vXyz, Type w);

  /// Initializes all 4 components with xyzw
  explicit WVec4Template(Type v); // [tested]
  // no copy-constructor and operator= since the default-generated ones will be faster

  /// Returns a vector with all components set to Not-a-Number (NaN).
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec4Template<Type> MakeNaN() { return WVec4Template<Type>(WMath::NaN<Type>()); }

  /// Returns a vector with all components set to zero.
  [[nodiscard]] static WVec4Template<Type> MakeZero() { return WVec4Template<Type>(0); } // [tested]

  /// Returns a vector initialized to x,y,z,w
  [[nodiscard]] static WVec4Template<Type> Make(Type x, Type y, Type z, Type w) { return WVec4Template<Type>(x, y, z, w); } // [tested]

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    W_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                               "check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversions ***
public:
  /// Returns an WVec2Template with x and y from this vector.
  const WVec2Template<Type> GetAsVec2() const; // [tested]

  /// Returns an WVec3Template with x,y and z from this vector.
  const WVec3Template<Type> GetAsVec3() const; // [tested]

  /// Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// Sets all 4 components to this value.
  void Set(Type xyzw); // [tested]

  /// Sets the vector to these values.
  void Set(Type x, Type y, Type z, Type w); // [tested]

  /// Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// Returns the length of the vector.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetLength() const; // [tested]

  /// Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetLengthAndNormalize(); // [tested]

  /// Returns a normalized version of this vector, leaves the vector itself unchanged.
  W_DECLARE_IF_FLOAT_TYPE
  const WVec4Template<Type> GetNormalized() const; // [tested]

  /// Normalizes this vector.
  W_DECLARE_IF_FLOAT_TYPE
  void Normalize(); // [tested]

  /// Tries to normalize this vector. If the vector is too close to zero, W_FAILURE is returned and the vector is set to the given
  /// fallback value.
  W_DECLARE_IF_FLOAT_TYPE
  WResult NormalizeIfNotZero(const WVec4Template<Type>& vFallback = WVec4Template<Type>(1, 0, 0, 0), Type fEpsilon = WMath::SmallEpsilon<Type>()); // [tested]

  /// Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero() const; // [tested]

  /// Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero(Type fEpsilon) const; // [tested]

  /// Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  W_DECLARE_IF_FLOAT_TYPE
  bool IsNormalized(Type fEpsilon = WMath::HugeEpsilon<Type>()) const; // [tested]

  /// Returns true, if any of x, y, z or w is NaN.
  bool IsNaN() const; // [tested]

  /// Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


  // *** Operators ***
public:
  /// Returns the negation of this vector.
  const WVec4Template<Type> operator-() const; // [tested]

  /// Adds cc component-wise to this vector.
  void operator+=(const WVec4Template<Type>& vCc); // [tested]

  /// Subtracts cc component-wise from this vector.
  void operator-=(const WVec4Template<Type>& vCc); // [tested]

  /// Multiplies all components of this vector with f.
  void operator*=(Type f); // [tested]

  /// Divides all components of this vector by f.
  void operator/=(Type f); // [tested]

  /// Equality Check (bitwise).
  bool IsIdentical(const WVec4Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon.
  bool IsEqual(const WVec4Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// Returns the dot-product of the two vectors (commutative, order does not matter).
  Type Dot(const WVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise minimum of *this and rhs.
  const WVec4Template<Type> CompMin(const WVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise maximum of *this and rhs.
  const WVec4Template<Type> CompMax(const WVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise clamped value of *this between low and high.
  const WVec4Template<Type> CompClamp(const WVec4Template<Type>& vLow, const WVec4Template<Type>& vHigh) const; // [tested]

  /// Returns the component-wise multiplication of *this and rhs.
  const WVec4Template<Type> CompMul(const WVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise division of *this and rhs.
  const WVec4Template<Type> CompDiv(const WVec4Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  const WVec4Template<Type> Abs() const; // [tested]
};

// *** Operators ***

template <typename Type>
const WVec4Template<Type> operator+(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2); // [tested]

template <typename Type>
const WVec4Template<Type> operator-(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2); // [tested]


template <typename Type>
const WVec4Template<Type> operator*(Type f, const WVec4Template<Type>& v); // [tested]

template <typename Type>
const WVec4Template<Type> operator*(const WVec4Template<Type>& v, Type f); // [tested]


template <typename Type>
const WVec4Template<Type> operator/(const WVec4Template<Type>& v, Type f); // [tested]


template <typename Type>
bool operator==(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2); // [tested]

template <typename Type>
bool operator!=(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2); // [tested]

/// Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2); // [tested]

#include <Foundation/Math/Implementation/Vec4_inl.h>
