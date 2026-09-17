#include <AiPlugin/Navigation/Steering.h>

void WAiSteering::Calculate(float fTimeDiff, WDebugRendererContext ctxt)
{
  const float fRunSpeed = m_fMaxSpeed;
  const float fJogSpeed = m_fMaxSpeed * 0.75f;
  const float fWalkSpeed = m_fMaxSpeed * 0.5f;

  const float fBrakingDistanceRun = 1.2f * (WMath::Square(fRunSpeed) / (2.0f * m_fDecceleration));
  const float fBrakingDistanceJog = 1.2f * (WMath::Square(fJogSpeed) / (2.0f * m_fDecceleration));
  const float fBrakingDistanceWalk = 1.2f * (WMath::Square(fWalkSpeed) / (2.0f * m_fDecceleration));

  if (m_Info.m_fArrivalDistance <= fBrakingDistanceWalk)
    m_fMaxSpeed = 0.0f;
  else if (m_Info.m_fArrivalDistance < fBrakingDistanceJog)
    m_fMaxSpeed = WMath::Min(m_fMaxSpeed, fWalkSpeed);
  else if (m_Info.m_fArrivalDistance < fBrakingDistanceRun)
    m_fMaxSpeed = WMath::Min(m_fMaxSpeed, fJogSpeed);

  if (m_Info.m_AbsRotationTowardsWaypoint > WAngle::MakeFromDegree(80))
    m_fMaxSpeed = 0.0f;
  else if (m_Info.m_AbsRotationTowardsWaypoint > WAngle::MakeFromDegree(55))
    m_fMaxSpeed = WMath::Min(m_fMaxSpeed, fWalkSpeed);
  else if (m_Info.m_AbsRotationTowardsWaypoint > WAngle::MakeFromDegree(30))
    m_fMaxSpeed = WMath::Min(m_fMaxSpeed, fJogSpeed);

  WAngle maxRotation = m_Info.m_AbsRotationTowardsWaypoint;

  if (m_Info.m_fDistanceToWaypoint <= fBrakingDistanceRun)
  {
    if (m_Info.m_MaxAbsRotationAfterWaypoint > WAngle::MakeFromDegree(40))
      m_fMaxSpeed = WMath::Min(m_fMaxSpeed, fWalkSpeed);
    else if (m_Info.m_MaxAbsRotationAfterWaypoint > WAngle::MakeFromDegree(20))
      m_fMaxSpeed = WMath::Min(m_fMaxSpeed, fJogSpeed);

    maxRotation = WMath::Max(maxRotation, m_Info.m_MaxAbsRotationAfterWaypoint);
  }

  WVec3 vLookDir = m_qRotation * WVec3::MakeAxisX();
  vLookDir.z = 0;
  vLookDir.Normalize();

  float fCurSpeed = m_vVelocity.GetAsVec2().GetLength();

  WAngle turnSpeed = m_MinTurnSpeed;

  {
    const float fTurnRadius = 1.0f; // WMath::Clamp(m_Info.m_fWaypointCorridorWidth, 0.5f, 5.0f);
    const float fCircumference = 2.0f * WMath::Pi<float>() * fTurnRadius;
    const float fCircleFraction = maxRotation / WAngle::MakeFromDegree(360);
    const float fTurnDistance = fCircumference * fCircleFraction;
    const float fTurnDuration = fTurnDistance / fCurSpeed;

    turnSpeed = WMath::Max(m_MinTurnSpeed, maxRotation / fTurnDuration);
  }

  // WDebugRenderer::DrawInfoText(ctxt, WDebugRenderer::ScreenPlacement::BottomLeft, "Steering", WFmt("Turn Speed: {}", turnSpeed));
  // WDebugRenderer::DrawInfoText(ctxt, WDebugRenderer::ScreenPlacement::BottomLeft, "Steering", WFmt("Corridor Width: {}", m_Info.m_fWaypointCorridorWidth));

  if (!m_Info.m_vDirectionTowardsWaypoint.IsZero())
  {
    const WVec3 vTargetDir = m_Info.m_vDirectionTowardsWaypoint.GetAsVec3(0);
    WVec3 vRotAxis = vLookDir.CrossRH(vTargetDir);
    vRotAxis.NormalizeIfNotZero(WVec3::MakeAxisZ()).IgnoreResult();
    const WAngle toRotate = WMath::Min(vLookDir.GetAngleBetween(vTargetDir), fTimeDiff * turnSpeed);
    const WQuat qRot = WQuat::MakeFromAxisAndAngle(vRotAxis, toRotate);

    vLookDir = qRot * vLookDir;

    m_qRotation = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), vLookDir);
  }


  if (fCurSpeed < m_fMaxSpeed)
  {
    fCurSpeed += fTimeDiff * m_fAcceleration;
    fCurSpeed = WMath::Min(fCurSpeed, m_fMaxSpeed);
  }
  else if (fCurSpeed > m_fMaxSpeed)
  {
    fCurSpeed -= fTimeDiff * m_fDecceleration;
    fCurSpeed = WMath::Max(fCurSpeed, m_fMaxSpeed);
  }

  WVec3 vDir = vLookDir;
  vDir *= fCurSpeed;
  vDir *= fTimeDiff;
  m_vPosition += vDir;
}
