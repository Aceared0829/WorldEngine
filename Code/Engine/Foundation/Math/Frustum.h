#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdBSphere.h>
#include <Foundation/SimdMath/SimdVec4b.h>
#include <Foundation/SimdMath/SimdVec4f.h>

/// Enum that describes where in a volume another object is located.
struct WVolumePosition
{
  /// Enum that describes where in a volume another object is located.
  enum Enum
  {
    Outside,      //< means an object is ENTIRELY inside a volume
    Inside,       //< means an object is outside a volume
    Intersecting, //< means an object is PARTIALLY inside/outside a volume
  };
};

/// Represents the frustum of some camera and can be used for culling objects.
///
/// The frustum always consists of exactly 6 planes (near, far, left, right, top, bottom).
///
/// The frustum planes point outwards, ie. when an object is in front of one of the planes, it is considered to be outside
/// the frustum.
///
/// Planes can be automatically extracted from a projection matrix or passed in manually.
/// In the latter case, make sure to pass them in in the order defined in the PlaneType enum.
class W_FOUNDATION_DLL WFrustum
{
public:
  enum PlaneType : WUInt8
  {
    NearPlane,
    LeftPlane,
    RightPlane,
    FarPlane,
    BottomPlane,
    TopPlane,

    PLANE_COUNT
  };

  enum FrustumCorner : WUInt8
  {
    NearTopLeft,
    NearTopRight,
    NearBottomLeft,
    NearBottomRight,
    FarTopLeft,
    FarTopRight,
    FarBottomLeft,
    FarBottomRight,

    CORNER_COUNT = 8
  };

  /// The constructor does NOT initialize the frustum planes, make sure to call SetFrustum() before trying to use it.
  WFrustum();
  ~WFrustum();

  /// Returns an invalid frustum with all planes set to zero.
  [[nodiscard]] static WFrustum MakeInvalid();

  /// Sets the frustum manually by specifying the planes directly.
  ///
  /// \note Make sure to pass in the planes in the order of the PlaneType enum, otherwise WFrustum may not always work as expected.
  [[nodiscard]] static WFrustum MakeFromPlanes(const WPlane* pPlanes); // [tested]

  /// Sets the frustum manually by specifying the planes directly.
  ///
  /// \note Make sure to pass in the planes in the order of the PlaneType enum, otherwise WFrustum may not always work as expected.
  ///
  /// Returns W_SUCESS with a valid outFrustum if the operation was successful and W_FAILURE otherwise.
  [[nodiscard]] static WResult TryMakeFromPlanes(WFrustum& out_frustum, const WPlane* pPlanes);

  /// Creates the frustum by extracting the planes from the given (model-view / projection) matrix.
  ///
  /// If the matrix is just the projection matrix, the frustum will be in local space. Pass the full ModelViewProjection
  /// matrix to create the frustum in world-space. If the projection matrix contained in ModelViewProjection is an infinite
  /// plane projection matrix, the resulting frustum will yield a far plane with infinite distance.
  [[nodiscard]] static WFrustum MakeFromMVP(const WMat4& mModelViewProjection, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Creates the frustum by extracting the planes from the given (model-view / projection) matrix.
  ///
  /// If the matrix is just the projection matrix, the frustum will be in local space. Pass the full ModelViewProjection
  /// matrix to create the frustum in world-space. If the projection matrix contained in ModelViewProjection is an infinite
  /// plane projection matrix, the resulting frustum will yield a far plane with infinite distance.
  ///
  /// Returns W_SUCESS with a valid outFrustum if the operation was successful and W_FAILURE otherwise.
  [[nodiscard]] static WResult TryMakeFromMVP(WFrustum& out_frustum, const WMat4& mModelViewProjection, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WHandedness::Enum handedness = WHandedness::Default);

  /// Creates a frustum from the given camera position, direction vectors and the field-of-view along X and Y.
  ///
  /// The up vector does not need to be exactly orthogonal to the forwards vector, it will get recomputed properly.
  /// FOV X and Y define the entire field-of-view, so a FOV of 180 degree would mean the entire half-space in front of the camera.
  [[nodiscard]] static WFrustum MakeFromFOV(const WVec3& vPosition, const WVec3& vForwards, const WVec3& vUp, WAngle fovX, WAngle fovY, float fNearPlane, float fFarPlane); // [tested]

  /// Creates a frustum from the given camera position, direction vectors and the field-of-view along X and Y.
  ///
  /// The up vector does not need to be exactly orthogonal to the forwards vector, it will get recomputed properly.
  /// FOV X and Y define the entire field-of-view, so a FOV of 180 degree would mean the entire half-space in front of the camera.
  ///
  /// Returns W_SUCESS with a valid outFrustum if the operation was successful and W_FAILURE otherwise.
  [[nodiscard]] static WResult TryMakeFromFOV(WFrustum& out_frustum, const WVec3& vPosition, const WVec3& vForwards, const WVec3& vUp, WAngle fovX, WAngle fovY, float fNearPlane, float fFarPlane);

  /// Creates a frustum from 8 corner points.
  ///
  /// Asserts that the frustum is valid after construction. Thus the given points must form a proper frustum.
  [[nodiscard]] static WFrustum MakeFromCorners(const WVec3 pCorners[FrustumCorner::CORNER_COUNT]);

  /// Creates a frustum from 8 corner points.
  ///
  /// Returns W_SUCESS with a valid outFrustum if the operation was successful and W_FAILURE otherwise.
  [[nodiscard]] static WResult TryMakeFromCorners(WFrustum& out_frustum, const WVec3 pCorners[FrustumCorner::CORNER_COUNT]);

  /// Returns the n-th plane of the frustum.
  const WPlane& GetPlane(WUInt8 uiPlane) const; // [tested]

  /// Returns the n-th plane of the frustum and allows modification.
  WPlane& AccessPlane(WUInt8 uiPlane);

  /// Checks that all planes are valid.
  bool IsValid() const;

  /// Transforms the frustum by the given matrix. This allows to adjust the frustum to a new orientation when a camera is moved or
  /// when it is necessary to cull from a different position.
  void TransformFrustum(const WMat4& mTransform); // [tested]

  /// Returns frustum transformed by given matrix
  WFrustum GetTransformedFrustum(const WMat4& mTransform) const; // [tested]

  /// Flips all frustum planes around. Might be necessary after creating the frustum from a mirror projection matrix.
  void InvertFrustum(); // [tested]

  /// Computes the frustum corner points.
  ///
  /// Note: If the frustum contains an infinite far plane, the far plane corners (out_points[4..7])
  /// will be at infinity.
  WResult ComputeCornerPoints(WVec3 out_pPoints[FrustumCorner::CORNER_COUNT]) const; // [tested]

  /// Checks whether the given object is inside or outside the frustum.
  ///
  /// A concave object might be classified as 'intersecting' although it is outside the frustum, if it overlaps the planes just right.
  /// However an object that overlaps the frustum is definitely never classified as 'outside'.
  WVolumePosition::Enum GetObjectPosition(const WVec3* pVertices, WUInt32 uiNumVertices) const; // [tested]

  /// Same as GetObjectPosition(), but applies a transformation to the given object first. This allows to do culling on instanced
  /// objects.
  WVolumePosition::Enum GetObjectPosition(const WVec3* pVertices, WUInt32 uiNumVertices, const WMat4& mObjectTransform) const; // [tested]

  /// Checks whether the given object is inside or outside the frustum.
  WVolumePosition::Enum GetObjectPosition(const WBoundingSphere& sphere) const; // [tested]

  /// Checks whether the given object is inside or outside the frustum.
  WVolumePosition::Enum GetObjectPosition(const WBoundingBox& box) const; // [tested]

  /// Returns true if the object is fully inside the frustum or partially overlaps it. Returns false when the object is fully outside
  /// the frustum.
  ///
  /// This function is more efficient than GetObjectPosition() and should be preferred when possible.
  bool Overlaps(const WSimdBBox& object) const; // [tested]

  /// Returns true if the object is fully inside the frustum or partially overlaps it. Returns false when the object is fully outside
  /// the frustum.
  ///
  /// This function is more efficient than GetObjectPosition() and should be preferred when possible.
  bool Overlaps(const WSimdBSphere& object) const; // [tested]

private:
  WPlane m_Planes[PLANE_COUNT];
};

#include <Foundation/Math/Implementation/Frustum_inl.h>
