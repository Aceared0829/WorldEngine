#pragma once

#include <Foundation/SimdMath/SimdTransform.h>

class W_FOUNDATION_DLL WSimdBSphere
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor does not initialize any data.
  WSimdBSphere();

  /// Creates a sphere with the given radius around the given center.
  WSimdBSphere(const WSimdVec4f& vCenter, const WSimdFloat& fRadius); // [tested]

  /// Creates a sphere at the origin with radius zero.
  [[nodiscard]] static WSimdBSphere MakeZero();

  /// Creates an 'invalid' sphere, with its center at the given position and a negative radius.
  ///
  /// Such a sphere can be made 'valid' through ExpandToInclude(), but be aware that the originally provided center position
  /// will always be part of the sphere.
  [[nodiscard]] static WSimdBSphere MakeInvalid(const WSimdVec4f& vCenter = WSimdVec4f::MakeZero()); // [tested]

  /// Creates a sphere with the provided center and radius.
  [[nodiscard]] static WSimdBSphere MakeFromCenterAndRadius(const WSimdVec4f& vCenter, const WSimdFloat& fRadius); // [tested]

  /// Creates a bounding sphere around the provided points.
  ///
  /// The center of the sphere will be at the 'center of mass' of all the points, and the radius will be the distance to the
  /// farthest point from there.
  [[nodiscard]] static WSimdBSphere MakeFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f));


public:
  /// Sets the bounding sphere to invalid values.
  [[deprecated("Use MakeInvalid() instead.")]] void SetInvalid(); // [tested]

  /// Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// Returns whether any value is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the center
  WSimdVec4f GetCenter() const; // [tested]

  /// Returns the radius
  WSimdFloat GetRadius() const; // [tested]

  /// Initializes the sphere to be the bounding sphere of all the given points.
  [[deprecated("Use MakeFromPoints() instead.")]] void SetFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f));

  /// Increases the sphere's radius to include this point.
  void ExpandToInclude(const WSimdVec4f& vPoint); // [tested]

  /// Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not
  /// a very tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f)); // [tested]

  /// Increases this sphere's radius, such that it encloses the other sphere.
  void ExpandToInclude(const WSimdBSphere& rhs); // [tested]

public:
  /// Transforms the sphere in its local space.
  void Transform(const WSimdTransform& t); // [tested]

  /// Transforms the sphere in its local space.
  void Transform(const WSimdMat4f& mMat); // [tested]

public:
  /// Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  WSimdFloat GetDistanceTo(const WSimdVec4f& vPoint) const; // [tested]

  /// Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for
  /// overlapping spheres.
  WSimdFloat GetDistanceTo(const WSimdBSphere& rhs) const; // [tested]

  /// Returns true if the given point is inside the sphere.
  bool Contains(const WSimdVec4f& vPoint) const; // [tested]

  /// Returns whether the other sphere is completely inside this sphere.
  bool Contains(const WSimdBSphere& rhs) const; // [tested]

  /// Checks whether the two objects overlap.
  bool Overlaps(const WSimdBSphere& rhs) const; // [tested]

  /// Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the
  /// closest distance to the original point.
  [[nodiscard]] WSimdVec4f GetClampedPoint(const WSimdVec4f& vPoint); // [tested]

  [[nodiscard]] bool operator==(const WSimdBSphere& rhs) const;        // [tested]
  [[nodiscard]] bool operator!=(const WSimdBSphere& rhs) const;        // [tested]

public:
  WSimdVec4f m_CenterAndRadius;
};

#include <Foundation/SimdMath/Implementation/SimdBSphere_inl.h>
