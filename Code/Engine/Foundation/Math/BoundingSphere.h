#pragma once

#include <Foundation/Math/Vec3.h>

/// An implementation of a bounding sphere.
///
/// This class allows to construct and manipulate bounding spheres.
/// It also provides a large set of functions to do overlap queries, ray casts and other useful operations.
template <typename Type>
class WBoundingSphereTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// Default constructor does not initialize any data.
  WBoundingSphereTemplate();


  /// Creates a sphere at the origin with radius zero.
  [[nodiscard]] static WBoundingSphereTemplate<Type> MakeZero();

  /// Creates an 'invalid' sphere, with its center at the given position and a negative radius.
  ///
  /// Such a sphere can be made 'valid' through ExpandToInclude(), but be aware that the originally provided center position
  /// will always be part of the sphere.
  [[nodiscard]] static WBoundingSphereTemplate<Type> MakeInvalid(const WVec3Template<Type>& vCenter = WVec3Template<Type>::MakeZero());

  /// Creates a sphere with the provided center and radius.
  [[nodiscard]] static WBoundingSphereTemplate<Type> MakeFromCenterAndRadius(const WVec3Template<Type>& vCenter, Type fRadius);

  /// Creates a bounding sphere around the provided points.
  ///
  /// The center of the sphere will be at the 'center of mass' of all the points, and the radius will be the distance to the
  /// farthest point from there.
  [[nodiscard]] static WBoundingSphereTemplate<Type> MakeFromPoints(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>));

#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    W_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// Checks whether the sphere is all zero.
  bool IsZero(Type fEpsilon = WMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// Returns whether any value is NaN.
  bool IsNaN() const; // [tested]

  /// Increases the sphere's radius to include this point. Does NOT change its position, thus the resulting sphere might be not a very tight
  /// fit.
  void ExpandToInclude(const WVec3Template<Type>& vPoint); // [tested]

  /// Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not a very
  /// tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)); // [tested]

  /// Increases this sphere's radius, such that it encloses the other sphere. Does not change the center position of this sphere.
  void ExpandToInclude(const WBoundingSphereTemplate& rhs); // [tested]

  /// Increases this sphere's radius, such that it encloses the box. Does not change the center position of this sphere.
  void ExpandToInclude(const WBoundingBoxTemplate<Type>& rhs); // [tested]

  /// Increases the size of the sphere by the given amount.
  void Grow(Type fDiff); // [tested]

  /// Tests whether two spheres are identical.
  bool IsIdentical(const WBoundingSphereTemplate& rhs) const; // [tested]

  /// Tests whether two spheres are equal within some threshold.
  bool IsEqual(const WBoundingSphereTemplate& rhs, Type fEpsilon = WMath::DefaultEpsilon<Type>()) const; // [tested]

  /// Moves the sphere by the given vector.
  void Translate(const WVec3Template<Type>& vTranslation); // [tested]

  /// Scales the sphere's size, does not change its center position.
  void ScaleFromCenter(Type fScale); // [tested]

  /// Scales the sphere in world unites, meaning its center position will change as well.
  void ScaleFromOrigin(const WVec3Template<Type>& vScale); // [tested]

  /// Transforms the sphere with the given matrix from the world origin. I.e. scalings and rotations will influence its position.
  void TransformFromOrigin(const WMat4Template<Type>& mTransform); // [tested]

  /// Transforms the sphere with the given matrix from its own center. I.e. rotations have no effect, scalings will only affect the radius, and
  /// only translations will affect its position.
  void TransformFromCenter(const WMat4Template<Type>& mTransform); // [tested]

  /// Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  Type GetDistanceTo(const WVec3Template<Type>& vPoint) const; // [tested]

  /// Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for overlapping
  /// spheres.
  Type GetDistanceTo(const WBoundingSphereTemplate& rhs) const; // [tested]

  /// Returns the minimum distance between the box and the sphere. Zero if both are exactly touching, negative values if they overlap.
  Type GetDistanceTo(const WBoundingBoxTemplate<Type>& rhs) const; // [tested]

  /// Returns the minimum distance of any of the points to the sphere.
  Type GetDistanceTo(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)) const; // [tested]

  /// Returns true if the given point is inside the sphere.
  bool Contains(const WVec3Template<Type>& vPoint) const; // [tested]

  /// Returns whether all the given points are inside this sphere.
  bool Contains(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)) const; // [tested]

  /// Returns whether the other sphere is completely inside this sphere.
  bool Contains(const WBoundingSphereTemplate& rhs) const; // [tested]

  /// Returns whether the given box is completely inside this sphere.
  bool Contains(const WBoundingBoxTemplate<Type>& rhs) const; // [tested]

  /// Checks whether any of the given points is inside the sphere
  bool Overlaps(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>)) const; // [tested]

  /// Checks whether the two objects overlap.
  bool Overlaps(const WBoundingSphereTemplate& rhs) const; // [tested]

  /// Checks whether the two objects overlap.
  bool Overlaps(const WBoundingBoxTemplate<Type>& rhs) const; // [tested]

  /// Returns a bounding box that encloses this sphere.
  const WBoundingBoxTemplate<Type> GetBoundingBox() const; // [tested]

  /// Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the closest
  /// distance to the original point.
  [[nodiscard]] const WVec3Template<Type> GetClampedPoint(const WVec3Template<Type>& vPoint); // [tested]

  /// Computes the intersection of a ray with this sphere. Returns true if there was an intersection. May optionally return the intersection
  /// time and position. The ray's direction must be normalized. The function will also return true, if the ray already starts inside the sphere, but
  /// it will still compute the intersection with the surface of the sphere.
  [[nodiscard]] bool GetRayIntersection(const WVec3Template<Type>& vRayStartPos, const WVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance = nullptr,
    WVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// Returns true if the line segment intersects the sphere.
  [[nodiscard]] bool GetLineSegmentIntersection(const WVec3Template<Type>& vLineStartPos, const WVec3Template<Type>& vLineEndPos,
    Type* out_pHitFraction = nullptr, WVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]


public:
  WVec3Template<Type> m_vCenter;
  Type m_fRadius;
};

/// Checks whether this sphere and the other are identical.
template <typename Type>
bool operator==(const WBoundingSphereTemplate<Type>& lhs, const WBoundingSphereTemplate<Type>& rhs); // [tested]

/// Checks whether this sphere and the other are not identical.
template <typename Type>
bool operator!=(const WBoundingSphereTemplate<Type>& lhs, const WBoundingSphereTemplate<Type>& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingSphere_inl.h>
