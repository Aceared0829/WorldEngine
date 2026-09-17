#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/MemoryStream.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltHeightfieldColliderComponent.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltStreamUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltHeightfieldColliderComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Heightfield", m_hHeightfield),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WHiddenAttribute(),
    new WCategoryAttribute("Physics/Jolt/Actors"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WJoltHeightfieldColliderComponent::WJoltHeightfieldColliderComponent() = default;
WJoltHeightfieldColliderComponent::~WJoltHeightfieldColliderComponent() = default;

void WJoltHeightfieldColliderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_hHeightfield;
}

void WJoltHeightfieldColliderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  if (uiVersion >= 2)
  {
    s >> m_hHeightfield;
  }
}

void WJoltHeightfieldColliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hHeightfield.IsValid())
    return;

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  if (pModule == nullptr)
    return;

  WResourceLock<WJoltHeightfieldResource> pRes(m_hHeightfield, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pRes.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    WLog::Warning("WJoltHeightfieldColliderComponent: could not load heightfield resource '{}'.", m_hHeightfield.GetResourceIdOrDescription());
    return;
  }

  const WDataBuffer& shapeData = pRes->GetShapeData();
  WRawMemoryStreamReader memReader(shapeData);
  WJoltStreamIn jStream(&memReader);
  auto shapeResult = JPH::Shape::sRestoreFromBinaryState(jStream);
  if (shapeResult.HasError())
  {
    WLog::Error("WJoltHeightfieldColliderComponent: failed to restore Jolt shape from '{}': {}", m_hHeightfield.GetResourceIdOrDescription(), shapeResult.GetError().c_str());
    return;
  }

  {
    const auto& surfaces = pRes->GetSurfaces();
    WTempHybridArray<JPH::PhysicsMaterialRefC, 8> materials;
    materials.SetCount(surfaces.GetCount());
    for (WUInt32 i = 0; i < surfaces.GetCount(); ++i)
    {
      const WJoltMaterial* pResolved = nullptr;
      if (surfaces[i].IsValid())
      {
        WResourceLock<WSurfaceResource> pSurf(surfaces[i], WResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (pSurf.GetAcquireResult() == WResourceAcquireResult::Final && pSurf->m_pPhysicsMaterialJolt != nullptr)
          pResolved = reinterpret_cast<const WJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);
      }
      materials[i] = pResolved != nullptr ? pResolved : WJoltCore::GetDefaultMaterial();
    }
    if (!materials.IsEmpty())
      shapeResult.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());
  }

  // Wrap in a RotatedTranslatedShape to apply the +90° X rotation that maps Jolt's Y-up to WorldEngine's Z-up.
  const JPH::Quat qRotX90 = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), 0.5f * JPH::JPH_PI);
  JPH::RotatedTranslatedShapeSettings rtsSettings(JPH::Vec3::sZero(), qRotX90, shapeResult.Get());
  JPH::ShapeSettings::ShapeResult rtsResult = rtsSettings.Create();
  if (rtsResult.HasError())
  {
    WLog::Error("WJoltHeightfieldColliderComponent: failed to wrap shape: {}", rtsResult.GetError().c_str());
    return;
  }

  WJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  // Use the first resolved material for body-level friction/restitution, or the default.
  const WJoltMaterial* pFirstMaterial = WJoltCore::GetDefaultMaterial();
  if (!pRes->GetSurfaces().IsEmpty() && pRes->GetSurfaces()[0].IsValid())
  {
    WResourceLock<WSurfaceResource> pSurf(pRes->GetSurfaces()[0], WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSurf.GetAcquireResult() == WResourceAcquireResult::Final && pSurf->m_pPhysicsMaterialJolt != nullptr)
      pFirstMaterial = reinterpret_cast<const WJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);
  }

  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  JPH::BodyCreationSettings bodyCfg;
  bodyCfg.SetShape(rtsResult.Get());
  bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(pRes->GetCollisionLayer(), WJoltBroadphaseLayer::Static);
  bodyCfg.mRestitution = pFirstMaterial->m_fRestitution;
  bodyCfg.mFriction = pFirstMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mEnhancedInternalEdgeRemoval = true;
  bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserData);

  auto* pBodies = &pModule->GetJoltSystem()->GetBodyInterface();
  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  if (pBody == nullptr)
  {
    WLog::Error("WJoltHeightfieldColliderComponent: Jolt body creation failed. Increase the maximum number of bodies.");
    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
    m_uiUserDataIndex = WInvalidIndex;
    m_uiObjectFilterID = WInvalidIndex;
    return;
  }

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();
  pModule->QueueBodyToAdd(pBody, false);
}

void WJoltHeightfieldColliderComponent::OnDeactivated()
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();

  if (pModule != nullptr)
  {
    JPH::BodyID bodyId(m_uiJoltBodyID);

    if (!bodyId.IsInvalid())
    {
      auto* pSystem = pModule->GetJoltSystem();
      auto* pBodies = &pSystem->GetBodyInterface();

      if (pBodies->IsAdded(bodyId))
      {
        pBodies->RemoveBody(bodyId);
      }
      else
      {
        pModule->RemoveBodyFromQueue(bodyId);
      }

      pBodies->DestroyBody(bodyId);
      m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
    }

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  SUPER::OnDeactivated();
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltHeightfieldColliderComponent);
