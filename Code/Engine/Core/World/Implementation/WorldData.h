#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/SharedPtr.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/GameObject.h>
#include <Core/World/WorldDesc.h>

namespace WInternal
{
  class W_CORE_DLL WorldData
  {
  private:
    friend class ::WWorld;
    friend class ::WComponentManagerBase;

    WorldData(WWorldDesc& desc);
    ~WorldData();

    void Clear();

    WHashedString m_sName;
    mutable WProxyAllocator m_Allocator;
    WLocalAllocatorWrapper m_AllocatorWrapper;
    WInternal::WorldLargeBlockAllocator m_BlockAllocator;
    WDoubleBufferedLinearAllocator m_LinearAllocator;

    enum
    {
      GAME_OBJECTS_PER_BLOCK = WDataBlock<WGameObject, WInternal::DEFAULT_BLOCK_SIZE>::CAPACITY,
      TRANSFORMATION_DATA_PER_BLOCK = WDataBlock<WGameObject::TransformationData, WInternal::DEFAULT_BLOCK_SIZE>::CAPACITY
    };

    // object storage
    using ObjectStorage = WBlockStorage<WGameObject, WInternal::DEFAULT_BLOCK_SIZE, WBlockStorageType::Compact>;
    WIdTable<WGameObjectId, WGameObject*, WLocalAllocatorWrapper> m_Objects;
    ObjectStorage m_ObjectStorage;

    WSet<WGameObject*, WCompareHelper<WGameObject*>, WLocalAllocatorWrapper> m_DeadObjects;
    WEvent<const WGameObject*> m_ObjectDeletionEvent;

  public:
    class W_CORE_DLL ConstObjectIterator
    {
    public:
      const WGameObject& operator*() const;
      const WGameObject* operator->() const;

      operator const WGameObject*() const;

      /// Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// Shorthand for 'Next'
      void operator++();

    private:
      friend class ::WWorld;

      ConstObjectIterator(ObjectStorage::ConstIterator iterator);

      ObjectStorage::ConstIterator m_Iterator;
    };

    class W_CORE_DLL ObjectIterator
    {
    public:
      WGameObject& operator*();
      WGameObject* operator->();

      operator WGameObject*();

      /// Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// Shorthand for 'Next'
      void operator++();

    private:
      friend class ::WWorld;

      ObjectIterator(ObjectStorage::Iterator iterator);

      ObjectStorage::Iterator m_Iterator;
    };

  private:
    // hierarchy structures
    struct Hierarchy
    {
      using DataBlock = WDataBlock<WGameObject::TransformationData, WInternal::DEFAULT_BLOCK_SIZE>;
      using DataBlockArray = WDynamicArray<DataBlock>;

      WHybridArray<DataBlockArray*, 8, WLocalAllocatorWrapper> m_Data;
    };

    struct HierarchyType
    {
      enum Enum
      {
        Static,
        Dynamic,
        COUNT
      };
    };

    Hierarchy m_Hierarchies[HierarchyType::COUNT];

    static HierarchyType::Enum GetHierarchyType(bool bDynamic);

    WGameObject::TransformationData* CreateTransformationData(bool bDynamic, WUInt32 uiHierarchyLevel);

    void DeleteTransformationData(bool bDynamic, WUInt32 uiHierarchyLevel, WGameObject::TransformationData* pData);

    template <typename VISITOR>
    static WVisitorExecution::Enum TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);
    template <typename VISITOR>
    WVisitorExecution::Enum TraverseHierarchyLevelMultiThreaded(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);

    using VisitorFunc = WDelegate<WVisitorExecution::Enum(WGameObject*)>;
    void TraverseBreadthFirst(VisitorFunc& func);
    void TraverseDepthFirst(VisitorFunc& func);
    static WVisitorExecution::Enum TraverseObjectDepthFirst(WGameObject* pObject, VisitorFunc& func);

    static void UpdateGlobalTransform(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter);
    static void UpdateGlobalTransformWithParent(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter);

    static void UpdateGlobalTransformAndSpatialData(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter, WSpatialSystem& spatialSystem);
    static void UpdateGlobalTransformWithParentAndSpatialData(WGameObject::TransformationData* pData, WUInt32 uiUpdateCounter, WSpatialSystem& spatialSystem);

    void UpdateGlobalTransforms();

    void ResourceEventHandler(const WResourceEvent& e);

    // game object lookups
    WHashTable<WUInt64, WGameObjectId, WHashHelper<WUInt64>, WLocalAllocatorWrapper> m_GlobalKeyToIdTable;
    WHashTable<WUInt64, WHashedString, WHashHelper<WUInt64>, WLocalAllocatorWrapper> m_IdToGlobalKeyTable;

    // modules
    WDynamicArray<WWorldModule*, WLocalAllocatorWrapper> m_Modules;
    WDynamicArray<WWorldModule*, WLocalAllocatorWrapper> m_ModulesToStartSimulation;

    // component management
    WSet<WComponent*, WCompareHelper<WComponent*>, WLocalAllocatorWrapper> m_DeadComponents;

    struct InitBatch
    {
      InitBatch(WAllocator* pAllocator, WStringView sName, bool bMustFinishWithinOneFrame);

      WHashedString m_sName;
      bool m_bMustFinishWithinOneFrame = true;
      bool m_bIsReady = false;

      WUInt32 m_uiNextComponentToInitialize = 0;
      WUInt32 m_uiNextComponentToStartSimulation = 0;
      WDynamicArray<WComponentHandle> m_ComponentsToInitialize;
      WDynamicArray<WComponentHandle> m_ComponentsToStartSimulation;
    };

    WTime m_MaxInitializationTimePerFrame;
    WIdTable<WComponentInitBatchId, WUniquePtr<InitBatch>, WLocalAllocatorWrapper> m_InitBatches;
    InitBatch* m_pDefaultInitBatch = nullptr;
    InitBatch* m_pCurrentInitBatch = nullptr;

    struct RegisteredUpdateFunction
    {
      WWorldModule::UpdateFunction m_Function;
      WHashedString m_sFunctionName;
      float m_fPriority;
      WUInt16 m_uiAsyncPhaseBatchSize;
      bool m_bOnlyUpdateWhenSimulating;

      void FillFromDesc(const WWorldModule::UpdateFunctionDesc& desc);
      bool operator<(const RegisteredUpdateFunction& other) const;
    };

    struct UpdateTask final : public WTask
    {
      virtual void Execute() override;

      WWorldModule::UpdateFunction m_Function;
      WUInt32 m_uiStartIndex;
      WUInt32 m_uiCount;
    };

    WDynamicArray<RegisteredUpdateFunction, WLocalAllocatorWrapper> m_UpdateFunctions[WWorldUpdatePhase::COUNT];
    WDynamicArray<WWorldModule::UpdateFunctionDesc, WLocalAllocatorWrapper> m_UpdateFunctionsToRegister;
    WDynamicArray<WWorldModule::UpdateFunctionDesc, WLocalAllocatorWrapper> m_UpdateFunctionsToDeregister;

    WDynamicArray<WSharedPtr<UpdateTask>, WLocalAllocatorWrapper> m_UpdateTasks;

    WUniquePtr<WSpatialSystem> m_pSpatialSystem;
    WSharedPtr<WCoordinateSystemProvider> m_pCoordinateSystemProvider;
    WUniquePtr<WTimeStepSmoothing> m_pTimeStepSmoothing;
    WSharedPtr<WBlackboard> m_pBlackboard;

    WClock m_Clock;
    WRandom m_Random;

    struct QueuedMsg
    {
      W_DECLARE_POD_TYPE();

      W_ALWAYS_INLINE QueuedMsg()
        : m_uiReceiverData(0)
      {
      }

      WMessage* m_pMessage = nullptr;
      mutable WUInt64 m_uiMessageHash = 0;

      union
      {
        struct
        {
          WUInt64 m_uiReceiverObjectOrComponent : 62;
          WUInt64 m_uiReceiverIsComponent : 1;
          WUInt64 m_uiRecursive : 1;
        };

        WUInt64 m_uiReceiverData;
      };

      WTime m_Due;
    };

    using MessageQueue = WDeque<QueuedMsg, WLocalAllocatorWrapper>;
    mutable WMutex m_MessageQueueMutex[WObjectMsgQueueType::COUNT];
    mutable MessageQueue m_MessageQueues[WObjectMsgQueueType::COUNT];
    mutable MessageQueue m_MessageProcessingQueues[WObjectMsgQueueType::COUNT];
    mutable MessageQueue m_TimedMessageQueues[WObjectMsgQueueType::COUNT];
    WTime m_MessageTime; // Used to determine when delayed messages are due. Advanced with the world clock when the simulation is running, otherwise the global clock is used.

    WThreadID m_WriteThreadID;
    WInt32 m_iWriteCounter = 0;
    mutable WAtomicInteger32 m_iReadCounter;

    WUInt32 m_uiUpdateCounter = 0;
    bool m_bSimulateWorld = true;
    bool m_bReportErrorWhenStaticObjectMoves = true;

    /// Maps some data (given as void*) to an WGameObjectHandle. Only available in special situations (e.g. editor use cases).
    WDelegate<WGameObjectHandle(const void*, WComponentHandle, WStringView)> m_GameObjectReferenceResolver;

    struct ResourceReloadContext
    {
      WWorld* m_pWorld = nullptr;
      WComponent* m_pComponent = nullptr;
      void* m_pUserData = nullptr;
    };

    using ResourceReloadFunc = WDelegate<void(ResourceReloadContext&)>;

    struct ResourceReloadFunctionData
    {
      WComponentHandle m_hComponent;
      void* m_pUserData = nullptr;
      ResourceReloadFunc m_Func;
    };

    using ReloadFunctionList = WHybridArray<ResourceReloadFunctionData, 8>;
    WHashTable<WTypelessResourceHandle, ReloadFunctionList> m_ReloadFunctions;
    WHashSet<WTypelessResourceHandle> m_NeedReload;
    ReloadFunctionList m_TempReloadFunctions;

  public:
    class ReadMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::WInternal::WorldData;

      ReadMarker(const WorldData& data);
      const WorldData& m_Data;
    };

    class WriteMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::WInternal::WorldData;

      WriteMarker(WorldData& data);
      WorldData& m_Data;
    };

  private:
    mutable ReadMarker m_ReadMarker;
    WriteMarker m_WriteMarker;

    void* m_pUserData = nullptr;

    /// Protects m_BoundsUpdateQueue for concurrent access during the async update phase.
    WMutex m_BoundsUpdateMutex;
    /// Game objects whose local bounds need to be recomputed at the end of the current update phase.
    WDynamicArray<WGameObjectHandle> m_BoundsUpdateQueue;
  };
} // namespace WInternal

#include <Core/World/Implementation/WorldData_inl.h>
