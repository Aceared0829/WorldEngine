#include <Core/CorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/Progress.h>

WWorldReader::FindComponentTypeCallback WWorldReader::s_FindComponentTypeCallback;

thread_local WWorldReader::InstantiationContextBase* tl_pReaderContext = nullptr;

WWorldReader::WWorldReader() = default;
WWorldReader::~WWorldReader() = default;

WResult WWorldReader::ReadWorldDescription(WStreamReader& inout_stream, bool bWarningOnUnknownSkip)
{
  m_pReadStream = &inout_stream;

  m_uiVersion = 0;
  inout_stream >> m_uiVersion;

  if (m_uiVersion < 8 || m_uiVersion > 10)
  {
    WLog::Error("Invalid world version (got {}).", m_uiVersion);
    return W_FAILURE;
  }

  // destroy old context first
  m_pStringDedupReadContext = nullptr;
  m_pStringDedupReadContext = W_DEFAULT_NEW(WStringDeduplicationReadContext, inout_stream);

  if (m_uiVersion == 8)
  {
    // add tags from the stream
    W_SUCCEED_OR_RETURN(WTagRegistry::GetGlobalRegistry().Load(inout_stream));
  }

  WUInt32 uiNumRootObjects = 0;
  inout_stream >> uiNumRootObjects;

  WUInt32 uiNumChildObjects = 0;
  inout_stream >> uiNumChildObjects;

  WUInt32 uiNumComponentTypes = 0;
  inout_stream >> uiNumComponentTypes;

  if (uiNumComponentTypes > WMath::MaxValue<WUInt16>())
  {
    WLog::Error("World description has too many component types, got {0} - maximum allowed are {1}", uiNumComponentTypes, WMath::MaxValue<WUInt16>());
    return W_FAILURE;
  }

  m_RootObjectsToCreate.Reserve(uiNumRootObjects);
  m_ChildObjectsToCreate.Reserve(uiNumChildObjects);

  for (WUInt32 i = 0; i < uiNumRootObjects; ++i)
  {
    ReadGameObjectDesc(m_RootObjectsToCreate.ExpandAndGetRef());
  }

  for (WUInt32 i = 0; i < uiNumChildObjects; ++i)
  {
    ReadGameObjectDesc(m_ChildObjectsToCreate.ExpandAndGetRef());
  }

  m_ComponentTypes.SetCount(uiNumComponentTypes);
  m_ComponentTypeVersions.Reserve(uiNumComponentTypes);
  for (WUInt32 i = 0; i < uiNumComponentTypes; ++i)
  {
    ReadComponentTypeInfo(i);
  }

  // read all component data
  ReadComponentDataToMemStream(bWarningOnUnknownSkip);
  m_pStringDedupReadContext->SetActive(false);

  return W_SUCCESS;
}

WUniquePtr<WWorldReader::InstantiationContextBase> WWorldReader::InstantiateWorld(WWorld& ref_world, const WUInt16* pOverrideTeamID, WTime maxStepTime, WProgress* pProgress)
{
  WPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = pOverrideTeamID;
  options.m_MaxStepTime = maxStepTime;
  options.m_pProgress = pProgress;
  options.m_RandomSeedMode = WPrefabInstantiationOptions::RandomSeedMode::FixedFromSerialization;

  return Instantiate(ref_world, false, WTransform(), options);
}

WUniquePtr<WWorldReader::InstantiationContextBase> WWorldReader::InstantiatePrefab(WWorld& ref_world, const WTransform& rootTransform, const WPrefabInstantiationOptions& options)
{
  return Instantiate(ref_world, true, rootTransform, options);
}

WStreamReader& WWorldReader::GetStream() const
{
  WWorldReader::InstantiationContext* pContext = ((WWorldReader::InstantiationContext*)tl_pReaderContext);

  return pContext->m_CurrentReader;
}

WGameObjectHandle WWorldReader::ReadGameObjectHandle()
{
  WWorldReader::InstantiationContext* pContext = ((WWorldReader::InstantiationContext*)tl_pReaderContext);

  WUInt32 idx = 0;
  pContext->m_CurrentReader >> idx;

  return pContext->m_IndexToGameObjectHandle[idx];
}

void WWorldReader::ReadComponentHandle(WComponentHandle& out_hComponent)
{
  WWorldReader::InstantiationContext* pContext = ((WWorldReader::InstantiationContext*)tl_pReaderContext);

  WUInt16 uiTypeIndex = 0;
  WUInt32 uiIndex = 0;

  pContext->m_CurrentReader >> uiTypeIndex;
  pContext->m_CurrentReader >> uiIndex;

  out_hComponent.Invalidate();

  if (uiTypeIndex < m_ComponentTypes.GetCount())
  {
    auto& indexToHandle = pContext->m_ComponentTypeStates[uiTypeIndex].m_ComponentIndexToHandle;
    if (uiIndex < indexToHandle.GetCount())
    {
      out_hComponent = indexToHandle[uiIndex];
    }
  }
}

WUInt32 WWorldReader::GetComponentTypeVersion(const WRTTI* pRtti) const
{
  WUInt32 uiVersion = 0xFFFFFFFF;
  m_ComponentTypeVersions.TryGetValue(pRtti, uiVersion);

  return uiVersion;
}

bool WWorldReader::HasComponentOfType(const WRTTI* pRtti) const
{
  return m_ComponentTypeVersions.Contains(pRtti);
}

void WWorldReader::ClearAndCompact()
{
  m_RootObjectsToCreate.Clear();
  m_RootObjectsToCreate.Compact();

  m_ChildObjectsToCreate.Clear();
  m_ChildObjectsToCreate.Compact();

  m_ComponentTypes.Clear();
  m_ComponentTypes.Compact();

  m_ComponentTypeVersions.Clear();
  m_ComponentTypeVersions.Compact();

  m_ComponentCreationStream.Clear();
  m_ComponentCreationStream.Compact();

  m_ComponentDataStream.Clear();
  m_ComponentDataStream.Compact();
}

WUInt64 WWorldReader::GetHeapMemoryUsage() const
{
  return m_RootObjectsToCreate.GetHeapMemoryUsage() + m_ChildObjectsToCreate.GetHeapMemoryUsage() +
         m_ComponentTypes.GetHeapMemoryUsage() + m_ComponentTypeVersions.GetHeapMemoryUsage() +
         m_ComponentCreationStream.GetHeapMemoryUsage() + m_ComponentDataStream.GetHeapMemoryUsage();
}

WUInt32 WWorldReader::GetRootObjectCount() const
{
  return m_RootObjectsToCreate.GetCount();
}


WUInt32 WWorldReader::GetChildObjectCount() const
{
  return m_ChildObjectsToCreate.GetCount();
}

void WWorldReader::SetMaxStepTime(InstantiationContextBase* pContext, WTime maxStepTime)
{
  return static_cast<InstantiationContext*>(pContext)->SetMaxStepTime(maxStepTime);
}

WTime WWorldReader::GetMaxStepTime(InstantiationContextBase* pContext)
{
  return static_cast<InstantiationContext*>(pContext)->GetMaxStepTime();
}

void WWorldReader::ReadGameObjectDesc(GameObjectToCreate& godesc)
{
  WGameObjectDesc& desc = godesc.m_Desc;
  WStringBuilder sName, sGlobalKey;

  *m_pReadStream >> godesc.m_uiParentHandleIdx;
  *m_pReadStream >> sName;

  *m_pReadStream >> sGlobalKey;
  godesc.m_sGlobalKey = sGlobalKey;

  *m_pReadStream >> desc.m_LocalPosition;
  *m_pReadStream >> desc.m_LocalRotation;
  *m_pReadStream >> desc.m_LocalScaling;
  *m_pReadStream >> desc.m_LocalUniformScaling;

  *m_pReadStream >> desc.m_bActiveFlag;
  *m_pReadStream >> desc.m_bDynamic;

  desc.m_Tags.Load(*m_pReadStream, WTagRegistry::GetGlobalRegistry());

  *m_pReadStream >> desc.m_uiTeamID;

  desc.m_sName.Assign(sName.GetData());

  if (m_uiVersion >= 10)
  {
    *m_pReadStream >> desc.m_uiStableRandomSeed;
  }
}

void WWorldReader::ReadComponentTypeInfo(WUInt32 uiComponentTypeIdx)
{
  WStreamReader& s = *m_pReadStream;

  WStringBuilder sRttiName;
  WUInt32 uiRttiVersion = 0;

  s >> sRttiName;
  s >> uiRttiVersion;

  const WRTTI* pRtti = nullptr;

  if (s_FindComponentTypeCallback.IsValid())
  {
    pRtti = s_FindComponentTypeCallback(sRttiName);
  }
  else
  {
    pRtti = WRTTI::FindTypeByName(sRttiName);

    if (pRtti == nullptr)
    {
      WLog::Error("Unknown component type '{0}'. Components of this type will be skipped.", sRttiName);
    }
  }

  m_ComponentTypes[uiComponentTypeIdx].m_pRtti = pRtti;
  m_ComponentTypeVersions[pRtti] = uiRttiVersion;
}

void WWorldReader::ReadComponentDataToMemStream(bool warningOnUnknownSkip)
{
  auto WriteToMemStream = [&](WMemoryStreamWriter& ref_writer, bool bReadNumComponents)
  {
    WUInt8 Temp[4096];
    for (auto& compTypeInfo : m_ComponentTypes)
    {
      WUInt32 uiAllComponentsSize = 0;
      *m_pReadStream >> uiAllComponentsSize;

      if (compTypeInfo.m_pRtti == nullptr)
      {
        if (warningOnUnknownSkip)
        {
          WLog::Warning("Skipping components of unknown type");
        }

        m_pReadStream->SkipBytes(uiAllComponentsSize);
      }
      else
      {
        if (bReadNumComponents)
        {
          *m_pReadStream >> compTypeInfo.m_uiNumComponents;
          uiAllComponentsSize -= sizeof(WUInt32);

          m_uiTotalNumComponents += compTypeInfo.m_uiNumComponents;
        }

        compTypeInfo.m_uiComponentDataSize = uiAllComponentsSize;

        while (uiAllComponentsSize > 0)
        {
          const WUInt64 uiRead = m_pReadStream->ReadBytes(Temp, WMath::Min<WUInt32>(uiAllComponentsSize, W_ARRAY_SIZE(Temp)));

          ref_writer.WriteBytes(Temp, uiRead).IgnoreResult();

          uiAllComponentsSize -= (WUInt32)uiRead;
        }
      }
    }
  };

  {
    WMemoryStreamWriter writer(&m_ComponentCreationStream);
    WriteToMemStream(writer, true);
  }

  {
    WMemoryStreamWriter writer(&m_ComponentDataStream);
    WriteToMemStream(writer, false);
  }
}

WUniquePtr<WWorldReader::InstantiationContextBase> WWorldReader::Instantiate(WWorld& world, bool bUseTransform, const WTransform& rootTransform, const WPrefabInstantiationOptions& options)
{
  if (options.m_MaxStepTime <= WTime::MakeZero())
  {
    InstantiationContext context = InstantiationContext(*this, &world, bUseTransform, rootTransform, options, WTempAllocator::Get());

    W_VERIFY(context.Step() == InstantiationContextBase::StepResult::Finished, "Instantiation should be completed after this call");
    return nullptr;
  }

  WUniquePtr<InstantiationContext> pContext = W_DEFAULT_NEW(InstantiationContext, *this, &world, bUseTransform, rootTransform, options, WFoundation::GetDefaultAllocator());

  return std::move(pContext);
}

WWorldReader::InstantiationContext::InstantiationContext(WWorldReader& ref_worldReader, WWorld* pWorld, bool bUseTransform, const WTransform& rootTransform, const WPrefabInstantiationOptions& options, WAllocator* pAllocator)
  : m_WorldReader(ref_worldReader)
  , m_bUseTransform(bUseTransform)
  , m_RootTransform(rootTransform)
  , m_Options(options)
  , m_IndexToGameObjectHandle(pAllocator)
  , m_ComponentTypeStates(pAllocator)
{
  m_Phase = Phase::CreateRootObjects;

  m_pWorld = pWorld;

  const WUInt32 uiRootObjectsToCreate = m_WorldReader.m_RootObjectsToCreate.GetCount();
  const WUInt32 uiChildObjectsToCreate = m_WorldReader.m_ChildObjectsToCreate.GetCount();

  m_IndexToGameObjectHandle.Reserve(uiRootObjectsToCreate + uiChildObjectsToCreate + 1);
  m_IndexToGameObjectHandle.PushBack(WGameObjectHandle());

  m_ComponentTypeStates.Reserve(m_WorldReader.m_ComponentTypes.GetCount());
  for (WUInt32 i = 0; i < m_WorldReader.m_ComponentTypes.GetCount(); ++i)
  {
    m_ComponentTypeStates.PushBack(ComponentTypeState(pAllocator));

    auto& ct = m_ComponentTypeStates.PeekBack();
    ct.m_ComponentIndexToHandle.Reserve(m_WorldReader.m_ComponentTypes[i].m_uiNumComponents + 1);
    ct.m_ComponentIndexToHandle.PushBack(WComponentHandle());
  }

  if (m_Options.m_MaxStepTime.IsZeroOrNegative())
  {
    m_Options.m_MaxStepTime = WTime::MakeFromHours(24 * 365);
  }

  if (options.m_MaxStepTime.IsPositive())
  {
    m_hComponentInitBatch = m_pWorld->CreateComponentInitBatch("WorldReaderBatch", options.m_MaxStepTime.IsPositive() ? false : true);
  }

  if (options.m_pProgress != nullptr)
  {
    m_pOverallProgressRange = W_DEFAULT_NEW(WProgressRange, "Instantiate", Phase::Count, false, options.m_pProgress);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateRootObjects, uiRootObjectsToCreate / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateChildObjects, uiChildObjectsToCreate / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateComponents, m_WorldReader.m_uiTotalNumComponents / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::DeserializeComponents, m_WorldReader.m_uiTotalNumComponents / 100.0f);
    // Ten times more weight since init components takes way longer than the rest
    m_pOverallProgressRange->SetStepWeighting(Phase::InitComponents, m_WorldReader.m_uiTotalNumComponents / 10.0f);

    m_pOverallProgressRange->BeginNextStep("CreateRootObjects");
  }
}

WWorldReader::InstantiationContext::~InstantiationContext()
{
  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->DeleteComponentInitBatch(m_hComponentInitBatch);
    m_hComponentInitBatch.Invalidate();
  }
}

WWorldReader::InstantiationContext::StepResult WWorldReader::InstantiationContext::Step()
{
  W_ASSERT_DEV(m_Phase != Phase::Invalid, "InstantiationContext cannot be re-used.");

  W_PROFILE_SCOPE("WWorldReader::InstContext::Step");

  W_LOCK(m_pWorld->GetWriteMarker());

  WTime endTime = WTime::Now() + m_Options.m_MaxStepTime;

  if (m_Phase == Phase::CreateRootObjects)
  {
    if (!m_Options.m_ReplaceNamedRootWithParent.IsEmpty())
    {
      W_ASSERT_DEBUG(!m_Options.m_hParent.IsInvalidated(), "Parent must be provided when m_ReplaceNamedRootWithParent is specified.");

      if (m_WorldReader.m_RootObjectsToCreate.GetCount() == 1 && m_WorldReader.m_RootObjectsToCreate[0].m_Desc.m_sName == m_Options.m_ReplaceNamedRootWithParent)
      {
        m_uiCurrentIndex = 1;
        W_ASSERT_DEBUG(m_IndexToGameObjectHandle.GetCapacity() > m_IndexToGameObjectHandle.GetCount(), "m_IndexToGameObjectHandle should have enough capacity.");
        m_IndexToGameObjectHandle.PushBack(m_Options.m_hParent);

        WGameObject* pParent = nullptr;
        if (m_pWorld->TryGetObject(m_Options.m_hParent, pParent))
        {
          if (m_Options.m_pCreatedRootObjectsOut)
          {
            m_Options.m_pCreatedRootObjectsOut->PushBack(pParent);
          }

          if (m_WorldReader.m_RootObjectsToCreate[0].m_Desc.m_bDynamic)
          {
            pParent->MakeDynamic();
          }

          auto& tags = m_WorldReader.m_RootObjectsToCreate[0].m_Desc.m_Tags;
          if (!tags.IsEmpty())
          {
            // add all the tags from the instantiated object
            for (auto it = tags.GetIterator(); it.IsValid(); ++it)
            {
              pParent->SetTag(*it);
            }
          }
        }
      }
    }

    if (m_bUseTransform)
    {
      if (!CreateGameObjects<true>(m_WorldReader.m_RootObjectsToCreate, m_Options.m_hParent, m_Options.m_pCreatedRootObjectsOut, endTime))
        return StepResult::Continue;
    }
    else
    {
      if (!CreateGameObjects<false>(m_WorldReader.m_RootObjectsToCreate, m_Options.m_hParent, m_Options.m_pCreatedRootObjectsOut, endTime))
        return StepResult::Continue;
    }

    m_Phase = Phase::CreateChildObjects;
    BeginNextProgressStep("CreateChildObjects");
  }

  if (m_Phase == Phase::CreateChildObjects)
  {
    if (!CreateGameObjects<false>(m_WorldReader.m_ChildObjectsToCreate, WGameObjectHandle(), m_Options.m_pCreatedChildObjectsOut, endTime))
      return StepResult::Continue;

    m_CurrentReader.SetStorage(&m_WorldReader.m_ComponentCreationStream);
    m_Phase = Phase::CreateComponents;
    BeginNextProgressStep("CreateComponents");
  }

  if (m_Phase == Phase::CreateComponents)
  {
    if (m_WorldReader.m_ComponentCreationStream.GetStorageSize64() > 0)
    {
      m_WorldReader.m_pStringDedupReadContext->SetActive(true);
      tl_pReaderContext = this;

      // WStreamReader* pPrevReader = m_WorldReader.m_pStream;
      // m_WorldReader.m_pStream = &m_CurrentReader;

      W_SCOPE_EXIT(/*m_WorldReader.m_pStream = pPrevReader; */ m_WorldReader.m_pStringDedupReadContext->SetActive(false); tl_pReaderContext = nullptr;);

      if (!CreateComponents(endTime))
        return StepResult::Continue;
    }

    m_CurrentReader.SetStorage(&m_WorldReader.m_ComponentDataStream);
    m_Phase = Phase::DeserializeComponents;
    BeginNextProgressStep("DeserializeComponents");
  }

  if (m_Phase == Phase::DeserializeComponents)
  {
    if (m_WorldReader.m_ComponentDataStream.GetStorageSize64() > 0)
    {
      m_WorldReader.m_pStringDedupReadContext->SetActive(true);
      tl_pReaderContext = this;

      // WStreamReader* pPrevReader = m_WorldReader.m_pStream;
      // m_WorldReader.m_pStream = &m_CurrentReader;

      W_SCOPE_EXIT(/*m_WorldReader.m_pStream = pPrevReader;*/ m_WorldReader.m_pStringDedupReadContext->SetActive(false); tl_pReaderContext = nullptr;);

      if (!DeserializeComponents(endTime))
        return StepResult::Continue;
    }

    m_CurrentReader.SetStorage(nullptr);
    m_Phase = Phase::AddComponentsToBatch;
    BeginNextProgressStep("AddComponentsToBatch");
  }

  if (m_Phase == Phase::AddComponentsToBatch)
  {
    if (!AddComponentsToBatch(endTime))
      return StepResult::Continue;

    m_Phase = Phase::InitComponents;
    BeginNextProgressStep("InitComponents");
  }

  if (m_Phase == Phase::InitComponents)
  {
    if (!m_hComponentInitBatch.IsInvalidated())
    {
      double fCompletionFactor = 0.0;
      if (!m_pWorld->IsComponentInitBatchCompleted(m_hComponentInitBatch, &fCompletionFactor))
      {
        SetSubProgressCompletion(fCompletionFactor);
        return StepResult::ContinueNextFrame;
      }
    }

    m_Phase = Phase::Invalid;
    m_pSubProgressRange = nullptr;
    m_pOverallProgressRange = nullptr;
  }

  return StepResult::Finished;
}

void WWorldReader::InstantiationContext::Cancel()
{
  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->CancelComponentInitBatch(m_hComponentInitBatch);
  }

  m_Phase = Phase::Invalid;
  m_pSubProgressRange = nullptr;
  m_pOverallProgressRange = nullptr;
}

// a super simple, but also efficient random number generator
inline static WUInt32 NextStableRandomSeed(WUInt32& ref_uiSeed)
{
  ref_uiSeed = 214013L * ref_uiSeed + 2531011L;
  return ((ref_uiSeed >> 16) & 0x7FFFF);
}

template <bool UseTransform>
bool WWorldReader::InstantiationContext::CreateGameObjects(const WDynamicArray<GameObjectToCreate>& objects, WGameObjectHandle hParent, WDynamicArray<WGameObject*>* out_pCreatedObjects, WTime endTime)
{
  W_PROFILE_SCOPE("WWorldReader::CreateGameObjects");

  while (m_uiCurrentIndex < objects.GetCount())
  {
    auto& godesc = objects[m_uiCurrentIndex];

    WGameObjectDesc desc = godesc.m_Desc; // make a copy
    desc.m_hParent = hParent.IsInvalidated() ? m_IndexToGameObjectHandle[godesc.m_uiParentHandleIdx] : hParent;
    desc.m_bDynamic |= m_Options.m_bForceDynamic;

    switch (m_Options.m_RandomSeedMode)
    {
      case WPrefabInstantiationOptions::RandomSeedMode::DeterministicFromParent:
        desc.m_uiStableRandomSeed = 0xFFFFFFFF; // WWorld::CreateObject() will either derive a deterministic value from the parent object, or assign a random value, if no parent exists
        break;

      case WPrefabInstantiationOptions::RandomSeedMode::CompletelyRandom:
        desc.m_uiStableRandomSeed = 0; // WWorld::CreateObject() will assign a random value to this object
        break;

      case WPrefabInstantiationOptions::RandomSeedMode::FixedFromSerialization:
        // keep deserialized value
        break;

      case WPrefabInstantiationOptions::RandomSeedMode::CustomRootValue:
        // we use the given seed root value to assign a deterministic (but different) value to each game object
        desc.m_uiStableRandomSeed = NextStableRandomSeed(m_Options.m_uiCustomRandomSeedRootValue);
        break;
    }

    if (m_Options.m_pOverrideTeamID != nullptr)
    {
      desc.m_uiTeamID = *m_Options.m_pOverrideTeamID;
    }

    if (UseTransform)
    {
      WTransform tChild(desc.m_LocalPosition, desc.m_LocalRotation, desc.m_LocalScaling);
      WTransform tFinal;
      tFinal = WTransform::MakeGlobalTransform(m_RootTransform, tChild);

      desc.m_LocalPosition = tFinal.m_vPosition;
      desc.m_LocalRotation = tFinal.m_qRotation;
      desc.m_LocalScaling = tFinal.m_vScale;
    }

    WGameObject* pObject = nullptr;
    W_ASSERT_DEBUG(m_IndexToGameObjectHandle.GetCapacity() > m_IndexToGameObjectHandle.GetCount(), "m_IndexToGameObjectHandle should have enough capacity.");
    m_IndexToGameObjectHandle.PushBack(m_pWorld->CreateObject(desc, pObject));

    if (!godesc.m_sGlobalKey.IsEmpty())
    {
      pObject->SetGlobalKey(godesc.m_sGlobalKey);
    }

    if (out_pCreatedObjects)
    {
      out_pCreatedObjects->PushBack(pObject);
    }

    ++m_uiCurrentIndex;

    // exit here to ensure that we at least did some work
    if (WTime::Now() >= endTime)
    {
      SetSubProgressCompletion(static_cast<double>(m_uiCurrentIndex) / objects.GetCount());
      return false;
    }
  }

  m_uiCurrentIndex = 0;

  return true;
}

bool WWorldReader::InstantiationContext::CreateComponents(WTime endTime)
{
  W_PROFILE_SCOPE("WWorldReader::CreateComponents");

  WStreamReader& s = m_CurrentReader;

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    const auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    auto& compTypeState = m_ComponentTypeStates[m_uiCurrentComponentTypeIndex];

    // will be the case for all abstract component types
    if (compTypeInfo.m_pRtti == nullptr || compTypeInfo.m_uiNumComponents == 0)
      continue;

    WComponentManagerBase* pManager = m_pWorld->GetOrCreateManagerForComponentType(compTypeInfo.m_pRtti);
    W_ASSERT_DEV(pManager != nullptr, "Cannot create components of type '{0}', manager is not available.", compTypeInfo.m_pRtti->GetTypeName());

    while (m_uiCurrentIndex < compTypeInfo.m_uiNumComponents)
    {
      const WGameObjectHandle hOwner = m_WorldReader.ReadGameObjectHandle();

      WUInt32 uiComponentIdx = 0;
      s >> uiComponentIdx;

      bool bActive = true;
      s >> bActive;

      WUInt8 userFlags = 0;
      s >> userFlags;

      WGameObject* pOwnerObject = nullptr;
      if (!m_pWorld->TryGetObject(hOwner, pOwnerObject))
      {
        W_REPORT_FAILURE("Owner object must not be null");
      }

      WComponent* pComponent = nullptr;
      auto hComponent = pManager->CreateComponentNoInit(pOwnerObject, pComponent);

      pComponent->SetActiveFlag(bActive);

      for (WUInt8 j = 0; j < 8; ++j)
      {
        pComponent->SetUserFlag(j, (userFlags & W_BIT(j)) != 0);
      }

      W_ASSERT_DEBUG(uiComponentIdx == compTypeState.m_ComponentIndexToHandle.GetCount(), "Component index doesn't match");
      W_ASSERT_DEBUG(compTypeState.m_ComponentIndexToHandle.GetCapacity() > compTypeState.m_ComponentIndexToHandle.GetCount(), "m_ComponentIndexToHandle should have enough capacity.");
      compTypeState.m_ComponentIndexToHandle.PushBack(hComponent);

      ++m_uiCurrentIndex;
      ++m_uiCurrentNumComponentsProcessed;

      // exit here to ensure that we at least did some work
      if (WTime::Now() >= endTime)
      {
        SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);
        return false;
      }
    }

    m_uiCurrentIndex = 0;
  }

  m_uiCurrentIndex = 0;
  m_uiCurrentComponentTypeIndex = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

bool WWorldReader::InstantiationContext::DeserializeComponents(WTime endTime)
{
  W_PROFILE_SCOPE("WWorldReader::DeserializeComponents");

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    const auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    if (compTypeInfo.m_pRtti == nullptr)
      continue;

    auto& compTypeState = m_ComponentTypeStates[m_uiCurrentComponentTypeIndex];

    if (m_uiCurrentIndex == 0)
    {
      compTypeState.m_uiDataReadOffset = m_CurrentReader.GetReadPosition();
    }

    while (m_uiCurrentIndex < compTypeState.m_ComponentIndexToHandle.GetCount())
    {
      WComponent* pComponent = nullptr;
      if (m_pWorld->TryGetComponent(compTypeState.m_ComponentIndexToHandle[m_uiCurrentIndex++], pComponent))
      {
        pComponent->DeserializeComponent(m_WorldReader);

        ++m_uiCurrentNumComponentsProcessed;

        // exit here to ensure that we at least did some work
        if (WTime::Now() >= endTime)
        {
          SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);
          return false;
        }
      }
    }

    const WUInt64 uiBytesRead = m_CurrentReader.GetReadPosition() - compTypeState.m_uiDataReadOffset;

    if (uiBytesRead != compTypeInfo.m_uiComponentDataSize)
    {
      W_REPORT_FAILURE("Component type '{}' (version {}) deserialized {} of the stored {} bytes.\nCheck that the serialization and deserialization functions assume the same data layout.", compTypeInfo.m_pRtti->GetTypeName(), compTypeInfo.m_pRtti->GetTypeVersion(), uiBytesRead, compTypeInfo.m_uiComponentDataSize);
    }

    m_uiCurrentIndex = 0;
  }

  m_uiCurrentIndex = 0;
  m_uiCurrentComponentTypeIndex = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

bool WWorldReader::InstantiationContext::AddComponentsToBatch(WTime endTime)
{
  W_PROFILE_SCOPE("WWorldReader::AddComponentsToBatch");

  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->BeginAddingComponentsToInitBatch(m_hComponentInitBatch);
  }

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    const auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    if (compTypeInfo.m_pRtti == nullptr)
      continue;

    const auto& compTypeState = m_ComponentTypeStates[m_uiCurrentComponentTypeIndex];

    while (m_uiCurrentIndex < compTypeState.m_ComponentIndexToHandle.GetCount())
    {
      WComponent* pComponent = nullptr;
      if (m_pWorld->TryGetComponent(compTypeState.m_ComponentIndexToHandle[m_uiCurrentIndex++], pComponent))
      {
        pComponent->GetOwningManager()->InitializeComponent(pComponent);

        ++m_uiCurrentNumComponentsProcessed;

        // exit here to ensure that we at least did some work
        if (WTime::Now() >= endTime)
        {
          SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);

          if (!m_hComponentInitBatch.IsInvalidated())
          {
            m_pWorld->EndAddingComponentsToInitBatch(m_hComponentInitBatch);
          }
          return false;
        }
      }
    }

    m_uiCurrentIndex = 0;
  }

  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->SubmitComponentInitBatch(m_hComponentInitBatch);
  }

  m_uiCurrentIndex = 0;
  m_uiCurrentComponentTypeIndex = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

void WWorldReader::InstantiationContext::SetMaxStepTime(WTime stepTime)
{
  m_Options.m_MaxStepTime = stepTime;
}

WTime WWorldReader::InstantiationContext::GetMaxStepTime() const
{
  return m_Options.m_MaxStepTime;
}

void WWorldReader::InstantiationContext::BeginNextProgressStep(WStringView sName)
{
  if (m_pOverallProgressRange != nullptr)
  {
    m_pOverallProgressRange->BeginNextStep(sName);
    m_pSubProgressRange = nullptr;
    m_pSubProgressRange = W_DEFAULT_NEW(WProgressRange, sName, false, m_pOverallProgressRange->GetProgressbar());
  }
}

void WWorldReader::InstantiationContext::SetSubProgressCompletion(double fCompletion)
{
  if (m_pSubProgressRange != nullptr)
  {
    m_pSubProgressRange->SetCompletion(fCompletion);
  }
}
