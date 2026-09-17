#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// A 3-component vector class.
template <typename Type>
class WVec3Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  Type x, y, z;

  // *** Constructors ***
public:
  /// default-constructed vector is uninitialized (for speed)
  WVec3Template<Type>(); // [tested]

  /// Initializes the vector with x,y,z
  WVec3Template<Type>(Type x, Type y, Type z); // [tested]

  /// Initializes all 3 components with xyz
  explicit WVec3Template<Type>(Type v); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// Returns a vector with all components set to Not-a-Number (NaN).
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type> MakeNaN() { return WVec3Template<Type>(WMath::NaN<Type>()); }

  /// Returns a vector with all components set to zero.
  [[nodiscard]] static WVec3Template<Type> MakeZero() { return WVec3Template<Type>(0); } // [tested]

  /// Returns a vector initialized to the X unit vector (1, 0, 0).
  [[nodiscard]] static WVec3Template<Type> MakeAxisX() { return WVec3Template<Type>(1, 0, 0); } // [tested]

  /// Returns a vector initialized to the Y unit vector (0, 1, 0).
  [[nodiscard]] static WVec3Template<Type> MakeAxisY() { return WVec3Template<Type>(0, 1, 0); } // [tested]

  /// Returns a vector initialized to the Z unit vector (0, 0, 1).
  [[nodiscard]] static WVec3Template<Type> MakeAxisZ() { return WVec3Template<Type>(0, 0, 1); } // [tested]

  /// Returns a vector initialized to x,y,z
  [[nodiscard]] static WVec3Template<Type> Make(Type x, Type y, Type z) { return WVec3Template<Type>(x, y, z); } // [tested]

  /// Returns a vector that is orthogonal to vDirection.
  ///
  /// Uses the vBasis1 and vBasis2 vectors as candidates to create the orthogonal vector from. The basis that is less similar to the direction
  /// will be used to to compute the orthogonal vector.
  ///
  /// All input vectors must be normalized.
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type> MakeOrthogonalVector(const WVec3Template<Type>& vDirection, const WVec3Template<Type>& vBasis1 = MakeAxisX(), const WVec3Template<Type>& vBasis2 = MakeAxisY()); // [tested]

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

  /// Returns an WVec4Template with x,y,z from this vector and w set to the parameter.
  const WVec4Template<Type> GetAsVec4(Type w) const; // [tested]

  /// Returns an WVec4Template with x,y,z from this vector and w set 1.
  const WVec4Template<Type> GetAsPositionVec4() const; // [tested]

  /// Returns an WVec4Template with x,y,z from this vector and w set 0.
  const WVec4Template<Type> GetAsDirectionVec4() const; // [tested]

  /// Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// Sets all 3 components to this value.
  void Set(Type xyz); // [tested]

  /// Sets the vector to these values.
  void Set(Type x, Type y, Type z); // [tested]

  /// Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// Returns the length of the vector.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetLength() const; // [tested]

  /// Returns the length between this position and rhs.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetDistanceTo(const WVec3Template<Type>& rhs) const;

  /// Returns the squared length between this position and rhs.
  W_DECLARE_IF_FLOAT_TYPE
  Type GetSquaredDistanceTo(const WVec3Template<Type>& rhs) const;

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
  [[nodiscard]] const WVec3Template<Type> GetNormalized() const; // [tested]

  /// Normalizes this vector.
  W_DECLARE_IF_FLOAT_TYPE
  void Normalize(); // [tested]

  /// Tries to normalize this vector. If the vector is too close to zero, W_FAILURE is returned and the vector is set to the given
  /// fallback value.
  W_DECLARE_IF_FLOAT_TYPE
  WResult NormalizeIfNotZero(const WVec3Template<Type>& vFallback = WVec3Template<Type>(1, 0, 0), Type fEpsilon = WMath::SmallEpsilon<Type>()); // [tested]

  /// Returns, whether this vector is (0, 0, 0).
  bool IsZero() const; // [tested]

  /// Returns, whether this vector is (0, 0, 0) within a given epsilon.
  bool IsZero(Type fEpsilon) const; // [tested]

  /// Returns, whether the squared length of this vector is very close to 1 within the given epsilon
  W_DECLARE_IF_FLOAT_TYPE
  bool IsNormalized(Type fEpsilon = WMath::HugeEpsilon<Type>()) const; // [tested]

  /// Returns true, if any of x, y or z is NaN
  bool IsNaN() const; // [tested]

  /// Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


  // *** Operators ***
public:
  /// Returns the negation of this vector.
  const WVec3Template<Type> operator-() const; // [tested]

  /// Adds rhs component-wise to this vector
  void operator+=(const WVec3Template<Type>& rhs); // [tested]

  /// Subtracts rhs component-wise from this vector
  void operator-=(const WVec3Template<Type>& rhs); // [tested]

  /// Multiplies rhs component-wise to this vector
  void operator*=(const WVec3Template<Type>& rhs);

  /// Divides this vector component-wise by rhs
  void operator/=(const WVec3Template<Type>& rhs);

  /// Multiplies all components of this vector with f
  void operator*=(Type f); // [tested]

  /// Divides all components of this vector by f
  void operator/=(Type f); // [tested]

  /// Equality Check (bitwise)
  bool IsIdentical(const WVec3Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon
  bool IsEqual(const WVec3Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// Returns the shortest angle between *this and rhs.
  /// Both this and rhs must be normalized
  WAngleTemplate<Type> GetAngleBetween(const WVec3Template<Type>& rhs) const; // [tested]

  /// Returns the angle between vForward and *this, going around the vUp direction.
  ///
  /// Clockwise rotations (looking top down) result in a positive angle,
  /// counter-clockwise rotations give a negative angle.
  /// All vectors must be normalized. vUp must not coincide with vForward, but doesn't need to be orthogonal to it.
  ///
  /// NOTE: This function assumes a right-handed coordinate system.
  /// If you put in vectors from a left-handed coordinate system, the angles will simply invert.
  ///
  /// The order of operands is also important, if you swap this and vForward, the result also inverts.
  WAngleTemplate<Type> GetAngleBetween(const WVec3Template<Type>& vForward, const WVec3Template<Type>& vUp) const; // [tested]


  /// Returns the Dot-product of the two vectors (commutative, order does not matter)
  [[nodiscard]] Type Dot(const WVec3Template<Type>& rhs) const; // [tested]



  /// Returns the Cross-product of the two vectors (NOT commutative, order DOES matter)
  [[nodiscard]] const WVec3Template<Type> CrossRH(const WVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise minimum of *this and rhs
  [[nodiscard]] const WVec3Template<Type> CompMin(const WVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise maximum of *this and rhs
  [[nodiscard]] const WVec3Template<Type> CompMax(const WVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise clamped value of *this between low and high.
  [[nodiscard]] const WVec3Template<Type> CompClamp(const WVec3Template<Type>& vLow, const WVec3Template<Type>& vHigh) const; // [tested]

  /// Returns the component-wise multiplication of *this and rhs
  [[nodiscard]] const WVec3Template<Type> CompMul(const WVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise division of *this and rhs
  [[nodiscard]] const WVec3Template<Type> CompDiv(const WVec3Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  [[nodiscard]] const WVec3Template<Type> Abs() const; // [tested]


  // *** Other common operations ***
public:
  /// Calculates the normal of the triangle defined by the three vertices. Vertices are assumed to be ordered counter-clockwise.
  W_DECLARE_IF_FLOAT_TYPE
  WResult CalculateNormal(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2, const WVec3Template<Type>& v3); // [tested]

  /// Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  W_DECLARE_IF_FLOAT_TYPE
  void MakeOrthogonalTo(const WVec3Template<Type>& vNormal); // [tested]

  /// Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  W_DECLARE_IF_FLOAT_TYPE
  const WVec3Template<Type> GetOrthogonalVector() const; // [tested]

  /// Returns this vector reflected at vNormal.
  W_DECLARE_IF_FLOAT_TYPE
  const WVec3Template<Type> GetReflectedVector(const WVec3Template<Type>& vNormal) const; // [tested]

  /// Returns this vector, refracted at vNormal, using the refraction index of the current medium and the medium it enters.
  W_DECLARE_IF_FLOAT_TYPE
  const WVec3Template<Type> GetRefractedVector(const WVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const;

  /// Returns a random point inside a unit sphere (radius 1).
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type>
  MakeRandomPointInSphere(WRandom& inout_rng); // [tested]

  /// Creates a random direction vector. The vector is normalized.
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type>
  MakeRandomDirection(WRandom& inout_rng); // [tested]

  /// Creates a random vector around the x axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type>
  MakeRandomDeviationX(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation); // [tested]

  /// Creates a random vector around the y axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type>
  MakeRandomDeviationY(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation); // [tested]

  /// Creates a random vector around the z axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type>
  MakeRandomDeviationZ(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation); // [tested]

  /// Creates a random vector around the given normal with a maximum deviation.
  /// \note If you are going to do this many times with the same axis, rather than calling this function, instead manually
  /// do what this function does (see inline code) and only compute the quaternion once.
  W_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static WVec3Template<Type>
  MakeRandomDeviation(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation, const WVec3Template<Type>& vNormal); // [tested]
};

// *** Operators ***

template <typename Type>
const WVec3Template<Type> operator+(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2); // [tested]

template <typename Type>
const WVec3Template<Type> operator-(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2); // [tested]


template <typename Type>
const WVec3Template<Type> operator*(Type f, const WVec3Template<Type>& v); // [tested]

template <typename Type>
const WVec3Template<Type> operator*(const WVec3Template<Type>& v, Type f); // [tested]


template <typename Type>
const WVec3Template<Type> operator/(const WVec3Template<Type>& v, Type f); // [tested]


template <typename Type>
bool operator==(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2); // [tested]

template <typename Type>
bool operator!=(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2); // [tested]

/// Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2); // [tested]

#include <Foundation/Math/Implementation/Vec3_inl.h>
