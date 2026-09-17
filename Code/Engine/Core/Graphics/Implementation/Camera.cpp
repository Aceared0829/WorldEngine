#include <Core/CorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Utilities/GraphicsUtils.h>

class RemapCoordinateSystemProvider : public WCoordinateSystemProvider
{
public:
  RemapCoordinateSystemProvider()
    : WCoordinateSystemProvider(nullptr)
  {
  }

  virtual void GetCoordinateSystem(const WVec3& vGlobalPosition, WCoordinateSystem& out_coordinateSystem) const override
  {
    W_IGNORE_UNUSED(vGlobalPosition);

    out_coordinateSystem.m_vForwardDir = WBasisAxis::GetBasisVector(m_ForwardAxis);
    out_coordinateSystem.m_vRightDir = WBasisAxis::GetBasisVector(m_RightAxis);
    out_coordinateSystem.m_vUpDir = WBasisAxis::GetBasisVector(m_UpAxis);
  }

  WBasisAxis::Enum m_ForwardAxis = WBasisAxis::PositiveX;
  WBasisAxis::Enum m_RightAxis = WBasisAxis::PositiveY;
  WBasisAxis::Enum m_UpAxis = WBasisAxis::PositiveZ;
};

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WCameraMode, 1)
  W_ENUM_CONSTANT(WCameraMode::PerspectiveFixedFovX),
  W_ENUM_CONSTANT(WCameraMode::PerspectiveFixedFovY),
  W_ENUM_CONSTANT(WCameraMode::OrthoFixedWidth),
  W_ENUM_CONSTANT(WCameraMode::OrthoFixedHeight),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WCamera::WCamera()
{
  m_vCameraPosition[0].SetZero();
  m_vCameraPosition[1].SetZero();
  m_mViewMatrix[0].SetIdentity();
  m_mViewMatrix[1].SetIdentity();
  m_mStereoProjectionMatrix[0].SetIdentity();
  m_mStereoProjectionMatrix[1].SetIdentity();

  SetCoordinateSystem(WBasisAxis::PositiveX, WBasisAxis::PositiveY, WBasisAxis::PositiveZ);
}

void WCamera::SetCoordinateSystem(WBasisAxis::Enum forwardAxis, WBasisAxis::Enum rightAxis, WBasisAxis::Enum axis)
{
  auto provider = W_DEFAULT_NEW(RemapCoordinateSystemProvider);
  provider->m_ForwardAxis = forwardAxis;
  provider->m_RightAxis = rightAxis;
  provider->m_UpAxis = axis;

  m_pCoordinateSystem = provider;
}

void WCamera::SetCoordinateSystem(const WSharedPtr<WCoordinateSystemProvider>& pProvider)
{
  m_pCoordinateSystem = pProvider;
}

WVec3 WCamera::GetPosition(WCameraEye eye) const
{
  return MapInternalToExternal(m_vCameraPosition[static_cast<int>(eye)]);
}

WVec3 WCamera::GetDirForwards(WCameraEye eye) const
{
  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], WHandedness::LeftHanded);

  return MapInternalToExternal(decFwd);
}

WVec3 WCamera::GetDirUp(WCameraEye eye) const
{
  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], WHandedness::LeftHanded);

  return MapInternalToExternal(decUp);
}

WVec3 WCamera::GetDirRight(WCameraEye eye) const
{
  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], WHandedness::LeftHanded);

  return MapInternalToExternal(decRight);
}

WVec3 WCamera::InternalGetPosition(WCameraEye eye) const
{
  return m_vCameraPosition[static_cast<int>(eye)];
}

WVec3 WCamera::InternalGetDirForwards(WCameraEye eye) const
{
  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], WHandedness::LeftHanded);

  return decFwd;
}

WVec3 WCamera::InternalGetDirUp(WCameraEye eye) const
{
  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], WHandedness::LeftHanded);

  return decUp;
}

WVec3 WCamera::InternalGetDirRight(WCameraEye eye) const
{
  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], WHandedness::LeftHanded);

  return -decRight;
}

WVec3 WCamera::MapExternalToInternal(const WVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    WCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    WMat3 m;
    m.SetRow(0, system.m_vForwardDir);
    m.SetRow(1, system.m_vRightDir);
    m.SetRow(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

WVec3 WCamera::MapInternalToExternal(const WVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    WCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    WMat3 m;
    m.SetColumn(0, system.m_vForwardDir);
    m.SetColumn(1, system.m_vRightDir);
    m.SetColumn(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

WAngle WCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == WCameraMode::PerspectiveFixedFovX)
    return WAngle::MakeFromDegree(m_fFovOrDim);

  if (m_Mode == WCameraMode::PerspectiveFixedFovY)
    return WMath::ATan(WMath::Tan(WAngle::MakeFromDegree(m_fFovOrDim) * 0.5f) * fAspectRatioWidthDivHeight) * 2.0f;

  // TODO: HACK
  if (m_Mode == WCameraMode::Stereo)
    return WAngle::MakeFromDegree(90);

  W_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return WAngle();
}

WAngle WCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == WCameraMode::PerspectiveFixedFovX)
    return WMath::ATan(WMath::Tan(WAngle::MakeFromDegree(m_fFovOrDim) * 0.5f) / fAspectRatioWidthDivHeight) * 2.0f;

  if (m_Mode == WCameraMode::PerspectiveFixedFovY)
    return WAngle::MakeFromDegree(m_fFovOrDim);

  // TODO: HACK
  if (m_Mode == WCameraMode::Stereo)
    return WAngle::MakeFromDegree(90);

  W_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return WAngle();
}


float WCamera::GetDimensionX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == WCameraMode::OrthoFixedWidth)
    return m_fFovOrDim;

  if (m_Mode == WCameraMode::OrthoFixedHeight)
    return m_fFovOrDim * fAspectRatioWidthDivHeight;

  W_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}


float WCamera::GetDimensionY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == WCameraMode::OrthoFixedWidth)
    return m_fFovOrDim / fAspectRatioWidthDivHeight;

  if (m_Mode == WCameraMode::OrthoFixedHeight)
    return m_fFovOrDim;

  W_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}

void WCamera::SetCameraMode(WCameraMode::Enum mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == mode && m_fFovOrDim == fFovOrDim && m_fNearPlane == fNearPlane && m_fFarPlane == fFarPlane)
  {
    return;
  }

  m_Mode = mode;
  m_fFovOrDim = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane = fFarPlane;

  m_fAspectOfPrecomputedStereoProjection = -1.0f;

  CameraSettingsChanged();
}

void WCamera::SetStereoProjection(const WMat4& mProjectionLeftEye, const WMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight)
{
  if (m_mStereoProjectionMatrix[static_cast<int>(WCameraEye::Left)] == mProjectionLeftEye && m_mStereoProjectionMatrix[static_cast<int>(WCameraEye::Right)] == mProjectionRightEye && m_fAspectOfPrecomputedStereoProjection == fAspectRatioWidthDivHeight)
  {
    return;
  }

  m_mStereoProjectionMatrix[static_cast<int>(WCameraEye::Left)] = mProjectionLeftEye;
  m_mStereoProjectionMatrix[static_cast<int>(WCameraEye::Right)] = mProjectionRightEye;
  m_fAspectOfPrecomputedStereoProjection = fAspectRatioWidthDivHeight;

  CameraSettingsChanged();
}

void WCamera::LookAt(const WVec3& vCameraPos0, const WVec3& vTargetPos0, const WVec3& vUp0)
{
  const WVec3 vCameraPos = MapExternalToInternal(vCameraPos0);
  const WVec3 vTargetPos = MapExternalToInternal(vTargetPos0);
  const WVec3 vUp = MapExternalToInternal(vUp0);

  if (m_Mode == WCameraMode::Stereo)
  {
    W_REPORT_FAILURE("WCamera::LookAt is not possible for stereo cameras.");
    return;
  }

  m_mViewMatrix[0] = WGraphicsUtils::CreateLookAtViewMatrix(vCameraPos, vTargetPos, vUp, WHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];
  m_vCameraPosition[1] = m_vCameraPosition[0] = vCameraPos;

  CameraOrientationChanged();
}

void WCamera::SetViewMatrix(const WMat4& mLookAtMatrix, WCameraEye eye)
{
  const int iEyeIdx = static_cast<int>(eye);

  m_mViewMatrix[iEyeIdx] = mLookAtMatrix;

  WVec3 decFwd, decRight, decUp;
  WGraphicsUtils::DecomposeViewMatrix(
    m_vCameraPosition[iEyeIdx], decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], WHandedness::LeftHanded);

  if (m_Mode != WCameraMode::Stereo)
  {
    m_mViewMatrix[1 - iEyeIdx] = m_mViewMatrix[iEyeIdx];
    m_vCameraPosition[1 - iEyeIdx] = m_vCameraPosition[iEyeIdx];
  }

  CameraOrientationChanged();
}

void WCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, WMat4& out_mProjectionMatrix, WCameraEye eye, WClipSpaceDepthRange::Enum depthRange) const
{
  switch (m_Mode)
  {
    case WCameraMode::PerspectiveFixedFovX:
      out_mProjectionMatrix = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(WAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
      break;

    case WCameraMode::PerspectiveFixedFovY:
      out_mProjectionMatrix = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(WAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
      break;

    case WCameraMode::OrthoFixedWidth:
      out_mProjectionMatrix = WGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight, m_fNearPlane,
        m_fFarPlane, depthRange, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
      break;

    case WCameraMode::OrthoFixedHeight:
      out_mProjectionMatrix = WGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim * fAspectRatioWidthDivHeight, m_fFovOrDim, m_fNearPlane,
        m_fFarPlane, depthRange, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
      break;

    case WCameraMode::Stereo:
      if (WMath::IsEqual(m_fAspectOfPrecomputedStereoProjection, fAspectRatioWidthDivHeight, WMath::LargeEpsilon<float>()))
        out_mProjectionMatrix = m_mStereoProjectionMatrix[static_cast<int>(eye)];
      else
      {
        // Evade to FixedFovY
        out_mProjectionMatrix = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(WAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
          m_fNearPlane, m_fFarPlane, depthRange, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
      }
      break;

    default:
      W_REPORT_FAILURE("Invalid Camera Mode {0}", (int)m_Mode);
  }
}

void WCamera::CameraSettingsChanged()
{
  W_ASSERT_DEV(m_Mode != WCameraMode::None, "Invalid Camera Mode.");
  W_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  W_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

void WCamera::MoveLocally(float fForward, float fRight, float fUp)
{
  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() - WVec3(fRight, fUp, fForward));
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], WHandedness::LeftHanded);

  m_vCameraPosition[0] = m_vCameraPosition[1] = decPos;

  CameraOrientationChanged();
}

void WCamera::MoveGlobally(float fForward, float fRight, float fUp)
{
  WVec3 vMove(fForward, fRight, fUp);

  WVec3 decFwd, decRight, decUp, decPos;
  WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], WHandedness::LeftHanded);

  m_vCameraPosition[0] += vMove;
  m_vCameraPosition[1] = m_vCameraPosition[0];

  m_mViewMatrix[0] = WGraphicsUtils::CreateViewMatrix(m_vCameraPosition[0], decFwd, decRight, decUp, WHandedness::LeftHanded);

  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  CameraOrientationChanged();
}

void WCamera::ClampRotationAngles(bool bLocalSpace, WAngle& forwardAxis, WAngle& rightAxis, WAngle& upAxis)
{
  W_IGNORE_UNUSED(forwardAxis);
  W_IGNORE_UNUSED(upAxis);

  if (bLocalSpace)
  {
    if (rightAxis.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float fDot = InternalGetDirForwards().Dot(WVec3(0, 0, -1));
      const WAngle fCurAngle = WMath::ACos(fDot) - WAngle::MakeFromDegree(90.0f);
      const WAngle fNewAngle = fCurAngle + rightAxis;

      const WAngle fAllowedAngle = WMath::Clamp(fNewAngle, WAngle::MakeFromDegree(-85.0f), WAngle::MakeFromDegree(85.0f));

      rightAxis = fAllowedAngle - fCurAngle;
    }
  }
}

void WCamera::RotateLocally(WAngle forwardAxis, WAngle rightAxis, WAngle axis)
{
  ClampRotationAngles(true, forwardAxis, rightAxis, axis);

  WVec3 vDirForwards = InternalGetDirForwards();
  WVec3 vDirUp = InternalGetDirUp();
  WVec3 vDirRight = InternalGetDirRight();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    WMat3 m = WMat3::MakeAxisRotation(vDirForwards, forwardAxis);

    vDirUp = m * vDirUp;
    vDirRight = m * vDirRight;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    WMat3 m = WMat3::MakeAxisRotation(vDirRight, rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (axis.GetRadian() != 0.0f)
  {
    WMat3 m = WMat3::MakeAxisRotation(vDirUp, axis);

    vDirRight = m * vDirRight;
    vDirForwards = m * vDirForwards;
  }

  // Using WGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = WGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, WHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged();
}

void WCamera::RotateGlobally(WAngle forwardAxis, WAngle rightAxis, WAngle axis)
{
  ClampRotationAngles(false, forwardAxis, rightAxis, axis);

  WVec3 vDirForwards = InternalGetDirForwards();
  WVec3 vDirUp = InternalGetDirUp();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    WMat3 m;
    m = WMat3::MakeRotationX(forwardAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    WMat3 m;
    m = WMat3::MakeRotationY(rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (axis.GetRadian() != 0.0f)
  {
    WMat3 m;
    m = WMat3::MakeRotationZ(axis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  // Using WGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = WGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, WHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged();
}



W_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);
