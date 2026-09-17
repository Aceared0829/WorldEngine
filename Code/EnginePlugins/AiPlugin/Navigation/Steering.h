#pragma once

#include <AiPlugin/Navigation/Navigation.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

/// Work in progress, do not use.
///
/// Attempt to implement a steering behavior.
struct W_AIPLUGIN_DLL WAiSteering
{
  WVec3 m_vPosition = WVec3::MakeZero();
  WQuat m_qRotation = WQuat::MakeIdentity();
  WVec3 m_vVelocity = WVec3::MakeZero();
  float m_fMaxSpeed = 6.0f;
  float m_fAcceleration = 5.0f;
  float m_fDecceleration = 10.0f;
  WAngle m_MinTurnSpeed = WAngle::MakeFromDegree(180);

  WAiSteeringInfo m_Info;

  void Calculate(float fTimeDiff, WDebugRendererContext ctxt);
};
