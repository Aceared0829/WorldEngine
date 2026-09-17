#pragma once

#include <Foundation/Math/Mat4.h>

namespace WGraphicsUtils
{
  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  W_FOUNDATION_DLL void ConvertScreenPixelPosToNormalizedPos(const WUInt32 uiViewportX, const WUInt32 uiViewportY, const WUInt32 uiViewportWidth, const WUInt32 uiViewportHeight, WVec3& inout_vPixelPos);

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  W_FOUNDATION_DLL void ConvertScreenNormalizedPosToPixelPos(const WUInt32 uiViewportX, const WUInt32 uiViewportY, const WUInt32 uiViewportWidth, const WUInt32 uiViewportHeight, WVec3& inout_vNormalizedPos);

  /// Projects the given point from 3D world space into screen space, if possible.
  ///
  /// \param ModelViewProjection
  ///   The Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see WClipSpaceDepthRange
  ///
  /// Returns W_FAILURE, if the point could not be projected into screen space.
  /// \note The function reports W_SUCCESS, when the point could be projected, however, that does not mean that the point actually lies
  /// within the viewport, it might still be outside the viewport.
  ///
  /// out_vScreenPos.z is the depth of the point in [0;1] range. The z value is always 'normalized' to this range
  /// (as long as the DepthRange parameter is correct), to make it easier to make subsequent code platform independent.
  W_FOUNDATION_DLL WResult ConvertWorldPosToScreenPos(const WMat4& mModelViewProjection, const WUInt32 uiViewportX, const WUInt32 uiViewportY,
    const WUInt32 uiViewportWidth, const WUInt32 uiViewportHeight, const WVec3& vPoint, WVec3& out_vScreenPos,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]

  /// Overload of ConvertWorldPosToScreenPos() that returns the screen position in normalized space ([0; 1] range) and therefore doesn't require the viewport dimensions.
  W_FOUNDATION_DLL WResult ConvertWorldPosToScreenPos(const WMat4& mModelViewProjection, const WVec3& vPoint, WVec3& out_vScreenPosNormalized,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]

  /// Takes the screen space position (including depth in [0;1] range) and converts it into a world space position.
  ///
  /// \param InverseModelViewProjection
  ///   The inverse of the Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see WClipSpaceDepthRange
  ///
  /// Returns W_FAILURE when the screen coordinate could not be converted to a world position,
  /// which should generally not be possible as long as the coordinate is actually inside the viewport.
  ///
  /// Optionally this function also computes the direction vector through the world space position, that should be used for picking
  /// operations. Note that for perspective cameras this is the same as the direction from the camera position to the computed point,
  /// but for orthographic cameras it is not (it's simply the forward vector of the camera).
  /// This function handles both cases properly.
  ///
  /// The z value of vScreenPixelPos is always expected to be in [0; 1] range (meaning 0 is at the near plane, 1 at the far plane),
  /// even on platforms that use [-1; +1] range for clip-space z values. The DepthRange parameter needs to be correct to handle this case
  /// properly.
  ///
  /// vScreenPixelPos is expected to be in range [viewport x/y; viewport width/height]. There is an overload below that takes just a normalized value
  /// in range [0; 1].
  W_FOUNDATION_DLL WResult ConvertScreenPosToWorldPos(const WMat4& mInverseModelViewProjection, const WUInt32 uiViewportX,
    const WUInt32 uiViewportY, const WUInt32 uiViewportWidth, const WUInt32 uiViewportHeight, const WVec3& vScreenPixelPos, WVec3& out_vPoint,
    WVec3* out_pDirection = nullptr, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]

  /// Overload of ConvertScreenPosToWorldPos() that takes the coordinate in normalized space ([0; 1]) and therefore doesn't require the viewport dimensions.
  W_FOUNDATION_DLL WResult ConvertScreenPosToWorldPos(const WMat4& mInverseModelViewProjection, const WVec3& vNormalizedScreenPos, WVec3& out_vPoint,
    WVec3* out_pDirection = nullptr, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]

  /// A double-precision version of ConvertScreenPosToWorldPos()
  W_FOUNDATION_DLL WResult ConvertScreenPosToWorldPos(const WMat4d& mInverseModelViewProjection, const WUInt32 uiViewportX,
    const WUInt32 uiViewportY, const WUInt32 uiViewportWidth, const WUInt32 uiViewportHeight, const WVec3& vScreenPixelPos, WVec3& out_vPoint,
    WVec3* out_pDirection = nullptr, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]

  /// Double-precision overload of ConvertScreenPosToWorldPos() that takes the coordinate in normalized space ([0; 1]) and therefore doesn't require the viewport dimensions.
  W_FOUNDATION_DLL WResult ConvertScreenPosToWorldPos(const WMat4d& mInverseModelViewProjection, const WVec3& vNormalizedScreenPos,
    WVec3& out_vPoint, WVec3* out_pDirection = nullptr, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]

  /// Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  W_FOUNDATION_DLL bool IsTriangleFlipRequired(const WMat3& mTransformation);

  /// Converts a projection or view-projection matrix from one depth-range convention to another
  W_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(WMat4& inout_mMatrix, WClipSpaceDepthRange::Enum srcDepthRange, WClipSpaceDepthRange::Enum dstDepthRange); // [tested]

  /// Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  W_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const WMat4& mProjectionMatrix, WAngle& out_fovX, WAngle& out_fovY); // [tested]

  /// Extracts the field of view angles from a perspective matrix.
  /// \param ProjectionMatrix Perspective projection matrix to be decomposed.
  /// \param out_fFovLeft Left angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovRight Right angle of the frustum.
  /// \param out_fFovBottom Bottom angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovTop Top angle of the frustum.
  /// \param yRange The Y range used to construct the perspective matrix.
  W_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const WMat4& mProjectionMatrix, WAngle& out_fovLeft, WAngle& out_fovRight, WAngle& out_fovBottom, WAngle& out_fovTop, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular); // [tested]

  /// Extracts the field of view distances on the near plane from a perspective matrix.
  ///
  /// Convenience function that also extracts near / far values and returns the distances on the near plane to be the inverse of WGraphicsUtils::CreatePerspectiveProjectionMatrix.
  /// \sa WGraphicsUtils::CreatePerspectiveProjectionMatrix
  W_FOUNDATION_DLL WResult ExtractPerspectiveMatrixFieldOfView(const WMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular); // [tested]

  /// Computes the distances of the near and far clip planes from the given perspective projection matrix.
  ///
  /// Returns W_FAILURE when one of the values could not be computed, because it would result in a "division by zero".
  W_FOUNDATION_DLL WResult ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const WMat4& mProjectionMatrix,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]


  enum class FrustumPlaneInterpolation
  {
    LeftToRight,
    BottomToTop,
    NearToFar,
  };

  /// Computes an interpolated frustum plane by using linear interpolation in normalized clip space.
  ///
  /// Along left/right, up/down this makes it easy to create a regular grid of planes.
  /// Along near/far creating planes at regular intervals will result in planes in world-space that represent
  /// the same amount of depth-precision.
  ///
  /// \param dir Specifies which planes to interpolate.
  /// \param fLerpFactor The interpolation coefficient (usually in the interval [0;1]).
  W_FOUNDATION_DLL WPlane ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor, const WMat4& mProjectionMatrix,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default); // [tested]

  /// Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  W_FOUNDATION_DLL WMat4 CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular,
    WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Creates a perspective projection matrix.
  W_FOUNDATION_DLL WMat4 CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular,
    WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  W_FOUNDATION_DLL WMat4 CreatePerspectiveProjectionMatrixFromFovX(WAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular,
    WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  W_FOUNDATION_DLL WMat4 CreatePerspectiveProjectionMatrixFromFovY(WAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular,
    WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  W_FOUNDATION_DLL WMat4 CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular,
    WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Creates an orthographic projection matrix.
  W_FOUNDATION_DLL WMat4 CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default, WClipSpaceYMode::Enum range = WClipSpaceYMode::Regular,
    WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  W_FOUNDATION_DLL WMat3 CreateLookAtViewMatrix(const WVec3& vTarget, const WVec3& vUpDir, WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Same as CreateLookAtViewMatrix() but returns the inverse matrix
  W_FOUNDATION_DLL WMat3 CreateInverseLookAtViewMatrix(const WVec3& vTarget, const WVec3& vUpDir, WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Returns a look-at matrix with both rotation and translation
  W_FOUNDATION_DLL WMat4 CreateLookAtViewMatrix(const WVec3& vEyePos, const WVec3& vLookAtPos, const WVec3& vUpDir,
    WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Same as CreateLookAtViewMatrix() but returns the inverse matrix
  W_FOUNDATION_DLL WMat4 CreateInverseLookAtViewMatrix(const WVec3& vEyePos, const WVec3& vLookAtPos, const WVec3& vUpDir, WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  W_FOUNDATION_DLL WMat4 CreateViewMatrix(const WVec3& vPosition, const WVec3& vForwardDir, const WVec3& vRightDir, const WVec3& vUpDir, WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Similar to CreateViewMatrix() but creates the inverse matrix.
  W_FOUNDATION_DLL WMat4 CreateInverseViewMatrix(const WVec3& vPosition, const WVec3& vForwardDir, const WVec3& vRightDir, const WVec3& vUpDir, WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  W_FOUNDATION_DLL void DecomposeViewMatrix(WVec3& out_vPosition, WVec3& out_vForwardDir, WVec3& out_vRightDir, WVec3& out_vUpDir, const WMat4& mViewMatrix, WHandedness::Enum handedness = WHandedness::Default); // [tested]

  /// Computes the barycentric coordinates of a point in a 3D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns W_FAILURE.
  W_FOUNDATION_DLL WResult ComputeBarycentricCoordinates(WVec3& out_vCoordinates, const WVec3& v0, const WVec3& v1, const WVec3& v2, const WVec3& vPos);

  /// Computes the barycentric coordinates of a point in a 2D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns W_FAILURE.
  W_FOUNDATION_DLL WResult ComputeBarycentricCoordinates(WVec3& out_vCoordinates, const WVec2& v0, const WVec2& v1, const WVec2& v2, const WVec2& vPos);

  /// Returns a coverage value of how much space a sphere at a given location would take up on screen using a perspective projection.
  ///
  /// The coverage value is close to 0 for very small or far away spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction, it only depends on distance and the camera's field-of-view.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera FOV angle is used for the calculation, pass in either the horizontal or vertical FOV angle,
  /// depending on what is most relevant to you.
  /// Typically the 'fixed' angle is used (usually the vertical one) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(const WBoundingSphere& sphere, const WVec3& vCameraPosition, WAngle perspectiveCameraFov)
  {
    const float fDist = (sphere.m_vCenter - vCameraPosition).GetLength();
    const float fHalfHeight = WMath::Tan(perspectiveCameraFov * 0.5f) * fDist;
    return sphere.m_fRadius / fHalfHeight;
  }

  /// Returns a coverage value of how much space a sphere of a given size would take up on screen using an orthographic projection.
  ///
  /// The coverage value is close to 0 for very small spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction. In orthographic projections even the distance to the camera is irrelevant,
  /// only the dimensions of the ortho camera are needed.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera dimension is used for the calculation, pass in either the X or Y dimension, depending on what is most relevant to you.
  /// Typically the 'fixed' dimension is used (usually Y) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(float fSphereRadius, float fOrthoCameraDimensions)
  {
    const float fHalfHeight = fOrthoCameraDimensions * 0.5f;
    return fSphereRadius / fHalfHeight;
  }

} // namespace WGraphicsUtils
