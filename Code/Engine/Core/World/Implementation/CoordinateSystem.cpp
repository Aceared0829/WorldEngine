#include <Core/CorePCH.h>

#include <Core/World/CoordinateSystem.h>


WCoordinateSystemConversion::WCoordinateSystemConversion()
{
  m_mSourceToTarget.SetIdentity();
  m_mTargetToSource.SetIdentity();
}

void WCoordinateSystemConversion::SetConversion(const WCoordinateSystem& source, const WCoordinateSystem& target)
{
  float fSourceScale = source.m_vForwardDir.GetLengthSquared();
  W_ASSERT_DEV(WMath::IsEqual(fSourceScale, source.m_vRightDir.GetLengthSquared(), WMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  W_ASSERT_DEV(WMath::IsEqual(fSourceScale, source.m_vUpDir.GetLengthSquared(), WMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  WMat3 mSourceFromId;
  mSourceFromId.SetColumn(0, source.m_vRightDir);
  mSourceFromId.SetColumn(1, source.m_vUpDir);
  mSourceFromId.SetColumn(2, source.m_vForwardDir);

  float fTargetScale = target.m_vForwardDir.GetLengthSquared();
  W_ASSERT_DEV(WMath::IsEqual(fTargetScale, target.m_vRightDir.GetLengthSquared(), WMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  W_ASSERT_DEV(WMath::IsEqual(fTargetScale, target.m_vUpDir.GetLengthSquared(), WMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  WMat3 mTargetFromId;
  mTargetFromId.SetColumn(0, target.m_vRightDir);
  mTargetFromId.SetColumn(1, target.m_vUpDir);
  mTargetFromId.SetColumn(2, target.m_vForwardDir);

  m_mSourceToTarget = mTargetFromId * mSourceFromId.GetInverse();
  m_mSourceToTarget.SetColumn(0, m_mSourceToTarget.GetColumn(0).GetNormalized());
  m_mSourceToTarget.SetColumn(1, m_mSourceToTarget.GetColumn(1).GetNormalized());
  m_mSourceToTarget.SetColumn(2, m_mSourceToTarget.GetColumn(2).GetNormalized());

  m_fWindingSwap = m_mSourceToTarget.GetDeterminant() < 0 ? -1.0f : 1.0f;
  m_fSourceToTargetScale = 1.0f / WMath::Sqrt(fSourceScale) * WMath::Sqrt(fTargetScale);
  m_mTargetToSource = m_mSourceToTarget.GetInverse();
  m_fTargetToSourceScale = 1.0f / m_fSourceToTargetScale;
}

WVec3 WCoordinateSystemConversion::ConvertSourcePosition(const WVec3& vPos) const
{
  return m_mSourceToTarget * vPos * m_fSourceToTargetScale;
}

WQuat WCoordinateSystemConversion::ConvertSourceRotation(const WQuat& qOrientation) const
{
  WVec3 axis = m_mSourceToTarget * qOrientation.GetVectorPart();
  WQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float WCoordinateSystemConversion::ConvertSourceLength(float fLength) const
{
  return fLength * m_fSourceToTargetScale;
}

WVec3 WCoordinateSystemConversion::ConvertTargetPosition(const WVec3& vPos) const
{
  return m_mTargetToSource * vPos * m_fTargetToSourceScale;
}

WQuat WCoordinateSystemConversion::ConvertTargetRotation(const WQuat& qOrientation) const
{
  WVec3 axis = m_mTargetToSource * qOrientation.GetVectorPart();
  WQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float WCoordinateSystemConversion::ConvertTargetLength(float fLength) const
{
  return fLength * m_fTargetToSourceScale;
}
