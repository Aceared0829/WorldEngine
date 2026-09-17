#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

class WProcPlacementComponent;
struct WMsgUpdateLocalBounds;
struct WMsgExtractRenderData;

//////////////////////////////////////////////////////////////////////////

class W_PROCGENPLUGIN_DLL WProcPlacementComponentManager : public WComponentManager<WProcPlacementComponent, WBlockStorageType::Compact>
{
public:
  WProcPlacementComponentManager(WWorld* pWorld);
  ~WProcPlacementComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class WProcPlacementComponent;

  void FindTiles(const WWorldModule::UpdateContext& context);
  void PreparePlace(const WWorldModule::UpdateContext& context);
  void PlaceObjects(const WWorldModule::UpdateContext& context);

  bool DebugDrawTile(const WProcGenInternal::PlacementTileDesc& desc, const WColor& color, WUInt32 uiQueueIndex = WInvalidIndex);

  void AddComponent(WProcPlacementComponent* pComponent);
  void RemoveComponent(WProcPlacementComponent* pComponent);

  WUInt32 AllocateTile(const WProcGenInternal::PlacementTileDesc& desc, WSharedPtr<const WProcGenInternal::PlacementOutput>& pOutput);
  void DeallocateTile(WUInt32 uiTileIndex);

  WUInt32 AllocateProcessingTask(WUInt32 uiTileIndex);
  void DeallocateProcessingTask(WUInt32 uiTaskIndex);
  WUInt32 GetNumAllocatedProcessingTasks() const;

  void RemoveTilesForComponent(WProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved = nullptr);
  void OnResourceEvent(const WResourceEvent& resourceEvent);
  void OnAreaInvalidated(const WProcGenInternal::InvalidatedArea& area);

  void AddVisibleComponent(const WComponentHandle& hComponent, const WVec3& cameraPosition, const WVec3& cameraDirection) const;
  void ClearVisibleComponents();

  struct VisibleComponent
  {
    WComponentHandle m_hComponent;
    WVec3 m_vCameraPosition;
    WVec3 m_vCameraDirection;
  };

  mutable WMutex m_VisibleComponentsMutex;
  mutable WDynamicArray<VisibleComponent> m_VisibleComponents;

  WDynamicArray<WComponentHandle> m_ComponentsToUpdate;

  WDynamicArray<WProcGenInternal::PlacementTile, WAlignedAllocatorWrapper> m_ActiveTiles;
  WDynamicArray<WUInt32> m_FreeTiles;

  struct ProcessingTask
  {
    W_ALWAYS_INLINE bool IsValid() const { return m_uiTileIndex != WInvalidIndex; }
    W_ALWAYS_INLINE bool IsScheduled() const { return m_PlacementTaskGroupID.IsValid(); }
    W_ALWAYS_INLINE void Invalidate()
    {
      m_uiScheduledFrame = -1;
      m_PlacementTaskGroupID.Invalidate();
      m_uiTileIndex = WInvalidIndex;
    }

    WUInt64 m_uiScheduledFrame;
    WUniquePtr<WProcGenInternal::PlacementData> m_pData;
    WSharedPtr<WProcGenInternal::PreparePlacementTask> m_pPrepareTask;
    WSharedPtr<WProcGenInternal::PlacementTask> m_pPlacementTask;
    WTaskGroupID m_PlacementTaskGroupID;
    WUInt32 m_uiTileIndex;
  };

  WDynamicArray<ProcessingTask> m_ProcessingTasks;
  WDynamicArray<WUInt32> m_FreeProcessingTasks;

  struct SortedProcessingTask
  {
    WUInt64 m_uiScheduledFrame = 0;
    WUInt32 m_uiTaskIndex = 0;
  };

  WDynamicArray<SortedProcessingTask> m_SortedProcessingTasks;

  WDynamicArray<WProcGenInternal::PlacementTileDesc, WAlignedAllocatorWrapper> m_NewTiles;
  WTaskGroupID m_UpdateTilesTaskGroupID;
};

//////////////////////////////////////////////////////////////////////////

struct WProcGenBoxExtents
{
  WVec3 m_vOffset = WVec3::MakeZero();
  WQuat m_Rotation = WQuat::MakeIdentity();
  WVec3 m_vExtents = WVec3(10);

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcGenBoxExtents);

class W_PROCGENPLUGIN_DLL WProcPlacementComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WProcPlacementComponent, WComponent, WProcPlacementComponentManager);

public:
  WProcPlacementComponent();
  ~WProcPlacementComponent();

  WProcPlacementComponent& operator=(WProcPlacementComponent&& other);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResource(const WProcGenGraphResourceHandle& hResource);                // [ property ]
  const WProcGenGraphResourceHandle& GetResource() const { return m_hResource; } // [ property ]

  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg);
  void OnMsgExtractRenderData(WMsgExtractRenderData& ref_msg) const;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

private:
  WUInt32 BoxExtents_GetCount() const;
  const WProcGenBoxExtents& BoxExtents_GetValue(WUInt32 uiIndex) const;
  void BoxExtents_SetValue(WUInt32 uiIndex, const WProcGenBoxExtents& value);
  void BoxExtents_Insert(WUInt32 uiIndex, const WProcGenBoxExtents& value);
  void BoxExtents_Remove(WUInt32 uiIndex);

  void UpdateBoundsAndTiles();

  WProcGenGraphResourceHandle m_hResource;

  WDynamicArray<WProcGenBoxExtents> m_BoxExtents;

  // runtime data
  friend class WProcGenInternal::FindPlacementTilesTask;

  struct Bounds
  {
    W_DECLARE_POD_TYPE();

    WSimdBBox m_GlobalBoundingBox;
    WSimdMat4f m_GlobalToLocalBoxTransform;
  };

  WDynamicArray<Bounds, WAlignedAllocatorWrapper> m_Bounds;

  struct OutputContext
  {
    WSharedPtr<const WProcGenInternal::PlacementOutput> m_pOutput;

    struct TileIndexAndAge
    {
      W_DECLARE_POD_TYPE();

      WUInt32 m_uiIndex;
      WUInt64 m_uiLastSeenFrame;
    };

    WHashTable<WUInt64, TileIndexAndAge> m_TileIndices;

    WSharedPtr<WProcGenInternal::FindPlacementTilesTask> m_pUpdateTilesTask;

    bool IsValid() const { return m_pOutput != nullptr && m_pUpdateTilesTask != nullptr; }
  };

  WDynamicArray<OutputContext> m_OutputContexts;
};
