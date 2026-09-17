#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/SteeringUtils.h>
#include <Foundation/Math/Mat3.h>

namespace
{
  // Rebuilds qRotation from vForward alone, choosing right/up so that up is as close to world up
  // (+Z) as geometrically possible - i.e. no roll. vForward is assumed to be normalized.
  void MakeUprightOrientation(WQuat& inout_qRotation, const WVec3& vForward)
  {
    WVec3 vRight = WVec3::MakeAxisZ().CrossRH(vForward);
    if (vRight.NormalizeIfNotZero(inout_qRotation * WVec3::MakeAxisY()).Failed())
    {
      // vForward is ~parallel to world up (looking straight up/down) *and* the object's previous
      // right vector happened to be too - fall back to a fixed axis rather than leaving it zero.
      vRight.NormalizeIfNotZero(WVec3::MakeAxisY()).IgnoreResult();
    }

    WVec3 vUp = vForward.CrossRH(vRight);
    vUp.NormalizeIfNotZero(WVec3::MakeAxisZ()).IgnoreResult();

    WMat3 mRot = WMat3::MakeIdentity();
    mRot.SetColumn(0, vForward);
    mRot.SetColumn(1, vRight);
    mRot.SetColumn(2, vUp);

    inout_qRotation = WQuat::MakeFromMat3(mRot);
  }
} // namespace

WAngle WAiSteeringUtils::TurnTowards(WQuat& inout_qRotation, const WVec3& vTargetDirection, WAngle maxAngularSpeed, float fTimeDiff)
{
  const WVec3 vCurrentForward = inout_qRotation * WVec3::MakeAxisX();

  WVec3 vNewForward = vCurrentForward;
  WAngle remaining = WAngle::MakeFromRadian(0);

  WVec3 vTargetDir = vTargetDirection;
  if (vTargetDir.NormalizeIfNotZero(WVec3::MakeZero()).Succeeded())
  {
    const WAngle angleBetween = vCurrentForward.GetAngleBetween(vTargetDir);

    if (angleBetween > WAngle::MakeFromDegree(0.01f))
    {
      WVec3 vRotAxis = vCurrentForward.CrossRH(vTargetDir);
      // Handles the near-180-degree case (cross product is ~zero): fall back to the object's own
      // up axis, so a full reversal turns around a sensible axis regardless of orientation.
      vRotAxis.NormalizeIfNotZero(inout_qRotation * WVec3::MakeAxisZ()).IgnoreResult();

      const WAngle maxStep = maxAngularSpeed * fTimeDiff;
      const WAngle toRotate = WMath::Min(angleBetween, maxStep);

      const WQuat qDelta = WQuat::MakeFromAxisAndAngle(vRotAxis, toRotate);
      vNewForward = qDelta * vCurrentForward;

      remaining = WMath::Max(angleBetween - toRotate, WAngle::MakeFromRadian(0));
    }
  }

  // Always re-derive the orientation from the (possibly unchanged) forward direction, so the
  // object stays upright / any existing roll is removed even when it isn't actively turning.
  MakeUprightOrientation(inout_qRotation, vNewForward);

  return remaining;
}

WAngle WAiSteeringUtils::GetAngleTowards(const WQuat& qRotation, const WVec3& vTargetDirection)
{
  WVec3 vTargetDir = vTargetDirection;
  if (vTargetDir.NormalizeIfNotZero(WVec3::MakeZero()).Failed())
    return WAngle::MakeFromRadian(0);

  const WVec3 vCurrentForward = qRotation * WVec3::MakeAxisX();
  return vCurrentForward.GetAngleBetween(vTargetDir);
}

float WAiSteeringUtils::ApplyAcceleration(float fCurrentSpeed, float fTargetSpeed, float fAcceleration, float fDeceleration, float fTimeDiff)
{
  if (fCurrentSpeed < fTargetSpeed)
  {
    return WMath::Min(fCurrentSpeed + fAcceleration * fTimeDiff, fTargetSpeed);
  }
  else
  {
    return WMath::Max(fCurrentSpeed - fDeceleration * fTimeDiff, fTargetSpeed);
  }
}
