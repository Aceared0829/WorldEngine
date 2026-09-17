#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameComponentsPlugin/Gameplay/ProjectileComponent.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <GameEngine/Physics/ImpulseType.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WProjectileReaction, 2)
  W_ENUM_CONSTANT(WProjectileReaction::Absorb),
  W_ENUM_CONSTANT(WProjectileReaction::Reflect),
  W_ENUM_CONSTANT(WProjectileReaction::Bounce),
  W_ENUM_CONSTANT(WProjectileReaction::Attach),
  W_ENUM_CONSTANT(WProjectileReaction::PassThrough)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WProjectileBounceOrientation, 1)
  W_ENUM_CONSTANT(WProjectileBounceOrientation::Reflection),
  W_ENUM_CONSTANT(WProjectileBounceOrientation::Spinning)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WProjectileSurfaceInteraction, WNoBase, 3, WRTTIDefaultAllocator<WProjectileSurfaceInteraction>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Surface", m_hSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package), new WRequiredAttribute()),
    W_ENUM_MEMBER_PROPERTY("Reaction", WProjectileReaction, m_Reaction),
    W_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new WDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    W_MEMBER_PROPERTY("ImpulseType", m_uiImpulseType)->AddAttributes(new WDynamicEnumAttribute("PhysicsImpulseType")),
    W_MEMBER_PROPERTY("Impulse", m_fImpulse),
    W_MEMBER_PROPERTY("Damage", m_fDamage),
    W_MEMBER_PROPERTY("InertiaRatio", m_fInertiaRatio)->AddAttributes(new WDefaultValueAttribute(5.0f)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WProjectileComponent, 8, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    W_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new WClampValueAttribute(WTime(), WVariant())),
    W_MEMBER_PROPERTY("SpawnPrefabOnStatic", m_bSpawnPrefabOnStatic),
    W_RESOURCE_MEMBER_PROPERTY("OnDeathPrefab", m_hDeathPrefab)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ENUM_MEMBER_PROPERTY("BounceOrientation", WProjectileBounceOrientation, m_BounceOrientation)->AddAttributes(new WDefaultValueAttribute((WInt8)WProjectileBounceOrientation::Reflection)),
    W_MEMBER_PROPERTY("StaticVelocityRatio", m_fStaticVelocityRatio)->AddAttributes(new WDefaultValueAttribute(0.05f), new WClampValueAttribute(0.0f, WVariant())),
    W_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", WPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new WDefaultValueAttribute(WVariant(WPhysicsShapeType::Default & ~(WPhysicsShapeType::Trigger)))),
    W_ACCESSOR_PROPERTY("FallbackSurface", GetFallbackSurfaceFile, SetFallbackSurfaceFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    W_ARRAY_MEMBER_PROPERTY("Interactions", m_SurfaceInteractions),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnTriggered),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 0.4f, WColor::OrangeRed),
    new WSphereManipulatorAttribute("Radius"),
    new WSphereVisualizerAttribute("Radius"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Helper function to recalculate the sphere position from hit position
  /// \param vOrigin is expected to be the initial position of the particle before SweepTest
  WVec3 CalculateSphereCenterPosition(const WVec3& vOrigin, const WVec3& vDirection, const WPhysicsCastResult& castResult, float fPenetrationDepth)
  {
    if (castResult.m_fDistance == 0.0f)
    {
      // It says that the hit position is "behind" (if we move along vDirection) vOrigin
      // Thus, return the original position
      return vOrigin;
    }

    return vOrigin + vDirection * (castResult.m_fDistance + fPenetrationDepth);
  }
} // namespace

WProjectileComponent::WProjectileComponent()
{
  m_fMetersPerSecond = 10.0f;
  m_uiCollisionLayer = 0;
  m_fRadius = 0.0f;
  m_BounceOrientation = WProjectileBounceOrientation::Reflection;
  m_fStaticVelocityRatio = 0.05f;
  m_fGravityMultiplier = 0.0f;
  m_vVelocity.SetZero();
  m_bSpawnPrefabOnStatic = false;
}

WProjectileComponent::~WProjectileComponent() = default;

void WProjectileComponent::Update()
{
  const float fPenetrationDepth = m_fRadius * 0.5f;

  if (m_fGravityMultiplier == 0.0f && m_fMetersPerSecond == 0.0f)
    return;

  WPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetModule<WPhysicsWorldModuleInterface>();

  WGameObject* pEntity = GetOwner();
  const WVec3 vCurPosition = pEntity->GetGlobalPosition();

  const float fTimeDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  WVec3 vNewPosition;

  // gravity
  if (m_fGravityMultiplier != 0.0f && m_fMetersPerSecond > 0.0f) // mps == 0 for attached state
  {
    const WVec3 vGravity = pPhysicsInterface ? (pPhysicsInterface->GetGravity() * m_fGravityMultiplier) : WVec3::MakeZero();

    m_vVelocity += vGravity * fTimeDiff;
  }

  WVec3 vCurDirection = m_vVelocity * fTimeDiff;
  float fDistance = 0.0f;

  if (!vCurDirection.IsZero())
    fDistance = vCurDirection.GetLengthAndNormalize();

  WPhysicsQueryParameters queryParams(m_uiCollisionLayer);
  queryParams.m_bIgnoreInitialOverlap = true;
  queryParams.m_ShapeTypes = m_ShapeTypesToHit;

  WPhysicsCastResult castResult;
  if (pPhysicsInterface && QueryCollision(*pPhysicsInterface, castResult, vCurPosition, vCurDirection, fDistance, queryParams))
  {
    const WVec3 vNewCenterPosition = (m_fRadius > 0.0f)
                                        ? CalculateSphereCenterPosition(vCurPosition, vCurDirection, castResult, fPenetrationDepth)
                                        : castResult.m_vPosition;

    const WSurfaceResourceHandle hSurface = castResult.m_hSurface.IsValid() ? castResult.m_hSurface : m_hFallbackSurface;

    const WInt32 iInteraction = FindSurfaceInteraction(hSurface);

    if (iInteraction == -1)
    {
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
      vNewPosition = vNewCenterPosition;
    }
    else
    {
      const auto& interaction = m_SurfaceInteractions[iInteraction];

      if (!interaction.m_sInteraction.IsEmpty())
      {
        TriggerSurfaceInteraction(hSurface, castResult.m_hActorObject, castResult.m_vPosition, castResult.m_vNormal, vCurDirection, interaction.m_sInteraction);
      }

      // if we hit some valid object
      if (!castResult.m_hActorObject.IsInvalidated())
      {
        WGameObject* pObject = nullptr;

        // apply a physical impulse
        if (interaction.m_uiImpulseType >= WImpulseTypeConfig::FirstValidKey || (interaction.m_uiImpulseType == WImpulseTypeConfig::CustomValueKey && interaction.m_fImpulse > 0.0f))
        {
          if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
          {
            WMsgPhysicsAddImpulse msg;
            msg.m_uiImpulseType = interaction.m_uiImpulseType;
            msg.m_vGlobalPosition = castResult.m_vPosition;
            msg.m_vImpulse = vCurDirection;
            msg.m_uiObjectFilterID = castResult.m_uiObjectFilterID;
            msg.m_pInternalPhysicsShape = castResult.m_pInternalPhysicsShape;
            msg.m_pInternalPhysicsActor = castResult.m_pInternalPhysicsActor;

            if (interaction.m_uiImpulseType == WImpulseTypeConfig::CustomValueKey)
            {
              msg.m_vImpulse *= interaction.m_fImpulse;
            }

            pObject->SendMessage(msg);
          }
        }

        // apply damage
        if (interaction.m_fDamage > 0.0f)
        {
          // skip the TryGetObject if we already did that above
          if (pObject != nullptr || GetWorld()->TryGetObject(castResult.m_hShapeObject, pObject))
          {
            WMsgDamage msg;
            msg.m_fDamage = interaction.m_fDamage;
            msg.m_vGlobalPosition = castResult.m_vPosition;
            msg.m_vImpactDirection = vCurDirection;

            WGameObject* pHitShape = nullptr;
            if (GetWorld()->TryGetObject(castResult.m_hShapeObject, pHitShape))
            {
              msg.m_sHitObjectName = pHitShape->GetName();
            }
            else
            {
              msg.m_sHitObjectName = pObject->GetName();
            }

            pObject->SendEventMessage(msg, this);
          }
        }
      }

      if (interaction.m_Reaction == WProjectileReaction::Absorb)
      {
        SpawnDeathPrefab();


        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
        vNewPosition = vNewCenterPosition;
      }
      else if (interaction.m_Reaction == WProjectileReaction::Reflect || interaction.m_Reaction == WProjectileReaction::Bounce)
      {
        vNewPosition = vCurPosition;
        const float velocityToNormalProj = m_vVelocity.Dot(castResult.m_vNormal);
        if (velocityToNormalProj < 0.0f)
        {
          // the same as m_vVelocity.GetReflectedVector but reuse precalculated velocityToNormalProj
          WVec3 vNewVelocity = m_vVelocity - 2.0f * velocityToNormalProj * castResult.m_vNormal;

          // Position of the projectile at the moment of reflection

          if (interaction.m_Reaction == WProjectileReaction::Bounce)
          {
            WResourceLock<WSurfaceResource> pSurface(hSurface, WResourceAcquireMode::BlockTillLoaded);

            if (pSurface)
            {
              vNewVelocity *= pSurface->GetDescriptor().m_fPhysicsRestitution;
            }

            if (ShouldStopProjectile(*pPhysicsInterface, castResult, vNewVelocity))
            {
              vNewVelocity = WVec3::MakeZero();
              m_fGravityMultiplier = 0.0f;

              if (m_bSpawnPrefabOnStatic)
              {
                SpawnDeathPrefab();
                GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
              }
            }
          }

          const WVec3 vPositionOnReflection = vCurPosition + castResult.m_fDistance * vCurDirection;
          if (m_BounceOrientation == WProjectileBounceOrientation::Reflection)
          {
            ApplyReflectionRotation(vCurDirection, castResult.m_vNormal);
          }
          else
          {
            ApplySpinningRotation(interaction, castResult, vPositionOnReflection, vCurDirection, vNewVelocity);
          }
          const float fAbsVelocity = m_vVelocity.GetLength();
          // condition velocityToNormalProj < 0.0f implies fAbsVelocity is non-zero
          const float fTimeBeforeReflection = castResult.m_fDistance / fAbsVelocity;
          const float fTimeLeft = WMath::Max(0.0f, fTimeDiff - fTimeBeforeReflection);
          // Path travelled back after the reflection
          vNewPosition = vPositionOnReflection + vNewVelocity * fTimeLeft;
          m_vVelocity = vNewVelocity;
        }
        else
        {
          vNewPosition += m_vVelocity * fTimeDiff;
        }
      }
      else if (interaction.m_Reaction == WProjectileReaction::Attach)
      {
        m_fMetersPerSecond = 0.0f;
        m_fGravityMultiplier = 0.0f;
        vNewPosition = vNewCenterPosition;

        WGameObject* pObject;
        if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
        {
          pObject->AddChild(GetOwner()->GetHandle(), WTransformPreservation::Enum::PreserveGlobal);
        }
      }
      else if (interaction.m_Reaction == WProjectileReaction::PassThrough)
      {
        vNewPosition = vCurPosition + fDistance * vCurDirection;
      }
    }
  }
  else
  {
    vNewPosition = vCurPosition + fDistance * vCurDirection;
  }

  GetOwner()->SetGlobalPosition(vNewPosition);
}

void WProjectileComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fMetersPerSecond;
  s << m_fGravityMultiplier;
  s << m_uiCollisionLayer;
  s << m_MaxLifetime;
  s << m_hDeathPrefab;

  // Version 3
  s << m_hFallbackSurface;

  s << m_SurfaceInteractions.GetCount();
  for (const auto& ia : m_SurfaceInteractions)
  {
    s << ia.m_hSurface;

    WProjectileReaction::StorageType storage = ia.m_Reaction;
    s << storage;

    s << ia.m_sInteraction;

    // Version 3
    s << ia.m_fImpulse;

    // Version 4
    s << ia.m_fDamage;

    // Version 7
    s << ia.m_uiImpulseType;
  }

  // Version 5
  s << m_ShapeTypesToHit;

  // Version 6
  s << m_bSpawnPrefabOnStatic;

  // Version 8
  s << m_fRadius;
  WProjectileBounceOrientation::StorageType bounceOrientation = m_BounceOrientation;
  s << bounceOrientation;
  s << m_fStaticVelocityRatio;

  // Version 8
  for (const auto& ia : m_SurfaceInteractions)
  {
    s << ia.m_fInertiaRatio;
  }
}

void WProjectileComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_fGravityMultiplier;
  s >> m_uiCollisionLayer;
  s >> m_MaxLifetime;
  s >> m_hDeathPrefab;

  if (uiVersion >= 3)
  {
    s >> m_hFallbackSurface;
  }

  WUInt32 count;
  s >> count;
  m_SurfaceInteractions.SetCount(count);
  for (WUInt32 i = 0; i < count; ++i)
  {
    auto& ia = m_SurfaceInteractions[i];
    s >> ia.m_hSurface;

    WProjectileReaction::StorageType storage = 0;
    s >> storage;
    ia.m_Reaction = (WProjectileReaction::Enum)storage;

    s >> ia.m_sInteraction;

    if (uiVersion >= 3)
    {
      s >> ia.m_fImpulse;
    }

    if (uiVersion >= 4)
    {
      s >> ia.m_fDamage;
    }

    if (uiVersion >= 7)
    {
      s >> ia.m_uiImpulseType;
    }
  }

  if (uiVersion >= 5)
  {
    s >> m_ShapeTypesToHit;
  }

  if (uiVersion >= 6)
  {
    s >> m_bSpawnPrefabOnStatic;
  }

  if (uiVersion >= 8)
  {
    s >> m_fRadius;
    WProjectileBounceOrientation::StorageType bounceOrientation = WProjectileBounceOrientation::Reflection;
    s >> bounceOrientation;
    m_BounceOrientation = (WProjectileBounceOrientation::Enum)bounceOrientation;
    s >> m_fStaticVelocityRatio;

    for (auto& ia : m_SurfaceInteractions)
    {
      s >> ia.m_fInertiaRatio;
    }
  }
  else
  {
    m_fRadius = 0.0f;
    m_BounceOrientation = WProjectileBounceOrientation::Reflection;
    m_fStaticVelocityRatio = 0.05f;
  }
}


bool WProjectileComponent::QueryCollision(const WPhysicsWorldModuleInterface& physicsInterface, WPhysicsCastResult& out_result, const WVec3& vStart, const WVec3& vDirection, float fDistance, const WPhysicsQueryParameters& queryParams) const
{
  if (m_fRadius > 0.0f)
  {
    return physicsInterface.SweepTestSphere(out_result, m_fRadius, vStart, vDirection, fDistance, queryParams);
  }

  return physicsInterface.Raycast(out_result, vStart, vDirection, fDistance, queryParams);
}

void WProjectileComponent::ApplyReflectionRotation(const WVec3& vCurDirection, const WVec3& vSurfaceNormal)
{
  const WVec3 vNewDirection = vCurDirection.GetReflectedVector(vSurfaceNormal);
  const WQuat qRot = WQuat::MakeShortestRotation(vCurDirection, vNewDirection);
  GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());
}

void WProjectileComponent::ApplySpinningRotation(const WProjectileSurfaceInteraction& interaction,
  const WPhysicsCastResult& castResult,
  const WVec3& vPositionOnReflection,
  const WVec3& vCurDirection,
  const WVec3& vNewVelocity)
{
  if (interaction.m_fInertiaRatio == 0.0f)
    return;

  const WVec3 vRelativeHitPosOnReflection = castResult.m_vPosition - vPositionOnReflection;
  const float vRelativeHitPosLength = vRelativeHitPosOnReflection.GetLength();
  WVec3 vForceRadialVector;
  if (m_fRadius > 0.0f && vRelativeHitPosLength > 0.0f)
  {
    // if the projectile has a radius it is natural to assume that the torque is created
    // by the force acting on the radial vector connecting the center to the contact point
    vForceRadialVector = vRelativeHitPosOnReflection / vRelativeHitPosLength;
  }
  else
  {
    // Otherwise assume radial vector coincides with direction
    vForceRadialVector = vCurDirection;
  }

  // Assuming the timeDiff is small we can say that the difference between velocities
  // is proportional to the acting force
  WVec3 vEffectiveForce = m_vVelocity - vNewVelocity;
  // We assume that this force is responsible for torque and angular momentum
  WVec3 vEffectiveTorque = -vForceRadialVector.CrossRH(vEffectiveForce) / interaction.m_fInertiaRatio;

  const float fAngle = WMath::Min(vEffectiveTorque.GetLength(), WMath::Pi<float>());
  if (fAngle > 0.0f)
  {
    WAngle angle = WAngle::MakeFromRadian(fAngle);
    WQuat qRot = WQuat::MakeFromAxisAndAngle(vEffectiveTorque.GetNormalized(), angle);
    GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());
  }
}

bool WProjectileComponent::ShouldStopProjectile(const WPhysicsWorldModuleInterface& physicsInterface, const WPhysicsCastResult& castResult, const WVec3& vVelocity)
{
  const WVec3 vGravity = physicsInterface.GetGravity();
  if (!vGravity.IsZero())
  {
    const WVec3 vGravityDir = vGravity.GetNormalized();
    // Check that projectile has hit the ground
    // if not - return false to make sure it won't hang in the air after being stopped
    if (-vGravityDir.Dot(castResult.m_vNormal) < WMath::Cos(WAngle::MakeFromDegree(40.f)))
    {
      return false;
    }
  }

  return vVelocity.GetLength() < m_fStaticVelocityRatio * m_fMetersPerSecond;
}

WInt32 WProjectileComponent::FindSurfaceInteraction(const WSurfaceResourceHandle& hSurface) const
{
  WSurfaceResourceHandle hCurSurf = hSurface;

  while (hCurSurf.IsValid())
  {
    for (WUInt32 i = 0; i < m_SurfaceInteractions.GetCount(); ++i)
    {
      if (hCurSurf == m_SurfaceInteractions[i].m_hSurface)
        return i;
    }

    // get parent surface
    {
      WResourceLock<WSurfaceResource> pSurf(hCurSurf, WResourceAcquireMode::BlockTillLoaded);
      hCurSurf = pSurf->GetDescriptor().m_hBaseSurface;
    }
  }

  return -1;
}


void WProjectileComponent::TriggerSurfaceInteraction(const WSurfaceResourceHandle& hSurface, WGameObjectHandle hObject, const WVec3& vPos, const WVec3& vNormal, const WVec3& vDirection, const char* szInteraction)
{
  WResourceLock<WSurfaceResource> pSurface(hSurface, WResourceAcquireMode::BlockTillLoaded);
  pSurface->InteractWithSurface(GetWorld(), hObject, vPos, vNormal, vDirection, WTempHashedString(szInteraction), &GetOwner()->GetTeamID());
}

static WHashedString s_sSuicide = WMakeHashedString("Suicide");

void WProjectileComponent::OnSimulationStarted()
{
  if (m_MaxLifetime.GetSeconds() > 0.0)
  {
    WMsgComponentInternalTrigger msg;
    msg.m_sMessage = s_sSuicide;

    PostMessage(msg, m_MaxLifetime);

    // make sure the prefab is available when the projectile dies
    if (m_hDeathPrefab.IsValid())
    {
      WResourceManager::PreloadResource(m_hDeathPrefab);
    }
  }

  m_vVelocity = GetOwner()->GetGlobalDirForwards() * m_fMetersPerSecond;
}

void WProjectileComponent::SpawnDeathPrefab()
{
  if (!m_bSpawnPrefabOnStatic)
  {
    return;
  }

  if (m_hDeathPrefab.IsValid())
  {
    WResourceLock<WPrefabResource> pPrefab(m_hDeathPrefab, WResourceAcquireMode::AllowLoadingFallback);

    WPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options, nullptr);
  }
}

void WProjectileComponent::OnTriggered(WMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  SpawnDeathPrefab();

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

void WProjectileComponent::SetFallbackSurfaceFile(WStringView sFile)
{
  if (!sFile.IsEmpty())
  {
    m_hFallbackSurface = WResourceManager::LoadResource<WSurfaceResource>(sFile);
  }
  else
  {
    m_hFallbackSurface = {};
  }

  if (m_hFallbackSurface.IsValid())
    WResourceManager::PreloadResource(m_hFallbackSurface);
}

WStringView WProjectileComponent::GetFallbackSurfaceFile() const
{
  if (!m_hFallbackSurface.IsValid())
    return "";

  return m_hFallbackSurface.GetResourceID();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WProjectileComponentPatch_1_2 : public WGraphPatch
{
public:
  WProjectileComponentPatch_1_2()
    : WGraphPatch("WProjectileComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Gravity Multiplier", "GravityMultiplier");
    pNode->RenameProperty("Max Lifetime", "MaxLifetime");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
    pNode->RenameProperty("Collision Layer", "CollisionLayer");
  }
};

class WProjectileComponentPatch_5_6 : public WGraphPatch
{
public:
  WProjectileComponentPatch_5_6()
    : WGraphPatch("WProjectileComponent", 6)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("TimeoutPrefab", "DeathPrefab");
  }
};

WProjectileComponentPatch_1_2 g_WProjectileComponentPatch_1_2;


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_ProjectileComponent);
