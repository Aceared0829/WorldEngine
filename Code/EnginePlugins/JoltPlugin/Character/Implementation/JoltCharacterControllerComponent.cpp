#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WJoltCharacterDebugFlags, 1)
W_BITFLAGS_CONSTANTS(WJoltCharacterDebugFlags::PrintState, WJoltCharacterDebugFlags::VisShape, WJoltCharacterDebugFlags::VisContacts,  WJoltCharacterDebugFlags::VisCasts, WJoltCharacterDebugFlags::VisGroundContact, WJoltCharacterDebugFlags::VisFootCheck)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_ABSTRACT_COMPONENT_TYPE(WJoltCharacterControllerComponent, 3)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("PresenceCollisionLayer", m_uiPresenceCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("WeightCategory", m_uiWeightCategory)->AddAttributes(new WDynamicEnumAttribute("PhysicsWeightCategory")),
    W_ACCESSOR_PROPERTY("WeightScale", GetWeight_Scale, SetWeight_Scale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("Mass", GetWeight_Mass, SetWeight_Mass)->AddAttributes(new WSuffixAttribute(" kg"), new WDefaultValueAttribute(50.0f), new WClampValueAttribute(1.0f, 1000.0f)),
    W_ACCESSOR_PROPERTY("Strength", GetStrength, SetStrength)->AddAttributes(new WDefaultValueAttribute(500.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("MaxClimbingSlope", GetMaxClimbingSlope, SetMaxClimbingSlope)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(40))),
    W_BITFLAGS_MEMBER_PROPERTY("DebugFlags", WJoltCharacterDebugFlags , m_DebugFlags),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Character"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

WJoltCharacterControllerComponent::WJoltCharacterControllerComponent() = default;
WJoltCharacterControllerComponent::~WJoltCharacterControllerComponent() = default;

void WJoltCharacterControllerComponent::SetObjectToIgnore(WUInt32 uiObjectFilterID)
{
  m_BodyFilter.m_uiObjectFilterIDToIgnore = uiObjectFilterID;
}

void WJoltCharacterControllerComponent::ClearObjectToIgnore()
{
  m_BodyFilter.ClearFilter();
}

void WJoltCharacterControllerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_DebugFlags;

  s << m_uiCollisionLayer;
  s << m_uiPresenceCollisionLayer;
  s << m_uiWeightCategory;
  s << m_fWeightMass;
  s << m_fWeightScale;
  s << m_fStrength;
  s << m_MaxClimbingSlope;
}

void WJoltCharacterControllerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  W_ASSERT_DEBUG(uiVersion >= 3, "Outdated version, please re-transform asset.");
  if (uiVersion < 3)
    return;

  s >> m_DebugFlags;
  s >> m_uiCollisionLayer;
  s >> m_uiPresenceCollisionLayer;
  s >> m_uiWeightCategory;
  s >> m_fWeightMass;
  s >> m_fWeightScale;
  s >> m_fStrength;
  s >> m_MaxClimbingSlope;
}

void WJoltCharacterControllerComponent::OnDeactivated()
{
  if (m_pCharacter)
  {
    if (WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>())
    {
      pModule->ActivateCharacterController(this, false);
    }

    m_pCharacter->Release();
    m_pCharacter = nullptr;
  }

  RemovePresenceBody();

  SUPER::OnDeactivated();
}

void WJoltCharacterControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  m_fMass = WJoltCore::GetWeightCategoryConfig().GetMassForWeightCategory(m_uiWeightCategory, 50.0f, m_fWeightMass, m_fWeightScale);

  JPH::CharacterVirtualSettings opt;
  opt.mUp = JPH::Vec3::sAxisZ();
  opt.mSupportingVolume = JPH::Plane(opt.mUp, -GetShapeRadius()); // should use the half cylinder height instead of the radius
  opt.mShape = MakeNextCharacterShape();
  opt.mMaxSlopeAngle = m_MaxClimbingSlope.GetRadian();
  opt.mMass = m_fMass;
  opt.mMaxStrength = m_fStrength;

  const WTransform ownTrans = GetOwner()->GetGlobalTransform();

  m_pCharacter = new JPH::CharacterVirtual(&opt, WJoltConversionUtils::ToVec3(ownTrans.m_vPosition), WJoltConversionUtils::ToQuat(ownTrans.m_qRotation), pModule->GetJoltSystem());
  m_pCharacter->AddRef();

  pModule->ActivateCharacterController(this, true);

  CreatePresenceBody();
}

void WJoltCharacterControllerComponent::SetMaxClimbingSlope(WAngle slope)
{
  m_MaxClimbingSlope = slope;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxSlopeAngle(m_MaxClimbingSlope.GetRadian());
  }
}

void WJoltCharacterControllerComponent::SetStrength(float fStrength)
{
  m_fStrength = fStrength;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxStrength(m_fStrength);
  }
}

WResult WJoltCharacterControllerComponent::TryChangeShape(JPH::Shape* pNewShape)
{
  WJoltBroadPhaseLayerFilter broadphaseFilter(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);
  WJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  if (m_pCharacter->SetShape(pNewShape, 0.01f, broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator()))
  {
    RemovePresenceBody();
    CreatePresenceBody();

    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WJoltCharacterControllerComponent::RawMoveWithVelocity(const WVec3& vVelocity, float fMaxStairStepUp, float fMaxStepDown)
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  WJoltBroadPhaseLayerFilter broadphaseFilter(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);
  WJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  m_pCharacter->SetLinearVelocity(WJoltConversionUtils::ToVec3(vVelocity));

  // Settings for our update function
  JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
  updateSettings.mStickToFloorStepDown = JPH::Vec3(0, 0, -fMaxStepDown);
  updateSettings.mWalkStairsStepUp = fMaxStairStepUp > 0 ? JPH::Vec3(0, 0, fMaxStairStepUp) : JPH::Vec3::sZero();

  // Update the character position
  m_pCharacter->ExtendedUpdate(GetUpdateTimeDelta(), WJoltConversionUtils::ToVec3(pModule->GetCharacterGravity()), updateSettings, broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());

  GetOwner()->SetGlobalPosition(WJoltConversionUtils::ToSimdVec3(m_pCharacter->GetPosition()));
}


void WJoltCharacterControllerComponent::RawMoveIntoDirection(const WVec3& vDirection)
{
  if (vDirection.IsZero())
    return;

  RawMoveWithVelocity(vDirection * GetInverseUpdateTimeDelta(), 0.0f, 0.0f);
}

void WJoltCharacterControllerComponent::RawMoveToPosition(const WVec3& vTargetPosition)
{
  RawMoveIntoDirection(vTargetPosition - GetOwner()->GetGlobalPosition());
}

void WJoltCharacterControllerComponent::TeleportToPosition(const WVec3& vGlobalFootPos)
{
  m_pCharacter->SetPosition(WJoltConversionUtils::ToVec3(vGlobalFootPos));

  WJoltBroadPhaseLayerFilter broadphaseFilter(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);
  WJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  m_pCharacter->RefreshContacts(broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());
}

bool WJoltCharacterControllerComponent::StickToGround(float fMaxDist)
{
  if (m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::InAir || m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::NotSupported)
    return false;

  WJoltBroadPhaseLayerFilter broadphaseFilter(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);
  WJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  return m_pCharacter->StickToFloor(JPH::Vec3(0, 0, -fMaxDist), broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());
}

void WJoltCharacterControllerComponent::CollectCastContacts(WDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const WVec3& vQueryPosition, const WQuat& qQueryRotation, const WVec3& vSweepDir) const
{
  out_Contacts.Clear();

  class ContactCastCollector : public JPH::CastShapeCollector
  {
  public:
    WDynamicArray<ContactPoint>* m_pContacts = nullptr;
    const JPH::BodyLockInterface* m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::ShapeCastResult& result) override
    {
      auto& contact = m_pContacts->ExpandAndGetRef();
      contact.m_vPosition = WJoltConversionUtils::ToVec3(result.mContactPointOn2);
      contact.m_vContactNormal = WJoltConversionUtils::ToVec3(-result.mPenetrationAxis.Normalized());
      contact.m_BodyID = result.mBodyID2;
      contact.m_fCastFraction = result.mFraction;
      contact.m_fPenetrationDepth = result.mPenetrationDepth;
      contact.m_SubShapeID = result.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = WJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, result.mContactPointOn2));
    }
  };

  const WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  WJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);
  WJoltBroadPhaseLayerFilter broadphaseFilter(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);

  ContactCastCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qQueryRotation), WJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::RShapeCast castOpt(pShape, JPH::Vec3::sReplicate(1.0f), trans, WJoltConversionUtils::ToVec3(vSweepDir));

  JPH::ShapeCastSettings settings;
  pJoltSystem->GetNarrowPhaseQuery().CastShape(castOpt, settings, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

void WJoltCharacterControllerComponent::CollectContacts(WDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const WVec3& vQueryPosition, const WQuat& qQueryRotation, float fMaxSeparationDistance) const
{
  out_Contacts.Clear();

  class ContactCollector : public JPH::CollideShapeCollector
  {
  public:
    WDynamicArray<ContactPoint>* m_pContacts = nullptr;
    const JPH::BodyLockInterface* m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::CollideShapeResult& result) override
    {
      auto& contact = m_pContacts->ExpandAndGetRef();
      contact.m_fPenetrationDepth = result.mPenetrationDepth;
      contact.m_vPosition = WJoltConversionUtils::ToVec3(result.mContactPointOn2);
      contact.m_vContactNormal = WJoltConversionUtils::ToVec3(-result.mPenetrationAxis.Normalized());
      contact.m_BodyID = result.mBodyID2;
      contact.m_SubShapeID = result.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = WJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, result.mContactPointOn2));
    }
  };

  const WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  WJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);
  WJoltBroadPhaseLayerFilter broadphaseFilter(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);

  ContactCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qQueryRotation), WJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::CollideShapeSettings settings;
  settings.mMaxSeparationDistance = fMaxSeparationDistance;
  settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

  pJoltSystem->GetNarrowPhaseQuery().CollideShape(pShape, JPH::Vec3::sReplicate(1.0f), trans, settings, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

WVec3 WJoltCharacterControllerComponent::GetContactVelocityAndPushAway(const ContactPoint& contact, float fPushForce)
{
  if (contact.m_BodyID.IsInvalid())
    return WVec3::MakeZero();

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  auto pJoltSystem = pModule->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pJoltSystem->GetBodyLockInterface(), contact.m_BodyID);

  if (!bodyLock.Succeeded())
    return WVec3::MakeZero();

  const JPH::Vec3 vGroundPos = WJoltConversionUtils::ToVec3(contact.m_vPosition);

  if (fPushForce > 0 && bodyLock.GetBody().IsDynamic())
  {
    const WVec3 vPushDir = -contact.m_vSurfaceNormal * fPushForce;

    bodyLock.GetBody().AddForce(WJoltConversionUtils::ToVec3(vPushDir), vGroundPos);
    pJoltSystem->GetBodyInterfaceNoLock().ActivateBody(contact.m_BodyID);
  }

  WVec3 vGroundVelocity = WVec3::MakeZero();

  if (bodyLock.GetBody().IsKinematic())
  {
    vGroundVelocity = WJoltConversionUtils::ToVec3(bodyLock.GetBody().GetPointVelocity(vGroundPos));
    vGroundVelocity.z = 0;
  }

  return vGroundVelocity;
}

void WJoltCharacterControllerComponent::SpawnContactInteraction(const ContactPoint& contact, const WHashedString& sSurfaceInteraction, WSurfaceResourceHandle hFallbackSurface, const WVec3& vInteractionNormal)
{
  if (contact.m_BodyID.IsInvalid())
    return;

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  WSurfaceResourceHandle hSurface = hFallbackSurface;

  JPH::BodyLockRead lock(pModule->GetJoltSystem()->GetBodyLockInterfaceNoLock(), contact.m_BodyID);
  if (lock.Succeeded())
  {
    auto pMat = static_cast<const WJoltMaterial*>(lock.GetBody().GetShape()->GetMaterial(contact.m_SubShapeID));
    if (pMat && pMat->m_pSurface)
    {
      hSurface = static_cast<const WJoltMaterial*>(pMat)->m_pSurface->GetResourceHandle();
    }
  }

  if (hSurface.IsValid())
  {
    WResourceLock<WSurfaceResource> pSurface(hSurface, WResourceAcquireMode::AllowLoadingFallback);
    pSurface->InteractWithSurface(GetWorld(), WGameObjectHandle(), contact.m_vPosition, contact.m_vSurfaceNormal, vInteractionNormal, sSurfaceInteraction, &GetOwner()->GetTeamID());
  }
}

void WJoltCharacterControllerComponent::VisualizeContact(const ContactPoint& contact, const WColor& color) const
{
  WTransform trans;
  trans.m_vPosition = contact.m_vPosition;
  trans.m_qRotation = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), contact.m_vContactNormal);
  trans.m_vScale.Set(1.0f);

  WDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, WColor::MakeZero(), color, trans);
}

void WJoltCharacterControllerComponent::VisualizeContacts(const WDynamicArray<ContactPoint>& contacts, const WColor& color) const
{
  for (const auto& ct : contacts)
  {
    VisualizeContact(ct, color);
  }
}

void WJoltCharacterControllerComponent::Update(WTime deltaTime)
{
  m_fUpdateTimeDelta = deltaTime.AsFloatInSeconds();
  m_fInverseUpdateTimeDelta = static_cast<float>(1.0 / deltaTime.GetSeconds());

  UpdateCharacter();

  MovePresenceBody(deltaTime);
}

void WJoltCharacterControllerComponent::CreatePresenceBody()
{
  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;
  bodyCfg.SetShape(m_pCharacter->GetShape());

  WJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  WUInt32 m_uiObjectFilterID = pModule->CreateObjectFilterID();

  bodyCfg.mAllowSleeping = false;
  bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiPresenceCollisionLayer, WJoltBroadphaseLayer::Character);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  W_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiPresenceBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);
}

void WJoltCharacterControllerComponent::RemovePresenceBody()
{
  if (m_uiPresenceBodyID == WInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  m_uiPresenceBodyID = WInvalidIndex;

  if (bodyId.IsInvalid())
    return;

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  pBodies->RemoveBody(bodyId);
  pBodies->DestroyBody(bodyId);

  pModule->DeallocateUserData(m_uiUserDataIndex);
  // pModule->DeleteObjectFilterID(m_uiObjectFilterID);
}

void WJoltCharacterControllerComponent::MovePresenceBody(WTime deltaTime)
{
  if (m_uiPresenceBodyID == WInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  if (bodyId.IsInvalid())
    return;

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  if (!pBodies->IsAdded(bodyId))
    return;

  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  const float tDiff = deltaTime.AsFloatInSeconds();

  pBodies->MoveKinematic(bodyId, WJoltConversionUtils::ToVec3(trans.m_Position), WJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized(), tDiff);
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Character_Implementation_JoltCharacterControllerComponent);
