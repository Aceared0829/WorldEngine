#pragma once

#include <Foundation/Math/Vec3.h>

/// An axis-aligned bounding box implementation.
///
/// This class allows to construct AABBs and also provides a large set of functions to work with them,
/// e.g. for overlap queries and ray casts.

template <typename Type>
class WBoundingBoxTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// Default constructor does not initialize anything.
  WBoundingBoxTemplate();

  /// Constructs the box with the given minimum and maximum values.
  WBoundingBoxTemplate(const WVec3Template<Type>& vMin, const WVec3Template<Type>& vMax); // [tested]

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    W_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// Creates a box that is located at the origin and has zero size. This is a 'valid' box.
  [[nodiscard]] static WBoundingBoxTemplate<Type> MakeZero();

  /// Creates a box that is in an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  [[nodiscard]] static WBoundingBoxTemplate<Type> MakeInvalid(); // [tested]

  /// Creates a box from a center point and half-extents for each axis.
  [[nodiscard]] static WBoundingBoxTemplate<Type> MakeFromCenterAndHalfExtents(const WVec3Template<Type>& vCenter, const WVec3Template<Type>& vHalfExtents); // [tested]

  /// Creates a box with the given minimum and maximum values.
  [[nodiscard]] static WBoundingBoxTemplate<Type> MakeFromMinMax(const WVec3Template<Type>& vMin, const WVec3Template<Type>& vMax); // [tested]

  /// Creates a box around the given set of points. If uiNumPoints is zero, the returned box is invalid (same as MakeInvalid() returns).
  [[nodiscard]] static WBoundingBoxTemplate<Type> MakeFromPoints(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)); // [tested]

  /// Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Writes the 8 different corners of the box to the given array.
  void GetCorners(WVec3Template<Type>* out_pCorners) const; // [tested]

  /// Returns the center position of the box.
  const WVec3Template<Type> GetCenter() const; // [tested]

  /// Returns the extents of the box along each axis.
  const WVec3Template<Type> GetExtents() const; // [tested]

  /// Returns the half extents of the box along each axis.
  const WVec3Template<Type> GetHalfExtents() const; // [tested]

  /// Expands the box such that the given point is inside it.
  void ExpandToInclude(const WVec3Template<Type>& vPoint); // [tested]

  /// Expands the box such that the given box is inside it.
  void ExpandToInclude(const WBoundingBoxTemplate& rhs); // [tested]

  /// Expands the box such that all the given points are inside it.
  void ExpandToInclude(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)); // [tested]

  /// If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]

  /// Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const WVec3Template<Type>& vDiff); // [tested]

  /// Checks whether the given point is inside the box.
  bool Contains(const WVec3Template<Type>& vPoint) const; // [tested]

  /// Checks whether the given box is completely inside this box.
  bool Contains(const WBoundingBoxTemplate& rhs) const; // [tested]

  /// Checks whether all the given points are inside this box.
  bool Contains(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)) const; // [tested]

  /// Checks whether the given sphere is completely inside this box.
  bool Contains(const WBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// Checks whether this box overlaps with the given box.
  bool Overlaps(const WBoundingBoxTemplate& rhs) const; // [tested]

  /// Checks whether any of the given points is inside this box.
  bool Overlaps(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)) const; // [tested]

  /// Checks whether the given sphere overlaps with this box.
  bool Overlaps(const WBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// Checks whether this box and the other box are exactly identical.
  bool IsIdentical(const WBoundingBoxTemplate& rhs) const; // [tested]

  /// Checks whether this box and the other box are equal within some threshold.
  bool IsEqual(const WBoundingBoxTemplate& rhs, Type fEpsilon = WMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Moves the box by the given vector.
  void Translate(const WVec3Template<Type>& vDiff); // [tested]

  /// Scales the box along each axis, but keeps its center constant.
  void ScaleFromCenter(const WVec3Template<Type>& vScale); // [tested]

  /// Scales the box's corners by the given factors, thus also moves the box around.
  void ScaleFromOrigin(const WVec3Template<Type>& vScale); // [tested]

  /// Transforms the corners of the box in its local space. The center of the box does not change, unless the transform contains a translation.
  void TransformFromCenter(const WMat4Template<Type>& mTransform); // [tested]

  /// Transforms the corners of the box and recomputes the AABB of those transformed points. Rotations and scalings will influence the center position of the box.
  void TransformFromOrigin(const WMat4Template<Type>& mTransform); // [tested]

  /// The given point is clamped to the volume of the box, i.e. it will be either inside the box or on its surface and it will have the closest
  /// possible distance to the original point.
  const WVec3Template<Type> GetClampedPoint(const WVec3Template<Type>& vPoint) const; // [tested]

  /// Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  Type GetDistanceSquaredTo(const WVec3Template<Type>& vPoint) const; // [tested]

  /// Returns the minimum squared distance between the two boxes. Zero if the boxes overlap.
  Type GetDistanceSquaredTo(const WBoundingBoxTemplate& rhs) const; // [tested]

  /// Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  Type GetDistanceTo(const WVec3Template<Type>& vPoint) const; // [tested]

  /// Returns the minimum distance between the box and the sphere. Zero or negative if both overlap.
  Type GetDistanceTo(const WBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// Returns the minimum distance between the two boxes. Zero if the boxes overlap.
  Type GetDistanceTo(const WBoundingBoxTemplate& rhs) const; // [tested]

  /// Returns whether the given ray intersects the box. Optionally returns the intersection distance and position.
  /// Note that vRayDir is not required to be normalized.
  bool GetRayIntersection(const WVec3Template<Type>& vStartPos, const WVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance = nullptr,
    WVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// Checks whether the line segment intersects the box. Optionally returns the intersection point and the fraction along the line segment
  /// where the intersection occurred.
  bool GetLineSegmentIntersection(const WVec3Template<Type>& vStartPos, const WVec3Template<Type>& vEndPos, Type* out_pLineFraction = nullptr,
    WVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// Returns a bounding sphere that encloses this box.
  const WBoundingSphereTemplate<Type> GetBoundingSphere() const; // [tested]


public:
  WVec3Template<Type> m_vMin;
  WVec3Template<Type> m_vMax;
};

/// Checks whether this box and the other are identical.
template <typename Type>
bool operator==(const WBoundingBoxTemplate<Type>& lhs, const WBoundingBoxTemplate<Type>& rhs); // [tested]

/// Checks whether this box and the other are not identical.
template <typename Type>
bool operator!=(const WBoundingBoxTemplate<Type>& lhs, const WBoundingBoxTemplate<Type>& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingBox_inl.h>
