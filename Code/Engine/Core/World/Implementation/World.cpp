#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/Stats.h>

WIdTable<WWorldId, WWorld*> WWorld::s_Worlds;

static WGameObjectHandle DefaultGameObjectReferenceResolver(const void* pData, WComponentHandle hThis, WStringView sProperty)
{
  W_IGNORE_UNUSED(hThis);
  W_IGNORE_UNUSED(sProperty);

  const char* szRef = reinterpret_cast<const char*>(pData);

  if (WStringUtils::IsNullOrEmpty(szRef))
    return WGameObjectHandle();

  // this is a convention used by WPrefabReferenceComponent:
  // a string starting with this means a 'global game object reference', ie a reference that is valid within the current world
  // what follows is an integer that is the internal storage of an WGameObjectHandle
  // thus parsing the int and casting it to an WGameObjectHandle gives the desired result
  if (WStringUtils::StartsWith(szRef, "#!GGOR-"))
  {
    WInt64 id;
    if (WConversionUtils::StringToInt64(szRef + 7, id).Succeeded())
    {
      return WGameObjectHandle(WGameObjectId(reinterpret_cast<WUInt64&>(id)));
    }
  }

  return WGameObjectHandle();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WWorld, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_CreateGameObject, In, "Name", In, "Parent", In, "LocalPosition", In, "LocalRotation", In, "LocalScale", In, "LocalUniformScale", In, "Dynamic")->AddAttributes(
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute(WVec3(1.0f))),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute(1.0f))),
    W_SCRIPT_FUNCTION_PROPERTY(DeleteObjectDelayed, In, "GameObject", In, "DeleteEmptyParents")->AddAttributes(new WFunctionArgumentAttributes(1, new WDefaultValueAttribute(true))),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_TryGetObjectWithGlobalKey, In, "GlobalKey")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SearchForObject, In, "SearchPath", In, "ReferenceObject")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetClock)->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetRandomNumberGenerator)->AddFlags(WPropertyFlags::PureFunction),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WWorld::WWorld(WWorldDesc& ref_desc)
  : m_Data(ref_desc)
{
  m_pUpdateTask = W_DEFAULT_NEW(WDelegateTask<void>, "WorldUpdate", WTaskNesting::Never, WMakeDelegate(&WWorld::UpdateFromThread, this));
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  WStringBuilder sb = ref_desc.m_sName.GetString();
  sb.Append(".Update");
  m_pUpdateTask->ConfigureTask(sb, WTaskNesting::Maybe);

  W_ASSERT_DEV(GetWorldCount() < GetMaxNumWorlds(), "Max number of worlds reached: {}", GetMaxNumWorlds());
  static_assert(GetMaxNumWorlds() == W_MAX_WORLDS);

  m_InternalId = s_Worlds.Insert(this);

  SetGameObjectReferenceResolver(DefaultGameObjectReferenceResolver);
}

WWorld::~WWorld()
{
  SetWorldSimulationEnabled(false);

  W_LOCK(GetWriteMarker());
  m_Data.Clear();

  s_Worlds.Remove(m_InternalId);
  m_InternalId.Invalidate();
}


void WWorld::Clear()
{
  CheckForWriteAccess();

  while (GetObjectCount() > 0)
  {
    for (auto it = GetObjects(); it.IsValid(); ++it)
    {
      DeleteObjectNow(it->GetHandle());
    }

    if (GetObjectCount() > 0)
    {
      WLog::Dev("Remaining objects after WWorld::Clear: {}", GetObjectCount());
    }
  }

  for (WWorldModule* pModule : m_Data.m_Modules)
  {
    if (pModule != nullptr)
    {
      pModule->WorldClear();
    }
  }

  // make sure all dead objects and components are cleared right now
  DeleteDeadObjects();
  DeleteDeadComponents();

  WEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(this);

  // reset the message time
  m_Data.m_MessageTime = WTime::MakeZero();
}

void WWorld::SetCoordinateSystemProvider(const WSharedPtr<WCoordinateSystemProvider>& pProvider)
{
  W_ASSERT_DEV(pProvider != nullptr, "Coordinate System Provider must not be null");

  m_Data.m_pCoordinateSystemProvider = pProvider;
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;
}

// a super simple, but also efficient random number generator
inline static WUInt32 NextStableRandomSeed(WUInt32& ref_uiSeed)
{
  ref_uiSeed = 214013L * ref_uiSeed + 2531011L;
  return ((ref_uiSeed >> 16) & 0x7FFFF);
}

WGameObjectHandle WWorld::CreateObject(const WGameObjectDesc& desc, WGameObject*& out_pObject)
{
  CheckForWriteAccess();

  W_ASSERT_DEV(m_Data.m_Objects.GetCount() < GetMaxNumGameObjects(), "Max number of game objects reached: {}", GetMaxNumGameObjects());

  WGameObject* pParentObject = nullptr;
  WGameObject::TransformationData* pParentData = nullptr;
  WUInt32 uiParentIndex = 0;
  WUInt64 uiHierarchyLevel = 0;
  bool bDynamic = desc.m_bDynamic;

  if (TryGetObject(desc.m_hParent, pParentObject))
  {
    pParentData = pParentObject->m_pTransformationData;
    uiParentIndex = desc.m_hParent.m_InternalId.m_InstanceIndex;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1; // if there is a parent hierarchy level is parent level + 1
    W_ASSERT_DEV(uiHierarchyLevel < GetMaxNumHierarchyLevels(), "Max hierarchy level reached: {}", GetMaxNumHierarchyLevels());
    bDynamic |= pParentObject->IsDynamic();
  }

  // get storage for the transformation data
  WGameObject::TransformationData* pTransformationData = m_Data.CreateTransformationData(bDynamic, static_cast<WUInt32>(uiHierarchyLevel));

  // get storage for the object itself
  WGameObject* pNewObject = m_Data.m_ObjectStorage.Create();

  // insert the new object into the id mapping table
  WGameObjectId newId = m_Data.m_Objects.Insert(pNewObject);
  newId.m_WorldIndex = GetIndex();

  // fill out some data
  pNewObject->m_InternalId = newId;
  pNewObject->m_Flags = WObjectFlags::None;
  pNewObject->m_Flags.AddOrRemove(WObjectFlags::Dynamic, bDynamic);
  pNewObject->m_Flags.AddOrRemove(WObjectFlags::ActiveFlag, desc.m_bActiveFlag);
  pNewObject->m_sName = desc.m_sName;
  pNewObject->m_uiParentIndex = uiParentIndex;
  pNewObject->m_Tags = desc.m_Tags;
  pNewObject->m_uiTeamID = desc.m_uiTeamID;

  static_assert((GetMaxNumHierarchyLevels() - 1) <= WMath::MaxValue<WUInt16>());
  pNewObject->m_uiHierarchyLevel = static_cast<WUInt16>(uiHierarchyLevel);

  // fill out the transformation data
  pTransformationData->m_pObject = pNewObject;
  pTransformationData->m_pParentData = pParentData;
  pTransformationData->m_localPosition = WSimdConversion::ToVec3(desc.m_LocalPosition);
  pTransformationData->m_localRotation = WSimdConversion::ToQuat(desc.m_LocalRotation);
  pTransformationData->m_localScaling = WSimdConversion::ToVec4(desc.m_LocalScaling.GetAsVec4(desc.m_LocalUniformScaling));
  pTransformationData->m_globalTransform = WSimdTransform::MakeIdentity();
#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
  pTransformationData->m_lastGlobalTransform = WSimdTransform::MakeIdentity();
  pTransformationData->m_uiLastGlobalTransformUpdateCounter = WInvalidIndex;
#endif
  pTransformationData->m_localBounds = WSimdBBoxSphere::MakeInvalid();
  pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(WSimdFloat::MakeZero());
  pTransformationData->m_globalBounds = pTransformationData->m_localBounds;
  pTransformationData->m_hSpatialData.Invalidate();
  pTransformationData->m_uiSpatialDataCategoryBitmask = 0;
  pTransformationData->m_uiStableRandomSeed = desc.m_uiStableRandomSeed;

  // if seed is set to 0xFFFFFFFF, use the parent's seed to create a deterministic value for this object
  if (pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF && pTransformationData->m_pParentData != nullptr)
  {
    WUInt32 seed = pTransformationData->m_pParentData->m_uiStableRandomSeed + pTransformationData->m_pParentData->m_pObject->GetChildCount();

    do
    {
      pTransformationData->m_uiStableRandomSeed = NextStableRandomSeed(seed);

    } while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF);
  }

  // if the seed is zero (or there was no parent to derive the seed from), assign a random value
  while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF)
  {
    pTransformationData->m_uiStableRandomSeed = GetRandomNumberGenerator().UInt();
  }

  pTransformationData->UpdateGlobalTransformNonRecursive(0);

  // link the transformation data to the game object
  pNewObject->m_pTransformationData = pTransformationData;

  // fix links
  LinkToParent(pNewObject);

  pNewObject->UpdateActiveState(pParentObject == nullptr ? true : pParentObject->IsActive());

  out_pObject = pNewObject;
  return WGameObjectHandle(newId);
}

void WWorld::DeleteObjectNow(const WGameObjectHandle& hObject0, bool bAlsoDeleteEmptyParents /*= true*/)
{
  CheckForWriteAccess();

  WGameObject* pObject = nullptr;
  if (!m_Data.m_Objects.TryGetValue(hObject0, pObject))
    return;

  WGameObjectHandle hObject = hObject0;

  if (bAlsoDeleteEmptyParents)
  {
    WGameObject* pParent = pObject->GetParent();

    while (pParent)
    {
      if (pParent->GetChildCount() != 1)
        break;

      if (pParent->GetComponents().GetCount() > 1)
        break;

      // special case for in-editor simulation:
      // also consider parents that only have a prefab component as "empty"
      // at game runtime, prefab component delete themselves, but in the editor they don't, otherwise objects wouldn't be selectable anymore (while simulating)
      WPrefabReferenceComponent* pPrefab = nullptr;
      if (pParent->GetComponents().GetCount() == 1 && pParent->TryGetComponentOfBaseType(pPrefab) == false)
        break;

      pObject = pParent;

      pParent = pParent->GetParent();
    }

    hObject = pObject->GetHandle();
  }

  // inform external systems that we are about to delete this object
  m_Data.m_ObjectDeletionEvent.Broadcast(pObject);

  // set object to inactive so components and children know that they shouldn't access the object anymore.
  pObject->m_Flags.Remove(WObjectFlags::ActiveFlag | WObjectFlags::ActiveState);

  // delete children
  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteObjectNow(it->GetHandle(), false);
  }

  // delete attached components
  while (!pObject->m_Components.IsEmpty())
  {
    WComponent* pComponent = pObject->m_Components[0];
    pComponent->DeleteComponent();
  }
  W_ASSERT_DEV(pObject->m_Components.GetCount() == 0, "Components should already be removed");

  // fix parent and siblings
  UnlinkFromParent(pObject);

  // remove from global key tables
  SetObjectGlobalKey(pObject, WHashedString());

  // invalidate (but preserve world index) and remove from id table
  pObject->m_InternalId.Invalidate();
  pObject->m_InternalId.m_WorldIndex = GetIndex();

  m_Data.m_DeadObjects.Insert(pObject);
  W_VERIFY(m_Data.m_Objects.Remove(hObject), "Implementation error.");
}

void WWorld::DeleteObjectDelayed(const WGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents /*= true*/)
{
  WMsgDeleteGameObject msg;
  msg.m_bDeleteEmptyParents = bAlsoDeleteEmptyParents;
  PostMessage(hObject, msg, WTime::MakeZero());
}

WComponentInitBatchHandle WWorld::CreateComponentInitBatch(WStringView sBatchName, bool bMustFinishWithinOneFrame /*= true*/)
{
  auto pInitBatch = W_NEW(GetAllocator(), WInternal::WorldData::InitBatch, GetAllocator(), sBatchName, bMustFinishWithinOneFrame);
  return WComponentInitBatchHandle(m_Data.m_InitBatches.Insert(pInitBatch));
}

void WWorld::DeleteComponentInitBatch(const WComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  W_IGNORE_UNUSED(pInitBatch);
  W_ASSERT_DEV(pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty(), "Init batch has not been completely processed");
  m_Data.m_InitBatches.Remove(hBatch.GetInternalID());
}

void WWorld::BeginAddingComponentsToInitBatch(const WComponentInitBatchHandle& hBatch)
{
  W_ASSERT_DEV(m_Data.m_pCurrentInitBatch == m_Data.m_pDefaultInitBatch, "Nested init batches are not supported");
  m_Data.m_pCurrentInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()].Borrow();
}

void WWorld::EndAddingComponentsToInitBatch(const WComponentInitBatchHandle& hBatch)
{
  W_ASSERT_DEV(m_Data.m_InitBatches[hBatch.GetInternalID()] == m_Data.m_pCurrentInitBatch, "Init batch with id {} is currently not active", hBatch.GetInternalID().m_Data);
  W_IGNORE_UNUSED(hBatch);
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

void WWorld::SubmitComponentInitBatch(const WComponentInitBatchHandle& hBatch)
{
  m_Data.m_InitBatches[hBatch.GetInternalID()]->m_bIsReady = true;
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

bool WWorld::IsComponentInitBatchCompleted(const WComponentInitBatchHandle& hBatch, double* pCompletionFactor /*= nullptr*/)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  W_ASSERT_DEV(pInitBatch->m_bIsReady, "Batch is not submitted yet");

  if (pCompletionFactor != nullptr)
  {
    if (pInitBatch->m_ComponentsToInitialize.IsEmpty())
    {
      if (m_Data.m_bSimulateWorld)
      {
        double fStartSimCompletion = pInitBatch->m_ComponentsToStartSimulation.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToStartSimulation / pInitBatch->m_ComponentsToStartSimulation.GetCount();
        *pCompletionFactor = fStartSimCompletion * 0.5 + 0.5;
      }
      else
      {
        *pCompletionFactor = 1.0;

        W_ASSERT_DEV(m_Data.m_pDefaultInitBatch != pInitBatch, "");

        m_Data.m_pDefaultInitBatch->m_ComponentsToStartSimulation.PushBackRange(pInitBatch->m_ComponentsToStartSimulation);
        pInitBatch->m_ComponentsToStartSimulation.Clear();
        return true;
      }
    }
    else
    {
      double fInitCompletion = pInitBatch->m_ComponentsToInitialize.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToInitialize / pInitBatch->m_ComponentsToInitialize.GetCount();

      if (m_Data.m_bSimulateWorld)
      {
        *pCompletionFactor = fInitCompletion * 0.5;
      }
      else
      {
        *pCompletionFactor = fInitCompletion;
      }
    }
  }

  return pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty();
}

void WWorld::CancelComponentInitBatch(const WComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  pInitBatch->m_ComponentsToInitialize.Clear();
  pInitBatch->m_ComponentsToStartSimulation.Clear();
}

void WWorld::PostMessage(const WGameObjectHandle& receiverObject, const WMessage& msg, WObjectMsgQueueType::Enum queueType, WTime delay, bool bRecursive) const
{
  // This method is allowed to be called from multiple threads.
  auto& mutex = m_Data.m_MessageQueueMutex[queueType];

  W_ASSERT_DEBUG((receiverObject.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in object id must not be set");

  QueuedMsg queuedMsg;
  queuedMsg.m_uiReceiverObjectOrComponent = receiverObject.m_InternalId.m_Data;
  queuedMsg.m_uiReceiverIsComponent = false;
  queuedMsg.m_uiRecursive = bRecursive;

  WRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
  {
    queuedMsg.m_pMessage = pMsgRTTIAllocator->Clone<WMessage>(&msg, &m_Data.m_Allocator);
    queuedMsg.m_Due = m_Data.m_MessageTime + delay;

    W_LOCK(mutex);
    m_Data.m_TimedMessageQueues[queueType].PushBack(queuedMsg);
  }
  else
  {
    queuedMsg.m_pMessage = pMsgRTTIAllocator->Clone<WMessage>(&msg, m_Data.m_LinearAllocator.GetCurrentAllocator());

    W_LOCK(mutex);
    m_Data.m_MessageQueues[queueType].PushBack(queuedMsg);
  }
}

void WWorld::PostMessage(const WComponentHandle& hReceiverComponent, const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  auto& mutex = m_Data.m_MessageQueueMutex[queueType];

  W_ASSERT_DEBUG((hReceiverComponent.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in component id must not be set");

  QueuedMsg queuedMsg;
  queuedMsg.m_uiReceiverObjectOrComponent = hReceiverComponent.m_InternalId.m_Data;
  queuedMsg.m_uiReceiverIsComponent = true;
  queuedMsg.m_uiRecursive = false;

  WRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
  {
    queuedMsg.m_pMessage = pMsgRTTIAllocator->Clone<WMessage>(&msg, &m_Data.m_Allocator);
    queuedMsg.m_Due = m_Data.m_MessageTime + delay;

    W_LOCK(mutex);
    m_Data.m_TimedMessageQueues[queueType].PushBack(queuedMsg);
  }
  else
  {
    queuedMsg.m_pMessage = pMsgRTTIAllocator->Clone<WMessage>(&msg, m_Data.m_LinearAllocator.GetCurrentAllocator());

    W_LOCK(mutex);
    m_Data.m_MessageQueues[queueType].PushBack(queuedMsg);
  }
}

void WWorld::FindEventMsgHandlers(const WMessage& msg, const WComponent* pSenderComponent, WGameObject* pSearchObject, WDynamicArray<WComponent*>& out_components)
{
  FindEventMsgHandlers(*this, msg, pSenderComponent, pSearchObject, out_components);
}

void WWorld::FindEventMsgHandlers(const WMessage& msg, const WComponent* pSenderComponent, const WGameObject* pSearchObject, WDynamicArray<const WComponent*>& out_components) const
{
  FindEventMsgHandlers(*this, msg, pSenderComponent, pSearchObject, out_components);
}

void WWorld::Update()
{
  CheckForWriteAccess();

  W_LOG_BLOCK(m_Data.m_sName.GetData());

  {
    WStringBuilder sStatName;
    sStatName.SetFormat("World Update/{0}/Game Object Count", m_Data.m_sName);

    WStringBuilder sStatValue;
    WStats::SetStat(sStatName, GetObjectCount());
  }

  ++m_Data.m_uiUpdateCounter;

  if (!m_Data.m_bSimulateWorld)
  {
    // only change the pause mode temporarily
    // so that user choices don't get overridden

    const bool bClockPaused = m_Data.m_Clock.GetPaused();
    m_Data.m_Clock.SetPaused(true);
    m_Data.m_Clock.Update();
    m_Data.m_Clock.SetPaused(bClockPaused);
  }
  else
  {
    m_Data.m_Clock.Update();
  }

  UpdateMessageTime();

  if (m_Data.m_pSpatialSystem != nullptr)
  {
    m_Data.m_pSpatialSystem->StartNewFrame();
  }

  // reload resources
  {
    W_PROFILE_SCOPE("Reload Resources");
    ProcessResourceReloadFunctions();
  }

  // initialize phase
  {
    W_PROFILE_SCOPE("Initialize Phase");
    ProcessComponentsToInitialize();
    ProcessUpdateFunctionsToDeregister();
    ProcessUpdateFunctionsToRegister();

    ProcessQueuedMessages(WObjectMsgQueueType::AfterInitialized);
    ProcessLocalBoundsUpdateQueue();
  }

  // pre-async phase
  {
    W_PROFILE_SCOPE("Pre-Async Phase");
    ProcessQueuedMessages(WObjectMsgQueueType::NextFrame);
    UpdateSynchronous(m_Data.m_UpdateFunctions[WWorldUpdatePhase::PreAsync]);
  }

  // async phase
  {
    // remove write marker but keep the read marker. Thus no one can mark the world for writing now. Only reading is allowed in async phase.
    m_Data.m_WriteThreadID = (WThreadID)0;

    W_PROFILE_SCOPE("Async Phase");
    UpdateAsynchronous();

    // restore write marker
    m_Data.m_WriteThreadID = WThreadUtils::GetCurrentThreadID();
  }

  // post-async phase
  {
    W_PROFILE_SCOPE("Post-Async Phase");
    ProcessQueuedMessages(WObjectMsgQueueType::PostAsync);
    UpdateSynchronous(m_Data.m_UpdateFunctions[WWorldUpdatePhase::PostAsync]);
    ProcessLocalBoundsUpdateQueue();
  }

  // delete dead objects and update the object hierarchy
  {
    W_PROFILE_SCOPE("Delete Dead Objects");
    DeleteDeadObjects();
    DeleteDeadComponents();
  }

  // update transforms
  {
    W_PROFILE_SCOPE("Update Transforms");
    m_Data.UpdateGlobalTransforms();
  }

  // post-transform phase
  {
    W_PROFILE_SCOPE("Post-Transform Phase");
    ProcessQueuedMessages(WObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[WWorldUpdatePhase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  {
    W_PROFILE_SCOPE("Initialize Phase 2");
    // Only process the default init batch here since it contains the components created at runtime.
    // Also make sure that all initialization is finished after this call by giving it enough time.
    ProcessInitializationBatch(*m_Data.m_pDefaultInitBatch, WTime::Now() + WTime::MakeFromHours(10000));

    ProcessQueuedMessages(WObjectMsgQueueType::AfterInitialized);
  }

  // Swap our double buffered stack allocator
  m_Data.m_LinearAllocator.Swap();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

WWorldModule* WWorld::GetOrCreateModule(const WRTTI* pRtti)
{
  CheckForWriteAccess();

  const WWorldModuleTypeId uiTypeId = WWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId == 0xFFFF)
  {
    return nullptr;
  }

  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  WWorldModule* pModule = m_Data.m_Modules[uiTypeId];
  if (pModule == nullptr)
  {
    pModule = WWorldModuleFactory::GetInstance()->CreateWorldModule(uiTypeId, this);
    pModule->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;

    if (m_Data.m_bSimulateWorld)
    {
      pModule->OnSimulationStarted();
    }
    else
    {
      m_Data.m_ModulesToStartSimulation.PushBack(pModule);
    }
  }

  return pModule;
}

void WWorld::DeleteModule(const WRTTI* pRtti)
{
  CheckForWriteAccess();

  const WWorldModuleTypeId uiTypeId = WWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (WWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      pModule->Deinitialize();
      DeregisterUpdateFunctionsInternal(pModule);
      W_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

WWorldModule* WWorld::GetModule(const WRTTI* pRtti)
{
  CheckForWriteAccess();

  const WWorldModuleTypeId uiTypeId = WWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

const WWorldModule* WWorld::GetModule(const WRTTI* pRtti) const
{
  CheckForReadAccess();

  const WWorldModuleTypeId uiTypeId = WWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

WGameObject* WWorld::Reflection_CreateGameObject(WHashedString sName, const WGameObjectHandle& hParent, const WVec3& vLocalPosition, const WQuat& qLocalRotation, const WVec3& vLocalScale, float fLocalUniformScale, bool bDynamic)
{
  WGameObjectDesc desc;
  desc.m_bDynamic = bDynamic;
  desc.m_sName = sName;
  desc.m_hParent = hParent;
  desc.m_LocalPosition = vLocalPosition;
  desc.m_LocalRotation = qLocalRotation;
  desc.m_LocalScaling = vLocalScale;
  desc.m_LocalUniformScaling = fLocalUniformScale;

  // Prevent zero scale, which can easily happen when creating objects from script, as this can easily break all sort of things down the line.
  if (desc.m_LocalScaling.IsZero(WMath::DefaultEpsilon<float>()))
  {
    desc.m_LocalScaling.Set(0.0001f);
  }

  if (WMath::IsZero(desc.m_LocalUniformScaling, WMath::DefaultEpsilon<float>()))
  {
    desc.m_LocalUniformScaling = 0.0001f;
  }

  WGameObject* pObject = nullptr;
  CreateObject(desc, pObject);
  return pObject;
}

WGameObject* WWorld::Reflection_TryGetObjectWithGlobalKey(WTempHashedString sGlobalKey)
{
  WGameObject* pObject = nullptr;
  bool res = TryGetObjectWithGlobalKey(sGlobalKey, pObject);
  W_IGNORE_UNUSED(res);
  return pObject;
}

WClock* WWorld::Reflection_GetClock()
{
  return &m_Data.m_Clock;
}

WRandom* WWorld::Reflection_GetRandomNumberGenerator()
{
  return &m_Data.m_Random;
}

void WWorld::SetParent(WGameObject* pObject, WGameObject* pNewParent, WTransformPreservation::Enum preserve)
{
  W_ASSERT_DEV(pObject != pNewParent, "Object can't be its own parent!");
  W_ASSERT_DEV(pNewParent == nullptr || pObject->IsDynamic() || pNewParent->IsStatic(), "Can't attach a static object to a dynamic parent!");
  CheckForWriteAccess();

  if (GetObjectUnchecked(pObject->m_uiParentIndex) == pNewParent)
    return;

  UnlinkFromParent(pObject);
  // UnlinkFromParent does not clear these as they are still needed in DeleteObjectNow to allow deletes while iterating.
  pObject->m_uiNextSiblingIndex = 0;
  pObject->m_uiPrevSiblingIndex = 0;
  if (pNewParent != nullptr)
  {
    // Ensure that the parent's global transform is up-to-date otherwise the object's local transform will be wrong afterwards.
    pNewParent->UpdateGlobalTransform();

    pObject->m_uiParentIndex = pNewParent->m_InternalId.m_InstanceIndex;
    LinkToParent(pObject);
  }

  PatchHierarchyData(pObject, preserve);

  // TODO: the functions above send messages such as WMsgChildrenChanged, which will not arrive for inactive components, is that a problem ?
  // 1) if a component was active before and now gets deactivated, it may not care about the message anymore anyway
  // 2) if a component was inactive before, it did not get the message, but upon activation it can update the state for which it needed the message
  // so probably it is fine, only components that were active and stay active need the message, and that will be the case
  pObject->UpdateActiveState(pNewParent == nullptr ? true : pNewParent->IsActive());
}

void WWorld::LinkToParent(WGameObject* pObject)
{
  W_ASSERT_DEBUG(pObject->m_uiNextSiblingIndex == 0 && pObject->m_uiPrevSiblingIndex == 0, "Object is either still linked to another parent or data was not cleared.");
  if (WGameObject* pParentObject = pObject->GetParent())
  {
    const WUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (pParentObject->m_uiFirstChildIndex != 0)
    {
      pObject->m_uiPrevSiblingIndex = pParentObject->m_uiLastChildIndex;
      GetObjectUnchecked(pParentObject->m_uiLastChildIndex)->m_uiNextSiblingIndex = uiIndex;
    }
    else
    {
      pParentObject->m_uiFirstChildIndex = uiIndex;
    }

    pParentObject->m_uiLastChildIndex = uiIndex;
    pParentObject->m_uiChildCount++;

    pObject->m_pTransformationData->m_pParentData = pParentObject->m_pTransformationData;

    if (pObject->m_Flags.IsSet(WObjectFlags::ParentChangesNotifications))
    {
      WMsgParentChanged msg;
      msg.m_Type = WMsgParentChanged::Type::ParentLinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

    if (pParentObject->m_Flags.IsSet(WObjectFlags::ChildChangesNotifications))
    {
      WMsgChildrenChanged msg;
      msg.m_Type = WMsgChildrenChanged::Type::ChildAdded;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
  }
}

void WWorld::UnlinkFromParent(WGameObject* pObject)
{
  if (WGameObject* pParentObject = pObject->GetParent())
  {
    const WUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (uiIndex == pParentObject->m_uiFirstChildIndex)
      pParentObject->m_uiFirstChildIndex = pObject->m_uiNextSiblingIndex;

    if (uiIndex == pParentObject->m_uiLastChildIndex)
      pParentObject->m_uiLastChildIndex = pObject->m_uiPrevSiblingIndex;

    if (WGameObject* pNextObject = GetObjectUnchecked(pObject->m_uiNextSiblingIndex))
      pNextObject->m_uiPrevSiblingIndex = pObject->m_uiPrevSiblingIndex;

    if (WGameObject* pPrevObject = GetObjectUnchecked(pObject->m_uiPrevSiblingIndex))
      pPrevObject->m_uiNextSiblingIndex = pObject->m_uiNextSiblingIndex;

    pParentObject->m_uiChildCount--;
    pObject->m_uiParentIndex = 0;
    pObject->m_pTransformationData->m_pParentData = nullptr;

    if (pObject->m_Flags.IsSet(WObjectFlags::ParentChangesNotifications))
    {
      WMsgParentChanged msg;
      msg.m_Type = WMsgParentChanged::Type::ParentUnlinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

    // Note that the sibling indices must not be set to 0 here.
    // They are still needed if we currently iterate over child objects.

    if (pParentObject->m_Flags.IsSet(WObjectFlags::ChildChangesNotifications))
    {
      WMsgChildrenChanged msg;
      msg.m_Type = WMsgChildrenChanged::Type::ChildRemoved;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
  }
}

void WWorld::SetObjectGlobalKey(WGameObject* pObject, const WHashedString& sGlobalKey)
{
  if (auto it = m_Data.m_GlobalKeyToIdTable.Find(sGlobalKey.GetHash()); it.IsValid())
  {
    if (it.Value() == pObject->m_InternalId) // same object, same global key ?
    {
      return;
    }

    // we allow overwriting a global key to a different object here
    // so that we can delete an object in a frame and spawn a new one in the same frame, that takes over
    // due to the delayed deletion at the end of the frame, this would otherwise not work
    // the only work-around would be to manually clear the global key before deleting an object
    // but that would effectively do the same as this, it's just more complicated for the user

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    WLog::Warning("An object with the global key '{}' already exists. Overwriting with different object reference.", sGlobalKey);
#endif

    W_VERIFY(m_Data.m_IdToGlobalKeyTable.Remove(it.Value().m_InstanceIndex), "Implementation error.");
    m_Data.m_GlobalKeyToIdTable.Remove(it);
  }

  const WUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  // Remove existing entry first.
  WHashedString* pOldGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pOldGlobalKey))
  {
    if (sGlobalKey == *pOldGlobalKey)
    {
      return;
    }

    W_VERIFY(m_Data.m_GlobalKeyToIdTable.Remove(pOldGlobalKey->GetHash()), "Implementation error.");
    W_VERIFY(m_Data.m_IdToGlobalKeyTable.Remove(uiId), "Implementation error.");
  }

  // Insert new one if key is valid.
  if (!sGlobalKey.IsEmpty())
  {
    m_Data.m_GlobalKeyToIdTable.Insert(sGlobalKey.GetHash(), pObject->m_InternalId);
    m_Data.m_IdToGlobalKeyTable.Insert(uiId, sGlobalKey);
  }
}

WStringView WWorld::GetObjectGlobalKey(const WGameObject* pObject) const
{
  const WUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  const WHashedString* pGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pGlobalKey))
  {
    return pGlobalKey->GetView();
  }

  return {};
}

void WWorld::ProcessQueuedMessage(const QueuedMsg& entry)
{
  if (entry.m_uiReceiverIsComponent)
  {
    WComponentHandle hComponent(WComponentId(entry.m_uiReceiverObjectOrComponent));

    WComponent* pReceiverComponent = nullptr;
    if (TryGetComponent(hComponent, pReceiverComponent))
    {
      pReceiverComponent->SendMessageInternal(*entry.m_pMessage, true);
    }
    else
    {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        WLog::Warning("WWorld::ProcessQueuedMessage: Receiver WComponent for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
  else
  {
    WGameObjectHandle hObject(WGameObjectId(entry.m_uiReceiverObjectOrComponent));

    WGameObject* pReceiverObject = nullptr;
    if (TryGetObject(hObject, pReceiverObject))
    {
      if (entry.m_uiRecursive)
      {
        pReceiverObject->SendMessageRecursiveInternal(*entry.m_pMessage, true);
      }
      else
      {
        pReceiverObject->SendMessageInternal(*entry.m_pMessage, true);
      }
    }
    else
    {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        WLog::Warning("WWorld::ProcessQueuedMessage: Receiver WGameObject for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
}

void WWorld::UpdateMessageTime()
{
  WTime deltaTime;
  if (GetWorldSimulationEnabled())
  {
    deltaTime = GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = WClock::GetGlobalClock()->GetTimeDiff();
  }

  m_Data.m_MessageTime += deltaTime;
}

void WWorld::ProcessQueuedMessages(WObjectMsgQueueType::Enum queueType)
{
  W_PROFILE_SCOPE("Process Queued Messages");

  struct MessageComparer
  {
    W_FORCE_INLINE bool Less(const QueuedMsg& a, const QueuedMsg& b) const
    {
      if (a.m_Due != b.m_Due)
        return a.m_Due < b.m_Due;

      const WInt32 iKeyA = a.m_pMessage->GetSortingKey();
      const WInt32 iKeyB = b.m_pMessage->GetSortingKey();
      if (iKeyA != iKeyB)
        return iKeyA < iKeyB;

      if (a.m_pMessage->GetId() != b.m_pMessage->GetId())
        return a.m_pMessage->GetId() < b.m_pMessage->GetId();

      if (a.m_uiReceiverData != b.m_uiReceiverData)
        return a.m_uiReceiverData < b.m_uiReceiverData;

      if (a.m_uiMessageHash == 0)
      {
        a.m_uiMessageHash = a.m_pMessage->GetHash();
      }

      if (b.m_uiMessageHash == 0)
      {
        b.m_uiMessageHash = b.m_pMessage->GetHash();
      }

      return a.m_uiMessageHash < b.m_uiMessageHash;
    }
  };

  auto& mutex = m_Data.m_MessageQueueMutex[queueType];

  // regular messages
  {
    WInternal::WorldData::MessageQueue& queue = m_Data.m_MessageProcessingQueues[queueType];

    {
      W_LOCK(mutex);
      queue.Swap(m_Data.m_MessageQueues[queueType]);
    }

    queue.Sort(MessageComparer());

    for (WUInt32 i = 0; i < queue.GetCount(); ++i)
    {
      ProcessQueuedMessage(queue[i]);

      // no need to deallocate these messages, they are allocated through a frame allocator
    }

    queue.Clear();
  }

  // timed messages
  {
    W_LOCK(mutex);

    WInternal::WorldData::MessageQueue& queue = m_Data.m_TimedMessageQueues[queueType];
    queue.Sort(MessageComparer());

    while (!queue.IsEmpty())
    {
      auto& entry = queue.PeekFront();
      if (entry.m_Due > m_Data.m_MessageTime)
        break;

      ProcessQueuedMessage(entry);

      W_DELETE(&m_Data.m_Allocator, entry.m_pMessage);

      queue.PopFront();
    }
  }
}

// static
template <typename World, typename GameObject, typename Component>
void WWorld::FindEventMsgHandlers(World& world, const WMessage& msg, const WComponent* pSenderComponent, GameObject pSearchObject, WDynamicArray<Component>& out_components)
{
  using EventMessageHandlerComponentType = typename std::conditional<std::is_const<World>::value, const WEventMessageHandlerComponent*, WEventMessageHandlerComponent*>::type;

  out_components.Clear();

  // walk the graph upwards until an object is found with at least one WComponent that handles this type of message
  {
    auto pCurrentObject = pSearchObject;

    while (pCurrentObject != nullptr)
    {
      bool bContinueSearch = true;
      for (auto pComponent : pCurrentObject->GetComponents())
      {
        if (pComponent == pSenderComponent)
          continue;

        if constexpr (std::is_const<World>::value == false)
        {
          pComponent->EnsureInitialized();
        }

        if (pComponent->HandlesMessage(msg))
        {
          out_components.PushBack(pComponent);
          bContinueSearch = false;
        }
        else
        {
          if constexpr (std::is_const<World>::value)
          {
            if (pComponent->IsInitialized() == false)
            {
              WLog::Warning("Component of type '{}' was not initialized (yet) and thus might have reported an incorrect result in HandlesMessage(). "
                             "To allow this component to be automatically initialized at this point in time call the non-const variant of SendEventMessage.",
                pComponent->GetDynamicRTTI()->GetTypeName());
            }
          }

          // only continue to search on parent objects if all event handlers on the current object have the "pass through unhandled events" flag set.
          if (auto pEventMessageHandlerComponent = WDynamicCast<EventMessageHandlerComponentType>(pComponent))
          {
            bContinueSearch &= pEventMessageHandlerComponent->GetPassThroughUnhandledEvents();
          }
        }
      }

      if (!bContinueSearch)
      {
        // stop searching as we found at least one WEventMessageHandlerComponent or one doesn't have the "pass through" flag set.
        return;
      }

      pCurrentObject = pCurrentObject->GetParent();
      pSenderComponent = nullptr;
    }
  }

  // if no components have been found, check all event handler components that are registered as 'global event handlers'
  if (out_components.IsEmpty())
  {
    auto globalEventMessageHandler = WEventMessageHandlerComponent::GetAllGlobalEventHandler(&world);
    for (auto hEventMessageHandlerComponent : globalEventMessageHandler)
    {
      EventMessageHandlerComponentType pEventMessageHandlerComponent = nullptr;
      if (world.TryGetComponent(hEventMessageHandlerComponent, pEventMessageHandlerComponent))
      {
        if (pEventMessageHandlerComponent->HandlesMessage(msg))
        {
          out_components.PushBack(pEventMessageHandlerComponent);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WWorld::RegisterUpdateFunction(const WComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  W_ASSERT_DEV(desc.m_Phase != WWorldUpdatePhase::Async || desc.m_DependsOn.GetCount() == 0, "Asynchronous update functions must not have dependencies");
  W_ASSERT_DEV(desc.m_Function.IsComparable(), "Delegates with captures are not allowed as WWorld update functions.");

  m_Data.m_UpdateFunctionsToRegister.PushBack(desc);
}

void WWorld::DeregisterUpdateFunction(const WComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  m_Data.m_UpdateFunctionsToDeregister.PushBack(desc);
}

void WWorld::AddComponentToInitialize(WComponentHandle hComponent)
{
  m_Data.m_pCurrentInitBatch->m_ComponentsToInitialize.PushBack(hComponent);
}

void WWorld::UpdateFromThread()
{
  W_LOCK(GetWriteMarker());

  Update();
}

void WWorld::UpdateSynchronous(const WArrayPtr<WInternal::WorldData::RegisteredUpdateFunction>& updateFunctions)
{
  WWorldModule::UpdateContext context;
  context.m_uiFirstComponentIndex = 0;
  context.m_uiComponentCount = WInvalidIndex;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    {
      W_PROFILE_SCOPE(updateFunction.m_sFunctionName);
      updateFunction.m_Function(context);
    }
  }
}

void WWorld::UpdateAsynchronous()
{
  WTaskGroupID taskGroupId = WTaskSystem::CreateTaskGroup(WTaskPriority::EarlyThisFrame);

  WDynamicArrayBase<WInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[WWorldUpdatePhase::Async];

  WUInt32 uiCurrentTaskIndex = 0;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    WWorldModule* pModule = static_cast<WWorldModule*>(updateFunction.m_Function.GetClassInstance());
    WComponentManagerBase* pManager = WDynamicCast<WComponentManagerBase*>(pModule);

    // a world module can also register functions in the async phase so we want at least one task
    const WUInt32 uiTotalCount = pManager != nullptr ? pManager->GetComponentCount() : 1;
    const WUInt32 uiGranularity = (updateFunction.m_uiAsyncPhaseBatchSize != 0) ? updateFunction.m_uiAsyncPhaseBatchSize : uiTotalCount;

    WUInt32 uiStartIndex = 0;
    while (uiStartIndex < uiTotalCount)
    {
      WSharedPtr<WInternal::WorldData::UpdateTask> pTask;
      if (uiCurrentTaskIndex < m_Data.m_UpdateTasks.GetCount())
      {
        pTask = m_Data.m_UpdateTasks[uiCurrentTaskIndex];
      }
      else
      {
        pTask = W_NEW(&m_Data.m_Allocator, WInternal::WorldData::UpdateTask);
        m_Data.m_UpdateTasks.PushBack(pTask);
      }

      pTask->ConfigureTask(updateFunction.m_sFunctionName, WTaskNesting::Maybe);
      pTask->m_Function = updateFunction.m_Function;
      pTask->m_uiStartIndex = uiStartIndex;
      pTask->m_uiCount = (uiStartIndex + uiGranularity < uiTotalCount) ? uiGranularity : WInvalidIndex;
      WTaskSystem::AddTaskToGroup(taskGroupId, pTask);

      ++uiCurrentTaskIndex;
      uiStartIndex += uiGranularity;
    }
  }

  WTaskSystem::StartTaskGroup(taskGroupId);
  WTaskSystem::WaitForGroup(taskGroupId);
}

bool WWorld::ProcessInitializationBatch(WInternal::WorldData::InitBatch& batch, WTime endTime)
{
  CheckForWriteAccess();

  // ensure that all components that are created during this batch (e.g. from prefabs)
  // will also get initialized within this batch
  m_Data.m_pCurrentInitBatch = &batch;
  W_SCOPE_EXIT(m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch);

  if (!batch.m_ComponentsToInitialize.IsEmpty())
  {
    WStringBuilder profileScopeName("Init ", batch.m_sName);
    W_PROFILE_SCOPE(profileScopeName);

    // Reserve for later use
    batch.m_ComponentsToStartSimulation.Reserve(batch.m_ComponentsToInitialize.GetCount());

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToInitialize < batch.m_ComponentsToInitialize.GetCount(); ++batch.m_uiNextComponentToInitialize)
    {
      WComponentHandle hComponent = batch.m_ComponentsToInitialize[batch.m_uiNextComponentToInitialize];

      // if it is in the editor, the component might have been added and already deleted, without ever running the simulation
      WComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
        continue;

      W_ASSERT_DEBUG(pComponent->GetOwner() != nullptr, "Component must have a valid owner");

      // make sure the object's transform is up to date before the component is initialized.
      pComponent->GetOwner()->UpdateGlobalTransform();

      pComponent->EnsureInitialized();

      if (pComponent->IsActive())
      {
        pComponent->OnActivated();

        batch.m_ComponentsToStartSimulation.PushBack(hComponent);
      }

      // Check if there is still time left to initialize more components
      if (WTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToInitialize;
        return false;
      }
    }

    batch.m_ComponentsToInitialize.Clear();
    batch.m_uiNextComponentToInitialize = 0;
  }

  if (m_Data.m_bSimulateWorld)
  {
    WStringBuilder startSimName("Start Sim ", batch.m_sName);
    W_PROFILE_SCOPE(startSimName);

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToStartSimulation < batch.m_ComponentsToStartSimulation.GetCount(); ++batch.m_uiNextComponentToStartSimulation)
    {
      WComponentHandle hComponent = batch.m_ComponentsToStartSimulation[batch.m_uiNextComponentToStartSimulation];

      // if it is in the editor, the component might have been added and already deleted,  without ever running the simulation
      WComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
        continue;

      if (pComponent->IsActiveAndInitialized())
      {
        pComponent->EnsureSimulationStarted();
      }

      // Check if there is still time left to initialize more components
      if (WTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToStartSimulation;
        return false;
      }
    }

    batch.m_ComponentsToStartSimulation.Clear();
    batch.m_uiNextComponentToStartSimulation = 0;
  }

  return true;
}

void WWorld::ProcessComponentsToInitialize()
{
  CheckForWriteAccess();

  if (m_Data.m_bSimulateWorld)
  {
    W_PROFILE_SCOPE("Modules Start Simulation");

    // Can't use foreach here because the array might be resized during iteration.
    for (WUInt32 i = 0; i < m_Data.m_ModulesToStartSimulation.GetCount(); ++i)
    {
      m_Data.m_ModulesToStartSimulation[i]->OnSimulationStarted();
    }

    m_Data.m_ModulesToStartSimulation.Clear();
  }

  W_PROFILE_SCOPE("Initialize Components");

  WTime endTime = WTime::Now() + m_Data.m_MaxInitializationTimePerFrame;

  // First process all component init batches that have to finish within this frame
  for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
  {
    auto& pInitBatch = it.Value();
    if (pInitBatch->m_bIsReady && pInitBatch->m_bMustFinishWithinOneFrame)
    {
      ProcessInitializationBatch(*pInitBatch, WTime::Now() + WTime::MakeFromHours(10000));
    }
  }

  // If there is still time left process other component init batches
  if (WTime::Now() < endTime)
  {
    for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
    {
      auto& pInitBatch = it.Value();
      if (!pInitBatch->m_bIsReady || pInitBatch->m_bMustFinishWithinOneFrame)
        continue;

      if (!ProcessInitializationBatch(*pInitBatch, endTime))
        return;
    }
  }
}

void WWorld::ProcessUpdateFunctionsToRegister()
{
  CheckForWriteAccess();

  if (m_Data.m_UpdateFunctionsToRegister.IsEmpty())
    return;

  W_PROFILE_SCOPE("Register update functions");

  while (!m_Data.m_UpdateFunctionsToRegister.IsEmpty())
  {
    const WUInt32 uiNumFunctionsToRegister = m_Data.m_UpdateFunctionsToRegister.GetCount();

    for (WUInt32 i = uiNumFunctionsToRegister; i-- > 0;)
    {
      if (RegisterUpdateFunctionInternal(m_Data.m_UpdateFunctionsToRegister[i]).Succeeded())
      {
        m_Data.m_UpdateFunctionsToRegister.RemoveAtAndCopy(i);
      }
    }

    W_ASSERT_DEV(m_Data.m_UpdateFunctionsToRegister.GetCount() < uiNumFunctionsToRegister, "No functions have been registered because the dependencies could not be found.");
  }
}

WResult WWorld::RegisterUpdateFunctionInternal(const WWorldModule::UpdateFunctionDesc& desc)
{
  WDynamicArrayBase<WInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];
  WUInt32 uiInsertionIndex = 0;

  for (WUInt32 i = 0; i < desc.m_DependsOn.GetCount(); ++i)
  {
    WUInt32 uiDependencyIndex = WInvalidIndex;

    for (WUInt32 j = 0; j < updateFunctions.GetCount(); ++j)
    {
      if (updateFunctions[j].m_sFunctionName == desc.m_DependsOn[i])
      {
        uiDependencyIndex = j;
        break;
      }
    }

    if (uiDependencyIndex == WInvalidIndex) // dependency not found
    {
      return W_FAILURE;
    }
    else
    {
      uiInsertionIndex = WMath::Max(uiInsertionIndex, uiDependencyIndex + 1);
    }
  }

  WInternal::WorldData::RegisteredUpdateFunction newFunction;
  newFunction.FillFromDesc(desc);

  while (uiInsertionIndex < updateFunctions.GetCount())
  {
    const auto& existingFunction = updateFunctions[uiInsertionIndex];
    if (newFunction < existingFunction)
    {
      break;
    }

    ++uiInsertionIndex;
  }

  updateFunctions.InsertAt(uiInsertionIndex, newFunction);

  return W_SUCCESS;
}

void WWorld::ProcessUpdateFunctionsToDeregister()
{
  CheckForWriteAccess();

  for (const WWorldModule::UpdateFunctionDesc& updateFunction : m_Data.m_UpdateFunctionsToDeregister)
  {
    DeregisterUpdateFunctionInternal(updateFunction);
  }

  m_Data.m_UpdateFunctionsToDeregister.Clear();
}

void WWorld::DeregisterUpdateFunctionInternal(const WWorldModule::UpdateFunctionDesc& desc)
{
  WDynamicArrayBase<WInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];

  for (WUInt32 i = updateFunctions.GetCount(); i-- > 0;)
  {
    if (updateFunctions[i].m_Function.IsEqualIfComparable(desc.m_Function))
    {
      updateFunctions.RemoveAtAndCopy(i);
    }
  }
}

void WWorld::DeregisterUpdateFunctionsInternal(WWorldModule* pModule)
{
  CheckForWriteAccess();

  for (WUInt32 phase = WWorldUpdatePhase::PreAsync; phase < WWorldUpdatePhase::COUNT; ++phase)
  {
    WDynamicArrayBase<WInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[phase];

    for (WUInt32 i = updateFunctions.GetCount(); i-- > 0;)
    {
      if (updateFunctions[i].m_Function.GetClassInstance() == pModule)
      {
        updateFunctions.RemoveAtAndCopy(i);
      }
    }
  }
}

void WWorld::DeleteDeadObjects()
{
  while (!m_Data.m_DeadObjects.IsEmpty())
  {
    WGameObject* pObject = m_Data.m_DeadObjects.GetIterator().Key();

    if (!pObject->m_pTransformationData->m_hSpatialData.IsInvalidated())
    {
      m_Data.m_pSpatialSystem->DeleteSpatialData(pObject->m_pTransformationData->m_hSpatialData);
    }

    m_Data.DeleteTransformationData(pObject->IsDynamic(), pObject->m_uiHierarchyLevel, pObject->m_pTransformationData);

    WGameObject* pMovedObject = nullptr;
    m_Data.m_ObjectStorage.Delete(pObject, pMovedObject);

    if (pObject != pMovedObject)
    {
      // patch the id table: the last element in the storage has been moved to deleted object's location,
      // thus the pointer now points to another object
      WGameObjectId id = pObject->m_InternalId;
      if (id.m_InstanceIndex != WGameObjectId::INVALID_INSTANCE_INDEX)
        m_Data.m_Objects[id] = pObject;

      // The moved object might be deleted as well so we remove it from the dead objects set instead.
      // If that is not the case we remove the original object from the set.
      if (m_Data.m_DeadObjects.Remove(pMovedObject))
      {
        continue;
      }
    }

    m_Data.m_DeadObjects.Remove(pObject);
  }
}

void WWorld::DeleteDeadComponents()
{
  while (!m_Data.m_DeadComponents.IsEmpty())
  {
    WComponent* pComponent = m_Data.m_DeadComponents.GetIterator().Key();

    WComponentManagerBase* pManager = pComponent->GetOwningManager();
    WComponent* pMovedComponent = nullptr;
    pManager->DeleteComponentStorage(pComponent, pMovedComponent);

    // another component has been moved to the deleted component location
    if (pComponent != pMovedComponent)
    {
      pManager->PatchIdTable(pComponent);

      if (WGameObject* pOwner = pComponent->GetOwner())
      {
        pOwner->FixComponentPointer(pMovedComponent, pComponent);
      }

      // The moved component might be deleted as well so we remove it from the dead components set instead.
      // If that is not the case we remove the original component from the set.
      if (m_Data.m_DeadComponents.Remove(pMovedComponent))
      {
        continue;
      }
    }

    m_Data.m_DeadComponents.Remove(pComponent);
  }
}

void WWorld::PatchHierarchyData(WGameObject* pObject, WTransformPreservation::Enum preserve)
{
  WGameObject* pParent = pObject->GetParent();

  RecreateHierarchyData(pObject, pObject->IsDynamic());

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;

  if (preserve == WTransformPreservation::Enum::PreserveGlobal)
  {
    // SetGlobalTransform will internally trigger bounds update for static objects
    pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);
  }
  else
  {
    // Explicitly trigger transform AND bounds update, otherwise bounds would be outdated for static objects
    // Don't call pObject->UpdateGlobalTransformAndBounds() here since that would recursively update the parent global transform which is already up-to-date.
    pObject->m_pTransformationData->UpdateGlobalTransformNonRecursive(GetUpdateCounter());

    pObject->m_pTransformationData->UpdateGlobalBounds(GetSpatialSystem());
  }

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    PatchHierarchyData(it, preserve);
  }
  W_ASSERT_DEBUG(pObject->m_pTransformationData != pObject->m_pTransformationData->m_pParentData, "Hierarchy corrupted!");
}

void WWorld::RecreateHierarchyData(WGameObject* pObject, bool bWasDynamic)
{
  WGameObject* pParent = pObject->GetParent();

  const WUInt32 uiNewHierarchyLevel = pParent != nullptr ? pParent->m_uiHierarchyLevel + 1 : 0;
  const WUInt32 uiOldHierarchyLevel = pObject->m_uiHierarchyLevel;

  const bool bIsDynamic = pObject->IsDynamic();

  if (uiNewHierarchyLevel != uiOldHierarchyLevel || bIsDynamic != bWasDynamic)
  {
    WGameObject::TransformationData* pOldTransformationData = pObject->m_pTransformationData;

    WGameObject::TransformationData* pNewTransformationData = m_Data.CreateTransformationData(bIsDynamic, uiNewHierarchyLevel);
    WMemoryUtils::Copy(pNewTransformationData, pOldTransformationData, 1);

    pObject->m_uiHierarchyLevel = static_cast<WUInt16>(uiNewHierarchyLevel);
    pObject->m_pTransformationData = pNewTransformationData;

    // fix parent transform data for children as well
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      WGameObject::TransformationData* pTransformData = it->m_pTransformationData;
      pTransformData->m_pParentData = pNewTransformationData;
    }

    m_Data.DeleteTransformationData(bWasDynamic, uiOldHierarchyLevel, pOldTransformationData);
  }
}

void WWorld::ProcessResourceReloadFunctions()
{
  ResourceReloadContext context;
  context.m_pWorld = this;

  for (auto& hResource : m_Data.m_NeedReload)
  {
    if (m_Data.m_ReloadFunctions.TryGetValue(hResource, m_Data.m_TempReloadFunctions))
    {
      for (auto& data : m_Data.m_TempReloadFunctions)
      {
        W_VERIFY(data.m_hComponent.IsInvalidated() || TryGetComponent(data.m_hComponent, context.m_pComponent), "Reload function called on dead component");
        context.m_pUserData = data.m_pUserData;

        data.m_Func(context);
      }
    }
  }

  m_Data.m_NeedReload.Clear();
}

void WWorld::QueueLocalBoundsUpdate(WGameObjectHandle hObject)
{
  W_LOCK(m_Data.m_BoundsUpdateMutex);
  m_Data.m_BoundsUpdateQueue.PushBack(hObject);
}

void WWorld::ProcessLocalBoundsUpdateQueue()
{
  W_LOCK(m_Data.m_BoundsUpdateMutex);
  for (auto& hObj : m_Data.m_BoundsUpdateQueue)
  {
    WGameObject* pObj;
    if (TryGetObject(hObj, pObj))
    {
      pObj->UpdateLocalBounds();
    }
  }

  m_Data.m_BoundsUpdateQueue.Clear();
}

void WWorld::SetMaxInitializationTimePerFrame(WTime maxInitTime)
{
  CheckForWriteAccess();

  m_Data.m_MaxInitializationTimePerFrame = maxInitTime;
}

void WWorld::SetGameObjectReferenceResolver(const ReferenceResolver& resolver)
{
  m_Data.m_GameObjectReferenceResolver = resolver;
}

const WWorld::ReferenceResolver& WWorld::GetGameObjectReferenceResolver() const
{
  return m_Data.m_GameObjectReferenceResolver;
}

void WWorld::AddResourceReloadFunction(WTypelessResourceHandle hResource, WComponentHandle hComponent, void* pUserData, ResourceReloadFunc function)
{
  CheckForWriteAccess();

  if (hResource.IsValid() == false)
    return;

  auto& data = m_Data.m_ReloadFunctions[hResource].ExpandAndGetRef();
  data.m_hComponent = hComponent;
  data.m_pUserData = pUserData;
  data.m_Func = function;
}

void WWorld::RemoveResourceReloadFunction(WTypelessResourceHandle hResource, WComponentHandle hComponent, void* pUserData)
{
  CheckForWriteAccess();

  WInternal::WorldData::ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_Data.m_ReloadFunctions.TryGetValue(hResource, pReloadFunctions))
  {
    for (WUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      auto& data = (*pReloadFunctions)[i];
      if (data.m_hComponent == hComponent && data.m_pUserData == pUserData)
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

WGameObject* WWorld::SearchForObject(WStringView sSearchPath, WGameObject* pRefObj, const WRTTI* pExpectedComponent)
{
  // Possible paths:
  //
  // rel/path
  // ../rel/path
  // ..
  // G:key/rel/path
  // P:parent/rel/path
  // G:key/P:parent/rel/path
  // G:key/../rel/path
  // G:key/..
  // P:parent/../rel/path

  // if the search string starts with "G:", the next part of the path is the global key of an object
  // in this case, this object is not the reference object anymore, instead the object with that global key is the reference object
  if (sSearchPath.TrimWordStart("G:"))
  {
    WStringView sGlobalKey;

    if (const char* szSep = sSearchPath.FindSubString("/"))
    {
      sGlobalKey = WStringView(sSearchPath.GetStartPointer(), szSep);
      sSearchPath.SetStartPosition(szSep + 1);
    }
    else
    {
      sGlobalKey = sSearchPath;
      sSearchPath = {};
    }

    if (!TryGetObjectWithGlobalKey(WTempHashedString(sGlobalKey), pRefObj))
    {
      return nullptr;
    }
  }

  if (pRefObj == nullptr)
    return nullptr;

  // if the search string starts with "P:", the next part of the path is an object name of a parent object
  // of the reference object, so we search upwards until we find the object with that name
  if (sSearchPath.TrimWordStart("P:"))
  {
    WStringView sParentName;

    if (const char* szSep = sSearchPath.FindSubString("/"))
    {
      sParentName = WStringView(sSearchPath.GetStartPointer(), szSep);
      sSearchPath.SetStartPosition(szSep + 1);
    }
    else
    {
      sParentName = sSearchPath;
      sSearchPath = {};
    }

    const WTempHashedString sStartName(sParentName);
    while (!pRefObj->HasName(sStartName))
    {
      pRefObj = pRefObj->GetParent();

      if (pRefObj == nullptr)
        return nullptr;
    }
  }

  // if the path contains "..", we go up one parent
  // this is only allowed at the start of the relative path section
  while (sSearchPath.TrimWordStart("../") || sSearchPath.TrimWordStart(".."))
  {
    pRefObj = pRefObj->GetParent();

    if (pRefObj == nullptr)
      return nullptr;
  }

  return pRefObj->SearchForChildByNameSequence(sSearchPath, pExpectedComponent);
}


const WGameObject* WWorld::SearchForObject(WStringView sSearchPath, const WGameObject* pReferenceObject /*= nullptr*/, const WRTTI* pExpectedComponent /*= nullptr*/) const
{
  WWorld* pThis = const_cast<WWorld*>(this);
  WGameObject* pRef = const_cast<WGameObject*>(pReferenceObject);
  return pThis->SearchForObject(sSearchPath, pRef, pExpectedComponent);
}

W_STATICLINK_FILE(Core, Core_World_Implementation_World);
