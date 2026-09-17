#pragma once

#include <Foundation/SimdMath/SimdQuatd.h>

class W_FOUNDATION_DLL WSimdTransformd
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor: Does not do any initialization.
  WSimdTransformd(); // [tested]

  /// Sets position, rotation and scale.
  explicit WSimdTransformd(const WSimdVec4d& vPosition, const WSimdQuatd& qRotation = WSimdQuatd::MakeIdentity(), const WSimdVec4d& vScale = WSimdVec4d(1.0f)); // [tested]

  /// Sets rotation.
  explicit WSimdTransformd(const WSimdQuatd& qRotation); // [tested]

  /// Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static WSimdTransformd Make(const WSimdVec4d& vPosition, const WSimdQuatd& qRotation = WSimdQuatd::MakeIdentity(), const WSimdVec4d& vScale = WSimdVec4d(1.0f)); // [tested]

  /// Creates an identity transform.
  [[nodiscard]] static WSimdTransformd MakeIdentity(); // [tested]

  /// Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static WSimdTransformd MakeLocalTransform(const WSimdTransformd& globalTransformParent, const WSimdTransformd& globalTransformChild); // [tested]

  /// Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static WSimdTransformd MakeGlobalTransform(const WSimdTransformd& globalTransformParent, const WSimdTransformd& localTransformChild); // [tested]

  /// Returns the scale component with maximum magnitude.
  WSimdDouble GetMaxScale() const; // [tested]

  /// Returns whether this transform contains negative scaling aka mirroring.
  bool HasMirrorScaling() const;

  /// Returns whether this transform has only uniform scaling (including scale == 1).
  bool HasOnlyUniformScaling() const;

public:
  /// Equality Check with epsilon
  bool IsEqual(const WSimdTransformd& rhs, const WSimdDouble& fEpsilon) const; // [tested]

public:
  /// Inverts this transform.
  void Invert(); // [tested]

  /// Returns the inverse of this transform.
  WSimdTransformd GetInverse() const; // [tested]

  /// Returns the transformation as a matrix.
  WSimdMat4d GetAsMat4() const;                                            // [tested]

public:
  [[nodiscard]] WSimdVec4d TransformPosition(const WSimdVec4d& v) const;  // [tested]
  [[nodiscard]] WSimdVec4d TransformDirection(const WSimdVec4d& v) const; // [tested]

  /// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const WSimdTransformd& other); // [tested]

  /// Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const WSimdQuatd& q);  // [tested]

  void operator+=(const WSimdVec4d& v); // [tested]
  void operator-=(const WSimdVec4d& v); // [tested]

public:
  WSimdVec4d m_Position;
  WSimdQuatd m_Rotation;
  WSimdVec4d m_Scale;
};

// *** free functions ***

/// Transforms the vector v by the transform.
W_ALWAYS_INLINE const WSimdVec4d operator*(const WSimdTransformd& t, const WSimdVec4d& v); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the left with t.
W_ALWAYS_INLINE const WSimdTransformd operator*(const WSimdQuatd& q, const WSimdTransformd& t); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the right with t.
W_ALWAYS_INLINE const WSimdTransformd operator*(const WSimdTransformd& t, const WSimdQuatd& q); // [tested]

/// Translates the WSimdTransformd by the vector. This will move the object in global space.
W_ALWAYS_INLINE const WSimdTransformd operator+(const WSimdTransformd& t, const WSimdVec4d& v); // [tested]

/// Translates the WSimdTransformd by the vector. This will move the object in global space.
W_ALWAYS_INLINE const WSimdTransformd operator-(const WSimdTransformd& t, const WSimdVec4d& v); // [tested]

/// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
W_ALWAYS_INLINE const WSimdTransformd operator*(const WSimdTransformd& lhs, const WSimdTransformd& rhs); // [tested]

W_ALWAYS_INLINE bool operator==(const WSimdTransformd& t1, const WSimdTransformd& t2);                   // [tested]
W_ALWAYS_INLINE bool operator!=(const WSimdTransformd& t1, const WSimdTransformd& t2);                   // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransformd_inl.h>
