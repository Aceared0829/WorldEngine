#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class W_FOUNDATION_DLL WSimdTransform
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor: Does not do any initialization.
  WSimdTransform(); // [tested]

  /// Sets position, rotation and scale.
  explicit WSimdTransform(const WSimdVec4f& vPosition, const WSimdQuat& qRotation = WSimdQuat::MakeIdentity(), const WSimdVec4f& vScale = WSimdVec4f(1.0f)); // [tested]

  /// Sets rotation.
  explicit WSimdTransform(const WSimdQuat& qRotation); // [tested]

  /// Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static WSimdTransform Make(const WSimdVec4f& vPosition, const WSimdQuat& qRotation = WSimdQuat::MakeIdentity(), const WSimdVec4f& vScale = WSimdVec4f(1.0f)); // [tested]

  /// Creates an identity transform.
  [[nodiscard]] static WSimdTransform MakeIdentity(); // [tested]

  /// Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static WSimdTransform MakeLocalTransform(const WSimdTransform& globalTransformParent, const WSimdTransform& globalTransformChild); // [tested]

  /// Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static WSimdTransform MakeGlobalTransform(const WSimdTransform& globalTransformParent, const WSimdTransform& localTransformChild); // [tested]

  /// Returns the scale component with maximum magnitude.
  WSimdFloat GetMaxScale() const; // [tested]

  /// Returns whether this transform contains negative scaling aka mirroring.
  bool HasMirrorScaling() const;

  /// Returns whether this transform has only uniform scaling (including scale == 1).
  bool HasOnlyUniformScaling() const;

public:
  /// Equality Check with epsilon
  bool IsEqual(const WSimdTransform& rhs, const WSimdFloat& fEpsilon) const; // [tested]

public:
  /// Inverts this transform.
  void Invert(); // [tested]

  /// Returns the inverse of this transform.
  WSimdTransform GetInverse() const; // [tested]

  /// Returns the transformation as a matrix.
  WSimdMat4f GetAsMat4() const;                                            // [tested]

public:
  [[nodiscard]] WSimdVec4f TransformPosition(const WSimdVec4f& v) const;  // [tested]
  [[nodiscard]] WSimdVec4f TransformDirection(const WSimdVec4f& v) const; // [tested]

  /// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const WSimdTransform& other); // [tested]

  /// Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const WSimdQuat& q);  // [tested]

  void operator+=(const WSimdVec4f& v); // [tested]
  void operator-=(const WSimdVec4f& v); // [tested]

public:
  WSimdVec4f m_Position;
  WSimdQuat m_Rotation;
  WSimdVec4f m_Scale;
};

// *** free functions ***

/// Transforms the vector v by the transform.
W_ALWAYS_INLINE const WSimdVec4f operator*(const WSimdTransform& t, const WSimdVec4f& v); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the left with t.
W_ALWAYS_INLINE const WSimdTransform operator*(const WSimdQuat& q, const WSimdTransform& t); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the right with t.
W_ALWAYS_INLINE const WSimdTransform operator*(const WSimdTransform& t, const WSimdQuat& q); // [tested]

/// Translates the WSimdTransform by the vector. This will move the object in global space.
W_ALWAYS_INLINE const WSimdTransform operator+(const WSimdTransform& t, const WSimdVec4f& v); // [tested]

/// Translates the WSimdTransform by the vector. This will move the object in global space.
W_ALWAYS_INLINE const WSimdTransform operator-(const WSimdTransform& t, const WSimdVec4f& v); // [tested]

/// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
W_ALWAYS_INLINE const WSimdTransform operator*(const WSimdTransform& lhs, const WSimdTransform& rhs); // [tested]

W_ALWAYS_INLINE bool operator==(const WSimdTransform& t1, const WSimdTransform& t2);                   // [tested]
W_ALWAYS_INLINE bool operator!=(const WSimdTransform& t1, const WSimdTransform& t2);                   // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>
