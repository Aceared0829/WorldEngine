
namespace WInternal
{
  // static
  W_ALWAYS_INLINE WorldData::HierarchyType::Enum WorldData::GetHierarchyType(bool bIsDynamic)
  {
    return bIsDynamic ? HierarchyType::Dynamic : HierarchyType::Static;
  }

  // static
  template <typename VISITOR>
  W_FORCE_INLINE WVisitorExecution::Enum WorldData::TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData /* = nullptr*/)
  {
    for (WorldData::Hierarchy::DataBlock& block : blocks)
    {
      WGameObject::TransformationData* pCurrentData = block.m_pData;
      WGameObject::TransformationData* pEndData = block.m_pData + block.m_uiCount;

      while (pCurrentData < pEndData)
      {
        WVisitorExecution::Enum execution = VISITOR::Visit(pCurrentData, pUserData);
        if (execution != WVisitorExecution::Continue)
          return execution;

        ++pCurrentData;
      }
    }

    return WVisitorExecution::Continue;
  }

  // static
  template <typename VISITOR>
  W_FORCE_INLINE WVisitorExecution::Enum WorldData::TraverseHierarchyLevelMultiThreaded(
    Hierarchy::DataBlockArray& blocks, void* pUserData /* = nullptr*/)
  {
    WParallelForParams parallelForParams;
    parallelForParams.m_uiBinSize = 100;
    parallelForParams.m_uiMaxTasksPerThread = 2;
    parallelForParams.m_pTaskAllocator = m_LinearAllocator.GetCurrentAllocator();

    WTaskSystem::ParallelFor(
      blocks.GetArrayPtr(),
      [pUserData](WArrayPtr<WorldData::Hierarchy::DataBlock> blocksSlice)
      {
        for (WorldData::Hierarchy::DataBlock& block : blocksSlice)
        {
          WGameObject::TransformationData* pCurrentData = block.m_pData;
          WGameObject::TransformationData* pEndData = block.m_pData + block.m_uiCount;

          while (pCurrentData < pEndData)
          {
            VISITOR::Visit(pCurrentData, pUserData);
            ++pCurrentData;
          }
        }
      },
      "World DataBlock Traversal Task", parallelForParams);

    return WVisitorExecution::Continue;
  }

  // static
  W_FORCE_INLINE void WorldData::UpdateGlobalTransform(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter)
  {
    pData->UpdateGlobalTransformWithoutParent(uiUpdateCounter);
    pData->UpdateGlobalBounds();
  }

  // static
  W_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParent(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter)
  {
    pData->UpdateGlobalTransformWithParent(uiUpdateCounter);
    pData->UpdateGlobalBounds();
  }

  // static
  W_FORCE_INLINE void WorldData::UpdateGlobalTransformAndSpatialData(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter, WSpatialSystem& spatialSystem)
  {
    pData->UpdateGlobalTransformWithoutParent(uiUpdateCounter);
    pData->UpdateGlobalBoundsAndSpatialData(spatialSystem);
  }

  // static
  W_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParentAndSpatialData(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter, WSpatialSystem& spatialSystem)
  {
    pData->UpdateGlobalTransformWithParent(uiUpdateCounter);
    pData->UpdateGlobalBoundsAndSpatialData(spatialSystem);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  W_ALWAYS_INLINE const WGameObject& WorldData::ConstObjectIterator::operator*() const
  {
    return *m_Iterator;
  }

  W_ALWAYS_INLINE const WGameObject* WorldData::ConstObjectIterator::operator->() const
  {
    return m_Iterator;
  }

  W_ALWAYS_INLINE WorldData::ConstObjectIterator::operator const WGameObject*() const
  {
    return m_Iterator;
  }

  W_ALWAYS_INLINE void WorldData::ConstObjectIterator::Next()
  {
    m_Iterator.Next();

    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  W_ALWAYS_INLINE bool WorldData::ConstObjectIterator::IsValid() const
  {
    return m_Iterator.IsValid();
  }

  W_ALWAYS_INLINE void WorldData::ConstObjectIterator::operator++()
  {
    Next();
  }

  W_ALWAYS_INLINE WorldData::ConstObjectIterator::ConstObjectIterator(ObjectStorage::ConstIterator iterator)
    : m_Iterator(iterator)
  {
    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  W_ALWAYS_INLINE WGameObject& WorldData::ObjectIterator::operator*()
  {
    return *m_Iterator;
  }

  W_ALWAYS_INLINE WGameObject* WorldData::ObjectIterator::operator->()
  {
    return m_Iterator;
  }

  W_ALWAYS_INLINE WorldData::ObjectIterator::operator WGameObject*()
  {
    return m_Iterator;
  }

  W_ALWAYS_INLINE void WorldData::ObjectIterator::Next()
  {
    m_Iterator.Next();

    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  W_ALWAYS_INLINE bool WorldData::ObjectIterator::IsValid() const
  {
    return m_Iterator.IsValid();
  }

  W_ALWAYS_INLINE void WorldData::ObjectIterator::operator++()
  {
    Next();
  }

  W_ALWAYS_INLINE WorldData::ObjectIterator::ObjectIterator(ObjectStorage::Iterator iterator)
    : m_Iterator(iterator)
  {
    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  W_FORCE_INLINE WorldData::InitBatch::InitBatch(WAllocator* pAllocator, WStringView sName, bool bMustFinishWithinOneFrame)
    : m_bMustFinishWithinOneFrame(bMustFinishWithinOneFrame)
    , m_ComponentsToInitialize(pAllocator)
    , m_ComponentsToStartSimulation(pAllocator)
  {
    m_sName.Assign(sName);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  W_FORCE_INLINE void WorldData::RegisteredUpdateFunction::FillFromDesc(const WWorldModule::UpdateFunctionDesc& desc)
  {
    m_Function = desc.m_Function;
    m_sFunctionName = desc.m_sFunctionName;
    m_fPriority = desc.m_fPriority;
    m_uiAsyncPhaseBatchSize = desc.m_uiAsyncPhaseBatchSize;
    m_bOnlyUpdateWhenSimulating = desc.m_bOnlyUpdateWhenSimulating;
  }

  W_FORCE_INLINE bool WorldData::RegisteredUpdateFunction::operator<(const RegisteredUpdateFunction& other) const
  {
    // higher priority comes first
    if (m_fPriority != other.m_fPriority)
      return m_fPriority > other.m_fPriority;

    // sort by function name to ensure determinism
    WInt32 iNameComp = WStringUtils::Compare(m_sFunctionName, other.m_sFunctionName);
    W_ASSERT_DEV(iNameComp != 0, "An update function with the same name and same priority is already registered. This breaks determinism.");
    return iNameComp < 0;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  W_ALWAYS_INLINE WorldData::ReadMarker::ReadMarker(const WorldData& data)
    : m_Data(data)
  {
  }

  W_FORCE_INLINE void WorldData::ReadMarker::Lock()
  {
    W_ASSERT_DEV(m_Data.m_WriteThreadID == (WThreadID)0 || m_Data.m_WriteThreadID == WThreadUtils::GetCurrentThreadID(),
      "World '{0}' cannot be marked for reading because it is already marked for writing by another thread.", m_Data.m_sName);
    m_Data.m_iReadCounter.Increment();
  }

  W_ALWAYS_INLINE void WorldData::ReadMarker::Unlock()
  {
    m_Data.m_iReadCounter.Decrement();
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  W_ALWAYS_INLINE WorldData::WriteMarker::WriteMarker(WorldData& data)
    : m_Data(data)
  {
  }

  W_FORCE_INLINE void WorldData::WriteMarker::Lock()
  {
    // already locked by this thread?
    if (m_Data.m_WriteThreadID != WThreadUtils::GetCurrentThreadID())
    {
      W_ASSERT_DEV(m_Data.m_iReadCounter == 0, "World '{0}' cannot be marked for writing because it is already marked for reading.", m_Data.m_sName);
      W_ASSERT_DEV(m_Data.m_WriteThreadID == (WThreadID)0,
        "World '{0}' cannot be marked for writing because it is already marked for writing by another thread.", m_Data.m_sName);

      m_Data.m_WriteThreadID = WThreadUtils::GetCurrentThreadID();
      m_Data.m_iReadCounter.Increment(); // allow reading as well
    }

    m_Data.m_iWriteCounter++;
  }

  W_FORCE_INLINE void WorldData::WriteMarker::Unlock()
  {
    m_Data.m_iWriteCounter--;

    if (m_Data.m_iWriteCounter == 0)
    {
      m_Data.m_iReadCounter.Decrement();
      m_Data.m_WriteThreadID = (WThreadID)0;
    }
  }
} // namespace WInternal
