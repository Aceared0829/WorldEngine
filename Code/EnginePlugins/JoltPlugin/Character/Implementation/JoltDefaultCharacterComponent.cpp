#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltDefaultCharacterComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>

WCVarBool cvar_JoltCcFootCheck("Jolt.CC.FootCheck", true, WCVarFlags::Default, "Stay down");

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltDefaultCharacterComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ShapeRadius", m_fShapeRadius)->AddAttributes(new WDefaultValueAttribute(0.25f)),
    W_MEMBER_PROPERTY("CrouchHeight", m_fCylinderHeightCrouch)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("StandHeight", m_fCylinderHeightStand)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("FootRadius", m_fFootRadius)->AddAttributes(new WDefaultValueAttribute(0.15f)),
    W_MEMBER_PROPERTY("WalkSpeedCrouching", m_fWalkSpeedCrouching)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("WalkSpeedStanding", m_fWalkSpeedStanding)->AddAttributes(new WDefaultValueAttribute(2.5f), new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("WalkSpeedRunning", m_fWalkSpeedRunning)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new WDefaultValueAttribute(2.5f), new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("MaxStepUp", m_fMaxStepUp)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("MaxStepDown", m_fMaxStepDown)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, 1000.0f)),
    W_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(90.0f)), new WClampValueAttribute(WAngle::MakeFromDegree(1.0f), WAngle::MakeFromDegree(360.0f))),
    W_ACCESSOR_PROPERTY("WalkSurfaceInteraction", GetWalkSurfaceInteraction, SetWalkSurfaceInteraction)->AddAttributes(new WDynamicStringEnumAttribute("SurfaceInteractionTypeEnum"), new WDefaultValueAttribute(WStringView("Footstep"))),
    W_MEMBER_PROPERTY("WalkInteractionDistance", m_fWalkInteractionDistance)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("RunInteractionDistance", m_fRunInteractionDistance)->AddAttributes(new WDefaultValueAttribute(3.0f)),
    W_ACCESSOR_PROPERTY("FallbackWalkSurface", GetFallbackWalkSurfaceFile, SetFallbackWalkSurfaceFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    W_ACCESSOR_PROPERTY("HeadObject", DummyGetter, SetHeadObjectReference)->AddAttributes(new WGameObjectReferenceAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCapsuleVisualizerAttribute("StandHeight", "ShapeRadius", WColor::WhiteSmoke, nullptr, WVisualizerAnchor::NegZ),
    new WCapsuleVisualizerAttribute("CrouchHeight", "ShapeRadius", WColor::LightSlateGrey, nullptr, WVisualizerAnchor::NegZ),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgMoveCharacterController, SetInputState),
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgApplyRootMotion, OnApplyRootMotion),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    //W_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    W_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    W_SCRIPT_FUNCTION_PROPERTY(IsStandingOnGround),
    W_SCRIPT_FUNCTION_PROPERTY(IsSlidingOnGround),
    W_SCRIPT_FUNCTION_PROPERTY(IsInAir),
    W_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
    W_SCRIPT_FUNCTION_PROPERTY(Jump),
    W_SCRIPT_FUNCTION_PROPERTY(Run),
    W_SCRIPT_FUNCTION_PROPERTY(Crouch),
    W_SCRIPT_FUNCTION_PROPERTY(RotateZ, In, "Amount"),
    W_SCRIPT_FUNCTION_PROPERTY(Move, In, "Forward", In, "Right"),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// clang-format on

/// Custom contact listener to send WMsgPhysicCharacterContact to objects that the CC touches.
class WJoltDefaultCharacterContactListener : public JPH::CharacterContactListener
{
public:
  JPH::PhysicsSystem* m_pSystem = nullptr;
  WJoltDefaultCharacterComponent* m_pCharacter = nullptr;

  virtual void OnContactAdded(const JPH::CharacterVirtual* pCharacter, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& ref_settings) override
  {
    JPH::BodyLockRead lock(m_pSystem->GetBodyLockInterface(), contact.mBodyB);
    if (lock.Succeeded())
    {
      if (WComponent* pComponent = WJoltUserData::GetComponent(reinterpret_cast<const void*>(lock.GetBody().GetUserData())))
      {
        WMsgPhysicCharacterContact msg;
        msg.m_hCharacter = m_pCharacter->GetHandle();
        msg.m_vGlobalPosition = WJoltConversionUtils::ToVec3(contact.mPosition);
        msg.m_vNormal = WJoltConversionUtils::ToVec3(contact.mContactNormal);
        msg.m_vCharacterVelocity = WJoltConversionUtils::ToVec3(pCharacter->GetLinearVelocity());
        msg.m_fImpact = WMath::Abs(msg.m_vNormal.Dot(msg.m_vCharacterVelocity));

        pComponent->SendMessage(msg);
      }
    }

    JPH::CharacterContactListener::OnContactAdded(pCharacter, contact, ref_settings);
  }
};

WJoltDefaultCharacterComponent::WJoltDefaultCharacterComponent() = default;
WJoltDefaultCharacterComponent::~WJoltDefaultCharacterComponent() = default;

void WJoltDefaultCharacterComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3(0, 0, GetShapeRadius()), GetShapeRadius()), WInvalidSpatialDataCategory);
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3(0, 0, GetCurrentCapsuleHeight() - GetShapeRadius()), GetShapeRadius()), WInvalidSpatialDataCategory);
}

void WJoltDefaultCharacterComponent::OnApplyRootMotion(WMsgApplyRootMotion& msg)
{
  m_vAbsoluteRootMotion += msg.m_vTranslation;
  m_InputRotateZ += msg.m_RotationZ;
}

void WJoltDefaultCharacterComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_RotateSpeed;
  s << m_fShapeRadius;
  s << m_fCylinderHeightCrouch;
  s << m_fCylinderHeightStand;
  s << m_fWalkSpeedCrouching;
  s << m_fWalkSpeedStanding;
  s << m_fWalkSpeedRunning;
  s << m_fMaxStepUp;
  s << m_fMaxStepDown;
  s << m_fJumpImpulse;
  s << m_sWalkSurfaceInteraction;
  s << m_fWalkInteractionDistance;
  s << m_fRunInteractionDistance;
  s << m_hFallbackWalkSurface;
  s << m_fAirFriction;
  s << m_fAirSpeed;
  s << m_fFootRadius;

  inout_stream.WriteGameObjectHandle(m_hHeadObject);
}

void WJoltDefaultCharacterComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_RotateSpeed;
  s >> m_fShapeRadius;
  s >> m_fCylinderHeightCrouch;
  s >> m_fCylinderHeightStand;
  s >> m_fWalkSpeedCrouching;
  s >> m_fWalkSpeedStanding;
  s >> m_fWalkSpeedRunning;
  s >> m_fMaxStepUp;
  s >> m_fMaxStepDown;
  s >> m_fJumpImpulse;
  s >> m_sWalkSurfaceInteraction;
  s >> m_fWalkInteractionDistance;
  s >> m_fRunInteractionDistance;
  s >> m_hFallbackWalkSurface;
  s >> m_fAirFriction;
  s >> m_fAirSpeed;
  s >> m_fFootRadius;

  m_hHeadObject = inout_stream.ReadGameObjectHandle();

  ResetInternalState();
}

void WJoltDefaultCharacterComponent::ResetInternalState()
{
  m_fShapeRadius = WMath::Clamp(m_fShapeRadius, 0.05f, 5.0f);
  m_fFootRadius = WMath::Clamp(m_fFootRadius, 0.01f, m_fShapeRadius);
  m_fCylinderHeightCrouch = WMath::Max(m_fCylinderHeightCrouch, 0.01f);
  m_fCylinderHeightStand = WMath::Max(m_fCylinderHeightStand, m_fCylinderHeightCrouch);
  m_fMaxStepUp = WMath::Clamp(m_fMaxStepUp, 0.0f, m_fCylinderHeightStand);
  m_fMaxStepDown = WMath::Clamp(m_fMaxStepDown, 0.0f, m_fCylinderHeightStand);

  m_fNextCylinderHeight = m_fCylinderHeightStand;
  m_fCurrentCylinderHeight = m_fNextCylinderHeight;

  m_vVelocityLateral.SetZero();
  m_fVelocityUp = 0;

  m_fAccumulatedWalkDistance = 0;
}

void WJoltDefaultCharacterComponent::ResetInputState()
{
  m_vInputDirection.SetZero();
  m_InputRotateZ = WAngle();
  m_uiInputCrouchBit = 0;
  m_uiInputRunBit = 0;
  m_uiInputJumpBit = 0;
  m_vAbsoluteRootMotion.SetZero();
}

void WJoltDefaultCharacterComponent::SetInputState(WMsgMoveCharacterController& ref_msg)
{
  Move(static_cast<float>(ref_msg.m_fMoveForwards - ref_msg.m_fMoveBackwards), static_cast<float>(ref_msg.m_fStrafeRight - ref_msg.m_fStrafeLeft));

  RotateZ((float)(ref_msg.m_fRotateRight - ref_msg.m_fRotateLeft));

  if (ref_msg.m_bRun)
  {
    Run();
  }

  if (ref_msg.m_bJump)
  {
    Jump();
  }

  if (ref_msg.m_bCrouch)
  {
    Crouch();
  }
}

float WJoltDefaultCharacterComponent::GetCurrentCylinderHeight() const
{
  return m_fCurrentCylinderHeight;
}

float WJoltDefaultCharacterComponent::GetCurrentCapsuleHeight() const
{
  return GetCurrentCylinderHeight() + 2.0f * GetShapeRadius();
}

float WJoltDefaultCharacterComponent::GetShapeRadius() const
{
  return m_fShapeRadius;
}

void WJoltDefaultCharacterComponent::Jump()
{
  m_uiInputJumpBit = 1;
}

void WJoltDefaultCharacterComponent::Run()
{
  m_uiInputRunBit = 1;
}

void WJoltDefaultCharacterComponent::Crouch()
{
  m_uiInputCrouchBit = 1;
}

void WJoltDefaultCharacterComponent::Move(float fForward, float fRight)
{
  const float fDistanceToMove = WMath::Max(WMath::Abs(fForward), WMath::Abs(fRight));

  m_vInputDirection += WVec2(fForward, fRight);
  m_vInputDirection.NormalizeIfNotZero(WVec2::MakeZero()).IgnoreResult();
  m_vInputDirection *= fDistanceToMove;
}

void WJoltDefaultCharacterComponent::RotateZ(float fAmount)
{
  m_InputRotateZ += m_RotateSpeed * fAmount;
}

void WJoltDefaultCharacterComponent::TeleportCharacter(const WVec3& vGlobalFootPosition)
{
  m_vVelocityLateral.SetZero();
  m_fVelocityUp = 0;

  TeleportToPosition(vGlobalFootPosition);
}

void WJoltDefaultCharacterComponent::OnActivated()
{
  SUPER::OnActivated();

  ResetInternalState();

  GetOwner()->UpdateLocalBounds();
}

void WJoltDefaultCharacterComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_pContactListener.Clear();
}

JPH::Ref<JPH::Shape> WJoltDefaultCharacterComponent::MakeNextCharacterShape()
{
  const float fTotalCapsuleHeight = m_fNextCylinderHeight + 2.0f * GetShapeRadius();

  JPH::CapsuleShapeSettings opt;
  opt.mRadius = GetShapeRadius();
  opt.mHalfHeightOfCylinder = 0.5f * m_fNextCylinderHeight;

  JPH::RotatedTranslatedShapeSettings up;
  up.mInnerShapePtr = opt.Create().Get();
  up.mPosition = JPH::Vec3(0, 0, fTotalCapsuleHeight * 0.5f);
  up.mRotation = JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), JPH::Vec3::sAxisZ());

  return up.Create().Get();
}

void WJoltDefaultCharacterComponent::SetHeadObjectReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hHeadObject = resolver(szReference, GetHandle(), "HeadObject");
}

void WJoltDefaultCharacterComponent::OnSimulationStarted()
{
  ResetInternalState();

  // creates the CC, so the next shape size must be set already
  SUPER::OnSimulationStarted();

  if (m_pContactListener == nullptr)
  {
    m_pContactListener = W_DEFAULT_NEW(WJoltDefaultCharacterContactListener);
    WJoltDefaultCharacterContactListener* pListener = (WJoltDefaultCharacterContactListener*)m_pContactListener.Borrow();
    pListener->m_pSystem = GetWorld()->GetModule<WJoltWorldModule>()->GetJoltSystem();
    pListener->m_pCharacter = this;
  }

  // the default CC uses a custom contact listener to send WMsgPhysicCharacterContact messages to whatever it hits,
  // so that those objects can react to it (e.g. by breaking apart)
  GetJoltCharacter()->SetListener(m_pContactListener.Borrow());

  WGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadHeightOffset = pHeadObject->GetLocalPosition().z;
    m_fHeadTargetHeight = m_fHeadHeightOffset;
  }
}

void WJoltDefaultCharacterComponent::ApplyRotationZ()
{
  if (m_InputRotateZ.GetRadian() == 0.0f)
    return;

  WQuat qRotZ = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), m_InputRotateZ);
  m_InputRotateZ.SetRadian(0.0);

  GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());
}

void WJoltDefaultCharacterComponent::SetFallbackWalkSurfaceFile(WStringView sFile)
{
  if (!sFile.IsEmpty())
  {
    m_hFallbackWalkSurface = WResourceManager::LoadResource<WSurfaceResource>(sFile);
  }
  else
  {
    m_hFallbackWalkSurface = {};
  }

  if (m_hFallbackWalkSurface.IsValid())
    WResourceManager::PreloadResource(m_hFallbackWalkSurface);
}

WStringView WJoltDefaultCharacterComponent::GetFallbackWalkSurfaceFile() const
{
  return m_hFallbackWalkSurface.GetResourceID();
}

void WJoltDefaultCharacterComponent::ApplyCrouchState()
{
  if (m_uiInputCrouchBit == m_uiIsCrouchingBit)
    return;

  m_fNextCylinderHeight = m_uiInputCrouchBit ? m_fCylinderHeightCrouch : m_fCylinderHeightStand;

  if (TryChangeShape(MakeNextCharacterShape().GetPtr()).Succeeded())
  {
    m_uiIsCrouchingBit = m_uiInputCrouchBit;
    m_fCurrentCylinderHeight = m_fNextCylinderHeight;
  }
}

void WJoltDefaultCharacterComponent::StoreLateralVelocity()
{
  const WVec3 endPosition = GetOwner()->GetGlobalPosition();
  const WVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  m_vVelocityLateral.Set(vVelocity.x, vVelocity.y);
}

void WJoltDefaultCharacterComponent::ClampLateralVelocity()
{
  const WVec3 endPosition = GetOwner()->GetGlobalPosition();
  const WVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  WVec2 vRealDirLateral(vVelocity.x, vVelocity.y);

  if (!vRealDirLateral.IsZero())
  {
    vRealDirLateral.Normalize();

    const float fSpeedAlongRealDir = vRealDirLateral.Dot(m_vVelocityLateral);

    m_vVelocityLateral = vRealDirLateral * fSpeedAlongRealDir;
  }
  else
    m_vVelocityLateral.SetZero();
}

void WJoltDefaultCharacterComponent::ClampUpVelocity()
{
  const WVec3 endPosition = GetOwner()->GetGlobalPosition();
  const WVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  m_fVelocityUp = WMath::Min(m_fVelocityUp, vVelocity.z);
}

void WJoltDefaultCharacterComponent::InteractWithSurfaces(const ContactPoint& contact, const Config& cfg)
{
  if (cfg.m_sGroundInteraction.IsEmpty())
  {
    m_fAccumulatedWalkDistance = 0;
    return;
  }

  const WVec2 vIntendedWalkAmount = (cfg.m_vVelocity * GetUpdateTimeDelta()).GetAsVec2() + m_vAbsoluteRootMotion.GetAsVec2();

  const WVec3 vOldPos = m_PreviousTransform.m_vPosition;
  const WVec3 vNewPos = GetOwner()->GetGlobalPosition();

  m_fAccumulatedWalkDistance += WMath::Min(vIntendedWalkAmount.GetLength(), (vNewPos - vOldPos).GetLength());

  if (m_fAccumulatedWalkDistance < cfg.m_fGroundInteractionDistanceThreshold)
    return;

  m_fAccumulatedWalkDistance = 0.0f;

  SpawnContactInteraction(contact, cfg.m_sGroundInteraction, m_hFallbackWalkSurface);
}

void WJoltDefaultCharacterComponent::MoveHeadObject()
{
  WGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadTargetHeight = m_fHeadHeightOffset;

    if (IsCrouching())
    {
      m_fHeadTargetHeight -= (m_fCylinderHeightStand - m_fCylinderHeightCrouch);
    }

    WVec3 pos = pHeadObject->GetLocalPosition();

    const float fTimeDiff = WMath::Max(GetUpdateTimeDelta(), 0.005f); // prevent stuff from breaking at high frame rates
    const float fFactor = 1.0f - WMath::Pow(0.001f, fTimeDiff);
    pos.z = WMath::Lerp(pos.z, m_fHeadTargetHeight, fFactor);

    pHeadObject->SetLocalPosition(pos);
  }
}

void WJoltDefaultCharacterComponent::DebugVisualizations()
{
  if (m_DebugFlags.IsSet(WJoltCharacterDebugFlags::PrintState))
  {
    switch (GetJoltCharacter()->GetGroundState())
    {
      case JPH::CharacterBase::EGroundState::OnGround:
        WDebugRenderer::DrawInfoText(GetWorld(), WDebugTextPlacement::TopLeft, "JCC", "Jolt: On Ground", WColor::Brown);
        break;
      case JPH::CharacterBase::EGroundState::InAir:
        WDebugRenderer::DrawInfoText(GetWorld(), WDebugTextPlacement::TopLeft, "JCC", "Jolt: In Air", WColor::CornflowerBlue);
        break;
      case JPH::CharacterBase::EGroundState::NotSupported:
        WDebugRenderer::DrawInfoText(GetWorld(), WDebugTextPlacement::TopLeft, "JCC", "Jolt: Not Supported", WColor::Yellow);
        break;
      case JPH::CharacterBase::EGroundState::OnSteepGround:
        WDebugRenderer::DrawInfoText(GetWorld(), WDebugTextPlacement::TopLeft, "JCC", "Jolt: Steep", WColor::OrangeRed);
        break;
    }

    // const WTransform newTransform = GetOwner()->GetGlobalTransform();
    // const float fDistTraveled = (m_PreviousTransform.m_vPosition - newTransform.m_vPosition).GetLength();
    // const float fSpeedTraveled = fDistTraveled * GetInverseUpdateTimeDelta();
    // const float fSpeedTraveledLateral = fDistTraveled * GetInverseUpdateTimeDelta();
    // WDebugRenderer::DrawInfoText(GetWorld(), WDebugTextPlacement::TopLeft, "JCC", WFmt("Speed 1: {} m/s", fSpeedTraveled), WColor::WhiteSmoke);
    // WDebugRenderer::DrawInfoText(GetWorld(), WDebugTextPlacement::TopLeft, "JCC", WFmt("Speed 2: {} m/s", fSpeedTraveledLateral), WColor::WhiteSmoke);
  }

  if (m_DebugFlags.IsSet(WJoltCharacterDebugFlags::VisGroundContact))
  {
    WVec3 gpos = WJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    WVec3 gnom = WJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());

    if (!gnom.IsZero(0.01f))
    {
      WQuat rot = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), gnom);

      WDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, WColor::MakeZero(), WColor::Aquamarine, WTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsSet(WJoltCharacterDebugFlags::VisShape))
  {
    WTransform shapeTrans = GetOwner()->GetGlobalTransform();

    shapeTrans.m_vPosition.z += GetCurrentCapsuleHeight() * 0.5f;

    WDebugRenderer::DrawLineCapsuleZ(GetWorld(), GetCurrentCylinderHeight(), GetShapeRadius(), WColor::CornflowerBlue, shapeTrans);
  }
}

void WJoltDefaultCharacterComponent::CheckFeet()
{
  if (!cvar_JoltCcFootCheck)
  {
    // pretend we always touch the ground
    m_bFeetOnSolidGround = true;
    return;
  }

  if (m_fFootRadius <= 0 || m_fMaxStepDown <= 0.0f)
    return;

  m_bFeetOnSolidGround = false;

  WTransform shapeTrans = GetOwner()->GetGlobalTransform();
  WQuat shapeRot = WQuat::MakeShortestRotation(WVec3(0, 1, 0), WVec3(0, 0, 1));

  const float radius = m_fFootRadius;
  const float halfHeight = WMath::Max(0.01f, m_fMaxStepDown - radius);

  JPH::CapsuleShape shape(halfHeight, radius);
  shapeTrans.m_vPosition.z += halfHeight + radius;

  WTempHybridArray<ContactPoint, 32> contacts;
  CollectContacts(contacts, &shape, shapeTrans.m_vPosition, shapeRot, m_fMaxStepDown);

  for (const auto& contact : contacts)
  {
    WVec3 gpos = contact.m_vPosition;
    WVec3 gnom = contact.m_vSurfaceNormal;

    // if the object is not penetrated and stands further than foot radius then skip this object
    float fXYDistanceSquared = contact.m_fPenetrationDepth < 0 ? (contact.m_fPenetrationDepth * gnom).GetAsVec2().GetLengthSquared() : 0.f;
    if (fXYDistanceSquared > radius * radius)
      continue;

    WColor color = WColor::LightYellow;
    WQuat rot;

    if (gnom.IsZero(0.01f))
    {
      rot = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), WVec3::MakeAxisZ());
      color = WColor::OrangeRed;
    }
    else
    {
      rot = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), gnom);

      if (gnom.Dot(WVec3::MakeAxisZ()) > WMath::Cos(WAngle::MakeFromDegree(40)))
      {
        m_bFeetOnSolidGround = true;
        color = WColor::GreenYellow;
      }
    }

    if (m_DebugFlags.IsAnySet(WJoltCharacterDebugFlags::VisFootCheck))
    {
      WDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, WColor::MakeZero(), color, WTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsAnySet(WJoltCharacterDebugFlags::VisFootCheck))
  {
    WDebugRenderer::DrawLineCapsuleZ(GetWorld(), halfHeight * 2.0f, radius, WColor::YellowGreen, WTransform(shapeTrans.m_vPosition));
  }
}

void WJoltDefaultCharacterComponent::DetermineConfig(Config& out_inputs)
{
  // velocity
  {
    float fSpeed = 0;

    switch (GetGroundState())
    {
      case WJoltDefaultCharacterComponent::GroundState::OnGround:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        else if (m_uiInputRunBit)
        {
          fSpeed = m_fWalkSpeedRunning;
        }
        break;

      case WJoltDefaultCharacterComponent::GroundState::Sliding:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        break;

      case WJoltDefaultCharacterComponent::GroundState::InAir:
        fSpeed = m_fAirSpeed;
        break;
    }

    out_inputs.m_vVelocity = GetOwner()->GetGlobalRotation() * m_vInputDirection.GetAsVec3(0) * fSpeed;
  }

  // ground interaction
  {
    switch (GetGroundState())
    {
      case WJoltDefaultCharacterComponent::GroundState::OnGround:
        out_inputs.m_sGroundInteraction = (m_uiInputRunBit == 1) ? m_sWalkSurfaceInteraction : m_sWalkSurfaceInteraction; // TODO: run interaction
        out_inputs.m_fGroundInteractionDistanceThreshold = (m_uiInputRunBit == 1) ? m_fRunInteractionDistance : m_fWalkInteractionDistance;
        break;

      case WJoltDefaultCharacterComponent::GroundState::Sliding:
        // TODO: slide interaction
        break;

      case GroundState::InAir:
        break;
    }
  }

  out_inputs.m_bAllowCrouch = true;
  out_inputs.m_bAllowJump = (GetGroundState() == GroundState::OnGround) && !IsCrouching() && m_bFeetOnSolidGround;
  out_inputs.m_bApplyGroundVelocity = true;
  out_inputs.m_fPushDownForce = GetMass();
  out_inputs.m_fMaxStepUp = (m_bFeetOnSolidGround && !out_inputs.m_vVelocity.IsZero()) ? m_fMaxStepUp : 0.0f;
  out_inputs.m_fMaxStepDown = ((GetGroundState() == GroundState::OnGround) || (GetGroundState() == GroundState::Sliding)) && m_bFeetOnSolidGround ? m_fMaxStepDown : 0.0f;
}

void WJoltDefaultCharacterComponent::UpdateCharacter()
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  m_PreviousTransform = GetOwner()->GetGlobalTransform();

  switch (GetJoltCharacter()->GetGroundState())
  {
    case JPH::CharacterBase::EGroundState::InAir:
    case JPH::CharacterBase::EGroundState::NotSupported:
      m_LastGroundState = GroundState::InAir;
      // TODO: filter out 'sliding' when touching a ceiling (should be 'in air')
      break;

    case JPH::CharacterBase::EGroundState::OnGround:
      m_LastGroundState = GroundState::OnGround;
      break;

    case JPH::CharacterBase::EGroundState::OnSteepGround:
      m_LastGroundState = GroundState::Sliding;
      break;
  }

  CheckFeet();

  Config cfg;
  DetermineConfig(cfg);

  ApplyCrouchState();

  if (m_uiInputJumpBit && cfg.m_bAllowJump)
  {
    m_fVelocityUp = m_fJumpImpulse;
    cfg.m_fMaxStepUp = 0;
    cfg.m_fMaxStepDown = 0;
  }

  WVec3 vGroundVelocity = WVec3::MakeZero();

  ContactPoint groundContact;
  {
    groundContact.m_vPosition = WJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    groundContact.m_vContactNormal = WJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());
    groundContact.m_vSurfaceNormal = groundContact.m_vContactNormal;
    groundContact.m_BodyID = GetJoltCharacter()->GetGroundBodyID();
    groundContact.m_SubShapeID = GetJoltCharacter()->GetGroundSubShapeID();

    /*vGroundVelocity =*/GetContactVelocityAndPushAway(groundContact, cfg.m_fPushDownForce);

    // TODO: on rotating surfaces I see the same error with this value and the one returned above
    vGroundVelocity = WJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundVelocity());
    vGroundVelocity.z = 0.0f;

    if (!cfg.m_bApplyGroundVelocity)
      vGroundVelocity.SetZero();
  }

  const bool bWasOnGround = GetJoltCharacter()->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

  if (bWasOnGround)
  {
    m_vVelocityLateral.SetZero();
  }

  // AIR: apply 'drag' to the lateral velocity
  m_vVelocityLateral *= WMath::Pow(1.0f - m_fAirFriction, GetUpdateTimeDelta());

  WVec3 vRootVelocity = GetInverseUpdateTimeDelta() * (GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion);

  if (!m_vVelocityLateral.IsZero(WMath::FloatEpsilon<float>()))
  {
    // remove the lateral velocity component from the root motion
    // to prevent root motion being amplified when both values are active
    WVec3 vLatDir = m_vVelocityLateral.GetNormalized().GetAsVec3(0);
    float fProj = WMath::Max(0.0f, vLatDir.Dot(vRootVelocity));
    vRootVelocity -= vLatDir * fProj;
  }

  // retrieve the actual up velocity
  float groundVerticalVelocity = GetJoltCharacter()->GetGroundVelocity().GetZ();
  if (GetJoltCharacter()->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround // If on ground
      && (m_fVelocityUp - groundVerticalVelocity) < 0.1f)                                   // And not moving away from ground
  {
    m_fVelocityUp = groundVerticalVelocity;
  }

  WVec3 vVelocityToApply = cfg.m_vVelocity + vGroundVelocity;
  vVelocityToApply += m_vVelocityLateral.GetAsVec3(0);
  vVelocityToApply += vRootVelocity;
  vVelocityToApply.z = m_fVelocityUp;

  RawMoveWithVelocity(vVelocityToApply, cfg.m_fMaxStepUp, cfg.m_fMaxStepDown);

  if (!cfg.m_sGroundInteraction.IsEmpty())
  {
    if (groundContact.m_vContactNormal.IsValid() && !groundContact.m_vContactNormal.IsZero(0.001f))
    {
      // TODO: sometimes the CC reports contacts with zero normals
      InteractWithSurfaces(groundContact, cfg);
    }
  }

  if (bWasOnGround)
  {
    StoreLateralVelocity();
  }
  else
  {
    ClampLateralVelocity();
    ClampUpVelocity();
  }

  m_fVelocityUp += GetUpdateTimeDelta() * pModule->GetCharacterGravity().z;

  ApplyRotationZ();

  MoveHeadObject();

  DebugVisualizations();

  ResetInputState();
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Character_Implementation_JoltDefaultCharacterComponent);
