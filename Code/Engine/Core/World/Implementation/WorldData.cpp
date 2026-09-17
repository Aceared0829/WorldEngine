#include <Core/CorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Implementation/WorldData.h>
#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>

namespace WInternal
{
  class DefaultCoordinateSystemProvider : public WCoordinateSystemProvider
  {
  public:
    DefaultCoordinateSystemProvider()
      : WCoordinateSystemProvider(nullptr)
    {
    }

    virtual void GetCoordinateSystem(const WVec3& vGlobalPosition, WCoordinateSystem& out_coordinateSystem) const override
    {
      W_IGNORE_UNUSED(vGlobalPosition);

      out_coordinateSystem.m_vForwardDir = WVec3(1.0f, 0.0f, 0.0f);
      out_coordinateSystem.m_vRightDir = WVec3(0.0f, 1.0f, 0.0f);
      out_coordinateSystem.m_vUpDir = WVec3(0.0f, 0.0f, 1.0f);
    }
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  void WorldData::UpdateTask::Execute()
  {
    WWorldModule::UpdateContext context;
    context.m_uiFirstComponentIndex = m_uiStartIndex;
    context.m_uiComponentCount = m_uiCount;

    m_Function(context);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  WorldData::WorldData(WWorldDesc& desc)
    : m_sName(desc.m_sName)
    , m_Allocator(desc.m_sName, WFoundation::GetDefaultAllocator())
    , m_AllocatorWrapper(&m_Allocator)
    , m_BlockAllocator(desc.m_sName, &m_Allocator)
    , m_LinearAllocator(desc.m_sName, WFoundation::GetAlignedAllocator())
    , m_ObjectStorage(&m_BlockAllocator, &m_Allocator)
    , m_MaxInitializationTimePerFrame(desc.m_MaxComponentInitializationTimePerFrame)
    , m_Clock(desc.m_sName)
    , m_WriteThreadID((WThreadID)0)
    , m_bReportErrorWhenStaticObjectMoves(desc.m_bReportErrorWhenStaticObjectMoves)
    , m_ReadMarker(*this)
    , m_WriteMarker(*this)

  {
    m_AllocatorWrapper.Reset();

    if (desc.m_uiRandomNumberGeneratorSeed == 0)
    {
      m_Random.InitializeFromCurrentTime();
    }
    else
    {
      m_Random.Initialize(desc.m_uiRandomNumberGeneratorSeed);
    }

    // insert dummy entry to save some checks
    m_Objects.Insert(nullptr);

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
    static_assert(sizeof(WGameObject::TransformationData) == 240);
#else
    static_assert(sizeof(WGameObject::TransformationData) == 192);
#endif

    static_assert(sizeof(WGameObject) == 128);
    static_assert(W_COMPONENT_TYPE_INDEX_BITS <= sizeof(WWorldModuleTypeId) * 8);

    auto pDefaultInitBatch = W_NEW(&m_Allocator, InitBatch, &m_Allocator, "Default", true);
    pDefaultInitBatch->m_bIsReady = true;
    m_InitBatches.Insert(pDefaultInitBatch);
    m_pDefaultInitBatch = pDefaultInitBatch;
    m_pCurrentInitBatch = pDefaultInitBatch;

    // Spatial system
    {
      m_pSpatialSystem = std::move(desc.m_pSpatialSystem);

      if (m_pSpatialSystem == nullptr && desc.m_bAutoCreateSpatialSystem)
      {
        m_pSpatialSystem = W_NEW(WFoundation::GetAlignedAllocator(), WSpatialSystem_RegularGrid);
      }
    }

    // Coordinate system provider
    {
      m_pCoordinateSystemProvider = desc.m_pCoordinateSystemProvider;

      if (m_pCoordinateSystemProvider == nullptr)
      {
        m_pCoordinateSystemProvider = W_NEW(&m_Allocator, DefaultCoordinateSystemProvider);
      }
    }

    // Time step smoothing
    {
      m_pTimeStepSmoothing = std::move(desc.m_pTimeStepSmoothing);

      if (m_pTimeStepSmoothing == nullptr)
      {
        m_pTimeStepSmoothing = W_NEW(&m_Allocator, WDefaultTimeStepSmoothing);
      }

      m_Clock.SetTimeStepSmoothing(m_pTimeStepSmoothing.Borrow());
    }

    // Blackboard
    {
      m_pBlackboard = std::move(desc.m_pBlackboard);

      if (m_pBlackboard == nullptr)
      {
        // Can't use the world allocator here since blackboards use shared ownership and thus might outlive the world. Use the default allocator instead.
        m_pBlackboard = WBlackboard::Create(desc.m_sName);
      }
    }

    // BEGIN-DOCS-CODE-SNIPPET: resource-management-listen-all
    // Listening to all resource events
    WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WorldData::ResourceEventHandler, this));
    // END-DOCS-CODE-SNIPPET
  }

  WorldData::~WorldData()
  {
    WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WorldData::ResourceEventHandler, this));
  }

  void WorldData::Clear()
  {
    // allow reading and writing during destruction
    m_WriteThreadID = WThreadUtils::GetCurrentThreadID();
    m_iReadCounter.Increment();

    // deactivate all objects and components before destroying them
    for (auto it = m_ObjectStorage.GetIterator(); it.IsValid(); it.Next())
    {
      it->SetActiveFlag(false);
    }

    // deinitialize all modules before we invalidate the world. Components can still access the world during deinitialization.
    for (WWorldModule* pModule : m_Modules)
    {
      if (pModule != nullptr)
      {
        pModule->Deinitialize();
      }
    }

    // now delete all modules
    for (WWorldModule* pModule : m_Modules)
    {
      if (pModule != nullptr)
      {
        W_DELETE(&m_Allocator, pModule);
      }
    }
    m_Modules.Clear();

    // this deletes the WGameObject instances
    m_ObjectStorage.Clear();

    // delete all transformation data
    for (WUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
    {
      Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];

      for (WUInt32 i = hierarchy.m_Data.GetCount(); i-- > 0;)
      {
        Hierarchy::DataBlockArray* blocks = hierarchy.m_Data[i];
        for (WUInt32 j = blocks->GetCount(); j-- > 0;)
        {
          m_BlockAllocator.DeallocateBlock((*blocks)[j]);
        }
        W_DELETE(&m_Allocator, blocks);
      }

      hierarchy.m_Data.Clear();
    }

    // delete task storage
    m_UpdateTasks.Clear();

    // delete queued messages
    for (WUInt32 i = 0; i < WObjectMsgQueueType::COUNT; ++i)
    {
      {
        MessageQueue& queue = m_MessageQueues[i];

        // The messages in this queue are allocated through a frame allocator and thus mustn't (and don't need to be) deallocated
        queue.Clear();
      }

      {
        MessageQueue& queue = m_TimedMessageQueues[i];
        while (!queue.IsEmpty())
        {
          auto& entry = queue.PeekFront();
          W_DELETE(&m_Allocator, entry.m_pMessage);

          queue.PopFront();
        }
      }
    }
  }

  WGameObject::TransformationData* WorldData::CreateTransformationData(bool bDynamic, WUInt32 uiHierarchyLevel)
  {
    Hierarchy& hierarchy = m_Hierarchies[GetHierarchyType(bDynamic)];

    while (uiHierarchyLevel >= hierarchy.m_Data.GetCount())
    {
      hierarchy.m_Data.PushBack(W_NEW(&m_Allocator, Hierarchy::DataBlockArray, &m_Allocator));
    }

    Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];
    Hierarchy::DataBlock* pBlock = nullptr;

    if (!blocks.IsEmpty())
    {
      pBlock = &blocks.PeekBack();
    }

    if (pBlock == nullptr || pBlock->IsFull())
    {
      blocks.PushBack(m_BlockAllocator.AllocateBlock<WGameObject::TransformationData>());
      pBlock = &blocks.PeekBack();
    }

    return pBlock->ReserveBack();
  }

  void WorldData::DeleteTransformationData(bool bDynamic, WUInt32 uiHierarchyLevel, WGameObject::TransformationData* pData)
  {
    Hierarchy& hierarchy = m_Hierarchies[GetHierarchyType(bDynamic)];
    Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];

    Hierarchy::DataBlock& lastBlock = blocks.PeekBack();
    const WGameObject::TransformationData* pLast = lastBlock.PopBack();

    if (pData != pLast)
    {
      WMemoryUtils::Copy(pData, pLast, 1);
      pData->m_pObject->m_pTransformationData = pData;

      // fix parent transform data for children as well
      auto it = pData->m_pObject->GetChildren();
      while (it.IsValid())
      {
        auto pTransformData = it->m_pTransformationData;
        pTransformData->m_pParentData = pData;
        it.Next();
      }
    }

    if (lastBlock.IsEmpty())
    {
      m_BlockAllocator.DeallocateBlock(lastBlock);
      blocks.PopBack();
    }
  }

  void WorldData::TraverseBreadthFirst(VisitorFunc& func)
  {
    struct Helper
    {
      W_ALWAYS_INLINE static WVisitorExecution::Enum Visit(WGameObject::TransformationData* pData, void* pUserData) { return (*static_cast<VisitorFunc*>(pUserData))(pData->m_pObject); }
    };

    const WUInt32 uiMaxHierarchyLevel = WMath::Max(m_Hierarchies[HierarchyType::Static].m_Data.GetCount(), m_Hierarchies[HierarchyType::Dynamic].m_Data.GetCount());

    for (WUInt32 uiHierarchyLevel = 0; uiHierarchyLevel < uiMaxHierarchyLevel; ++uiHierarchyLevel)
    {
      for (WUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
      {
        Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
        if (uiHierarchyLevel < hierarchy.m_Data.GetCount())
        {
          WVisitorExecution::Enum execution = TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[uiHierarchyLevel], &func);
          W_ASSERT_DEV(execution != WVisitorExecution::Skip, "Skip is not supported when using breadth first traversal");
          if (execution == WVisitorExecution::Stop)
            return;
        }
      }
    }
  }

  void WorldData::TraverseDepthFirst(VisitorFunc& func)
  {
    struct Helper
    {
      W_ALWAYS_INLINE static WVisitorExecution::Enum Visit(WGameObject::TransformationData* pData, void* pUserData) { return WorldData::TraverseObjectDepthFirst(pData->m_pObject, *static_cast<VisitorFunc*>(pUserData)); }
    };

    for (WUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
    {
      Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
      if (!hierarchy.m_Data.IsEmpty())
      {
        if (TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[0], &func) == WVisitorExecution::Stop)
          return;
      }
    }
  }

  // static
  WVisitorExecution::Enum WorldData::TraverseObjectDepthFirst(WGameObject* pObject, VisitorFunc& func)
  {
    WVisitorExecution::Enum execution = func(pObject);
    if (execution == WVisitorExecution::Stop)
      return WVisitorExecution::Stop;

    if (execution != WVisitorExecution::Skip) // skip all children
    {
      for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
      {
        if (TraverseObjectDepthFirst(it, func) == WVisitorExecution::Stop)
          return WVisitorExecution::Stop;
      }
    }

    return WVisitorExecution::Continue;
  }

  void WorldData::UpdateGlobalTransforms()
  {
    struct UserData
    {
      WSpatialSystem* m_pSpatialSystem;
      WUInt32 m_uiUpdateCounter;
    };

    UserData userData;
    userData.m_pSpatialSystem = m_pSpatialSystem.Borrow();
    userData.m_uiUpdateCounter = m_uiUpdateCounter;

    struct RootLevel
    {
      W_ALWAYS_INLINE static WVisitorExecution::Enum Visit(WGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<const UserData*>(pUserData0);
        WorldData::UpdateGlobalTransform(pData, pUserData->m_uiUpdateCounter);
        return WVisitorExecution::Continue;
      }
    };

    struct WithParent
    {
      W_ALWAYS_INLINE static WVisitorExecution::Enum Visit(WGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<const UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformWithParent(pData, pUserData->m_uiUpdateCounter);
        return WVisitorExecution::Continue;
      }
    };

    struct RootLevelWithSpatialData
    {
      W_ALWAYS_INLINE static WVisitorExecution::Enum Visit(WGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformAndSpatialData(pData, pUserData->m_uiUpdateCounter, *pUserData->m_pSpatialSystem);
        return WVisitorExecution::Continue;
      }
    };

    struct WithParentWithSpatialData
    {
      W_ALWAYS_INLINE static WVisitorExecution::Enum Visit(WGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformWithParentAndSpatialData(pData, pUserData->m_uiUpdateCounter, *pUserData->m_pSpatialSystem);
        return WVisitorExecution::Continue;
      }
    };

    Hierarchy& hierarchy = m_Hierarchies[HierarchyType::Dynamic];
    if (!hierarchy.m_Data.IsEmpty())
    {
      auto dataPtr = hierarchy.m_Data.GetData();

      // If we have no spatial system, we perform multi-threaded update as we do not
      // have to acquire a write lock in the process.
      if (m_pSpatialSystem == nullptr)
      {
        TraverseHierarchyLevelMultiThreaded<RootLevel>(*dataPtr[0], &userData);

        for (WUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
        {
          TraverseHierarchyLevelMultiThreaded<WithParent>(*dataPtr[i], &userData);
        }
      }
      else
      {
        TraverseHierarchyLevel<RootLevelWithSpatialData>(*dataPtr[0], &userData);

        for (WUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
        {
          TraverseHierarchyLevel<WithParentWithSpatialData>(*dataPtr[i], &userData);
        }
      }
    }
  }

  void WorldData::ResourceEventHandler(const WResourceEvent& e)
  {
    if (e.m_Type != WResourceEvent::Type::ResourceContentUnloading || e.m_pResource->GetReferenceCount() == 0)
      return;

    WTypelessResourceHandle hResource(e.m_pResource);
    if (m_ReloadFunctions.Contains(hResource))
    {
      m_NeedReload.Insert(hResource);
    }
  }

} // namespace WInternal
