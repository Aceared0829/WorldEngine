#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/WorldLogLink.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

WJoltQueryShapeActorComponentManager::WJoltQueryShapeActorComponentManager(WWorld* pWorld)
  : WComponentManager<WJoltQueryShapeActorComponent, WBlockStorageType::FreeList>(pWorld)
{
}

WJoltQueryShapeActorComponentManager::~WJoltQueryShapeActorComponentManager() = default;

void WJoltQueryShapeActorComponentManager::UpdateMovingQueryShapes()
{
  W_PROFILE_SCOPE("UpdateMovingQueryShapes");

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  for (auto pComponent : m_MovingQueryShapes)
  {
    JPH::BodyID bodyId(pComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    WGameObject* pObject = pComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const WSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const WSimdQuat rot = pObject->GetGlobalRotationSimd();

    JPH::Quat jRot = WJoltConversionUtils::ToQuat(rot);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    // Jolt is overly strict about normalization
    jRot = jRot.Normalized();
#endif

    pBodies->SetPositionAndRotation(bodyId, WJoltConversionUtils::ToVec3(pos), jRot, JPH::EActivation::DontActivate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltQueryShapeActorComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
  }
  W_END_PROPERTIES;
}
W_END_COMPONENT_TYPE
// clang-format on

WJoltQueryShapeActorComponent::WJoltQueryShapeActorComponent() = default;
WJoltQueryShapeActorComponent::~WJoltQueryShapeActorComponent() = default;

void WJoltQueryShapeActorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hSurface;
}

void WJoltQueryShapeActorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSurface;
}

void WJoltQueryShapeActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, 1.0f, GetJoltMaterial()).Failed())
  {
    WLog::Error("Jolt query-shape actor component {} has no valid shape.", WArgComponent(this));
    return;
  }

  WJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, WJoltBroadphaseLayer::Query);
  bodyCfg.mMotionQuality = JPH::EMotionQuality::Discrete;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  W_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, false);

  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<WJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.PushBack(this);
  }
}

void WJoltQueryShapeActorComponent::OnDeactivated()
{
  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<WJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.RemoveAndSwap(this);
  }

  SUPER::OnDeactivated();
}

void WJoltQueryShapeActorComponent::SetSurfaceFile(WStringView sFile)
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

WStringView WJoltQueryShapeActorComponent::GetSurfaceFile() const
{
  return m_hSurface.GetResourceID();
}

const WJoltMaterial* WJoltQueryShapeActorComponent::GetJoltMaterial() const
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


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltQueryShapeActorComponent);
