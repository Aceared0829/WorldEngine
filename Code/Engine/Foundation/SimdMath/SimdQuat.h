#pragma once

#include <Foundation/SimdMath/SimdMat4f.h>

class W_FOUNDATION_DLL WSimdQuat
{
public:
  W_DECLARE_POD_TYPE();

  WSimdQuat();                              // [tested]

  explicit WSimdQuat(const WSimdVec4f& v); // [tested]

  /// Static function that returns a quaternion that represents the identity rotation (none).
  [[nodiscard]] static const WSimdQuat MakeIdentity(); // [tested]

  /// Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an
  /// angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  [[nodiscard]] static WSimdQuat MakeFromElements(WSimdFloat x, WSimdFloat y, WSimdFloat z, WSimdFloat w); // [tested]

  /// Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as an WAngle)
  [[nodiscard]] static WSimdQuat MakeFromAxisAndAngle(const WSimdVec4f& vRotationAxis, const WSimdFloat& fAngle); // [tested]

  /// Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  [[nodiscard]] static WSimdQuat MakeShortestRotation(const WSimdVec4f& vDirFrom, const WSimdVec4f& vDirTo); // [tested]

  /// Returns a quaternion that is the spherical linear interpolation of the other two.
  [[nodiscard]] static WSimdQuat MakeSlerp(const WSimdQuat& qFrom, const WSimdQuat& qTo, const WSimdFloat& t); // [tested]

public:
  /// Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  WResult GetRotationAxisAndAngle(WSimdVec4f& ref_vAxis, WSimdFloat& ref_fAngle, const WSimdFloat& fEpsilon = WMath::DefaultEpsilon<float>()) const; // [tested]

  /// Returns the Quaternion as a matrix.
  WSimdMat4f GetAsMat4() const; // [tested]

  /// Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const WSimdFloat& fEpsilon = WMath::DefaultEpsilon<float>()) const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const WSimdQuat& qOther, const WSimdFloat& fEpsilon) const; // [tested]

public:
  /// Returns a Quaternion that represents the negative / inverted rotation.
  [[nodiscard]] WSimdQuat operator-() const; // [tested]

  /// Rotates v by q
  [[nodiscard]] WSimdVec4f operator*(const WSimdVec4f& v) const; // [tested]

  /// Concatenates the rotations of q1 and q2
  [[nodiscard]] WSimdQuat operator*(const WSimdQuat& q2) const; // [tested]

  bool operator==(const WSimdQuat& q2) const;                    // [tested]
  bool operator!=(const WSimdQuat& q2) const;                    // [tested]

public:
  WSimdVec4f m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuat_inl.h>
