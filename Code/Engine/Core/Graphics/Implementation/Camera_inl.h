#pragma once

inline WVec3 WCamera::GetCenterPosition() const
{
  if (m_Mode == WCameraMode::Stereo)
    return (GetPosition(WCameraEye::Left) + GetPosition(WCameraEye::Right)) * 0.5f;
  else
    return GetPosition();
}

inline WVec3 WCamera::GetCenterDirForwards() const
{
  if (m_Mode == WCameraMode::Stereo)
    return (GetDirForwards(WCameraEye::Left) + GetDirForwards(WCameraEye::Right)).GetNormalized();
  else
    return GetDirForwards();
}

inline WVec3 WCamera::GetCenterDirUp() const
{
  if (m_Mode == WCameraMode::Stereo)
    return (GetDirUp(WCameraEye::Left) + GetDirUp(WCameraEye::Right)).GetNormalized();
  else
    return GetDirUp();
}

inline WVec3 WCamera::GetCenterDirRight() const
{
  if (m_Mode == WCameraMode::Stereo)
    return (GetDirRight(WCameraEye::Left) + GetDirRight(WCameraEye::Right)).GetNormalized();
  else
    return GetDirRight();
}

W_ALWAYS_INLINE float WCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

W_ALWAYS_INLINE float WCamera::GetFarPlane() const
{
  return m_fFarPlane;
}

W_ALWAYS_INLINE float WCamera::GetFovOrDim() const
{
  return m_fFovOrDim;
}

W_ALWAYS_INLINE WCameraMode::Enum WCamera::GetCameraMode() const
{
  return m_Mode;
}

W_ALWAYS_INLINE bool WCamera::IsPerspective() const
{
  return m_Mode == WCameraMode::PerspectiveFixedFovX || m_Mode == WCameraMode::PerspectiveFixedFovY ||
         m_Mode == WCameraMode::Stereo; // All HMD stereo cameras are perspective!
}

W_ALWAYS_INLINE bool WCamera::IsOrthographic() const
{
  return m_Mode == WCameraMode::OrthoFixedWidth || m_Mode == WCameraMode::OrthoFixedHeight;
}

W_ALWAYS_INLINE bool WCamera::IsStereoscopic() const
{
  return m_Mode == WCameraMode::Stereo;
}

W_ALWAYS_INLINE float WCamera::GetExposure() const
{
  return m_fExposure;
}

W_ALWAYS_INLINE void WCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}

W_ALWAYS_INLINE const WMat4& WCamera::GetViewMatrix(WCameraEye eye) const
{
  return m_mViewMatrix[static_cast<int>(eye)];
}
