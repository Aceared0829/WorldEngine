#pragma once

#include <Foundation/SimdMath/SimdBSphere.h>

class WSimdBBox
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor does not initialize anything.
  WSimdBBox();

  /// Constructs the box with the given minimum and maximum values.
  WSimdBBox(const WSimdVec4f& vMin, const WSimdVec4f& vMax); // [tested]

  /// Creates a box that is located at the origin and has zero size. This is a 'valid' box.
  [[nodiscard]] static WSimdBBox MakeZero();

  /// Creates a box that is in an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  [[nodiscard]] static WSimdBBox MakeInvalid(); // [tested]

  /// Creates a box from a center point and half-extents for each axis.
  [[nodiscard]] static WSimdBBox MakeFromCenterAndHalfExtents(const WSimdVec4f& vCenter, const WSimdVec4f& vHalfExtents); // [tested]

  /// Creates a box with the given minimum and maximum values.
  [[nodiscard]] static WSimdBBox MakeFromMinMax(const WSimdVec4f& vMin, const WSimdVec4f& vMax); // [tested]

  /// Creates a box around the given set of points. If uiNumPoints is zero, the returned box is invalid (same as MakeInvalid() returns).
  [[nodiscard]] static WSimdBBox MakeFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f)); // [tested]

public:
  /// Resets the box to an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  [[deprecated("Use MakeInvalid() instead.")]] void SetInvalid(); // [tested]

  /// Sets the box from a center point and half-extents for each axis.
  [[deprecated("Use MakeFromCenterAndHalfExtents() instead.")]] void SetCenterAndHalfExtents(const WSimdVec4f& vCenter, const WSimdVec4f& vHalfExtents); // [tested]

  /// Creates a new bounding-box around the given set of points.
  [[deprecated("Use MakeFromPoints() instead.")]] void SetFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f)); // [tested]

  /// Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the center position of the box.
  WSimdVec4f GetCenter() const; // [tested]

  /// Returns the extents of the box along each axis.
  WSimdVec4f GetExtents() const; // [tested]

  /// Returns the half extents of the box along each axis.
  WSimdVec4f GetHalfExtents() const; // [tested]

  /// Expands the box such that the given point is inside it.
  void ExpandToInclude(const WSimdVec4f& vPoint); // [tested]

  /// Expands the box such that all the given points are inside it.
  void ExpandToInclude(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WSimdVec4f)); // [tested]

  /// Expands the box such that the given box is inside it.
  void ExpandToInclude(const WSimdBBox& rhs); // [tested]

  /// If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]


  /// Checks whether the given point is inside the box.
  bool Contains(const WSimdVec4f& vPoint) const; // [tested]

  /// Checks whether the given box is completely inside this box.
  bool Contains(const WSimdBBox& rhs) const; // [tested]

  /// Checks whether the given sphere is completely inside this box.
  bool Contains(const WSimdBSphere& sphere) const; // [tested]

  /// Checks whether this box overlaps with the given box.
  bool Overlaps(const WSimdBBox& rhs) const; // [tested]

  /// Checks whether the given sphere overlaps with this box.
  bool Overlaps(const WSimdBSphere& sphere) const; // [tested]


  /// Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const WSimdVec4f& vDiff); // [tested]

  /// Moves the box by the given vector.
  void Translate(const WSimdVec4f& vDiff); // [tested]

  /// Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const WSimdTransform& transform); // [tested]

  /// Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const WSimdMat4f& mMat); // [tested]


  /// The given point is clamped to the volume of the box, i.e. it will be either inside the box or on its surface and it will have the closest
  /// possible distance to the original point.
  WSimdVec4f GetClampedPoint(const WSimdVec4f& vPoint) const; // [tested]

  /// Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  WSimdFloat GetDistanceSquaredTo(const WSimdVec4f& vPoint) const; // [tested]

  /// Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  WSimdFloat GetDistanceTo(const WSimdVec4f& vPoint) const; // [tested]


  bool operator==(const WSimdBBox& rhs) const;               // [tested]
  bool operator!=(const WSimdBBox& rhs) const;               // [tested]

public:
  WSimdVec4f m_Min;
  WSimdVec4f m_Max;
};

#include <Foundation/SimdMath/Implementation/SimdBBox_inl.h>
