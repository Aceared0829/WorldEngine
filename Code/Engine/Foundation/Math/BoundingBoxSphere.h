#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>

/// A combination of a bounding box and a bounding sphere with the same center.
///
/// This class uses less memory than storying a bounding box and sphere separate.

template <typename Type>
class WBoundingBoxSphereTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  W_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// Default constructor does not initialize anything.
  WBoundingBoxSphereTemplate(); // [tested]

  WBoundingBoxSphereTemplate(const WBoundingBoxSphereTemplate& rhs);

  void operator=(const WBoundingBoxSphereTemplate& rhs);

  /// Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  WBoundingBoxSphereTemplate(const WBoundingBoxTemplate<Type>& box); // [tested]

  /// Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  WBoundingBoxSphereTemplate(const WBoundingSphereTemplate<Type>& sphere); // [tested]

  /// Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static WBoundingBoxSphereTemplate<Type> MakeZero();

  /// Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static WBoundingBoxSphereTemplate<Type> MakeInvalid();

  /// Creates an object from the given center point and extents.
  [[nodiscard]] static WBoundingBoxSphereTemplate<Type> MakeFromCenterExtents(const WVec3Template<Type>& vCenter, const WVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius);

  /// Creates an object that contains all the provided points.
  [[nodiscard]] static WBoundingBoxSphereTemplate<Type> MakeFromPoints(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride = sizeof(WVec3Template<Type>));

  /// Creates an object from another bounding box.
  [[nodiscard]] static WBoundingBoxSphereTemplate<Type> MakeFromBox(const WBoundingBoxTemplate<Type>& box);

  /// Creates an object from another bounding sphere.
  [[nodiscard]] static WBoundingBoxSphereTemplate<Type> MakeFromSphere(const WBoundingSphereTemplate<Type>& sphere);

  /// Creates an object from another bounding box and a sphere.
  [[nodiscard]] static WBoundingBoxSphereTemplate<Type> MakeFromBoxAndSphere(const WBoundingBoxTemplate<Type>& box, const WBoundingSphereTemplate<Type>& sphere);


#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    W_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the bounding box.
  const WBoundingBoxTemplate<Type> GetBox() const; // [tested]

  /// Returns the bounding sphere.
  const WBoundingSphereTemplate<Type> GetSphere() const; // [tested]

  /// Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const WBoundingBoxSphereTemplate& rhs); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const WMat4Template<Type>& mTransform); // [tested]

public:
  WVec3Template<Type> m_vCenter;
  Type m_fSphereRadius;
  WVec3Template<Type> m_vBoxHalfExtents;
};

/// Checks whether this bounds and the other are identical.
template <typename Type>
bool operator==(const WBoundingBoxSphereTemplate<Type>& lhs, const WBoundingBoxSphereTemplate<Type>& rhs); // [tested]

/// Checks whether this bounds and the other are not identical.
template <typename Type>
bool operator!=(const WBoundingBoxSphereTemplate<Type>& lhs, const WBoundingBoxSphereTemplate<Type>& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingBoxSphere_inl.h>
