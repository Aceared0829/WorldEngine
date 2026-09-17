#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Types/TagRegistry.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltHeightfieldColliderComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Components/JoltRagdollComponent.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>
#include <JoltPlugin/Components/JoltWaterVolumeComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/Constraints/JoltFixedConstraintComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Collision/CollisionCollectorImpl.h>
#include <Physics/Collision/Shape/Shape.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WJoltWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WCVarBool cvar_JoltSimulationPause("Jolt.Simulation.Pause", false, WCVarFlags::None, "Pauses the physics simulation.");

#ifdef JPH_DEBUG_RENDERER
WCVarBool cvar_JoltDebugDrawConstraints("Jolt.DebugDraw.Constraints", false, WCVarFlags::None, "Visualize physics constraints.");
WCVarBool cvar_JoltDebugDrawConstraintLimits("Jolt.DebugDraw.ConstraintLimits", false, WCVarFlags::None, "Visualize physics constraint limits.");
WCVarBool cvar_JoltDebugDrawConstraintFrames("Jolt.DebugDraw.ConstraintFrames", false, WCVarFlags::None, "Visualize physics constraint frames.");
WCVarBool cvar_JoltDebugDrawBodies("Jolt.DebugDraw.Bodies", false, WCVarFlags::None, "Visualize physics bodies.");
#endif

WCVarBool cvar_JoltVisualizeGeometry("Jolt.Visualize.Geometry", false, WCVarFlags::None, "Renders collision geometry, color coded by shape type.");
WCVarBool cvar_JoltVisualizeSurfaces("Jolt.Visualize.Surfaces", false, WCVarFlags::None, "Renders collision geometry, color coded by the debug color of the assigned surface. Takes precedence over 'Jolt.Visualize.Geometry'.");
WCVarBool cvar_JoltVisualizeGeometryExclusive("Jolt.Visualize.Exclusive", false, WCVarFlags::Save, "Hides regularly rendered geometry.");
WCVarFloat cvar_JoltVisualizeDistance("Jolt.Visualize.Distance", 30.0f, WCVarFlags::Save, "How far away objects to visualize.");

WJoltWorldModule::WJoltWorldModule(WWorld* pWorld)
  : WPhysicsWorldModuleInterface(pWorld)
//, m_FreeObjectFilterIDs(WJolt::GetSingleton()->GetAllocator()) // could use a proxy allocator to bin those
{
  m_pSimulateTask = W_DEFAULT_NEW(WDelegateTask<void>, "Jolt::Simulate", WTaskNesting::Never, WMakeDelegate(&WJoltWorldModule::Simulate, this));
  m_pSimulateTask->ConfigureTask("Jolt Simulate", WTaskNesting::Maybe);
}

WJoltWorldModule::~WJoltWorldModule() = default;

class WJoltBodyActivationListener : public JPH::BodyActivationListener
{
public:
  virtual void OnBodyActivated(const JPH::BodyID& bodyID, JPH::uint64 inBodyUserData) override
  {
    const WJoltUserData* pUserData = reinterpret_cast<const WJoltUserData*>(inBodyUserData);

    switch (WJoltUserData::GetType(pUserData))
    {
      case WJoltUserData::Type::DynamicActorComponent:
        m_pActiveActors->Insert(static_cast<WJoltDynamicActorComponent*>(pUserData->GetObject()));
        return;

      case WJoltUserData::Type::RagdollComponent:
        (*m_pActiveRagdolls)[static_cast<WJoltRagdollComponent*>(pUserData->GetObject())]++;
        return;

      case WJoltUserData::Type::RopeComponent:
        (*m_pActiveRopes)[static_cast<WJoltRopeComponent*>(pUserData->GetObject())]++;
        return;

      case WJoltUserData::Type::BreakableSlabComponent:
        (*m_pActiveSlabs)[static_cast<WJoltBreakableSlabComponent*>(pUserData->GetObject())]++;
        return;

      default:
        return;
    }
  }

  virtual void OnBodyDeactivated(const JPH::BodyID& bodyID, JPH::uint64 inBodyUserData) override
  {
    const WJoltUserData* pUserData = reinterpret_cast<const WJoltUserData*>(inBodyUserData);

    switch (WJoltUserData::GetType(pUserData))
    {
      case WJoltUserData::Type::DynamicActorComponent:
      {
        m_pActiveActors->Remove(static_cast<WJoltDynamicActorComponent*>(pUserData->GetObject()));

        return;
      }

      case WJoltUserData::Type::RagdollComponent:
      {
        WJoltRagdollComponent* pActor = static_cast<WJoltRagdollComponent*>(pUserData->GetObject());
        if (--(*m_pActiveRagdolls)[pActor] == 0)
        {
          m_pActiveRagdolls->Remove(pActor);
          m_pRagdollsPutToSleep->PushBack(pActor);
        }

        return;
      }

      case WJoltUserData::Type::RopeComponent:
      {
        WJoltRopeComponent* pActor = static_cast<WJoltRopeComponent*>(pUserData->GetObject());
        if (--(*m_pActiveRopes)[pActor] == 0)
        {
          m_pActiveRopes->Remove(pActor);
        }

        return;
      }

      case WJoltUserData::Type::BreakableSlabComponent:
      {
        WJoltBreakableSlabComponent* pActor = static_cast<WJoltBreakableSlabComponent*>(pUserData->GetObject());
        if (--(*m_pActiveSlabs)[pActor] == 0)
        {
          m_pActiveSlabs->Remove(pActor);
          m_pSlabsPutToSleep->PushBack(pActor);
        }

        return;
      }

      default:
        return;
    }
  }

  WSet<WJoltDynamicActorComponent*>* m_pActiveActors = nullptr;
  WMap<WJoltRopeComponent*, WInt32>* m_pActiveRopes = nullptr;          // value is a ref-count
  WMap<WJoltRagdollComponent*, WInt32>* m_pActiveRagdolls = nullptr;    // value is a ref-count
  WDynamicArray<WJoltRagdollComponent*>* m_pRagdollsPutToSleep = nullptr;
  WMap<WJoltBreakableSlabComponent*, WInt32>* m_pActiveSlabs = nullptr; // value is a ref-count
  WDynamicArray<WJoltBreakableSlabComponent*>* m_pSlabsPutToSleep = nullptr;
};

class WJoltGroupFilter : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
  {
    const WUInt64 id = static_cast<WUInt64>(group1.GetGroupID()) << 32 | group2.GetGroupID();

    return !m_IgnoreCollisions.Contains(id);
  }

  WHashSet<WUInt64> m_IgnoreCollisions;
};

class WJoltGroupFilterIgnoreSame : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
  {
    return group1.GetGroupID() != group2.GetGroupID();
  }
};

void WJoltWorldModule::Deinitialize()
{
  m_RuntimeHeightfieldIDs.Clear();

  m_pSystem = nullptr;
  m_pTempAllocator = nullptr;

  WJoltBodyActivationListener* pActivationListener = reinterpret_cast<WJoltBodyActivationListener*>(m_pActivationListener);
  W_DEFAULT_DELETE(pActivationListener);
  m_pActivationListener = nullptr;

  WJoltContactListener* pContactListener = reinterpret_cast<WJoltContactListener*>(m_pContactListener);
  W_DEFAULT_DELETE(pContactListener);
  m_pContactListener = nullptr;

  WJoltSoftBodyContactListener* pSoftBodyContactListener = reinterpret_cast<WJoltSoftBodyContactListener*>(m_pSoftBodyContactListener);
  W_DEFAULT_DELETE(pSoftBodyContactListener);
  m_pSoftBodyContactListener = nullptr;

  m_pGroupFilter->Release();
  m_pGroupFilter = nullptr;

  m_pGroupFilterIgnoreSame->Release();
  m_pGroupFilterIgnoreSame = nullptr;
}

class WJoltTempAlloc : public JPH::TempAllocator
{
public:
  WJoltTempAlloc(const char* szName)
    : m_ProxyAlloc(szName, WFoundation::GetAlignedAllocator())
  {
    AddChunk(0);
    m_uiCurChunkIdx = 0;
  }

  ~WJoltTempAlloc()
  {
    for (WUInt32 i = 0; i < m_Chunks.GetCount(); ++i)
    {
      ClearChunk(i);
    }
  }

  virtual void* Allocate(JPH::uint inSize) override
  {
    if (inSize == 0)
      return nullptr;

    const WUInt32 uiNeeded = WMemoryUtils::AlignSize(inSize, 16u);

    while (true)
    {
      const WUInt32 uiRemaining = m_Chunks[m_uiCurChunkIdx].m_uiSize - m_Chunks[m_uiCurChunkIdx].m_uiLastOffset;

      if (uiRemaining >= uiNeeded)
        break;

      AddChunk(uiNeeded);
    }

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];

    void* pRes = WMemoryUtils::AddByteOffset(lastAlloc.m_pPtr, lastAlloc.m_uiLastOffset);
    lastAlloc.m_uiLastOffset += uiNeeded;
    return pRes;
  }

  virtual void Free(void* pInAddress, JPH::uint inSize) override
  {
    if (pInAddress == nullptr)
      return;

    const WUInt32 uiAllocSize = WMemoryUtils::AlignSize(inSize, 16u);

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];
    lastAlloc.m_uiLastOffset -= uiAllocSize;

    if (lastAlloc.m_uiLastOffset == 0 && m_uiCurChunkIdx > 0)
    {
      // move back to the previous chunk
      --m_uiCurChunkIdx;
    }
  }

  struct Chunk
  {
    void* m_pPtr = nullptr;
    WUInt32 m_uiSize = 0;
    WUInt32 m_uiLastOffset = 0;
  };

  void AddChunk(WUInt32 uiSize)
  {
    ++m_uiCurChunkIdx;

    if (m_uiCurChunkIdx < m_Chunks.GetCount())
      return;

    uiSize = WMath::Max(uiSize, 1024u * 1024u);

    auto& alloc = m_Chunks.ExpandAndGetRef();
    alloc.m_pPtr = W_NEW_RAW_BUFFER(&m_ProxyAlloc, WUInt8, uiSize);
    alloc.m_uiSize = uiSize;
  }

  void ClearChunk(WUInt32 uiChunkIdx)
  {
    W_DELETE_RAW_BUFFER(&m_ProxyAlloc, m_Chunks[uiChunkIdx].m_pPtr);
    m_Chunks[uiChunkIdx].m_pPtr = nullptr;
    m_Chunks[uiChunkIdx].m_uiSize = 0;
    m_Chunks[uiChunkIdx].m_uiLastOffset = 0;
  }

  WUInt32 m_uiCurChunkIdx = 0;
  WHybridArray<Chunk, 16> m_Chunks;
  WProxyAllocator m_ProxyAlloc;
};


void WJoltWorldModule::Initialize()
{
  // TODO: it would be better if this were in OnSimulationStarted() to guarantee that the system is always initialized with the latest values
  // however, that doesn't work because WJoltWorldModule is only created by calls to GetOrCreateWorldModule, where Initialize is called, but OnSimulationStarted
  // is queued and executed later

  // ensure the first element is reserved for 'invalid' objects
  m_AllocatedUserData.SetCount(1);

  UpdateSettingsCfg();

  WStringBuilder tmp("Jolt-", GetWorld()->GetName());
  m_pTempAllocator = std::make_unique<WJoltTempAlloc>(tmp);

  const uint32_t cMaxBodies = m_Settings.m_uiMaxBodies;
  const uint32_t cMaxContactConstraints = m_Settings.m_uiMaxBodies * 4;
  const uint32_t cMaxBodyPairs = cMaxContactConstraints * 10;
  const uint32_t cNumBodyMutexes = 0;

  m_pSystem = std::make_unique<JPH::PhysicsSystem>();
  m_pSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_ObjectToBroadphase, m_ObjectVsBroadphaseFilter, m_ObjectLayerPairFilter);

  {
    WJoltBodyActivationListener* pListener = W_DEFAULT_NEW(WJoltBodyActivationListener);
    m_pActivationListener = pListener;
    pListener->m_pActiveActors = &m_ActiveActors;
    pListener->m_pActiveRopes = &m_ActiveRopes;
    pListener->m_pActiveRagdolls = &m_ActiveRagdolls;
    pListener->m_pRagdollsPutToSleep = &m_RagdollsPutToSleep;
    pListener->m_pActiveSlabs = &m_ActiveSlabs;
    pListener->m_pSlabsPutToSleep = &m_SlabsPutToSleep;
    m_pSystem->SetBodyActivationListener(pListener);
  }

  {
    WJoltContactListener* pListener = W_DEFAULT_NEW(WJoltContactListener);
    pListener->m_pWorld = GetWorld();
    m_pContactListener = pListener;
    m_pSystem->SetContactListener(pListener);
  }

  {
    WJoltSoftBodyContactListener* pListener = W_DEFAULT_NEW(WJoltSoftBodyContactListener);
    m_pSoftBodyContactListener = pListener;
    m_pSystem->SetSoftBodyContactListener(pListener);
  }

  {
    m_pGroupFilter = new WJoltGroupFilter();
    m_pGroupFilter->AddRef();
  }

  {
    m_pGroupFilterIgnoreSame = new WJoltGroupFilterIgnoreSame();
    m_pGroupFilterIgnoreSame->AddRef();
  }
}

void WJoltWorldModule::OnSimulationStarted()
{
  {
    auto startSimDesc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltWorldModule::StartSimulation, this);
    startSimDesc.m_Phase = WWorldUpdatePhase::PreAsync;
    startSimDesc.m_bOnlyUpdateWhenSimulating = true;
    // Start physics simulation as late as possible in the first synchronous phase
    // so all kinematic objects have a chance to update their transform before.
    startSimDesc.m_fPriority = -100000.0f;

    RegisterUpdateFunction(startSimDesc);
  }

  {
    auto fetchResultsDesc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WJoltWorldModule::FetchResults, this);
    fetchResultsDesc.m_Phase = WWorldUpdatePhase::PostAsync;
    fetchResultsDesc.m_bOnlyUpdateWhenSimulating = true;
    // Fetch results as early as possible after async phase.
    fetchResultsDesc.m_fPriority = 100000.0f;

    RegisterUpdateFunction(fetchResultsDesc);
  }

  WJoltCore::ReloadConfigs();

  UpdateSettingsCfg();
  ApplySettingsCfg();

  m_AccumulatedTimeSinceUpdate = WTime::MakeZero();
}

WUInt32 WJoltWorldModule::CreateObjectFilterID()
{
  if (!m_FreeObjectFilterIDs.IsEmpty())
  {
    WUInt32 uiObjectFilterID = m_FreeObjectFilterIDs.PeekBack();
    m_FreeObjectFilterIDs.PopBack();

    return uiObjectFilterID;
  }

  return m_uiNextObjectFilterID++;
}

void WJoltWorldModule::DeleteObjectFilterID(WUInt32& ref_uiObjectFilterID)
{
  if (ref_uiObjectFilterID == WInvalidIndex)
    return;

  m_FreeObjectFilterIDs.PushBack(ref_uiObjectFilterID);

  ref_uiObjectFilterID = WInvalidIndex;
}

WUInt32 WJoltWorldModule::AllocateUserData(WJoltUserData*& out_pUserData)
{
  if (!m_FreeUserData.IsEmpty())
  {
    WUInt32 uiIndex = m_FreeUserData.PeekBack();
    m_FreeUserData.PopBack();

    out_pUserData = &m_AllocatedUserData[uiIndex];
    return uiIndex;
  }

  out_pUserData = &m_AllocatedUserData.ExpandAndGetRef();
  return m_AllocatedUserData.GetCount() - 1;
}

void WJoltWorldModule::DeallocateUserData(WUInt32& ref_uiUserDataId)
{
  if (ref_uiUserDataId == WInvalidIndex)
    return;

  m_AllocatedUserData[ref_uiUserDataId].Invalidate();

  m_FreeUserDataAfterSimulationStep.PushBack(ref_uiUserDataId);

  ref_uiUserDataId = WInvalidIndex;
}

const WJoltUserData& WJoltWorldModule::GetUserData(WUInt32 uiUserDataId) const
{
  W_ASSERT_DEBUG(uiUserDataId != WInvalidIndex, "Invalid WJoltUserData ID");

  return m_AllocatedUserData[uiUserDataId];
}

void WJoltWorldModule::SetGravity(const WVec3& vObjectGravity, const WVec3& vCharacterGravity)
{
  m_Settings.m_vObjectGravity = vObjectGravity;
  m_Settings.m_vCharacterGravity = vCharacterGravity;

  if (m_pSystem)
  {
    m_pSystem->SetGravity(WJoltConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

WJoltForceId WJoltWorldModule::AddOrUpdateForce(WJoltForceId forceId, WUInt32 uiBodyID, WTime duration, const WVec3& vForce)
{
  W_LOCK(m_ForcesMutex);

  WJoltForce* pForce;
  if (m_Forces.TryGetValue(forceId, pForce))
  {
    pForce->m_tDisable = GetWorld()->GetClock().GetAccumulatedTime() + duration;
    pForce->m_vForce = vForce;
    return forceId;
  }
  else
  {
    WJoltForce force;
    force.m_uiBodyID = uiBodyID;
    force.m_tDisable = GetWorld()->GetClock().GetAccumulatedTime() + duration;
    force.m_vForce = vForce;

    return m_Forces.Insert(force);
  }
}

void WJoltWorldModule::ClearForce(WJoltForceId id)
{
  W_LOCK(m_ForcesMutex);
  m_Forces.Remove(id);
}

void WJoltWorldModule::UpdateForces()
{
  if (m_Forces.IsEmpty())
    return;

  W_LOCK(m_ForcesMutex);

  WTempHybridArray<WJoltForceId, 32> forcesToRemove;

  auto* pBodies = &m_pSystem->GetBodyInterface();
  const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  for (auto it = m_Forces.GetIterator(); it.IsValid(); ++it)
  {
    WJoltForce& force = it.Value();

    if (force.m_tDisable < tNow)
    {
      forcesToRemove.PushBack(it.Id());
      continue;
    }

    const JPH::BodyID bodyId(force.m_uiBodyID);

    if (!pBodies->IsAdded(bodyId))
      continue;

    pBodies->AddForce(bodyId, WJoltConversionUtils::ToVec3(force.m_vForce));
  }

  for (WJoltForceId id : forcesToRemove)
  {
    m_Forces.Remove(id);
  }
}

WUInt32 WJoltWorldModule::GetCollisionLayerByName(WStringView sName) const
{
  return WJoltCore::GetCollisionFilterConfig().GetFilterGroupByName(sName);
}

WUInt8 WJoltWorldModule::GetWeightCategoryByName(WStringView sName) const
{
  return WJoltCore::GetWeightCategoryConfig().FindByName(WTempHashedString(sName));
}

WUInt8 WJoltWorldModule::GetImpulseTypeByName(WStringView sName) const
{
  return WJoltCore::GetImpulseTypeConfig().FindByName(WTempHashedString(sName));
}

void WJoltWorldModule::AddStaticCollisionBox(WGameObject* pObject, WVec3 vBoxSize)
{
  WJoltStaticActorComponent* pActor = nullptr;
  WJoltStaticActorComponent::CreateComponent(pObject, pActor);

  WJoltShapeBoxComponent* pBox;
  WJoltShapeBoxComponent::CreateComponent(pObject, pBox);
  pBox->SetHalfExtents(vBoxSize * 0.5f);
}

void WJoltWorldModule::AddFixedJointComponent(WGameObject* pOwner, const WPhysicsWorldModuleInterface::FixedJointConfig& cfg)
{
  WJoltFixedConstraintComponent* pConstraint = nullptr;
  m_pWorld->GetOrCreateComponentManager<WJoltFixedConstraintComponentManager>()->CreateComponent(pOwner, pConstraint);
  pConstraint->SetActors(cfg.m_hActorA, cfg.m_LocalFrameA, cfg.m_hActorB, cfg.m_LocalFrameB);
}

WBoundingBoxSphere WJoltWorldModule::GetWorldSpaceBounds(WGameObject* pOwner, WUInt32 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const
{
  WBoundingBoxSphere result = WBoundingBoxSphere::MakeInvalid();

  WJoltActorComponent* pActor = nullptr;
  if (pOwner->TryGetComponentOfBaseType(pActor))
  {
    WUInt32 uiBodyID = pActor->GetJoltBodyID();
    auto& lockInterface = m_pSystem->GetBodyLockInterfaceNoLock();
    JPH::BodyLockRead bodyLock(lockInterface, JPH::BodyID(uiBodyID));
    if (bodyLock.Succeeded())
    {
      const auto& body = bodyLock.GetBody();

      if ((shapeTypes.GetValue() & W_BIT(body.GetBroadPhaseLayer().GetValue())) != 0 && WJoltObjectLayerFilter(uiCollisionLayer).ShouldCollide(body.GetObjectLayer()))
      {
        const auto& aabb = body.GetWorldSpaceBounds();
        result = WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(WJoltConversionUtils::ToVec3(aabb.mMin), WJoltConversionUtils::ToVec3(aabb.mMax)));
      }
    }
  }

  if (bIncludeChildObjects)
  {
    for (auto it = pOwner->GetChildren(); it.IsValid(); it.Next())
    {
      WBoundingBoxSphere childBounds = GetWorldSpaceBounds(it, uiCollisionLayer, shapeTypes, bIncludeChildObjects);
      if (!childBounds.IsValid())
        continue;

      if (result.IsValid())
      {
        result.ExpandToInclude(childBounds);
      }
      else
      {
        result = childBounds;
      }
    }
  }

  return result;
}

void WJoltWorldModule::QueueBodyToAdd(JPH::Body* pBody, bool bAwake)
{
  if (bAwake)
    m_BodiesToAddAndActivate.PushBack(pBody->GetID().GetIndexAndSequenceNumber());
  else
    m_BodiesToAdd.PushBack(pBody->GetID().GetIndexAndSequenceNumber());
}

void WJoltWorldModule::RemoveBodyFromQueue(JPH::BodyID bodyId)
{
  const WUInt32 uiBodyID = bodyId.GetIndexAndSequenceNumber();
  bool bRemoved = m_BodiesToAdd.RemoveAndSwap(uiBodyID);
  bRemoved |= m_BodiesToAddAndActivate.RemoveAndSwap(uiBodyID);
  W_ASSERT_DEV(bRemoved, "Body was not in add queue");
}

void WJoltWorldModule::EnableJoinedBodiesCollisions(WUInt32 uiObjectFilterID1, WUInt32 uiObjectFilterID2, bool bEnable)
{
  WJoltGroupFilter* pFilter = static_cast<WJoltGroupFilter*>(m_pGroupFilter);

  const WUInt64 uiMask1 = static_cast<WUInt64>(uiObjectFilterID1) << 32 | uiObjectFilterID2;
  const WUInt64 uiMask2 = static_cast<WUInt64>(uiObjectFilterID2) << 32 | uiObjectFilterID1;

  if (bEnable)
  {
    pFilter->m_IgnoreCollisions.Remove(uiMask1);
    pFilter->m_IgnoreCollisions.Remove(uiMask2);
  }
  else
  {
    pFilter->m_IgnoreCollisions.Insert(uiMask1);
    pFilter->m_IgnoreCollisions.Insert(uiMask2);
  }
}

void WJoltWorldModule::ActivateCharacterController(WJoltCharacterControllerComponent* pCharacter, bool bActivate)
{
  if (bActivate)
  {
    W_ASSERT_DEBUG(!m_ActiveCharacters.Contains(pCharacter), "WJoltCharacterControllerComponent was activated more than once.");

    m_ActiveCharacters.PushBack(pCharacter);
  }
  else
  {
    if (!m_ActiveCharacters.RemoveAndSwap(pCharacter))
    {
      W_ASSERT_DEBUG(false, "WJoltCharacterControllerComponent was deactivated more than once.");
    }
  }
}

void WJoltWorldModule::CheckBreakableConstraints()
{
  WWorld* pWorld = GetWorld();

  for (auto it = m_BreakableConstraints.GetIterator(); it.IsValid();)
  {
    WJoltConstraintComponent* pConstraint;
    if (pWorld->TryGetComponent(*it, pConstraint) && pConstraint->IsActive())
    {
      if (pConstraint->ExceededBreakingPoint())
      {
        // notify interested parties, that this constraint is now broken
        WMsgPhysicsJointBroke msg;
        msg.m_hJointObject = pConstraint->GetOwner()->GetHandle();
        pConstraint->GetOwner()->SendEventMessage(msg, pConstraint);

        // currently we don't track the broken state separately, we just remove the component
        pConstraint->DeleteComponent();
        it = m_BreakableConstraints.Remove(it);
      }
      else
      {
        ++it;
      }
    }
    else
    {
      it = m_BreakableConstraints.Remove(it);
    }
  }
}

void WJoltWorldModule::FreeUserDataAfterSimulationStep()
{
  m_FreeUserData.PushBackRange(m_FreeUserDataAfterSimulationStep);
  m_FreeUserDataAfterSimulationStep.Clear();
}

void WJoltWorldModule::StartSimulation(const WWorldModule::UpdateContext& context)
{
  if (cvar_JoltSimulationPause)
    return;

  if (!m_BodiesToAdd.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAdd.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(WUInt32));

    WUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAdd.GetCount())
    {
      const WUInt32 uiCount = m_BodiesToAdd.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAdd[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::DontActivate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAdd.Clear();
  }

  if (!m_BodiesToAddAndActivate.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAddAndActivate.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(WUInt32));

    WUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAddAndActivate.GetCount())
    {
      const WUInt32 uiCount = m_BodiesToAddAndActivate.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAddAndActivate[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::Activate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAddAndActivate.Clear();
  }

  if (m_uiBodiesAddedSinceOptimize > 128)
  {
    // TODO: not clear whether this could be multi-threaded or done more efficiently somehow
    m_pSystem->OptimizeBroadPhase();
    m_uiBodiesAddedSinceOptimize = 0;
  }

  UpdateSettingsCfg();

  m_SimulatedTimeStep = CalculateUpdateSteps();

  if (m_UpdateSteps.IsEmpty())
    return;

  if (WJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<WJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateKinematicActors(m_SimulatedTimeStep);
  }

  if (WJoltQueryShapeActorComponentManager* pQueryShapesManager = GetWorld()->GetComponentManager<WJoltQueryShapeActorComponentManager>())
  {
    pQueryShapesManager->UpdateMovingQueryShapes();
  }

  if (WJoltTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<WJoltTriggerComponentManager>())
  {
    pTriggerManager->UpdateMovingTriggers();
  }

  if (WJoltRagdollComponentManager* pRagdollManager = GetWorld()->GetComponentManager<WJoltRagdollComponentManager>())
  {
    pRagdollManager->DriveAnimatedRagdolls(m_SimulatedTimeStep);
  }

  if (WJoltWaterVolumeComponentManager* pWaterVolumeManager = GetWorld()->GetComponentManager<WJoltWaterVolumeComponentManager>())
  {
    pWaterVolumeManager->UpdateWaterVolumes(m_SimulatedTimeStep);
  }

  ApplyImpulses();
  UpdateForces();
  UpdateConstraints();

  m_SimulateTaskGroupId = WTaskSystem::StartSingleTask(m_pSimulateTask, WTaskPriority::EarlyThisFrame);
}

void WJoltWorldModule::FetchResults(const WWorldModule::UpdateContext& context)
{
  W_PROFILE_SCOPE("WJoltWorldModule::FetchResults");

  {
    W_PROFILE_SCOPE("Wait for Simulate Task");
    WTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  }

#ifdef JPH_DEBUG_RENDERER
  if (cvar_JoltDebugDrawConstraints)
    m_pSystem->DrawConstraints(WJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintLimits)
    m_pSystem->DrawConstraintLimits(WJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintFrames)
    m_pSystem->DrawConstraintReferenceFrame(WJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawBodies)
  {
    JPH::BodyManager::DrawSettings opt;
    opt.mDrawShape = true;
    opt.mDrawShapeWireframe = true;
    m_pSystem->DrawBodies(opt, WJoltCore::s_pDebugRenderer.get());
  }

  WJoltCore::DebugDraw(GetWorld());
#endif

  // Nothing to fetch if no simulation step was executed
  if (m_UpdateSteps.IsEmpty())
    return;

  ++m_uiJoltUpdateCounter;

  if (WJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<WJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateDynamicActors();
  }

  if (WJoltStaticActorComponentManager* pStaticActorManager = GetWorld()->GetComponentManager<WJoltStaticActorComponentManager>())
  {
    pStaticActorManager->UpdateTemporarilyDynamicActors();
  }

  for (auto pCharacter : m_ActiveCharacters)
  {
    pCharacter->Update(m_SimulatedTimeStep);
  }

  if (WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView, GetWorld()))
  {
    reinterpret_cast<WJoltContactListener*>(m_pContactListener)->m_ContactEvents.m_vMainCameraPosition = pView->GetCamera()->GetPosition();
  }

  reinterpret_cast<WJoltContactListener*>(m_pContactListener)->m_ContactEvents.SpawnPhysicsImpactReactions();
  reinterpret_cast<WJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsSlideReactions();
  reinterpret_cast<WJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsRollReactions();

  CheckBreakableConstraints();

  FreeUserDataAfterSimulationStep();

  DebugDrawGeometry();
}

WTime WJoltWorldModule::CalculateUpdateSteps()
{
  WTime tSimulatedTimeStep = WTime::MakeZero();
  m_AccumulatedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();
  m_UpdateSteps.Clear();

  if (m_Settings.m_SteppingMode == WJoltSteppingMode::Variable)
  {
    // always do a single step with the entire time
    m_UpdateSteps.PushBack(m_AccumulatedTimeSinceUpdate);

    tSimulatedTimeStep = m_AccumulatedTimeSinceUpdate;
    m_AccumulatedTimeSinceUpdate = WTime::MakeZero();
  }
  else if (m_Settings.m_SteppingMode == WJoltSteppingMode::Fixed)
  {
    const WTime tFixedStep = WTime::MakeFromSeconds(1.0 / m_Settings.m_fFixedFrameRate);

    WUInt32 uiNumSubSteps = 0;

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep && uiNumSubSteps < m_Settings.m_uiMaxSubSteps)
    {
      m_UpdateSteps.PushBack(tFixedStep);
      ++uiNumSubSteps;

      tSimulatedTimeStep += tFixedStep;
      m_AccumulatedTimeSinceUpdate -= tFixedStep;
    }
  }
  else if (m_Settings.m_SteppingMode == WJoltSteppingMode::SemiFixed)
  {
    WTime tFixedStep = WTime::MakeFromSeconds(1.0 / m_Settings.m_fFixedFrameRate);
    const WTime tMinStep = tFixedStep * 0.25;

    if (tFixedStep * m_Settings.m_uiMaxSubSteps < m_AccumulatedTimeSinceUpdate) // in case too much time has passed
    {
      // if taking N steps isn't sufficient to catch up to the passed time, increase the fixed time step accordingly
      tFixedStep = m_AccumulatedTimeSinceUpdate / (double)m_Settings.m_uiMaxSubSteps;
    }

    while (m_AccumulatedTimeSinceUpdate > tMinStep)
    {
      // prefer fixed time steps
      // but if at the end there is still more than tMinStep time left, do another step with the remaining time
      const WTime tDeltaTime = WMath::Min(tFixedStep, m_AccumulatedTimeSinceUpdate);

      m_UpdateSteps.PushBack(tDeltaTime);

      tSimulatedTimeStep += tDeltaTime;
      m_AccumulatedTimeSinceUpdate -= tDeltaTime;
    }
  }

  return tSimulatedTimeStep;
}

void WJoltWorldModule::Simulate()
{
  if (m_UpdateSteps.IsEmpty())
    return;

  W_PROFILE_SCOPE("WJoltWorldModule::Simulate");

  WTime tDelta = m_UpdateSteps[0];
  WUInt32 uiSteps = 1;

  m_RagdollsPutToSleep.Clear();
  m_SlabsPutToSleep.Clear();

  for (WUInt32 i = 1; i < m_UpdateSteps.GetCount(); ++i)
  {
    W_PROFILE_SCOPE("Physics Sim Step");

    if (m_UpdateSteps[i] == tDelta)
    {
      ++uiSteps;
    }
    else
    {
      // do a single Update call with multiple sub-steps, if possible
      // this saves a bit of time compared to just doing multiple Update calls

      m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, m_pTempAllocator.get(), WJoltCore::GetJoltJobSystem());

      tDelta = m_UpdateSteps[i];
      uiSteps = 1;
    }
  }

  m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, m_pTempAllocator.get(), WJoltCore::GetJoltJobSystem());
}

void WJoltWorldModule::UpdateSettingsCfg()
{
  if (WJoltSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<WJoltSettingsComponentManager>())
  {
    WJoltSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();

    if (pSettings != nullptr && pSettings->IsModified())
    {
      m_Settings = pSettings->GetSettings();
      pSettings->ResetModified();

      ApplySettingsCfg();
    }
  }
}

void WJoltWorldModule::ApplySettingsCfg()
{
  SetGravity(m_Settings.m_vObjectGravity, m_Settings.m_vCharacterGravity);

  if (m_pSystem)
  {
    auto physicsSettings = m_pSystem->GetPhysicsSettings();
    physicsSettings.mPointVelocitySleepThreshold = m_Settings.m_fSleepVelocityThreshold;

    m_pSystem->SetPhysicsSettings(physicsSettings);
  }
}

void WJoltWorldModule::UpdateConstraints()
{
  if (m_RequireUpdate.IsEmpty())
    return;

  WJoltConstraintComponent* pComponent;
  for (auto& hComponent : m_RequireUpdate)
  {
    if (this->m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      pComponent->ApplySettings();
    }
  }

  m_RequireUpdate.Clear();
}

WAtomicInteger32 s_iColMeshVisGeoCounter;

struct DebugVis
{
  const char* m_szMaterial = nullptr;
  WColor m_Color;
};

const char* szMatSolid = "{ e6367876-ddb5-4149-ba80-180af553d463 }";       // Data/Base/Materials/Common/PhysicsColliders.WMaterialAsset
const char* szMatTransparent = "{ ca43dda3-a28c-41fe-ae20-419182e56f87 }"; // Data/Base/Materials/Common/PhysicsCollidersTransparent.WMaterialAsset
const char* szMatTwoSided = "{ b03df0e4-98b7-49ba-8413-d981014a77be }";    // Data/Base/Materials/Common/PhysicsCollidersSoft.WMaterialAsset

static const DebugVis s_Vis[WPhysicsShapeType::Count][2] =
  {
    // Static
    {
      {szMatSolid, WColor::LightSkyBlue}, // non-kinematic
      {szMatSolid, WColor::Red}           // kinematic
    },

    // Dynamic
    {
      {szMatSolid, WColor::Gold},      // non-kinematic
      {szMatSolid, WColor::DodgerBlue} // kinematic
    },

    // Query
    {
      {szMatTransparent, WColor::GreenYellow.WithAlpha(0.5f)}, // non-kinematic
      {szMatTransparent, WColor::GreenYellow.WithAlpha(0.5f)}  // kinematic
    },

    // Trigger
    {
      {szMatTransparent, WColor::Purple.WithAlpha(0.3f)}, // non-kinematic
      {szMatTransparent, WColor::Purple.WithAlpha(0.3f)}  // kinematic
    },

    // Character
    {
      {szMatTransparent, WColor::DarkTurquoise.WithAlpha(0.5f)}, // non-kinematic
      {szMatTransparent, WColor::DarkTurquoise.WithAlpha(0.5f)}  // kinematic
    },

    // Ragdoll
    {
      {szMatSolid, WColor::DeepPink}, // non-kinematic
      {szMatSolid, WColor::DeepPink}  // kinematic
    },

    // Rope
    {
      {szMatSolid, WColor::MediumVioletRed}, // non-kinematic
      {szMatSolid, WColor::MediumVioletRed}  // kinematic
    },

    // Cloth
    {
      {szMatTwoSided, WColor::Crimson}, // non-kinematic
      {szMatTwoSided, WColor::Red}      // kinematic
    },

    // Debris
    {
      {szMatSolid, WColor::Crimson}, // non-kinematic
      {szMatSolid, WColor::Crimson}  // kinematic
    },
};

void WJoltWorldModule::DebugDrawGeometry()
{
  WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView, GetWorld());

  if (pView == nullptr)
    return;

  ++m_uiDebugGeoLastSeenCounter;

  const WTag& tag = WTagRegistry::GetGlobalRegistry().RegisterTag("PhysicsCollider");

  // both visualizations draw the same geometry, so only one of them is active at a time
  const bool bSurfaceColors = cvar_JoltVisualizeSurfaces;
  const bool bVisualize = cvar_JoltVisualizeGeometry || cvar_JoltVisualizeSurfaces;

  if (bVisualize && cvar_JoltVisualizeGeometryExclusive)
  {
    // deactivate other geometry rendering
    pView->m_IncludeTags.Set(tag);
  }
  else
  {
    pView->m_IncludeTags.Remove(tag);
  }

  if (bSurfaceColors != m_bDebugGeoSurfaceColors)
  {
    // the colors are baked into the render components, so switching between the two visualizations has to rebuild them
    m_bDebugGeoSurfaceColors = bSurfaceColors;

    for (auto it : m_DebugDrawComponents)
    {
      GetWorld()->DeleteObjectDelayed(it.Value().m_hObject);
    }

    m_DebugDrawComponents.Clear();
    m_DebugDrawShapeGeo.Clear();
  }

  if (bVisualize)
  {
    const WVec3 vCenterPos = pView->GetCamera()->GetCenterPosition();

    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Static, tag, bSurfaceColors);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Dynamic, tag, bSurfaceColors);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Query, tag, bSurfaceColors);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Ragdoll, tag, bSurfaceColors);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Trigger, tag, bSurfaceColors);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Rope, tag, bSurfaceColors);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Cloth, tag, bSurfaceColors);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, WPhysicsShapeType::Debris, tag, bSurfaceColors);
  }

  for (auto it = m_DebugDrawComponents.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiLastSeenCounter < m_uiDebugGeoLastSeenCounter)
    {
      GetWorld()->DeleteObjectDelayed(it.Value().m_hObject);
      it = m_DebugDrawComponents.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  for (auto it = m_DebugDrawShapeGeo.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiLastSeenCounter < m_uiDebugGeoLastSeenCounter)
    {
      it = m_DebugDrawShapeGeo.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void WJoltWorldModule::DebugDrawGeometry(const WVec3& vCenter, float fRadius, WPhysicsShapeType::Enum shapeType, const WTag& tag, bool bSurfaceColors)
{
  const WVec3 vAabbMin = vCenter - WVec3(fRadius);
  const WVec3 vAabbMax = vCenter + WVec3(fRadius);

  JPH::AABox aabb;
  aabb.mMin = WJoltConversionUtils::ToVec3(vAabbMin);
  aabb.mMax = WJoltConversionUtils::ToVec3(vAabbMax);

  JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;

  WJoltBroadPhaseLayerFilter broadphaseFilter(shapeType);
  JPH::ObjectLayerFilter objectFilterAll;
  JPH::BodyFilter bodyFilterAll;

  m_pSystem->GetNarrowPhaseQuery().CollectTransformedShapes(aabb, collector, broadphaseFilter, objectFilterAll, bodyFilterAll);

  auto* pBodies = &m_pSystem->GetBodyInterface();

  const int cMaxTriangles = 128;


  WStaticArray<WVec3, cMaxTriangles * 3> positionsTmp;
  positionsTmp.SetCountUninitialized(cMaxTriangles * 3);

  WStaticArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp;
  materialsTmp.SetCountUninitialized(cMaxTriangles);

  WTempHybridArray<WVec3, cMaxTriangles * 3> positionsTmp2;
  WTempHybridArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp2;
  WTempHybridArray<const JPH::PhysicsMaterial*, 8> distinctMaterials;
  WTempHybridArray<WVec3, cMaxTriangles * 3> partPositions;

  for (const JPH::TransformedShape& ts : collector.mHits)
  {
    DebugBodyShapeKey key;
    key.m_uiBodyID = ts.mBodyID.GetIndexAndSequenceNumber();
    key.m_uiSubShapeID = ts.mSubShapeIDCreator.GetID().GetValue();
    key.m_pShapePtr = ts.mShape.GetPtr();

    bool bExisted = false;
    auto& geo = m_DebugDrawComponents.FindOrAdd(key, &bExisted).Value();
    geo.m_uiLastSeenCounter = m_uiDebugGeoLastSeenCounter;

    DebugGeoShape& shapeGeo = m_DebugDrawShapeGeo[key.m_pShapePtr];
    shapeGeo.m_uiLastSeenCounter = m_uiDebugGeoLastSeenCounter;

    WTransform objTrans;
    objTrans.m_vPosition = WJoltConversionUtils::ToVec3(ts.mShapePositionCOM);
    objTrans.m_qRotation = WJoltConversionUtils::ToQuat(ts.mShapeRotation);
    objTrans.m_vScale = WJoltConversionUtils::ToVec3(ts.mShapeScale);

    if (bExisted)
    {
      WGameObject* pObj;
      if (GetWorld()->TryGetObject(geo.m_hObject, pObj))
      {
        pObj->SetGlobalTransform(objTrans);
      }

      if (!geo.m_bMutableGeometry)
        continue;
    }

    JPH::BodyLockRead lock(m_pSystem->GetBodyLockInterface(), ts.mBodyID);
    if (!lock.Succeeded())
      continue;

    positionsTmp2.Clear();
    materialsTmp2.Clear();

    if (shapeGeo.m_Parts.IsEmpty() || (geo.m_bMutableGeometry && lock.GetBody().IsActive()))
    {
      shapeGeo.m_Bounds = WBoundingBox::MakeInvalid();

      JPH::Shape::GetTrianglesContext ctx;
      ts.mShape->GetTrianglesStart(ctx, JPH::AABox::sBiggest(), JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::Vec3::sReplicate(1.0f));

      while (true)
      {
        const int triCount = ts.mShape->GetTrianglesNext(ctx, cMaxTriangles, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), materialsTmp.GetData());

        if (triCount == 0)
          break;

        positionsTmp2.PushBackRange(positionsTmp.GetArrayPtr().GetSubArray(0, triCount * 3));
        materialsTmp2.PushBackRange(materialsTmp.GetArrayPtr().GetSubArray(0, triCount));
      }

      if (positionsTmp2.GetCount() >= 3)
      {
        distinctMaterials.Clear();
        for (const JPH::PhysicsMaterial* pMaterial : materialsTmp2)
        {
          if (!distinctMaterials.Contains(pMaterial))
            distinctMaterials.PushBack(pMaterial);
        }

        shapeGeo.m_Parts.SetCount(distinctMaterials.GetCount());

        for (WUInt32 uiPart = 0; uiPart < distinctMaterials.GetCount(); ++uiPart)
        {
          const JPH::PhysicsMaterial* pPartMaterial = distinctMaterials[uiPart];

          partPositions.Clear();
          for (WUInt32 uiTriIdx = 0; uiTriIdx < materialsTmp2.GetCount(); ++uiTriIdx)
          {
            if (materialsTmp2[uiTriIdx] == pPartMaterial)
            {
              partPositions.PushBackRange(positionsTmp2.GetArrayPtr().GetSubArray(uiTriIdx * 3, 3));
            }
          }

          auto& part = shapeGeo.m_Parts[uiPart];

          // every material that Jolt hands out is an WJoltMaterial, because WJoltCore also replaces Jolt's default material
          part.m_SurfaceColor = pPartMaterial ? static_cast<const WJoltMaterial*>(pPartMaterial)->m_DebugColor : WColorGammaUB(WColor::White);

          if (!part.m_hMesh.IsValid())
          {
            WDynamicMeshBufferResourceDescriptor desc;
            desc.m_Topology = WGALPrimitiveTopology::Triangles;
            desc.m_uiMaxVertices = partPositions.GetCount();
            desc.m_uiMaxPrimitives = partPositions.GetCount() / 3;
            desc.m_IndexType = WGALIndexType::None;
            desc.m_bColorStream = false;

            WStringBuilder sGuid;
            sGuid.SetFormat("ColMeshVisGeo_{}", s_iColMeshVisGeoCounter.Increment());

            part.m_hMesh = WResourceManager::CreateResource<WDynamicMeshBufferResource>(sGuid, std::move(desc));
          }

          WResourceLock<WDynamicMeshBufferResource> pMeshBuf(part.m_hMesh, WResourceAcquireMode::BlockTillLoaded);

          auto positionData = pMeshBuf->AccessPositionData();
          auto nttData = pMeshBuf->AccessNormalTangentTexCoord0Data();

          // for mutable geometry the mesh is reused, so it may not have the same size as the new data
          const WUInt32 uiNumVertices = WMath::Min(positionData.GetCount(), partPositions.GetCount());

          for (WUInt32 vtxIdx = 0; vtxIdx < uiNumVertices; ++vtxIdx)
          {
            const auto& pos = partPositions[vtxIdx];
            positionData[vtxIdx] = pos;

            auto& vtx = nttData[vtxIdx];
            vtx.m_vTexCoord.SetZero();
            vtx.m_vEncodedNormal.SetZero();
            vtx.m_vEncodedTangent.SetZero();

            shapeGeo.m_Bounds.ExpandToInclude(pos);
          }
        }
      }
    }

    if (bExisted)
      continue;

    WGameObjectDesc gd;
    gd.m_bDynamic = true;
    gd.m_LocalPosition = objTrans.m_vPosition;
    gd.m_LocalRotation = objTrans.m_qRotation;
    gd.m_LocalScaling = objTrans.m_vScale;

    gd.m_Tags.Set(tag);
    WGameObject* pObj;
    geo.m_hObject = GetWorld()->CreateObject(gd, pObj);
    geo.m_bMutableGeometry = lock.GetBody().IsSoftBody();

    const bool bKinematic = (lock.GetBody().GetMotionType() == JPH::EMotionType::Kinematic);
    const auto& vis = s_Vis[WMath::FirstBitLow((WUInt32)shapeType)][bKinematic ? 1 : 0];

    for (const auto& part : shapeGeo.m_Parts)
    {
      WCustomMeshComponent* pMesh;
      WCustomMeshComponent::CreateComponent(pObj, pMesh);

      pMesh->SetMeshResource(part.m_hMesh);
      pMesh->SetBounds(shapeGeo.m_Bounds);
      pMesh->SetMaterialFile(vis.m_szMaterial);

      // the alpha of the shape type color is kept either way, so that for example triggers stay transparent
      pMesh->SetColor(bSurfaceColors ? WColor(part.m_SurfaceColor).WithAlpha(vis.m_Color.a) : vis.m_Color);
    }
  }
}

void WJoltWorldModule::AddImpulse(WUInt32 uiBodyID, const WVec3& vImpulse)
{
  W_LOCK(m_ImpulsesMutex);
  auto& imp = m_Impulses.ExpandAndGetRef();
  imp.m_uiBodyID = uiBodyID;
  imp.m_vImpulse = vImpulse;
  imp.m_Type = WJoltImpulse::Type::Center;
}

void WJoltWorldModule::AddTorque(WUInt32 uiBodyID, const WVec3& vImpulse)
{
  W_LOCK(m_ImpulsesMutex);
  auto& imp = m_Impulses.ExpandAndGetRef();
  imp.m_uiBodyID = uiBodyID;
  imp.m_vImpulse = vImpulse;
  imp.m_Type = WJoltImpulse::Type::Angular;
}

void WJoltWorldModule::AddImpulse(WUInt32 uiBodyID, const WVec3& vImpulse, const WVec3& vGlobalPosition)
{
  W_LOCK(m_ImpulsesMutex);
  auto& imp = m_Impulses.ExpandAndGetRef();
  imp.m_uiBodyID = uiBodyID;
  imp.m_vImpulse = vImpulse;
  imp.m_vGlobalPosition = vGlobalPosition;
  imp.m_Type = WJoltImpulse::Type::AtGlobalPos;
}

void WJoltWorldModule::ApplyImpulses()
{
  if (m_Impulses.IsEmpty())
    return;

  auto* pBodies = &m_pSystem->GetBodyInterface();
  WTempHybridArray<WJoltImpulse, 64> retain;

  W_LOCK(m_ImpulsesMutex);

  for (WUInt32 i = 0; i < m_Impulses.GetCount(); ++i)
  {
    auto& imp = m_Impulses[i];

    const JPH::BodyID bodyId(imp.m_uiBodyID);

    if (bodyId.IsInvalid())
      continue;

    if (!pBodies->IsAdded(bodyId))
    {
      retain.PushBack(imp);
      continue;
    }

    switch (imp.m_Type)
    {
      case WJoltImpulse::Type::AtGlobalPos:
        pBodies->AddImpulse(bodyId, WJoltConversionUtils::ToVec3(imp.m_vImpulse), WJoltConversionUtils::ToVec3(imp.m_vGlobalPosition));
        break;
      case WJoltImpulse::Type::Center:
        pBodies->AddImpulse(bodyId, WJoltConversionUtils::ToVec3(imp.m_vImpulse));
        break;
      case WJoltImpulse::Type::Angular:
        pBodies->AddTorque(bodyId, WJoltConversionUtils::ToVec3(imp.m_vImpulse));
        break;
    }
  }

  m_Impulses.Clear();

  for (auto& imp : retain)
  {
    m_Impulses.PushBack(imp);
  }
}

//////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WJoltNavmeshGeoWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltNavmeshGeoWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WJoltNavmeshGeoWorldModule::WJoltNavmeshGeoWorldModule(WWorld* pWorld)
  : WNavmeshGeoWorldModuleInterface(pWorld)
{
  m_pJoltModule = pWorld->GetOrCreateModule<WJoltWorldModule>();
}

void WJoltNavmeshGeoWorldModule::RetrieveGeometryInArea(WUInt32 uiCollisionLayer, const WBoundingBox& box, WDynamicArray<WNavmeshTriangle>& out_triangles) const
{
  const WPhysicsQueryParameters params(uiCollisionLayer, WPhysicsShapeType::Static);
  m_pJoltModule->QueryGeometryInBox(params, box, out_triangles);
}

void WJoltWorldModule::AttachHeightfieldBody(WJoltWorldModule* pModule, WGameObject* pOwner, const WJoltHeightfieldResourceHandle& hRes)
{
  WJoltHeightfieldColliderComponent* pExisting = nullptr;
  if (pOwner->TryGetComponentOfBaseType(pExisting))
    pExisting->DeleteComponent();

  WJoltHeightfieldColliderComponent* pComp = nullptr;
  WJoltHeightfieldColliderComponent::CreateComponent(pOwner, pComp);
  pComp->m_hHeightfield = hRes;
}

WResult WJoltWorldModule::TrySetHeightfieldCollider(WGameObject* pOwner, WStringView sIdentifier)
{
  if (!m_RuntimeHeightfieldIDs.Contains(sIdentifier))
    return W_FAILURE;

  WStringBuilder sResourceID("[rt-hf]:", sIdentifier);
  WJoltHeightfieldResourceHandle hRes = WResourceManager::LoadResource<WJoltHeightfieldResource>(sResourceID);
  AttachHeightfieldBody(this, pOwner, hRes);
  return W_SUCCESS;
}

void WJoltWorldModule::CreateHeightfieldCollider(WGameObject* pOwner, WStringView sIdentifier, const HeightfieldColliderData& data)
{
  WJoltHeightfieldResourceDescriptor desc;
  desc.m_vHalfExtents = data.m_vHalfExtents;
  desc.m_uiResolution = data.m_uiResolution;
  desc.m_Heights = data.m_Heights;
  desc.m_MaterialIndices = data.m_MaterialIndices;
  desc.m_Surfaces = data.m_Surfaces;
  desc.m_uiCollisionLayer = data.m_uiCollisionLayer;

  WStringBuilder sResourceID("[rt-hf]:", sIdentifier);
  WJoltHeightfieldResourceHandle hRes = WResourceManager::GetOrCreateResource<WJoltHeightfieldResource>(
    sResourceID, std::move(desc), "Runtime Heightfield");

  m_RuntimeHeightfieldIDs.Insert(sIdentifier);
  AttachHeightfieldBody(this, pOwner, hRes);
}

void WJoltWorldModule::RemoveHeightfieldCollider(WGameObject* pOwner)
{
  WJoltHeightfieldColliderComponent* pComp = nullptr;
  if (pOwner->TryGetComponentOfBaseType(pComp))
  {
    pComp->DeleteComponent();
  }
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltWorldModule);
