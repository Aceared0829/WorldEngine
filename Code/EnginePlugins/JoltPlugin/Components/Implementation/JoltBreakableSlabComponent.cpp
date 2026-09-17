#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/Implementation/ResourceHandleReflection.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <JoltPlugin/Components/JoltBreakableSlabComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <Physics/Collision/Shape/ConvexShape.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WJoltBreakableSlabFlags, 1)
  W_ENUM_CONSTANT(WJoltBreakableSlabFlags::FixedEdgeTop),
    W_ENUM_CONSTANT(WJoltBreakableSlabFlags::FixedEdgeRight),
    W_ENUM_CONSTANT(WJoltBreakableSlabFlags::FixedEdgeBottom),
    W_ENUM_CONSTANT(WJoltBreakableSlabFlags::FixedEdgeLeft),
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_ENUM(WJoltBreakableShape, 1)
  W_ENUM_CONSTANT(WJoltBreakableShape::Rectangle),
    W_ENUM_CONSTANT(WJoltBreakableShape::Triangle),
    W_ENUM_CONSTANT(WJoltBreakableShape::Circle),
W_END_STATIC_REFLECTED_ENUM

WAtomicInteger32 WJoltBreakableSlabComponent::s_iShardMeshCounter;

WCVarBool cvar_BreakableSlabVis("Jolt.BreakableSlab.DebugVis", false, WCVarFlags::Default, "Debug draw the state of breakable slabs.");

WJoltBreakableSlabComponentManager::WJoltBreakableSlabComponentManager(WWorld* pWorld)
  : WComponentManager<WJoltBreakableSlabComponent, WBlockStorageType::FreeList>(pWorld)
{
}

WJoltBreakableSlabComponentManager::~WJoltBreakableSlabComponentManager() = default;

void WJoltBreakableSlabComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltBreakableSlabComponentManager::PreAsyncUpdate, this);
    desc.m_Phase = WWorldUpdatePhase::PreAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltBreakableSlabComponentManager::ReinitSlabs, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = false;
    desc.m_uiAsyncPhaseBatchSize = 16;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltBreakableSlabComponentManager::PostAsyncUpdate, this);
    desc.m_Phase = WWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    RegisterUpdateFunction(desc);
  }
}

void WJoltBreakableSlabComponentManager::PreAsyncUpdate(const WWorldModule::UpdateContext& context)
{
  m_iTriggerBoundsUpdateSlot = 0;
  m_TriggerBoundsUpdate.SetCount(m_ComponentStorage.GetCount());
}

void WJoltBreakableSlabComponentManager::ReinitSlabs(const WWorldModule::UpdateContext& context)
{
  for (auto it = m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    WJoltBreakableSlabComponent* pComponent = it;
    if (pComponent->IsActive() && pComponent->m_bReinitMeshes)
    {
      pComponent->ReinitMeshes();

      // we need to call TriggerUpdateBounds() on this component, but we can't do this on just any thread, so queue this for the PostAsync update
      const WInt32 iSlot = m_iTriggerBoundsUpdateSlot.PostIncrement();
      m_TriggerBoundsUpdate[iSlot] = pComponent;
    }
  }
}

void WJoltBreakableSlabComponentManager::PostAsyncUpdate(const WWorldModule::UpdateContext& context)
{
  W_PROFILE_SCOPE("UpdateBreakableSlabs");

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  // when updating slabs, we activate bodies, which would immediately modify the active slabs map
  // and then create a crash during iteration
  // therefore we have to make a copy of the active slabs first
  WTempHybridArray<WJoltBreakableSlabComponent*, 64> activeSlabs;
  activeSlabs.Reserve(pModule->GetActiveSlabs().GetCount());

  for (auto it : pModule->GetActiveSlabs())
  {
    WJoltBreakableSlabComponent* pComponent = it.Key();
    activeSlabs.PushBack(pComponent);
  }

  for (auto pComponent : activeSlabs)
  {
    pComponent->RetrieveShardTransforms();
  }

  for (WJoltBreakableSlabComponent* pComponent : pModule->GetSlabsPutToSleep())
  {
    pComponent->RetrieveShardTransforms();
  }

  for (auto hComponent : m_RequireBreakage)
  {
    WJoltBreakableSlabComponent* pComponent;
    if (TryGetComponent(hComponent, pComponent))
    {
      if (pComponent->IsActiveAndSimulating() && pComponent->m_pShatterTask == nullptr)
      {
        pComponent->m_pShatterTask = W_DEFAULT_NEW(WShatterTask);
        pComponent->m_pShatterTask->ConfigureTask("ShatterGlass", WTaskNesting::Never);
        pComponent->m_pShatterTask->m_pComponent = pComponent;
        pComponent->m_pShatterTask->m_ShatterPoints = pComponent->m_ShatterPoints;

        WTaskSystem::StartSingleTask(pComponent->m_pShatterTask, WTaskPriority::In3Frames); // allow some delay, this can take longer than a single frame

        m_RequireBreakUpdate.Insert(hComponent);
      }

      // make sure the shatter points are cleared, even if nothing was done
      // otherwise the component won't be put into m_RequireBreakage again
      pComponent->m_ShatterPoints.Clear();
    }
  }

  m_RequireBreakage.Clear();

  for (auto hComponent : m_RequireBreakUpdate)
  {
    WJoltBreakableSlabComponent* pComponent;
    if (TryGetComponent(hComponent, pComponent) && pComponent->IsActiveAndSimulating() && pComponent->m_pShatterTask != nullptr)
    {
      if (pComponent->m_pShatterTask->IsTaskFinished())
      {
        pComponent->ApplyBreak(pComponent->m_pShatterTask->m_Shapes, pComponent->m_pShatterTask->m_hShardsMesh, pComponent->m_pShatterTask->m_vFinalImpulse);
        pComponent->m_pShatterTask = nullptr;

        m_RequireBreakUpdate.Remove(hComponent);
        break; // sufficient to update one slab per frame
      }
    }
    else
    {
      m_RequireBreakUpdate.Remove(hComponent);
    }
  }

  for (WInt32 i = 0; i < m_iTriggerBoundsUpdateSlot; ++i)
  {
    m_TriggerBoundsUpdate[i]->TriggerLocalBoundsUpdate();
  }

  if (cvar_BreakableSlabVis)
  {
    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->IsActiveAndSimulating())
      {
        it->DebugDraw();
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltBreakableSlabComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 8.0f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 8.0f), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new WDefaultValueAttribute(0.02f), new WClampValueAttribute(0.005f, 1.0f), new WSuffixAttribute(" m")),
    W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("UVScale", GetUvScale, SetUvScale)->AddAttributes(new WDefaultValueAttribute(WVec2(1.0f))),
    W_MEMBER_PROPERTY("CollisionLayerStatic", m_uiCollisionLayerStatic)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("CollisionLayerDynamic", m_uiCollisionLayerDynamic)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_ENUM_ACCESSOR_PROPERTY("Shape", WJoltBreakableShape, GetShape, SetShape),
    W_BITFLAGS_ACCESSOR_PROPERTY("Flags", WJoltBreakableSlabFlags, GetFlags, SetFlags),
    W_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("ContactReportForceThreshold", m_fContactReportForceThreshold),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Effects"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgPhysicsAddImpulse, OnMsgPhysicsAddImpulse),
    W_MESSAGE_HANDLER(WMsgPhysicContact, OnMsgPhysicContactMsg),
    W_MESSAGE_HANDLER(WMsgPhysicCharacterContact, OnMsgPhysicCharacterContact),
    W_MESSAGE_HANDLER(WMsgCustomInstanceDataOffsetChanged, OnMsgCustomInstanceDataOffsetChanged),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Restore),
    W_SCRIPT_FUNCTION_PROPERTY(ShatterCellular, In, "vGlobalPosition", In, "fCellSize", In, "vImpulse", In, "fMakeDynamicRadius"),
    W_SCRIPT_FUNCTION_PROPERTY(ShatterRadial, In, "vGlobalPosition", In, "fImpactRadius", In, "vImpulse", In, "fMakeDynamicRadius"),
    W_SCRIPT_FUNCTION_PROPERTY(ShatterAll, In, "fShardSize", In, "vImpulse"),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WJoltBreakableSlabComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fWidth;
  s << m_fHeight;
  s << m_fThickness;
  s << m_hMaterial;
  s << m_vUvScale;
  s << m_uiCollisionLayerStatic;
  s << m_uiCollisionLayerDynamic;
  s << m_Flags;
  s << m_Shape;
  s << m_fGravityFactor;
  s << m_fContactReportForceThreshold;
}

void WJoltBreakableSlabComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_fWidth;
  s >> m_fHeight;
  s >> m_fThickness;
  s >> m_hMaterial;
  s >> m_vUvScale;
  s >> m_uiCollisionLayerStatic;
  s >> m_uiCollisionLayerDynamic;
  s >> m_Flags;
  s >> m_Shape;
  s >> m_fGravityFactor;
  s >> m_fContactReportForceThreshold;
}

void WJoltBreakableSlabComponent::OnActivated()
{
  SUPER::OnActivated();

  m_bReinitMeshes = true;
}

void WJoltBreakableSlabComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  if (m_uiObjectFilterID == WInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually
    m_uiObjectFilterID = pModule->CreateObjectFilterID();
  }

  if (m_uiUserDataIndexStatic == WInvalidIndex)
  {
    WJoltUserData* pUserData = nullptr;
    m_uiUserDataIndexStatic = pModule->AllocateUserData(pUserData);
    pUserData->Init(this, WOnJoltContact::SendContactMsg);
  }

  if (m_uiUserDataIndexDynamic == WInvalidIndex)
  {
    WJoltUserData* pUserData = nullptr;
    m_uiUserDataIndexDynamic = pModule->AllocateUserData(pUserData);
    pUserData->Init(this, WOnJoltContact::ImpactReactions);
  }

  WTempHybridArray<JPH::Ref<JPH::ConvexShape>, 2> shapes;

  if (m_Shape == WJoltBreakableShape::Rectangle)
  {
    // this code path is an optimization for PrepareShardColliders() because box colliders are much more efficient to set up (and simulate)
    shapes.SetCount(1);

    const float fShardThickness = WMath::Max(m_fThickness, JPH::cDefaultConvexRadius * 2.0f);
    const float fThick1 = -(fShardThickness - m_fThickness) * 0.5f;
    const float fThick2 = fShardThickness + fThick1;

    JPH::BoxShapeSettings shapeSettings;
    shapeSettings.mHalfExtent.SetX(m_fWidth * 0.5f);
    shapeSettings.mHalfExtent.SetY(m_fHeight * 0.5f);
    shapeSettings.mHalfExtent.SetZ(fShardThickness * 0.5f);
    shapeSettings.mDensity = 0.1f;
    shapeSettings.mUserData = 0;

    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (!shapeResult.HasError())
    {
      shapes[0] = (JPH::ConvexShape*)(shapeResult.Get().GetPtr());
    }
  }
  else
  {
    PrepareShardColliders(0, shapes);
  }

  CreateShardColliders(0, shapes);
  m_uiShardsSleeping = 190;

  InvalidateCachedRenderData();

  if (m_bReinitMeshes)
  {
    ReinitMeshes();
    TriggerLocalBoundsUpdate();
  }
}

void WJoltBreakableSlabComponent::OnDeactivated()
{
  Cleanup();

  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  if (WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>())
  {
    pModule->DeallocateUserData(m_uiUserDataIndexStatic);
    pModule->DeallocateUserData(m_uiUserDataIndexDynamic);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  SUPER::OnDeactivated();
}

WResult WJoltBreakableSlabComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = m_Bounds;
  return W_SUCCESS;
}

void WJoltBreakableSlabComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;
  m_bReinitMeshes = true;
}

float WJoltBreakableSlabComponent::GetWidth() const
{
  return m_fWidth;
}

void WJoltBreakableSlabComponent::SetHeight(float fHeight)
{
  if (fHeight <= 0.0f)
    return;

  m_fHeight = fHeight;
  m_bReinitMeshes = true;
}

float WJoltBreakableSlabComponent::GetHeight() const
{
  return m_fHeight;
}

void WJoltBreakableSlabComponent::SetThickness(float fThickness)
{
  if (fThickness <= 0.0f)
    return;

  m_fThickness = fThickness;
  m_bReinitMeshes = true;
}

float WJoltBreakableSlabComponent::GetThickness() const
{
  return m_fThickness;
}

void WJoltBreakableSlabComponent::SetUvScale(WVec2 vScale)
{
  if (m_vUvScale == vScale)
    return;

  m_vUvScale = vScale;
  m_bReinitMeshes = true;
}


WVec2 WJoltBreakableSlabComponent::GetUvScale() const
{
  return m_vUvScale;
}

void WJoltBreakableSlabComponent::SetFlags(WBitflags<WJoltBreakableSlabFlags> flags)
{
  m_Flags = flags;
  m_bReinitMeshes = true;
}

void WJoltBreakableSlabComponent::SetShape(WEnum<WJoltBreakableShape> shape)
{
  m_Shape = shape;
  m_bReinitMeshes = true;
}

void WJoltBreakableSlabComponent::Restore()
{
  Cleanup();
  ReinitMeshes();
  TriggerLocalBoundsUpdate();

  WTempHybridArray<JPH::Ref<JPH::ConvexShape>, 2> shapes;
  PrepareShardColliders(0, shapes);

  CreateShardColliders(0, shapes);
  m_uiShardsSleeping = 190;

  InvalidateCachedRenderData();
}

bool WJoltBreakableSlabComponent::IsPointOnSlab(const WVec3& vGlobalPosition) const
{
  const WPlane mainPlane = WPlane::MakeFromNormalAndPoint(GetOwner()->GetGlobalDirUp(), GetOwner()->GetGlobalPosition());
  const float fDist = WMath::Abs(mainPlane.GetDistanceTo(vGlobalPosition));
  if (fDist > WMath::Max(JPH::cDefaultConvexRadius * 2.0f, m_fThickness))
    return false;

  return true;
}

WUInt32 WJoltBreakableSlabComponent::FindClosestShard(const WVec3& vGlobalPosition) const
{
  const WVec2 vLocalPos = (GetOwner()->GetGlobalTransform().GetInverse() * vGlobalPosition).GetAsVec2();
  float fBestDistance = WMath::HighValue<float>();

  WUInt32 uiShardIdx = WInvalidIndex;

  const WUInt32 uiShards = m_Breakable.m_Shards.GetCount();
  for (WUInt32 i = 0; i < uiShards; ++i)
  {
    const auto& shard = m_Breakable.m_Shards[i];
    if (shard.m_bShattered || shard.m_bDynamic)
      continue;

    const float fDistSqr = (shard.m_vCenterPosition - vLocalPos).GetLengthSquared();

    if (fDistSqr > fBestDistance)
      continue;

    // if the shard is smaller than we are close to it, ignore it
    if (fDistSqr > WMath::Square(shard.m_fBoundingRadius))
      continue;

    fBestDistance = fDistSqr;
    uiShardIdx = i;
  }

  return uiShardIdx;
}

void WJoltBreakableSlabComponent::ShatterCellular(const WVec3& vGlobalPosition, float fCellSize, const WVec3& vImpulse, float fMakeDynamicRadius)
{
  if (!IsPointOnSlab(vGlobalPosition))
    return;

  const WUInt32 uiShardIdx = FindClosestShard(vGlobalPosition);
  if (uiShardIdx == WInvalidIndex)
    return;

  if (m_ShatterPoints.IsEmpty())
  {
    ((WJoltBreakableSlabComponentManager*)GetOwningManager())->m_RequireBreakage.Insert(GetHandle());
  }

  auto& pt = m_ShatterPoints.ExpandAndGetRef();
  pt.m_vGlobalPosition = vGlobalPosition;
  pt.m_vImpulse = vImpulse;
  pt.m_fCellSize = fCellSize;
  pt.m_uiShardIdx = uiShardIdx;
  pt.m_fMakeDynamicRadius = fMakeDynamicRadius;
  pt.m_uiAllowedBreakPatterns = (WUInt8)WBreakablePattern::Cellular;
}

void WJoltBreakableSlabComponent::ShatterRadial(const WVec3& vGlobalPosition, float fImpactRadius, const WVec3& vImpulse, float fMakeDynamicRadius)
{
  if (!IsPointOnSlab(vGlobalPosition))
    return;

  const WUInt32 uiShardIdx = FindClosestShard(vGlobalPosition);
  if (uiShardIdx == WInvalidIndex)
    return;

  if (m_ShatterPoints.IsEmpty())
  {
    ((WJoltBreakableSlabComponentManager*)GetOwningManager())->m_RequireBreakage.Insert(GetHandle());
  }

  auto& pt = m_ShatterPoints.ExpandAndGetRef();
  pt.m_vGlobalPosition = vGlobalPosition;
  pt.m_vImpulse = vImpulse;
  pt.m_fImpactRadius = fImpactRadius;
  pt.m_fCellSize = 0.4f;
  pt.m_uiShardIdx = uiShardIdx;
  pt.m_fMakeDynamicRadius = fMakeDynamicRadius;
  pt.m_uiAllowedBreakPatterns = (WUInt8)WBreakablePattern::Radial | (WUInt8)WBreakablePattern::Cellular;
}

void WJoltBreakableSlabComponent::ShatterAll(float fShardSize, const WVec3& vImpulse)
{
  m_fContactReportForceThreshold = 0.0f;

  if (m_ShatterPoints.IsEmpty())
  {
    ((WJoltBreakableSlabComponentManager*)GetOwningManager())->m_RequireBreakage.Insert(GetHandle());
  }

  auto& pt = m_ShatterPoints.ExpandAndGetRef();
  pt.m_vGlobalPosition.Set(1024, 1024, 1024);
  pt.m_vImpulse = vImpulse;
  pt.m_fCellSize = fShardSize;
  pt.m_uiShardIdx = WInvalidIndex;
}

void WJoltBreakableSlabComponent::PrepareBreakAsync(WDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes, WArrayPtr<const WShatterPoint> points)
{
  W_PROFILE_SCOPE("PrepareBreakAsync");


  for (WUInt32 i = 0; i < points.GetCount(); ++i)
  {
    const auto& point = points[i];

    if (point.m_vGlobalPosition == WVec3(1024, 1024, 1024))
    {
      // shatter all pieces
      m_Breakable.ShatterAll(point.m_fCellSize, GetWorld()->GetRandomNumberGenerator(), true);
      break;
    }
    else
    {

      if (point.m_uiShardIdx != WInvalidIndex && !m_Breakable.m_Shards[point.m_uiShardIdx].m_bShattered)
      {
        const WUInt32 uiShardIdxOffset = m_Breakable.m_Shards.GetCount();
        const WVec2 vLocalPos = (GetOwner()->GetGlobalTransform().GetInverse() * point.m_vGlobalPosition).GetAsVec2();

        m_Breakable.ShatterShard(point.m_uiShardIdx, vLocalPos, GetWorld()->GetRandomNumberGenerator(), WMath::Clamp(point.m_fImpactRadius, 0.05f, 0.4f), WMath::Clamp(point.m_fCellSize, 0.3f, 0.5f), point.m_uiAllowedBreakPatterns);

        const float fDistSqr = WMath::Square(point.m_fMakeDynamicRadius);

        for (WUInt32 idx = uiShardIdxOffset; idx < m_Breakable.m_Shards.GetCount(); ++idx)
        {
          // make everything dynamic that can't be broken further
          if (m_Breakable.m_Shards[idx].m_uiBreakablePatterns == (WUInt8)WBreakablePattern::None)
          {
            m_Breakable.m_Shards[idx].m_bDynamic = true;
          }
          else if ((m_Breakable.m_Shards[idx].m_vCenterPosition - vLocalPos).GetLengthSquared() <= fDistSqr)
          {
            // and everything that's close enough to the shatter point
            m_Breakable.m_Shards[idx].m_bDynamic = true;
          }
        }
      }
    }
  }


  m_Breakable.RecalculateDymamic();

  PrepareShardColliders(m_ShardBodyIDs.GetCount(), out_Shapes);
}

void WJoltBreakableSlabComponent::ApplyBreak(WArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes, const WMeshResourceHandle& hMesh, const WVec3& vImpulse)
{
  W_PROFILE_SCOPE("ApplyBreak");

  UpdateShardColliders();

  const WUInt32 uiFirstShard = m_ShardBodyIDs.GetCount();

  m_hMesh = hMesh;
  CreateShardColliders(uiFirstShard, shapes);

  ApplyImpulse(vImpulse, uiFirstShard);

  // make sure everyone around is awake
  WakeUpBodies();
}

void WJoltBreakableSlabComponent::Cleanup()
{
  if (m_pShatterTask)
  {
    WTaskSystem::CancelTask(m_pShatterTask, WOnTaskRunning::WaitTillFinished).IgnoreResult();
    m_pShatterTask = nullptr;
  }

  DestroyAllShardColliders();

  m_Breakable.Clear();
  m_ShatterPoints.Clear();

  m_hMesh.Invalidate();

  m_SkinningState.Clear();
}

void WJoltBreakableSlabComponent::ReinitMeshes()
{
  W_ASSERT_DEBUG(IsActive(), "Should only be called on active components.");

  m_bReinitMeshes = false;

  Cleanup();

  m_Breakable.Initialize();

  auto& shard = m_Breakable.m_Shards[0];

  if (m_Shape == WJoltBreakableShape::Rectangle)
  {
    shard.m_vCenterPosition.Set(m_fWidth * 0.5f, m_fHeight * 0.5f);
    shard.m_fBoundingRadius = WMath::Max(m_fWidth, m_fHeight) * 0.75f;
    shard.m_Edges.SetCount(4);

    shard.m_Edges[0].m_vStartPosition.Set(0, 0);
    if (m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeLeft))
      shard.m_Edges[0].m_uiOutsideShardIdx = WBreakableShard2D::FixedEdge;

    shard.m_Edges[1].m_vStartPosition.Set(0, m_fHeight);
    if (m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeTop))
      shard.m_Edges[1].m_uiOutsideShardIdx = WBreakableShard2D::FixedEdge;

    shard.m_Edges[2].m_vStartPosition.Set(m_fWidth, m_fHeight);
    if (m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeRight))
      shard.m_Edges[2].m_uiOutsideShardIdx = WBreakableShard2D::FixedEdge;

    shard.m_Edges[3].m_vStartPosition.Set(m_fWidth, 0);
    if (m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeBottom))
      shard.m_Edges[3].m_uiOutsideShardIdx = WBreakableShard2D::FixedEdge;
  }
  else if (m_Shape == WJoltBreakableShape::Triangle)
  {
    shard.m_vCenterPosition.Set(m_fWidth * 0.5f, m_fHeight * 0.5f);
    shard.m_fBoundingRadius = WMath::Max(m_fWidth, m_fHeight) * 0.75f;
    shard.m_Edges.SetCount(3);

    shard.m_Edges[0].m_vStartPosition.Set(0, 0);
    if (m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeLeft))
      shard.m_Edges[0].m_uiOutsideShardIdx = WBreakableShard2D::FixedEdge;

    shard.m_Edges[1].m_vStartPosition.Set(0, m_fHeight);
    if (m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeTop) || m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeRight))
      shard.m_Edges[1].m_uiOutsideShardIdx = WBreakableShard2D::FixedEdge;

    shard.m_Edges[2].m_vStartPosition.Set(m_fWidth, 0);
    if (m_Flags.IsSet(WJoltBreakableSlabFlags::FixedEdgeBottom))
      shard.m_Edges[2].m_uiOutsideShardIdx = WBreakableShard2D::FixedEdge;
  }
  else if (m_Shape == WJoltBreakableShape::Circle)
  {
    const WUInt32 num = 16;

    shard.m_vCenterPosition.Set(m_fWidth * 0.5f, m_fHeight * 0.5f);
    shard.m_fBoundingRadius = WMath::Max(m_fWidth, m_fHeight) * 0.75f;
    shard.m_Edges.SetCount(num);

    const float hw = m_fWidth * 0.5f;
    const float hh = m_fHeight * 0.5f;

    const WUInt32 uiOutsideIdx = m_Flags.IsAnySet(WJoltBreakableSlabFlags::Default) ? WBreakableShard2D::FixedEdge : WBreakableShard2D::LooseEdge;

    for (WUInt32 i = 0; i < num; ++i)
    {
      const WAngle a = WAngle::MakeFromDegree((360.0f / num) * i);

      shard.m_Edges[i].m_vStartPosition.Set(hw + WMath::Sin(a) * hw, hh + WMath::Cos(a) * hh);
      shard.m_Edges[i].m_uiOutsideShardIdx = uiOutsideIdx;
    }
  }

  m_hMesh = CreateShardsMesh();

  InvalidateCachedRenderData();

  m_Bounds = WBoundingBox::MakeFromMinMax(WVec3::MakeZero(), WVec3(m_fWidth, m_fHeight, m_fThickness));
}

void WJoltBreakableSlabComponent::BuildMeshResourceFromGeometry(WGeometry& Geometry, WMeshResourceDescriptor& MeshDesc, bool bWithSkinningData) const
{
  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddCommonStreams();

  if (bWithSkinningData)
  {
    MeshBufferDesc.AddStream(WMeshVertexStreamType::SkinningData);
  }
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, WGALPrimitiveTopology::Triangles);

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}

const WJoltMaterial* WJoltBreakableSlabComponent::GetPhysicsMaterial()
{
  if (m_hMaterial.IsValid())
  {
    if (!m_hSurface.IsValid())
    {
      WResourceLock<WMaterialResource> pMat(m_hMaterial, WResourceAcquireMode::BlockTillLoaded);

      if (!pMat->GetSurface().IsEmpty())
      {
        m_hSurface = WResourceManager::LoadResource<WSurfaceResource>(pMat->GetSurface());
      }
    }

    if (m_hSurface.IsValid())
    {
      WResourceLock<WSurfaceResource> pSurf(m_hSurface, WResourceAcquireMode::BlockTillLoaded);

      if (pSurf->m_pPhysicsMaterialJolt != nullptr)
      {
        return static_cast<WJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);
      }
    }
  }

  return WJoltCore::GetDefaultMaterial();
}

void AddSkirtPolygons(const WVec2& vPoint0, const WVec2& vPoint1, float fThickness, WGeometry& ref_geometry, const WGeometry::GeoOptions& opt)
{
  const float fSpanX = WMath::Abs(vPoint0.x - vPoint1.x);
  const float fSpanY = WMath::Abs(vPoint0.y - vPoint1.y);
  const float fSpan = WMath::Max(fSpanX, fSpanY);

  WVec3 vPoint0Front(vPoint0.x, vPoint0.y, fThickness);
  WVec2 vPoint0FrontUV(fThickness, 0);
  WVec3 vPoint0Back(vPoint0.x, vPoint0.y, 0);
  WVec2 vPoint0BackUV(0, 0);
  WVec3 vPoint1Front(vPoint1.x, vPoint1.y, fThickness);
  WVec2 vPoint1FrontUV(fThickness, fSpan);
  WVec3 vPoint1Back(vPoint1.x, vPoint1.y, 0);
  WVec2 vPoint1BackUV(0, fSpan);

  WVec3 FaceNormal;
  if (FaceNormal.CalculateNormal(vPoint0Front, vPoint1Front, vPoint0Back).Failed())
  {
    // ignore degenerate triangles
    return;
  }

  const WUInt32 uiIdx0 = ref_geometry.AddVertex(opt, vPoint0Front, FaceNormal, vPoint0FrontUV);
  const WUInt32 uiIdx1 = ref_geometry.AddVertex(opt, vPoint0Back, FaceNormal, vPoint0BackUV);
  const WUInt32 uiIdx2 = ref_geometry.AddVertex(opt, vPoint1Front, FaceNormal, vPoint1FrontUV);
  const WUInt32 uiIdx3 = ref_geometry.AddVertex(opt, vPoint1Back, FaceNormal, vPoint1BackUV);

  {
    WUInt32 idx[3] = {uiIdx0, uiIdx2, uiIdx1};
    ref_geometry.AddPolygon(idx, false);
  }

  {
    WUInt32 idx[3] = {uiIdx1, uiIdx2, uiIdx3};
    ref_geometry.AddPolygon(idx, false);
  }
}

WMeshResourceHandle WJoltBreakableSlabComponent::CreateShardsMesh() const
{
  W_PROFILE_SCOPE("CreateShardsMesh");

  WStringBuilder meshName;
  meshName.SetFormat("JoltSlab-{}-{}", WArgP(this), s_iShardMeshCounter.Increment());

  WGeometry geo;
  WTempHybridArray<WUInt32, 16> vtxIdx1;
  WTempHybridArray<WUInt32, 16> vtxIdx2;

  const WVec3 vNormal(0, 0, 1);

  const WVec3 vOffset = vNormal * m_fThickness;

  for (WUInt32 uiShardIdx = 0; uiShardIdx < m_Breakable.m_Shards.GetCount(); ++uiShardIdx)
  {
    const auto& shard = m_Breakable.m_Shards[uiShardIdx];
    vtxIdx1.Clear();
    vtxIdx2.Clear();

    WGeometry::GeoOptions opt;
    opt.m_uiBoneIndex = uiShardIdx;

    for (WUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      WVec3 v = shard.m_Edges[i].m_vStartPosition.GetAsVec3(0);

      WVec2 vTexCoord;
      vTexCoord.x = v.x / m_fWidth;
      vTexCoord.y = 1.0f - (v.y / m_fHeight);
      vTexCoord = vTexCoord.CompMul(m_vUvScale);

      v -= shard.m_vCenterPosition.GetAsVec3(0);

      vtxIdx1.PushBack(geo.AddVertex(opt, v, -vNormal, vTexCoord));
      vtxIdx2.PushBack(geo.AddVertex(opt, v + vOffset, vNormal, vTexCoord));
    }

    geo.AddPolygon(vtxIdx1, false);
    geo.AddPolygon(vtxIdx2, true);

    WUInt32 uiPrev = shard.m_Edges.GetCount() - 1;
    for (WUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      AddSkirtPolygons(shard.m_Edges[uiPrev].m_vStartPosition - shard.m_vCenterPosition, shard.m_Edges[i].m_vStartPosition - shard.m_vCenterPosition, m_fThickness, geo, opt);
      uiPrev = i;
    }
  }

  geo.TriangulatePolygons();
  geo.ComputeTangents();

  WMeshResourceDescriptor desc;
  BuildMeshResourceFromGeometry(geo, desc, true /* include skinning data */);

  return WResourceManager::CreateResource<WMeshResource>(meshName, std::move(desc));
}

void WJoltBreakableSlabComponent::PrepareShardColliders(WUInt32 uiFirstShard, WDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes) const
{
  W_PROFILE_SCOPE("PrepareShardColliders");

  out_Shapes.SetCount(m_Breakable.m_Shards.GetCount() - uiFirstShard);

  const float fShardThickness = WMath::Max(m_fThickness, JPH::cDefaultConvexRadius * 2.0f);
  const float fThick1 = -(fShardThickness - m_fThickness) * 0.5f;
  const float fThick2 = fShardThickness + fThick1;

  for (WUInt32 uiShardIdx = 0; uiShardIdx < out_Shapes.GetCount(); ++uiShardIdx)
  {
    const auto& shard = m_Breakable.m_Shards[uiFirstShard + uiShardIdx];

    if (shard.m_bShattered)
      continue;

    JPH::Array<JPH::Vec3> points;
    points.reserve(shard.m_Edges.GetCount() * 2);

    for (WUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      const WVec2 v = shard.m_Edges[i].m_vStartPosition - shard.m_vCenterPosition;
      points.push_back(JPH::Vec3(v.x, v.y, fThick1));
      points.push_back(JPH::Vec3(v.x, v.y, fThick2));
    }

    JPH::ConvexHullShapeSettings shapeSettings(points);
    shapeSettings.mDensity = 0.1f;
    shapeSettings.mUserData = 0;

    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError())
    {
      // WLog::Warning("Invalid shard shape: {} - {} points", shapeResult.GetError().c_str(), points.size());
      continue;
    }

    out_Shapes[uiShardIdx] = (JPH::ConvexShape*)(shapeResult.Get().GetPtr());
  }
}

void WJoltBreakableSlabComponent::UpdateShardColliders()
{
  W_PROFILE_SCOPE("UpdateShardColliders");

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto& jphBodies = pModule->GetBodyInterface();

  for (WUInt32 uiShardIdx = 0; uiShardIdx < m_ShardBodyIDs.GetCount(); ++uiShardIdx)
  {
    const auto& shard = m_Breakable.m_Shards[uiShardIdx];

    if (m_ShardBodyIDs[uiShardIdx] == WInvalidIndex)
      continue;

    if (shard.m_bShattered)
    {
      // remove shard colliders that are now destroyed
      DestroyShardCollider(uiShardIdx, jphBodies, true);
      continue;
    }

    if (!shard.m_bDynamic)
      continue;

    JPH::BodyID bodyId(m_ShardBodyIDs[uiShardIdx]);
    bool bRemoveBody = false;

    {
      JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

      if (bodyLock.Succeeded())
      {
        JPH::Body& body = bodyLock.GetBody();
        if (body.GetMotionType() == JPH::EMotionType::Static)
        {
          bRemoveBody = true;

          JPH::BodyCreationSettings bodyCfg;
          bodyCfg.SetShape(body.GetShape());
          bodyCfg.mRestitution = body.GetRestitution();
          bodyCfg.mFriction = body.GetFriction();
          bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
          bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
          bodyCfg.mUserData = body.GetUserData();
          bodyCfg.mMassPropertiesOverride.mMass = 1.0f;
          bodyCfg.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
          bodyCfg.mMotionType = JPH::EMotionType::Dynamic;
          bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayerDynamic, WJoltBroadphaseLayer::Debris);
          bodyCfg.mPosition = body.GetPosition();
          bodyCfg.mRotation = body.GetRotation();
          bodyCfg.mGravityFactor = m_fGravityFactor;

          // Create and add body
          JPH::Body* pBody = jphBodies.CreateBody(bodyCfg);
          pModule->QueueBodyToAdd(pBody, true);

          m_ShardBodyIDs[uiShardIdx] = pBody->GetID().GetIndexAndSequenceNumber();
        }
      }
    }

    if (bRemoveBody)
    {
      // can only do this outside the lock
      if (jphBodies.IsAdded(bodyId))
      {
        jphBodies.RemoveBody(bodyId);
      }

      jphBodies.DestroyBody(bodyId);
    }
  }
}

void WJoltBreakableSlabComponent::WakeUpBodies()
{
  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  auto& jphBodies = pModule->GetBodyInterface();

  WJoltObjectLayerFilter objectFilter(m_uiCollisionLayerStatic);

  WUInt32 broadphase = WPhysicsShapeType::Default;
  broadphase &= ~WPhysicsShapeType::Static;
  broadphase &= ~WPhysicsShapeType::Character;
  broadphase &= ~WPhysicsShapeType::Query;
  broadphase &= ~WPhysicsShapeType::Trigger;
  WJoltBroadPhaseLayerFilter broadphaseFilter(static_cast<WPhysicsShapeType::Enum>(broadphase));

  WBoundingBox bbox;
  bbox.m_vMin.SetZero();
  bbox.m_vMax = WVec3(m_fWidth, m_fHeight, m_fThickness);
  bbox.Grow(WVec3(0.3f));

  bbox.TransformFromOrigin(GetOwner()->GetGlobalTransform().GetAsMat4());

  const JPH::AABox box = JPH::AABox::sFromTwoPoints(WJoltConversionUtils::ToVec3(bbox.m_vMin), WJoltConversionUtils::ToVec3(bbox.m_vMax));
  jphBodies.ActivateBodiesInAABox(box, broadphaseFilter, objectFilter);
}

void WJoltBreakableSlabComponent::CreateShardColliders(WUInt32 uiFirstShard, WArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes)
{
  W_PROFILE_SCOPE("CreateShardColliders");

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  auto& jphBodies = pModule->GetBodyInterface();

  const WJoltUserData* pUserDataStatic = &pModule->GetUserData(m_uiUserDataIndexStatic);
  const WJoltUserData* pUserDataDynamic = &pModule->GetUserData(m_uiUserDataIndexDynamic);
  const WJoltMaterial* pMaterial = GetPhysicsMaterial();

  // GetOrCreateBoneTransformsForWriting will resize as necessary but will lose existing data so we need to make a copy here
  WTempArray<WShaderTransform> oldTransforms;
  oldTransforms = m_SkinningState.GetBoneTransformsForReading();

  auto transforms = m_SkinningState.GetOrCreateBoneTransformsForWriting(*this, m_Breakable.m_Shards.GetCount());
  WMemoryUtils::Copy(transforms.GetPtr(), oldTransforms.GetData(), oldTransforms.GetCount());

  m_ShardBodyIDs.SetCount(m_Breakable.m_Shards.GetCount(), WInvalidIndex);

  for (WUInt32 uiShardIdx = uiFirstShard; uiShardIdx < m_Breakable.m_Shards.GetCount(); ++uiShardIdx)
  {
    transforms[uiShardIdx] = WMat4::MakeScaling(WVec3(0));

    const auto& shard = m_Breakable.m_Shards[uiShardIdx];

    if (shard.m_bShattered)
      continue;

    JPH::ConvexShape* pShape = shapes[uiShardIdx - uiFirstShard];
    if (pShape == nullptr)
      continue;

    pShape->SetMaterial(pMaterial);

    // Body settings
    JPH::BodyCreationSettings bodyCfg;
    bodyCfg.SetShape(pShape);
    // bodyCfg.mMotionQuality = JPH::EMotionQuality::LinearCast; // don't use CCD, shards are allowed to tunnel through geometry, they will be destroyed, if necessary
    bodyCfg.mRestitution = pMaterial->m_fRestitution;
    bodyCfg.mFriction = pMaterial->m_fFriction;
    bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
    bodyCfg.mMassPropertiesOverride.mMass = 1.0f;
    bodyCfg.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    bodyCfg.mGravityFactor = m_fGravityFactor;

    if (shard.m_bDynamic == false)
    {
      bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserDataStatic);
      bodyCfg.mMotionType = JPH::EMotionType::Static;
      bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayerStatic, WJoltBroadphaseLayer::Static);
    }
    else
    {
      bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserDataDynamic);
      bodyCfg.mMotionType = JPH::EMotionType::Dynamic;
      bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayerDynamic, WJoltBroadphaseLayer::Debris);
    }

    // Set transform to owner
    WTransform trans = GetOwner()->GetGlobalTransform();
    trans.m_vPosition += trans.m_qRotation * shard.m_vCenterPosition.GetAsVec3(0);

    bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_vPosition);
    bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_qRotation).Normalized();

    transforms[uiShardIdx] = trans;

    // Create and add body
    JPH::Body* pBody = jphBodies.CreateBody(bodyCfg);
    pModule->QueueBodyToAdd(pBody, shard.m_bDynamic);

    m_ShardBodyIDs[uiShardIdx] = pBody->GetID().GetIndexAndSequenceNumber();
  }
}

void WJoltBreakableSlabComponent::DestroyAllShardColliders()
{
  if (m_ShardBodyIDs.IsEmpty())
    return;

  if (WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>())
  {
    auto& jphBodies = pModule->GetBodyInterface();

    for (WUInt32 uiShardIdx = 0; uiShardIdx < m_ShardBodyIDs.GetCount(); ++uiShardIdx)
    {
      DestroyShardCollider(uiShardIdx, jphBodies, false);
    }
  }

  m_ShardBodyIDs.Clear();
}

void WJoltBreakableSlabComponent::DestroyShardCollider(WUInt32 uiShardIdx, JPH::BodyInterface& jphBodies, bool bUpdateVis)
{
  JPH::BodyID bodyId(m_ShardBodyIDs[uiShardIdx]);
  m_ShardBodyIDs[uiShardIdx] = WInvalidIndex;

  if (bUpdateVis)
  {
    auto transforms = m_SkinningState.GetOrCreateBoneTransformsForWriting(*this, m_SkinningState.m_uiNumBones);
    transforms[uiShardIdx] = WMat4::MakeScaling(WVec3(0)); // make it disappear by scaling to zero
  }

  if (bodyId.IsInvalid())
    return;

  if (jphBodies.IsAdded(bodyId))
  {
    jphBodies.RemoveBody(bodyId);
  }

  jphBodies.DestroyBody(bodyId);
}

void WJoltBreakableSlabComponent::RetrieveShardTransforms()
{
  if (m_uiShardsSleeping >= 200)
  {
    // invalidate the cache, because at this point it is set to 'static'
    InvalidateCachedRenderData();
  }

  m_uiShardsSleeping = 0;

  WBoundingBox bbox = WBoundingBox::MakeInvalid();

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  auto& jphBodies = pModule->GetBodyInterface();

  const WVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  auto transforms = m_SkinningState.GetOrCreateBoneTransformsForWriting(*this, m_SkinningState.m_uiNumBones);

  for (WUInt32 idx = 0; idx < m_ShardBodyIDs.GetCount(); ++idx)
  {
    JPH::BodyID bodyId(m_ShardBodyIDs[idx]);

    if (bodyId.IsInvalid())
      continue;

    JPH::RVec3 pos;
    JPH::Quat rot;
    jphBodies.GetPositionAndRotation(bodyId, pos, rot);

    transforms[idx] = WJoltConversionUtils::ToTransform(pos, rot);

    const WVec3 vPos = WJoltConversionUtils::ToVec3(pos);

    if ((vPos - vOwnPos).GetLengthSquared() > WMath::Square(30.0f))
    {
      // if the shard moved pretty far away from the center, it probably tunneled through geometry and just keeps falling
      // we don't want to use continuous collision detection (CCD), because this is just low-priority eye-candy
      // so simply remove shards that appear problematic
      DestroyShardCollider(idx, jphBodies, true);
      continue;
    }

    bbox.ExpandToInclude(vPos);
  }

  if (bbox.IsValid())
  {
    const WTransform tInv = GetOwner()->GetGlobalTransform().GetInverse();

    bbox.TransformFromOrigin(tInv.GetAsMat4());
    bbox.Grow(WVec3(m_Breakable.m_fMaxRadius));

    m_Bounds = bbox;
    TriggerLocalBoundsUpdate();
  }
}

void WJoltBreakableSlabComponent::ApplyImpulse(const WVec3& vImpulse, WUInt32 uiFirstShard)
{
  const float fTorque = vImpulse.GetLength();
  if (fTorque <= 0.01f)
    return;

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  const WVec3 vTorques[] = {
    WVec3::MakeAxisX(),
    -WVec3::MakeAxisX(),
    WVec3::MakeAxisY(),
    -WVec3::MakeAxisY(),
    WVec3::MakeAxisZ(),
    -WVec3::MakeAxisZ(),
    WVec3(0.7f, 0.7f, 0.0f),
    WVec3(0.7f, -0.7f, 0.0f),
    WVec3(-0.7f, 0.7f, 0.0f),
    WVec3(-0.7f, -0.7f, 0.0f),
    WVec3(0.7f, 0, 0.7f),
    WVec3(0.7f, 0, -0.7f),
    WVec3(-0.7f, 0, -0.7f),
    WVec3(-0.7f, 0, 0.7f),
    WVec3(0, 0.7f, 0.7f),
    WVec3(0, -0.7f, 0.7f),
  };

  const WUInt32 uiNumTorques = W_ARRAY_SIZE(vTorques);

  for (WUInt32 idx = uiFirstShard; idx < m_Breakable.m_Shards.GetCount(); ++idx)
  {
    const auto& shard = m_Breakable.m_Shards[idx];

    if (shard.m_bDynamic)
    {
      const WUInt32 uiBodyID = m_ShardBodyIDs[idx];

      if (uiBodyID != WInvalidIndex)
      {
        pModule->AddImpulse(uiBodyID, vImpulse);
        pModule->AddTorque(uiBodyID, vTorques[idx % uiNumTorques] * fTorque);
      }
    }
  }
}

void WJoltBreakableSlabComponent::OnMsgPhysicsAddImpulse(WMsgPhysicsAddImpulse& ref_msg)
{
  // not implemented atm
}

void WJoltBreakableSlabComponent::OnMsgPhysicContactMsg(WMsgPhysicContact& ref_msg)
{
  if (m_fContactReportForceThreshold <= 0.0f)
    return;

  if (ref_msg.m_fImpactSqr < WMath::Square(m_fContactReportForceThreshold))
    return;

  // let the script decide what to do
  GetOwner()->PostEventMessage(ref_msg, this, WTime::MakeZero(), WObjectMsgQueueType::PostTransform);
}

void WJoltBreakableSlabComponent::OnMsgPhysicCharacterContact(WMsgPhysicCharacterContact& ref_msg)
{
  if (m_fContactReportForceThreshold <= 0.0f)
    return;

  if (ref_msg.m_fImpact < m_fContactReportForceThreshold)
    return;

  // let the script decide what to do
  GetOwner()->PostEventMessage(ref_msg, this, WTime::MakeZero(), WObjectMsgQueueType::PostTransform);
}

void WJoltBreakableSlabComponent::OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& ref_msg)
{
  m_SkinningState.m_DataOffset = ref_msg.m_NewOffset;
}

void WJoltBreakableSlabComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (m_hMesh.IsValid())
  {
    WMeshRenderData* pRenderData = nullptr;
    WTransform globalTransform;

    const bool bHasSkinning = m_SkinningState.HasBoneTransforms();
    if (bHasSkinning)
    {
      auto pSkinnedRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WSkinnedMeshRenderData>(GetOwner());
      pSkinnedRenderData->m_DataOffsets.m_uiSkinning = m_SkinningState.m_DataOffset.m_uiOffset;
      pSkinnedRenderData->m_hSkinningBuffer = msg.m_pRenderDataManager->GetSkinningDataBuffer();
      pRenderData = pSkinnedRenderData;

      globalTransform = WTransform::MakeIdentity();
    }
    else
    {
      pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(GetOwner());

      globalTransform = GetOwner()->GetGlobalTransform();
      globalTransform.m_vPosition += globalTransform.m_qRotation * WVec3(m_fWidth * 0.5f, m_fHeight * 0.5f, 0);
    }

    const bool bDynamic = GetOwner()->IsDynamic();
    auto hInstanceBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, globalTransform, m_InstanceDataOffset, GetUniqueIdForRendering());

    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceBuffer, m_hMaterial, m_hMesh);

    WRenderData::Category category = WMaterialResource::GetRenderDataCategory(m_hMaterial);

    if (bHasSkinning)
    {
      if (m_uiShardsSleeping >= 200)
      {
        msg.AddRenderData(pRenderData, category, WRenderData::Caching::IfStatic);
      }
      else
      {
        ++m_uiShardsSleeping;
        msg.AddRenderData(pRenderData, category, WRenderData::Caching::Never);
      }
    }
    else
    {
      msg.AddRenderData(pRenderData, category, WRenderData::Caching::IfStatic);
    }
  }
}

void WJoltBreakableSlabComponent::DebugDraw()
{
  const WTransform trans = GetOwner()->GetGlobalTransform();

  WDynamicArray<WDebugRendererLine> lines;

  const WColor edgeColor = WColor::White;
  const WColor centerColor = WColor::Yellow;

  for (const auto& shard : m_Breakable.m_Shards)
  {
    if (shard.m_bShattered)
      continue;

    auto& center = lines.ExpandAndGetRef();
    center.m_start = shard.m_vCenterPosition.GetAsVec3(0);
    center.m_end = center.m_start + WVec3(0, 0, 0.1f);
    center.m_startColor = centerColor;
    center.m_endColor = centerColor;

    const WColor c = shard.m_bDynamic ? WColor::GreenYellow : WColor::OrangeRed;


    WUInt32 uiPrevEdge = shard.m_Edges.GetCount() - 1;
    for (WUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {

      auto& l = lines.ExpandAndGetRef();
      l.m_start = shard.m_Edges[uiPrevEdge].m_vStartPosition.GetAsVec3(0);
      l.m_end = shard.m_Edges[i].m_vStartPosition.GetAsVec3(0);

      const WUInt32 uiOut = shard.m_Edges[uiPrevEdge].m_uiOutsideShardIdx;
      if (uiOut > WInvalidIndex - 32 && uiOut < WInvalidIndex)
      {
        l.m_startColor = edgeColor;
        l.m_endColor = edgeColor;
      }
      else
      {
        l.m_startColor = c;
        l.m_endColor = c;
      }

      uiPrevEdge = i;
    }
  }

  WDebugRenderer::DrawLines(GetWorld(), lines, WColor::White, trans);

  {
    const WVec3 vCenter = GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalRotation() * (WVec3(m_fWidth * 0.5f, m_fHeight * 0.2f, 0));

    WStringBuilder txt;
    txt.AppendFormat("Shards: {}\n", m_Breakable.m_Shards.GetCount());

    if (m_uiShardsSleeping >= 200)
      txt.Append("Sleeping\n");
    else
      txt.Append("Moving\n");

    WDebugRenderer::Draw3DText(GetWorld(), txt, vCenter, WColor::LightGray);
  }
}

void WShatterTask::Execute()
{
  m_vFinalImpulse.SetZero();

  if (!m_ShatterPoints.IsEmpty())
  {
    for (const auto& pt : m_ShatterPoints)
    {
      m_vFinalImpulse += pt.m_vImpulse;
    }

    m_vFinalImpulse /= (float)m_ShatterPoints.GetCount();
  }

  m_pComponent->PrepareBreakAsync(m_Shapes, m_ShatterPoints);
  m_hShardsMesh = m_pComponent->CreateShardsMesh();
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltBreakableSlabComponent);
