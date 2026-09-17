#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

/// Specifies in which mode this camera is configured.
struct W_CORE_DLL WCameraMode
{
  using StorageType = WInt8;

  enum Enum
  {
    None,                 ///< Not initialized
    PerspectiveFixedFovX, ///< Perspective camera, the fov for X is fixed, Y depends on the aspect ratio
    PerspectiveFixedFovY, ///< Perspective camera, the fov for Y is fixed, X depends on the aspect ratio
    OrthoFixedWidth,      ///< Orthographic camera, the width is fixed, the height depends on the aspect ratio
    OrthoFixedHeight,     ///< Orthographic camera, the height is fixed, the width depends on the aspect ratio
    Stereo,               ///< A stereo camera with view/projection matrices provided by an HMD.
    Default = PerspectiveFixedFovY
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WCameraMode);

/// Determines left or right eye of a stereo camera.
///
/// As a general rule, this parameter does not matter for mono-scopic cameras and will always return the same value.
enum class WCameraEye
{
  Left,
  Right,
  // Two eyes should be enough for everyone.
};

/// A camera class that stores the orientation and camera settings for rendering.
///
/// The camera supports multiple modes including perspective and orthographic projection,
/// as well as stereoscopic rendering. Camera positions and orientations can be set directly
/// via view matrices or manipulated through movement and rotation functions.
///
/// The camera uses a configurable coordinate system for input and output coordinates.
/// By default, forward = +X, right = +Y, up = +Z.
///
/// For stereo cameras, separate view and projection matrices are maintained for each eye.
/// Modification counters track changes to camera settings and orientation for cache invalidation.
class W_CORE_DLL WCamera
{
public:
  WCamera();

  /// Allows to specify a different coordinate system in which the camera input and output coordinates are given.
  ///
  /// The default in z is forward = PositiveX, right = PositiveY, Up = PositiveZ.
  void SetCoordinateSystem(WBasisAxis::Enum forwardAxis, WBasisAxis::Enum rightAxis, WBasisAxis::Enum axis);

  /// Allows to specify a full WCoordinateSystemProvider to determine forward/right/up vectors for camera movement
  void SetCoordinateSystem(const WSharedPtr<WCoordinateSystemProvider>& pProvider);

  /// Returns the position of the camera that should be used for rendering etc.
  WVec3 GetPosition(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the forwards vector that should be used for rendering etc.
  WVec3 GetDirForwards(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the up vector that should be used for rendering etc.
  WVec3 GetDirUp(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the right vector that should be used for rendering etc.
  WVec3 GetDirRight(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the horizontal FOV.
  ///
  /// Works only with WCameraMode::PerspectiveFixedFovX and WCameraMode::PerspectiveFixedFovY
  WAngle GetFovX(float fAspectRatioWidthDivHeight) const;

  /// Returns the vertical FOV.
  ///
  /// Works only with WCameraMode::PerspectiveFixedFovX and WCameraMode::PerspectiveFixedFovY
  WAngle GetFovY(float fAspectRatioWidthDivHeight) const;

  /// Returns the horizontal dimension for an orthographic view.
  ///
  /// Works only with WCameraMode::OrthoFixedWidth and WCameraMode::OrthoFixedWidth
  float GetDimensionX(float fAspectRatioWidthDivHeight) const;

  /// Returns the vertical dimension for an orthographic view.
  ///
  /// Works only with WCameraMode::OrthoFixedWidth and WCameraMode::OrthoFixedWidth
  float GetDimensionY(float fAspectRatioWidthDivHeight) const;

  /// Returns the average camera position.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetPosition()
  WVec3 GetCenterPosition() const;

  /// Returns the average forwards vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirForwards()
  WVec3 GetCenterDirForwards() const;

  /// Returns the average up vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirUp()
  WVec3 GetCenterDirUp() const;

  /// Returns the average right vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirRight()
  WVec3 GetCenterDirRight() const;

  /// Returns the near plane distance that was passed to SetCameraProjectionAndMode().
  float GetNearPlane() const;

  /// Returns the far plane distance that was passed to SetCameraProjectionAndMode().
  float GetFarPlane() const;

  /// Specifies the mode and the projection settings that this camera uses.
  ///
  /// \param fFovOrDim
  ///   Fov X/Y in degree or width/height (depending on Mode).
  void SetCameraMode(WCameraMode::Enum mode, float fFovOrDim, float fNearPlane, float fFarPlane);

  /// Sets the camera mode to stereo and specifies projection matrices directly.
  ///
  /// \param fAspectRatio
  ///   These stereo projection matrices will only be returned by getProjectionMatrix for the given aspectRatio.
  void SetStereoProjection(const WMat4& mProjectionLeftEye, const WMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight);

  /// Returns the fFovOrDim parameter that was passed to SetCameraProjectionAndMode().
  float GetFovOrDim() const;

  /// Returns the current camera mode.
  WCameraMode::Enum GetCameraMode() const;

  bool IsPerspective() const;

  bool IsOrthographic() const;

  /// Whether this is a stereoscopic camera.
  bool IsStereoscopic() const;

  /// Sets the view matrix directly.
  ///
  /// Works with all camera types. Position- and direction- getter/setter will work as usual.
  void SetViewMatrix(const WMat4& mLookAtMatrix, WCameraEye eye = WCameraEye::Left);

  /// Repositions the camera such that it looks at the given target position.
  ///
  /// Not supported for stereo cameras.
  void LookAt(const WVec3& vCameraPos, const WVec3& vTargetPos, const WVec3& vUp);

  /// Moves the camera in its local space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveLocally(float fForward, float fRight, float fUp);

  /// Moves the camera in global space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveGlobally(float fForward, float fRight, float fUp);

  /// Rotates the camera around the forward, right and up axis in its own local space.
  ///
  /// Rotate around \a rightAxis for looking up/down. \forwardAxis is roll. For turning left/right use RotateGlobally().
  /// Not supported for stereo cameras.
  void RotateLocally(WAngle forwardAxis, WAngle rightAxis, WAngle axis);

  /// Rotates the camera around the forward, right and up axis of the coordinate system in global space.
  ///
  /// Rotate around Z for turning the camera left/right.
  /// Not supported for stereo cameras.
  void RotateGlobally(WAngle forwardAxis, WAngle rightAxis, WAngle axis);

  /// Returns the view matrix for the given eye.
  ///
  /// \note The view matrix is given in OpenGL convention.
  const WMat4& GetViewMatrix(WCameraEye eye = WCameraEye::Left) const;

  /// Calculates the projection matrix from the current camera properties and stores it in out_projectionMatrix.
  ///
  /// If the camera is stereo and the given aspect ratio is close to the aspect ratio passed in SetStereoProjection,
  /// the matrix set in SetStereoProjection will be used.
  void GetProjectionMatrix(float fAspectRatioWidthDivHeight, WMat4& out_mProjectionMatrix, WCameraEye eye = WCameraEye::Left,
    WClipSpaceDepthRange::Enum depthRange = WClipSpaceDepthRange::Default) const;

  float GetExposure() const;

  void SetExposure(float fExposure);

  /// Returns a counter that is increased every time the camera settings are modified.
  ///
  /// The camera settings are used to compute the projection matrix. This counter can be used to determine whether the projection matrix
  /// has changed and thus whether cached values need to be updated.
  WUInt32 GetSettingsModificationCounter() const { return m_uiSettingsModificationCounter; }

  /// Returns a counter that is increased every time the camera orientation is modified.
  ///
  /// The camera orientation is used to compute the view matrix. This counter can be used to determine whether the view matrix
  /// has changed and thus whether cached values need to be updated.
  WUInt32 GetOrientationModificationCounter() const { return m_uiOrientationModificationCounter; }

private:
  /// This function is called whenever the camera position or rotation changed.
  void CameraOrientationChanged() { ++m_uiOrientationModificationCounter; }

  /// This function is called when the camera mode or projection changes (e.g. SetCameraProjectionAndMode was called).
  void CameraSettingsChanged();

  /// This function is called by RotateLocally() and RotateGlobally() BEFORE the values are applied,
  /// and allows to adjust them (e.g. for limiting how far the camera can rotate).
  void ClampRotationAngles(bool bLocalSpace, WAngle& forwardAxis, WAngle& rightAxis, WAngle& upAxis);

  WVec3 InternalGetPosition(WCameraEye eye = WCameraEye::Left) const;
  WVec3 InternalGetDirForwards(WCameraEye eye = WCameraEye::Left) const;
  WVec3 InternalGetDirUp(WCameraEye eye = WCameraEye::Left) const;
  WVec3 InternalGetDirRight(WCameraEye eye = WCameraEye::Left) const;

  float m_fNearPlane = 0.1f;
  float m_fFarPlane = 1000.0f;

  WCameraMode::Enum m_Mode = WCameraMode::None;

  float m_fFovOrDim = 90.0f;

  float m_fExposure = 1.0f;

  WVec3 m_vCameraPosition[2];
  WMat4 m_mViewMatrix[2];

  /// If the camera mode is stereo and the aspect ratio given in getProjectio is close to this value, one of the stereo projection matrices
  /// is returned.
  float m_fAspectOfPrecomputedStereoProjection = -1.0;
  WMat4 m_mStereoProjectionMatrix[2];

  WUInt32 m_uiSettingsModificationCounter = 0;
  WUInt32 m_uiOrientationModificationCounter = 0;

  WSharedPtr<WCoordinateSystemProvider> m_pCoordinateSystem;

  WVec3 MapExternalToInternal(const WVec3& v) const;
  WVec3 MapInternalToExternal(const WVec3& v) const;
};


#include <Core/Graphics/Implementation/Camera_inl.h>
