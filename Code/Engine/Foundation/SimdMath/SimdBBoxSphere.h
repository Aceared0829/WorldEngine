#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class W_FOUNDATION_DLL WSimdBBoxSphere
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor does not initialize anything.
  WSimdBBoxSphere(); // [tested]

  /// Constructs the bounds from the center position, the box half extends and the sphere radius.
  [[deprecated("Use MakeFromCenterExtents() instead.")]] WSimdBBoxSphere(const WSimdVec4f& vCenter, const WSimdVec4f& vBoxHalfExtents, const WSimdFloat& fSphereRadius); // [tested]

  /// Constructs the bounds from the given box and sphere.
  [[deprecated("Use MakeFromBoxAndSphere() instead.")]] WSimdBBoxSphere(const WSimdBBox& box, const WSimdBSphere& sphere); // [tested]

  /// Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  WSimdBBoxSphere(const WSimdBBox& box); // [tested]

  /// Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  WSimdBBoxSphere(const WSimdBSphere& sphere); // [tested]

  /// Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static WSimdBBoxSphere MakeZero();

  /// Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static WSimdBBoxSphere MakeInvalid(); // [tested]

  /// Creates an object from the given center point and extents.
  [[nodiscard]] static WSimdBBoxSphere MakeFromCenterExtents(const WSimdVec4f& vCenter, const WSimdVec4f& vBoxHalfExtents, const WSimdFloat& fSphereRadius);

  /// Creates an object that contains all the provided points.
  [[nodiscard]] static WSimdBBoxSphere MakeFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f));

  /// Creates an object from another bounding box.
  [[nodiscard]] static WSimdBBoxSphere MakeFromBox(const WSimdBBox& box);

  /// Creates an object from another bounding sphere.
  [[nodiscard]] static WSimdBBoxSphere MakeFromSphere(const WSimdBSphere& sphere);

  /// Creates an object from another bounding box and a sphere.
  [[nodiscard]] static WSimdBBoxSphere MakeFromBoxAndSphere(const WSimdBBox& box, const WSimdBSphere& sphere);


public:
  /// Resets the bounds to an invalid state.
  [[deprecated("Use MakeInvalid() instead.")]] void SetInvalid(); // [tested]

  /// Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Calculates the bounds from given set of points.
  [[deprecated("Use MakeFromPoints() instead.")]] void SetFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f)); // [tested]

  /// Returns the bounding box.
  WSimdBBox GetBox() const; // [tested]

  /// Returns the bounding sphere.
  WSimdBSphere GetSphere() const; // [tested]

  /// Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const WSimdBBoxSphere& rhs); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const WSimdTransform& t); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const WSimdMat4f& mMat);                          // [tested]

  [[nodiscard]] bool operator==(const WSimdBBoxSphere& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const WSimdBBoxSphere& rhs) const; // [tested]

public:
  WSimdVec4f m_CenterAndRadius;
  WSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>
