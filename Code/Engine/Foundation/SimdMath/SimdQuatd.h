#pragma once

#include <Foundation/SimdMath/SimdMat4d.h>

class W_FOUNDATION_DLL WSimdQuatd
{
public:
  W_DECLARE_POD_TYPE();

  WSimdQuatd();                              // [tested]

  explicit WSimdQuatd(const WSimdVec4d& v); // [tested]

  /// Static function that returns a quaternion that represents the identity rotation (none).
  [[nodiscard]] static const WSimdQuatd MakeIdentity(); // [tested]

  /// Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an
  /// angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  [[nodiscard]] static WSimdQuatd MakeFromElements(WSimdDouble x, WSimdDouble y, WSimdDouble z, WSimdDouble w); // [tested]

  /// Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as an WAngle)
  [[nodiscard]] static WSimdQuatd MakeFromAxisAndAngle(const WSimdVec4d& vRotationAxis, const WSimdDouble& fAngle); // [tested]

  /// Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  [[nodiscard]] static WSimdQuatd MakeShortestRotation(const WSimdVec4d& vDirFrom, const WSimdVec4d& vDirTo); // [tested]

  /// Returns a quaternion that is the spherical linear interpolation of the other two.
  [[nodiscard]] static WSimdQuatd MakeSlerp(const WSimdQuatd& qFrom, const WSimdQuatd& qTo, const WSimdDouble& t); // [tested]

public:
  /// Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  WResult GetRotationAxisAndAngle(WSimdVec4d& ref_vAxis, WSimdDouble& ref_fAngle, const WSimdDouble& fEpsilon = WMath::DefaultEpsilon<double>()) const; // [tested]

  /// Returns the Quaternion as a matrix.
  WSimdMat4d GetAsMat4() const; // [tested]

  /// Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const WSimdDouble& fEpsilon = WMath::DefaultEpsilon<float>()) const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const WSimdQuatd& qOther, const WSimdDouble& fEpsilon) const; // [tested]

public:
  /// Returns a Quaternion that represents the negative / inverted rotation.
  [[nodiscard]] WSimdQuatd operator-() const; // [tested]

  /// Rotates v by q
  [[nodiscard]] WSimdVec4d operator*(const WSimdVec4d& v) const; // [tested]

  /// Concatenates the rotations of q1 and q2
  [[nodiscard]] WSimdQuatd operator*(const WSimdQuatd& q2) const; // [tested]

  bool operator==(const WSimdQuatd& q2) const;                    // [tested]
  bool operator!=(const WSimdQuatd& q2) const;                    // [tested]

public:
  WSimdVec4d m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuatd_inl.h>
