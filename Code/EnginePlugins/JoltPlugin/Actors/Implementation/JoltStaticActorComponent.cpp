#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/WorldLogLink.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

WJoltStaticActorComponentManager::WJoltStaticActorComponentManager(WWorld* pWorld)
  : WComponentManager<WJoltStaticActorComponent, WBlockStorageType::FreeList>(pWorld)
{
}

WJoltStaticActorComponentManager::~WJoltStaticActorComponentManager() = default;

void WJoltStaticActorComponentManager::UpdateTemporarilyDynamicActors()
{
  if (m_TemporarilyDynamicActors.IsEmpty())
    return;

  W_PROFILE_SCOPE("UpdateTemporarilyDynamicActors");

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  for (WComponentHandle hActor : m_TemporarilyDynamicActors)
  {
    WJoltStaticActorComponent* pActor = nullptr;
    if (!TryGetComponent(hActor, pActor))
      continue;

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

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltStaticActorComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("CollisionMesh", m_hCollisionMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("PullSurfacesFromGraphicsMesh", m_bPullSurfacesFromGraphicsMesh),
    W_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
    W_MESSAGE_HANDLER(WMsgPhysicsMakeTemporarilyDynamic, OnMsgPhysicsMakeTemporarilyDynamic),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltStaticActorComponent::WJoltStaticActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

WJoltStaticActorComponent::~WJoltStaticActorComponent() = default;

void WJoltStaticActorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
  bool m_bIncludeInNavmesh = true; // dummy
  s << m_bIncludeInNavmesh;
  s << m_bPullSurfacesFromGraphicsMesh;
  s << m_hSurface;
}


void WJoltStaticActorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;
  bool m_bIncludeInNavmesh = true; // dummy
  s >> m_bIncludeInNavmesh;
  s >> m_bPullSurfacesFromGraphicsMesh;
  s >> m_hSurface;
}

void WJoltStaticActorComponent::OnDeactivated()
{
  m_UsedSurfaces.Clear();

  SUPER::OnDeactivated();
}

bool WJoltStaticActorComponent::CanBeMadeDynamic()
{
  // a triangle mesh can only ever be part of a static body
  if (m_hCollisionMesh.IsValid())
  {
    WResourceLock<WJoltMeshResource> pMesh(m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pMesh.GetAcquireResult() != WResourceAcquireResult::Final)
      return false;

    if (pMesh->HasTriangleMesh())
      return false;

    if (pMesh->GetNumConvexParts() > 0)
      return true;
  }

  // otherwise the sub-shape components attached to this object have to provide the geometry,
  // and those are all convex
  WTempHybridArray<WJoltSubShape, 16> shapes;
  WTransform towner = GetOwner()->GetGlobalTransform();
  towner.m_vScale.Set(1.0f);

  GatherShapes(shapes, GetOwner(), towner, 1.0f, nullptr);

  const bool bHasShapes = !shapes.IsEmpty();

  for (auto& sub : shapes)
  {
    if (sub.m_pShape)
    {
      sub.m_pShape->Release();
    }
  }

  return bHasShapes;
}

void WJoltStaticActorComponent::OnMsgPhysicsMakeTemporarilyDynamic(WMsgPhysicsMakeTemporarilyDynamic& msg)
{
  W_IGNORE_UNUSED(msg);

  if (m_uiJoltBodyID == JPH::BodyID::cInvalidBodyID)
    return;

  WJoltStaticActorComponentManager* pManager = GetWorld()->GetOrCreateComponentManager<WJoltStaticActorComponentManager>();

  if (pManager->m_TemporarilyDynamicActors.Contains(GetHandle()))
    return;

  if (!CanBeMadeDynamic())
    return;

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto* pBodies = &pModule->GetJoltSystem()->GetBodyInterface();

  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, pMaterial).Failed())
    return;

  if (pMaterial == nullptr)
    pMaterial = WJoltCore::GetDefaultMaterial();

  // A static Jolt body has no motion properties and can't be switched to a dynamic motion type, so the existing body
  // is thrown away and a dynamic one is created in its place. The user data and the object filter ID are kept, so
  // that everything that refers to this actor keeps working.
  {
    const JPH::BodyID oldBodyId(m_uiJoltBodyID);

    if (pBodies->IsAdded(oldBodyId))
      pBodies->RemoveBody(oldBodyId);
    else
      pModule->RemoveBodyFromQueue(oldBodyId);

    pBodies->DestroyBody(oldBodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = JPH::EMotionType::Dynamic;
  bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, WJoltBroadphaseLayer::Dynamic);
  bodyCfg.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<WUInt64>(GetUserData());

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  if (pBody == nullptr)
  {
    WLog::Error("Jolt body creation failed. You need to increase the maximum number of bodies.");
    return;
  }

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();
  pModule->QueueBodyToAdd(pBody, true);

  GetOwner()->MakeDynamic();

  pManager->m_TemporarilyDynamicActors.PushBack(GetHandle());
}

void WJoltStaticActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  WJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, pMaterial).Failed())
  {
    WLog::Error("Jolt static actor component {} has no valid shape.", WArgComponent(this));
    return;
  }

  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  if (pMaterial == nullptr)
    pMaterial = WJoltCore::GetDefaultMaterial();

  bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, WJoltBroadphaseLayer::Static);
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mEnhancedInternalEdgeRemoval = true;
  bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  W_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, false);
}

void WJoltStaticActorComponent::CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
    return;

  WResourceLock<WJoltMeshResource> pMesh(m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() > 0)
  {
    for (WUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
    {
      auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<WUInt64>(GetUserData()), pMaterial, fDensity);

      WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
      sub.m_pShape = pShape;
      sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
    }
  }

  if (auto pTriMesh = pMesh->HasTriangleMesh())
  {
    WTempHybridArray<const WJoltMaterial*, 32> materials;

    if (pMaterial != nullptr)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount(), pMaterial);
    }

    if (m_bPullSurfacesFromGraphicsMesh)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount());
      PullSurfacesFromGraphicsMesh(materials);
    }

    auto pNewShape = pMesh->InstantiateTriangleMesh(reinterpret_cast<WUInt64>(GetUserData()), materials);

    WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape = pNewShape;
    sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void WJoltStaticActorComponent::PullSurfacesFromGraphicsMesh(WDynamicArray<const WJoltMaterial*>& ref_materials)
{
  // the materials don't hold a handle to the surfaces, so they don't keep them alive
  // therefore, we need to keep them alive by storing a handle
  m_UsedSurfaces.Clear();

  WMeshComponent* pMeshComp;
  if (!GetOwner()->TryGetComponentOfBaseType(pMeshComp))
    return;

  auto hMeshRes = pMeshComp->GetMesh();
  if (!hMeshRes.IsValid())
    return;

  WResourceLock<WMeshResource> pMeshRes(hMeshRes, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pMeshRes.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  if (pMeshRes->GetMaterials().GetCount() != ref_materials.GetCount())
    return;

  const WUInt32 uiNumMats = ref_materials.GetCount();
  m_UsedSurfaces.SetCount(uiNumMats);

  for (WUInt32 s = 0; s < uiNumMats; ++s)
  {
    // first check whether the component has a material override
    auto hMat = pMeshComp->GetMaterial(s);

    if (!hMat.IsValid())
    {
      // otherwise ask the mesh resource about the material
      hMat = pMeshRes->GetMaterials()[s];
    }

    if (!hMat.IsValid())
      continue;

    WResourceLock<WMaterialResource> pMat(hMat, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMat.GetAcquireResult() != WResourceAcquireResult::Final)
      continue;

    if (pMat->GetSurface().IsEmpty())
      continue;

    m_UsedSurfaces[s] = WResourceManager::LoadResource<WSurfaceResource>(pMat->GetSurface().GetString());

    WResourceLock<WSurfaceResource> pSurface(m_UsedSurfaces[s], WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSurface.GetAcquireResult() != WResourceAcquireResult::Final)
      continue;

    W_ASSERT_DEV(pSurface->m_pPhysicsMaterialJolt != nullptr, "Invalid Jolt material pointer on surface");
    ref_materials[s] = static_cast<WJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
  }
}

void WJoltStaticActorComponent::OnMsgExtractGeometry(WMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == WWorldGeoExtractionUtil::ExtractionMode::CollisionMesh)
  {
    if (m_hCollisionMesh.IsValid())
    {
      WResourceLock<WJoltMeshResource> pMesh(m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded);

      msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
    }

    ExtractSubShapeGeometry(GetOwner(), msg);
  }
}

void WJoltStaticActorComponent::SetMesh(const WJoltMeshResourceHandle& hMesh)
{
  m_hCollisionMesh = hMesh;
}

const WJoltMaterial* WJoltStaticActorComponent::GetJoltMaterial() const
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

void WJoltStaticActorComponent::SetSurfaceFile(WStringView sFile)
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

WStringView WJoltStaticActorComponent::GetSurfaceFile() const
{
  return m_hSurface.GetResourceID();
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltStaticActorComponent);
