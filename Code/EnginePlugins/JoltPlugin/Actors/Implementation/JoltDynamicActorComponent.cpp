#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/WorldLogLink.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Collision/Shape/OffsetCenterOfMassShape.h>

WJoltDynamicActorComponentManager::WJoltDynamicActorComponentManager(WWorld* pWorld)
  : WComponentManager<WJoltDynamicActorComponent, WBlockStorageType::FreeList>(pWorld)
{
}

WJoltDynamicActorComponentManager::~WJoltDynamicActorComponentManager() = default;

void WJoltDynamicActorComponentManager::UpdateDynamicActors()
{
  W_PROFILE_SCOPE("UpdateDynamicActors");

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  for (auto itActor : pModule->GetActiveActors())
  {
    WJoltDynamicActorComponent* pActor = itActor;

    JPH::BodyID bodyId(pActor->GetJoltBodyID());

    JPH::BodyLockRead bodyLock(pSystem->GetBodyLockInterface(), bodyId);
    if (!bodyLock.Succeeded())
      continue;

    const JPH::Body& body = bodyLock.GetBody();

    if (!body.IsDynamic())
      continue;

    WSimdTransform trans = pActor->GetOwner()->GetGlobalTransformSimd();

    trans.m_Position = WJoltConversionUtils::ToSimdVec3(body.GetPosition());
    trans.m_Rotation = WJoltConversionUtils::ToSimdQuat(body.GetRotation());

    pActor->GetOwner()->SetGlobalTransform(trans);
  }
}

void WJoltDynamicActorComponentManager::UpdateKinematicActors(WTime deltaTime)
{
  W_PROFILE_SCOPE("UpdateKinematicActors");

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  const float tDiff = deltaTime.AsFloatInSeconds();

  for (auto pKinematicActorComponent : m_KinematicActorComponents)
  {
    JPH::BodyID bodyId(pKinematicActorComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    WGameObject* pObject = pKinematicActorComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const WSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const WSimdQuat rot = pObject->GetGlobalRotationSimd();

    pBodies->MoveKinematic(bodyId, WJoltConversionUtils::ToVec3(pos), WJoltConversionUtils::ToQuat(rot).Normalized(), tDiff);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltDynamicActorComponent, 8, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
      W_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
      W_MEMBER_PROPERTY("StartAsleep", m_bStartAsleep),
      W_MEMBER_PROPERTY("AllowSleeping", m_bAllowSleeping)->AddAttributes(new WDefaultValueAttribute(true)),
      W_MEMBER_PROPERTY("WeightCategory", m_uiWeightCategory)->AddAttributes(new WDynamicEnumAttribute("PhysicsWeightCategoryWithDensity")),
      W_ACCESSOR_PROPERTY("WeightScale", GetWeight_Scale, SetWeight_Scale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
      W_ACCESSOR_PROPERTY("Mass", GetWeight_Mass, SetWeight_Mass)->AddAttributes(new WSuffixAttribute(" kg"), new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.1f, 10000.0f)),
      W_ACCESSOR_PROPERTY("Density", GetWeight_Density, SetWeight_Density)->AddAttributes(new WDefaultValueAttribute(100.0f), new WSuffixAttribute(" kg/m^3")),
      W_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
      W_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new WDefaultValueAttribute(1.0f)),
      W_ACCESSOR_PROPERTY("BuoyancyFactor", GetBuoyancyFactor, SetBuoyancyFactor)->AddAttributes(new WDefaultValueAttribute(1.1f), new WClampValueAttribute(0.1f, 10.0f)),
      W_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new WDefaultValueAttribute(0.2f)),
      W_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new WDefaultValueAttribute(0.2f)),
      W_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
      W_BITFLAGS_MEMBER_PROPERTY("OnContact", WOnJoltContact, m_OnContact),
      W_ACCESSOR_PROPERTY("CustomCenterOfMass", GetUseCustomCoM, SetUseCustomCoM),
      W_MEMBER_PROPERTY("CenterOfMass", m_vCenterOfMass),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
      W_MESSAGE_HANDLER(WMsgPhysicsAddImpulse, AddLinearImpulseAtPos),
      W_MESSAGE_HANDLER(WMsgPhysicsMakeTemporarilyDynamic, OnMsgPhysicsMakeTemporarilyDynamic),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(AddLinearImpulse, In, "vImpulse", In, "uiImpulseType")->AddAttributes(new WFunctionArgumentAttributes(1, new WDefaultValueAttribute(0))),
    W_SCRIPT_FUNCTION_PROPERTY(AddAngularImpulse, In, "vImpulse", In, "uiImpulseType")->AddAttributes(new WFunctionArgumentAttributes(1, new WDefaultValueAttribute(0))),
    W_SCRIPT_FUNCTION_PROPERTY(AddOrUpdateForce, In, "uiForceID", In, "duration", In, "vForce"),
    W_SCRIPT_FUNCTION_PROPERTY(ClearForce, In, "uiForceID"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WTransformManipulatorAttribute("CenterOfMass")
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WJoltDynamicActorComponent::WJoltDynamicActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

WJoltDynamicActorComponent::~WJoltDynamicActorComponent() = default;

void WJoltDynamicActorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bKinematic;
  s << m_bCCD;
  s << m_fLinearDamping;
  s << m_fAngularDamping;
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_OnContact;
  s << GetUseCustomCoM();
  s << m_vCenterOfMass;
  s << m_bStartAsleep;
  s << m_bAllowSleeping;
  s << m_uiWeightCategory;
  s << (float)m_fWeightScale;
  s << (float)m_fWeightMass;
  s << (float)m_fWeightDensity;
  s << (float)m_fBuoyancyFactor;
}

void WJoltDynamicActorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  W_ASSERT_DEBUG(uiVersion >= 7, "Outdated version, please re-transform asset.");
  if (uiVersion < 7)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_bKinematic;
  s >> m_bCCD;
  s >> m_fLinearDamping;
  s >> m_fAngularDamping;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_OnContact;

  {
    bool com;
    s >> com;
    SetUseCustomCoM(com);
    s >> m_vCenterOfMass;
  }

  s >> m_bStartAsleep;
  s >> m_bAllowSleeping;

  {
    float f;
    s >> m_uiWeightCategory;

    s >> f;
    m_fWeightScale = f;
    s >> f;
    m_fWeightMass = f;
    s >> f;
    m_fWeightDensity = f;
  }

  if (uiVersion >= 8)
  {
    float f;
    s >> f;
    m_fBuoyancyFactor = f;
  }
}

void WJoltDynamicActorComponent::OnMsgPhysicsMakeTemporarilyDynamic(WMsgPhysicsMakeTemporarilyDynamic& msg)
{
  W_IGNORE_UNUSED(msg);

  // a kinematic actor doesn't fall down, so switch it to a fully simulated one.
  // Nothing is restored afterwards, because this only ever happens in a simulation whose world gets discarded.
  SetKinematic(false);

  GetOwner()->MakeDynamic();
}

void WJoltDynamicActorComponent::SetKinematic(bool b)
{
  if (m_bKinematic == b)
    return;

  m_bKinematic = b;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (m_bKinematic && !bodyId.IsInvalid())
  {
    // do not insert this, until we actually have an actor pointer
    GetWorld()->GetOrCreateComponentManager<WJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
  else
  {
    GetWorld()->GetOrCreateComponentManager<WJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  if (bodyId.IsInvalid())
    return;

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  {
    JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

    if (bodyLock.Succeeded())
    {
      JPH::Body& body = bodyLock.GetBody();
      body.SetMotionType(m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
    }
  }

  if (!m_bKinematic && pSystem->GetBodyInterface().IsAdded(bodyId))
  {
    pSystem->GetBodyInterface().ActivateBody(bodyId);
  }
}

void WJoltDynamicActorComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<WJoltWorldModule>()->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

  if (bodyLock.Succeeded())
  {
    bodyLock.GetBody().GetMotionProperties()->SetGravityFactor(m_fGravityFactor);

    if (pSystem->GetBodyInterfaceNoLock().IsAdded(bodyId))
    {
      pSystem->GetBodyInterfaceNoLock().ActivateBody(bodyId);
    }
  }
}

void WJoltDynamicActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();
  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, m_fWeightDensity, pMaterial).Failed())
  {
    WLog::Error("Jolt dynamic actor component {} has no valid shape.", WArgComponent(this));
    return;
  }

  if (pMaterial == nullptr)
    pMaterial = WJoltCore::GetDefaultMaterial();

  WJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this, m_OnContact);

  const float fInitialMass = WJoltCore::GetWeightCategoryConfig().GetMassForWeightCategory(m_uiWeightCategory, 10.0f, m_fWeightMass, m_fWeightScale);

  bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
  bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, WJoltBroadphaseLayer::Dynamic);
  bodyCfg.mMotionQuality = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
  bodyCfg.mAllowSleeping = m_bAllowSleeping;
  bodyCfg.mLinearDamping = m_fLinearDamping;
  bodyCfg.mAngularDamping = m_fAngularDamping;
  bodyCfg.mMassPropertiesOverride.mMass = fInitialMass;
  bodyCfg.mOverrideMassProperties = fInitialMass > 0.0f ? JPH::EOverrideMassProperties::CalculateInertia : JPH::EOverrideMassProperties::CalculateMassAndInertia;
  bodyCfg.mGravityFactor = m_fGravityFactor;
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserData);

  if (GetUseCustomCoM())
  {
    const WVec3 vGlobalScale = GetOwner()->GetGlobalScaling();
    const float scale = WMath::Min(vGlobalScale.x, vGlobalScale.y, vGlobalScale.z);
    auto vLocalCenterOfMass = WSimdVec4f(scale * m_vCenterOfMass.x, scale * m_vCenterOfMass.y, scale * m_vCenterOfMass.z);
    auto vPrevCoM = WJoltConversionUtils::ToSimdVec3(bodyCfg.GetShape()->GetCenterOfMass());

    auto vComShift = vLocalCenterOfMass - vPrevCoM;

    JPH::OffsetCenterOfMassShapeSettings com;
    com.mOffset = WJoltConversionUtils::ToVec3(vComShift);
    com.mInnerShapePtr = bodyCfg.GetShape();

    bodyCfg.SetShape(com.Create().Get());
  }

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  W_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, !m_bStartAsleep);

  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<WJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
}

void WJoltDynamicActorComponent::OnDeactivated()
{
  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<WJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  auto allConstraints = m_Constraints;
  m_Constraints.Clear();
  m_Constraints.Compact();

  WJoltMsgDisconnectConstraints msg;
  msg.m_pActor = this;
  msg.m_uiJoltBodyID = GetJoltBodyID();

  WWorld* pWorld = GetWorld();

  for (WComponentHandle hConstraint : allConstraints)
  {
    pWorld->SendMessage(hConstraint, msg);
  }

  SUPER::OnDeactivated();
}

void WJoltDynamicActorComponent::AddLinearImpulse(const WVec3& vImpulse, WUInt8 uiImpulseType)
{
  if (m_bKinematic || m_uiJoltBodyID == WInvalidIndex)
    return;

  const float fImpulse = WJoltCore::GetImpulseTypeConfig().GetImpulseForWeight(uiImpulseType, m_uiWeightCategory);

  GetWorld()->GetModule<WJoltWorldModule>()->AddImpulse(m_uiJoltBodyID, vImpulse * fImpulse);
}

void WJoltDynamicActorComponent::AddAngularImpulse(const WVec3& vImpulse, WUInt8 uiImpulseType)
{
  if (m_bKinematic || m_uiJoltBodyID == WInvalidIndex)
    return;

  const float fImpulse = WJoltCore::GetImpulseTypeConfig().GetImpulseForWeight(uiImpulseType, m_uiWeightCategory);

  GetWorld()->GetModule<WJoltWorldModule>()->AddTorque(m_uiJoltBodyID, vImpulse * fImpulse);
}

void WJoltDynamicActorComponent::AddConstraint(WComponentHandle hComponent)
{
  m_Constraints.PushBack(hComponent);
}

void WJoltDynamicActorComponent::RemoveConstraint(WComponentHandle hComponent)
{
  m_Constraints.RemoveAndSwap(hComponent);
}

void WJoltDynamicActorComponent::AddLinearImpulseAtPos(WMsgPhysicsAddImpulse& ref_msg)
{
  if (m_bKinematic || m_uiJoltBodyID == WInvalidIndex)
    return;

  const float fImpulse = WJoltCore::GetImpulseTypeConfig().GetImpulseForWeight(ref_msg.m_uiImpulseType, m_uiWeightCategory);

  GetWorld()->GetModule<WJoltWorldModule>()->AddImpulse(m_uiJoltBodyID, ref_msg.m_vImpulse * fImpulse, ref_msg.m_vGlobalPosition);
}

float WJoltDynamicActorComponent::GetMass() const
{
  if (m_bKinematic || m_uiJoltBodyID == WInvalidIndex)
    return 0.0f;

  auto& bodyLockInterface = GetWorld()->GetModule<WJoltWorldModule>()->GetJoltSystem()->GetBodyLockInterface();
  JPH::BodyLockRead bodyLock(bodyLockInterface, JPH::BodyID(m_uiJoltBodyID));

  const float fInverseMass = bodyLock.GetBody().GetMotionProperties()->GetInverseMass();
  return fInverseMass > 0.0f ? 1.0f / fInverseMass : 0.0f;
}

WUInt32 WJoltDynamicActorComponent::AddOrUpdateForce(WUInt32 uiForceID, WTime duration, const WVec3& vForce)
{
  if (m_uiJoltBodyID == WInvalidIndex)
    return 0;

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  WJoltForceId forceID(uiForceID);

  return pModule->AddOrUpdateForce(forceID, m_uiJoltBodyID, duration, vForce).m_Data;
}

void WJoltDynamicActorComponent::ClearForce(WUInt32 uiForceID)
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  WJoltForceId forceID(uiForceID);
  pModule->ClearForce(forceID);
}

const WJoltMaterial* WJoltDynamicActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    WResourceLock<WSurfaceResource> pSurface(m_hSurface, WResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<WJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return nullptr;
}

void WJoltDynamicActorComponent::SetSurfaceFile(WStringView sFile)
{
  if (!sFile.IsEmpty())
  {
    m_hSurface = WResourceManager::LoadResource<WSurfaceResource>(sFile);
  }
  else
  {
    m_hSurface = {};
  }

  if (m_hSurface.IsValid())
    WResourceManager::PreloadResource(m_hSurface);
}

WStringView WJoltDynamicActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltDynamicActorComponent);
