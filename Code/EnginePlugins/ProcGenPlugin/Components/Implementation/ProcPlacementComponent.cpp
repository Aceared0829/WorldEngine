#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

using namespace WProcGenInternal;

WCVarInt cvar_ProcGenProcessingMaxTiles("ProcGen.Processing.MaxTiles", 8, WCVarFlags::Default, "Maximum number of tiles in process");
WCVarInt cvar_ProcGenProcessingMaxNewObjectsPerFrame("ProcGen.Processing.MaxNewObjectsPerFrame", 256, WCVarFlags::Default, "Maximum number of objects placed per frame");
WCVarBool cvar_ProcGenVisTiles("ProcGen.VisTiles.Enable", false, WCVarFlags::Default, "Enables debug visualization of procedural placement tiles");
WCVarString cvar_ProcGenVisTilesOutputFilter("ProcGen.VisTiles.OutputFilter", "", WCVarFlags::Default, "When set only tiles form the matching output are shown");
WCVarInt cvar_ProcGenVisTileTileX("ProcGen.VisTiles.PosX", WMath::MaxValue<int>(), WCVarFlags::Default, "The x position of the tile to visualize");
WCVarInt cvar_ProcGenVisTileTileY("ProcGen.VisTiles.PosY", WMath::MaxValue<int>(), WCVarFlags::Default, "The y position of the tile to visualize");

WProcPlacementComponentManager::WProcPlacementComponentManager(WWorld* pWorld)
  : WComponentManager<WProcPlacementComponent, WBlockStorageType::Compact>(pWorld)
{
}

WProcPlacementComponentManager::~WProcPlacementComponentManager() = default;

void WProcPlacementComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WProcPlacementComponentManager::FindTiles, this);
    desc.m_Phase = WWorldUpdatePhase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WProcPlacementComponentManager::PreparePlace, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WProcPlacementComponentManager::PlaceObjects, this);
    desc.m_Phase = WWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WProcPlacementComponentManager::OnResourceEvent, this));

  WProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(WMakeDelegate(&WProcPlacementComponentManager::OnAreaInvalidated, this));
}

void WProcPlacementComponentManager::Deinitialize()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WProcPlacementComponentManager::OnResourceEvent, this));

  WProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(WMakeDelegate(&WProcPlacementComponentManager::OnAreaInvalidated, this));

  for (auto& activeTile : m_ActiveTiles)
  {
    activeTile.Deinitialize(*GetWorld());
  }
  m_ActiveTiles.Clear();

  SUPER::Deinitialize();
}

void WProcPlacementComponentManager::FindTiles(const WWorldModule::UpdateContext& context)
{
  // Update resource data
  bool bAnyObjectsRemoved = false;

  for (auto& hComponent : m_ComponentsToUpdate)
  {
    WProcPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(hComponent, pComponent))
    {
      continue;
    }

    RemoveTilesForComponent(pComponent, &bAnyObjectsRemoved);

    WResourceLock<WProcGenGraphResource> pResource(pComponent->m_hResource, WResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetPlacementOutputs();

    pComponent->m_OutputContexts.Clear();
    pComponent->m_OutputContexts.SetCount(outputs.GetCount());
    for (WUInt32 uiIndex = 0; uiIndex < outputs.GetCount(); ++uiIndex)
    {
      const auto& pOutput = outputs[uiIndex];
      if (pOutput->IsValid())
      {
        auto& outputContext = pComponent->m_OutputContexts[uiIndex];
        outputContext.m_pOutput = pOutput;
        outputContext.m_pUpdateTilesTask = W_DEFAULT_NEW(FindPlacementTilesTask, pComponent, uiIndex);
      }
    }
  }
  m_ComponentsToUpdate.Clear();

  // If we removed any objects during resource update do nothing else this frame so objects are actually deleted before we place new ones.
  if (bAnyObjectsRemoved)
  {
    return;
  }

  if (GetWorldSimulationEnabled())
  {
    // Schedule find tiles tasks
    m_UpdateTilesTaskGroupID = WTaskSystem::CreateTaskGroup(WTaskPriority::EarlyThisFrame);

    for (auto& visibleComponent : m_VisibleComponents)
    {
      WProcPlacementComponent* pComponent = nullptr;
      if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
      {
        continue;
      }

      auto& outputContexts = pComponent->m_OutputContexts;
      for (auto& outputContext : outputContexts)
      {
        if (outputContext.IsValid() == false)
          continue;

        outputContext.m_pUpdateTilesTask->AddCameraPosition(visibleComponent.m_vCameraPosition);

        if (outputContext.m_pUpdateTilesTask->IsTaskFinished())
        {
          WTaskSystem::AddTaskToGroup(m_UpdateTilesTaskGroupID, outputContext.m_pUpdateTilesTask);
        }
      }
    }

    WTaskSystem::StartTaskGroup(m_UpdateTilesTaskGroupID);
  }
  else
  {
    ClearVisibleComponents();
  }
}

void WProcPlacementComponentManager::PreparePlace(const WWorldModule::UpdateContext& context)
{
  // Find new active tiles and remove old ones
  {
    W_PROFILE_SCOPE("Add new/remove old tiles");

    WTaskSystem::WaitForGroup(m_UpdateTilesTaskGroupID);
    m_UpdateTilesTaskGroupID.Invalidate();

    for (auto& visibleComponent : m_VisibleComponents)
    {
      WProcPlacementComponent* pComponent = nullptr;
      if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
      {
        continue;
      }

      auto& outputContexts = pComponent->m_OutputContexts;
      for (auto& outputContext : outputContexts)
      {
        if (outputContext.IsValid() == false)
          continue;

        auto oldTiles = outputContext.m_pUpdateTilesTask->GetOldTiles();
        for (WUInt64 uiOldTileKey : oldTiles)
        {
          WProcPlacementComponent::OutputContext::TileIndexAndAge tileIndex;
          if (outputContext.m_TileIndices.Remove(uiOldTileKey, &tileIndex))
          {
            if (tileIndex.m_uiIndex != NewTileIndex)
            {
              DeallocateTile(tileIndex.m_uiIndex);
            }
          }

          // Also remove from new tiles list
          for (WUInt32 i = 0; i < m_NewTiles.GetCount(); ++i)
          {
            auto& newTile = m_NewTiles[i];
            WUInt64 uiTileKey = GetTileKey(newTile.m_iPosX, newTile.m_iPosY);
            if (uiTileKey == uiOldTileKey)
            {
              m_NewTiles.RemoveAtAndSwap(i);
              break;
            }
          }
        }

        for (const auto& newTile : outputContext.m_pUpdateTilesTask->GetNewTiles())
        {
          if (!m_NewTiles.Contains(newTile))
          {
            m_NewTiles.PushBack(newTile);
          }
        }
      }
    }

    // Sort new tiles
    {
      W_PROFILE_SCOPE("Sort new tiles");

      // Update distance to camera
      for (auto& newTile : m_NewTiles)
      {
        WVec2 tilePos = WVec2((float)newTile.m_iPosX, (float)newTile.m_iPosY);
        newTile.m_fDistanceToCamera = WMath::MaxValue<float>();

        for (auto& visibleComponent : m_VisibleComponents)
        {
          WVec2 cameraPos = visibleComponent.m_vCameraPosition.GetAsVec2() / newTile.m_fTileSize;

          float fDistance = (tilePos - cameraPos).GetLengthSquared();
          newTile.m_fDistanceToCamera = WMath::Min(newTile.m_fDistanceToCamera, fDistance);
        }
      }

      // Sort by distance, larger distances come first since new tiles are processed in reverse order.
      m_NewTiles.Sort([](auto& ref_tileA, auto& ref_tileB)
        { return ref_tileA.m_fDistanceToCamera > ref_tileB.m_fDistanceToCamera; });
    }

    ClearVisibleComponents();
  }

  // Allocate new tiles and placement tasks
  {
    W_PROFILE_SCOPE("Allocate new tiles");

    while (!m_NewTiles.IsEmpty() && GetNumAllocatedProcessingTasks() < (WUInt32)cvar_ProcGenProcessingMaxTiles)
    {
      const PlacementTileDesc& newTile = m_NewTiles.PeekBack();

      WProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(newTile.m_hComponent, pComponent))
      {
        auto& pOutput = pComponent->m_OutputContexts[newTile.m_uiOutputIndex].m_pOutput;
        WUInt32 uiNewTileIndex = AllocateTile(newTile, pOutput);

        AllocateProcessingTask(uiNewTileIndex);
      }

      m_NewTiles.PopBack();
    }
  }

  // Debug draw tiles
  WHashSet<WUInt32> debugDrawnTiles(WTempAllocator::Get());
  if (cvar_ProcGenVisTiles)
  {
    WStringBuilder sb;
    sb.SetFormat("Procedural Placement Stats:\nNum Tiles to process: {}", m_ProcessingTasks.GetCount());

    WColor textColor = WColorScheme::LightUI(WColorScheme::Grape);
    WDebugRenderer::DrawInfoText(GetWorld(), WDebugTextPlacement::TopLeft, "ProcPlaceStats", sb, textColor);

    for (WUInt32 uiTileIndex = 0; uiTileIndex < m_ActiveTiles.GetCount(); ++uiTileIndex)
    {
      auto& activeTile = m_ActiveTiles[uiTileIndex];
      if (!activeTile.IsValid())
        continue;

      if (DebugDrawTile(activeTile.GetDesc(), activeTile.GetDebugColor()))
      {
        debugDrawnTiles.Insert(uiTileIndex);
      }
    }
  }

  const WWorld* pWorld = GetWorld();

  // Update processing tasks
  {
    W_PROFILE_SCOPE("Prepare processing tasks");

    WTaskGroupID prepareTaskGroupID = WTaskSystem::CreateTaskGroup(WTaskPriority::EarlyThisFrame);

    for (auto& processingTask : m_ProcessingTasks)
    {
      if (!processingTask.IsValid() || processingTask.IsScheduled())
        continue;

      auto& activeTile = m_ActiveTiles[processingTask.m_uiTileIndex];
      const bool bDebugVisualization = debugDrawnTiles.Contains(processingTask.m_uiTileIndex);
      activeTile.PreparePlacementData(pWorld, pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>(), bDebugVisualization, *processingTask.m_pData);

      WTaskSystem::AddTaskToGroup(prepareTaskGroupID, processingTask.m_pPrepareTask);
    }

    WTaskSystem::StartTaskGroup(prepareTaskGroupID);
    WTaskSystem::WaitForGroup(prepareTaskGroupID);
  }

  {
    W_PROFILE_SCOPE("Kickoff placement tasks");

    for (auto& processingTask : m_ProcessingTasks)
    {
      if (!processingTask.IsValid() || processingTask.IsScheduled())
        continue;

      processingTask.m_uiScheduledFrame = WRenderWorld::GetFrameCounter();
      processingTask.m_PlacementTaskGroupID = WTaskSystem::StartSingleTask(processingTask.m_pPlacementTask, WTaskPriority::LongRunningHighPriority);
    }
  }
}

void WProcPlacementComponentManager::PlaceObjects(const WWorldModule::UpdateContext& context)
{
  m_SortedProcessingTasks.Clear();
  for (WUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
  {
    auto& sortedTask = m_SortedProcessingTasks.ExpandAndGetRef();
    sortedTask.m_uiScheduledFrame = m_ProcessingTasks[i].m_uiScheduledFrame;
    sortedTask.m_uiTaskIndex = i;
  }

  m_SortedProcessingTasks.Sort([](auto& ref_taskA, auto& ref_taskB)
    { return ref_taskA.m_uiScheduledFrame < ref_taskB.m_uiScheduledFrame; });

  WUInt32 uiTotalNumPlacedObjects = 0;

  for (auto& sortedTask : m_SortedProcessingTasks)
  {
    auto& task = m_ProcessingTasks[sortedTask.m_uiTaskIndex];
    if (!task.IsValid() || !task.IsScheduled())
      continue;

    if (task.m_pPlacementTask->IsTaskFinished())
    {
      WUInt32 uiPlacedObjects = 0;

      WUInt32 uiTileIndex = task.m_uiTileIndex;
      auto& activeTile = m_ActiveTiles[uiTileIndex];

      auto& tileDesc = activeTile.GetDesc();
      WProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(tileDesc.m_hComponent, pComponent))
      {
        auto& outputContext = pComponent->m_OutputContexts[tileDesc.m_uiOutputIndex];

        WUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY);
        if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
        {
          uiPlacedObjects = activeTile.PlaceObjects(*GetWorld(), task.m_pPlacementTask->GetOutputTransforms());

          pTile->m_uiIndex = uiPlacedObjects > 0 ? uiTileIndex : EmptyTileIndex;
          pTile->m_uiLastSeenFrame = WRenderWorld::GetFrameCounter();
        }
      }

      if (uiPlacedObjects == 0)
      {
        // mark tile for re-use
        DeallocateTile(uiTileIndex);
      }

      // mark task for re-use
      DeallocateProcessingTask(sortedTask.m_uiTaskIndex);

      uiTotalNumPlacedObjects += uiPlacedObjects;
    }

    if (uiTotalNumPlacedObjects >= (WUInt32)cvar_ProcGenProcessingMaxNewObjectsPerFrame)
    {
      break;
    }
  }
}

bool WProcPlacementComponentManager::DebugDrawTile(const WProcGenInternal::PlacementTileDesc& desc, const WColor& color, WUInt32 uiQueueIndex)
{
  const WProcPlacementComponent* pComponent = nullptr;
  if (!TryGetComponent(desc.m_hComponent, pComponent))
    return false;

  auto& outputContext = pComponent->m_OutputContexts[desc.m_uiOutputIndex];

  WStringView sOutputFilter = cvar_ProcGenVisTilesOutputFilter.GetValue();
  if (sOutputFilter.IsEmpty() == false && outputContext.m_pOutput->m_sName.GetView().FindSubString_NoCase(sOutputFilter) == nullptr)
    return false;

  if ((cvar_ProcGenVisTileTileX != WMath::MaxValue<int>() && desc.m_iPosX != cvar_ProcGenVisTileTileX) ||
      (cvar_ProcGenVisTileTileY != WMath::MaxValue<int>() && desc.m_iPosY != cvar_ProcGenVisTileTileY))
  {
    return false;
  }

  WBoundingBox bbox = desc.GetBoundingBox();
  WDebugRenderer::DrawLineBox(GetWorld(), bbox, color);

  const WUInt64 uiTileKey = GetTileKey(desc.m_iPosX, desc.m_iPosY);
  WUInt64 uiAge = -1;
  if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
  {
    uiAge = WRenderWorld::GetFrameCounter() - pTile->m_uiLastSeenFrame;
  }

  WStringBuilder sb;
  sb.SetFormat("Tile: {}x{}\n", desc.m_iPosX, desc.m_iPosY);
  if (uiQueueIndex != WInvalidIndex)
  {
    sb.AppendFormat("Queue Index: {}\n", uiQueueIndex);
  }
  sb.AppendFormat("Age: {}\nDistance: {}", uiAge, desc.m_fDistanceToCamera);
  WDebugRenderer::Draw3DText(GetWorld(), sb, bbox.GetCenter(), color);

  return true;
}

void WProcPlacementComponentManager::AddComponent(WProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
}

void WProcPlacementComponentManager::RemoveComponent(WProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  RemoveTilesForComponent(pComponent);
}

WUInt32 WProcPlacementComponentManager::AllocateTile(const PlacementTileDesc& desc, WSharedPtr<const PlacementOutput>& pOutput)
{
  WUInt32 uiNewTileIndex = WInvalidIndex;
  if (!m_FreeTiles.IsEmpty())
  {
    uiNewTileIndex = m_FreeTiles.PeekBack();
    m_FreeTiles.PopBack();
  }
  else
  {
    uiNewTileIndex = m_ActiveTiles.GetCount();
    m_ActiveTiles.ExpandAndGetRef();
  }

  m_ActiveTiles[uiNewTileIndex].Initialize(desc, pOutput);
  return uiNewTileIndex;
}

void WProcPlacementComponentManager::DeallocateTile(WUInt32 uiTileIndex)
{
  m_ActiveTiles[uiTileIndex].Deinitialize(*GetWorld());
  m_FreeTiles.PushBack(uiTileIndex);
}

WUInt32 WProcPlacementComponentManager::AllocateProcessingTask(WUInt32 uiTileIndex)
{
  WUInt32 uiNewTaskIndex = WInvalidIndex;
  if (!m_FreeProcessingTasks.IsEmpty())
  {
    uiNewTaskIndex = m_FreeProcessingTasks.PeekBack();
    m_FreeProcessingTasks.PopBack();
  }
  else
  {
    uiNewTaskIndex = m_ProcessingTasks.GetCount();
    auto& newTask = m_ProcessingTasks.ExpandAndGetRef();

    newTask.m_pData = W_DEFAULT_NEW(PlacementData);

    WStringBuilder sName;
    sName.SetFormat("Prepare Task {}", uiNewTaskIndex);
    newTask.m_pPrepareTask = W_DEFAULT_NEW(PreparePlacementTask, newTask.m_pData.Borrow(), sName);

    sName.SetFormat("Placement Task {}", uiNewTaskIndex);
    newTask.m_pPlacementTask = W_DEFAULT_NEW(PlacementTask, newTask.m_pData.Borrow(), sName);
  }

  m_ProcessingTasks[uiNewTaskIndex].m_uiTileIndex = uiTileIndex;
  return uiNewTaskIndex;
}

void WProcPlacementComponentManager::DeallocateProcessingTask(WUInt32 uiTaskIndex)
{
  auto& task = m_ProcessingTasks[uiTaskIndex];
  if (task.IsScheduled())
  {
    WTaskSystem::WaitForGroup(task.m_PlacementTaskGroupID);
  }

  task.m_pData->Clear();
  task.m_pPrepareTask->Clear();
  task.m_pPlacementTask->Clear();
  task.Invalidate();

  m_FreeProcessingTasks.PushBack(uiTaskIndex);
}

WUInt32 WProcPlacementComponentManager::GetNumAllocatedProcessingTasks() const
{
  return m_ProcessingTasks.GetCount() - m_FreeProcessingTasks.GetCount();
}

void WProcPlacementComponentManager::RemoveTilesForComponent(WProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved /*= nullptr*/)
{
  WComponentHandle hComponent = pComponent->GetHandle();

  for (WUInt32 uiNewTileIndex = 0; uiNewTileIndex < m_NewTiles.GetCount(); ++uiNewTileIndex)
  {
    if (m_NewTiles[uiNewTileIndex].m_hComponent == hComponent)
    {
      m_NewTiles.RemoveAtAndSwap(uiNewTileIndex);
      --uiNewTileIndex;
    }
  }

  for (WUInt32 uiTileIndex = 0; uiTileIndex < m_ActiveTiles.GetCount(); ++uiTileIndex)
  {
    auto& activeTile = m_ActiveTiles[uiTileIndex];
    if (!activeTile.IsValid())
      continue;

    auto& tileDesc = activeTile.GetDesc();
    if (tileDesc.m_hComponent == hComponent)
    {
      if (out_bAnyObjectsRemoved != nullptr && !m_ActiveTiles[uiTileIndex].GetPlacedObjects().IsEmpty())
      {
        *out_bAnyObjectsRemoved = true;
      }

      DeallocateTile(uiTileIndex);

      for (WUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
      {
        auto& taskInfo = m_ProcessingTasks[i];
        if (taskInfo.m_uiTileIndex == uiTileIndex)
        {
          DeallocateProcessingTask(i);
        }
      }
    }
  }
}

void WProcPlacementComponentManager::OnResourceEvent(const WResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != WResourceEvent::Type::ResourceContentUnloading || resourceEvent.m_pResource->GetReferenceCount() == 0)
    return;

  if (auto pResource = WDynamicCast<const WProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    WProcGenGraphResourceHandle hResource = pResource->GetResourceHandle();

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource && !m_ComponentsToUpdate.Contains(it->GetHandle()))
      {
        m_ComponentsToUpdate.PushBack(it->GetHandle());
      }
    }
  }
}

void WProcPlacementComponentManager::OnAreaInvalidated(const WProcGenInternal::InvalidatedArea& area)
{
  if (area.m_pWorld != GetWorld())
    return;

  WSimdBBox areaBox = WSimdConversion::ToBBox(area.m_Box);

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    bool bIntersects = false;
    for (auto& bounds : it->m_Bounds)
    {
      if (areaBox.Overlaps(bounds.m_GlobalBoundingBox))
      {
        bIntersects = true;
        break;
      }
    }

    if (bIntersects && !m_ComponentsToUpdate.Contains(it->GetHandle()))
    {
      m_ComponentsToUpdate.PushBack(it->GetHandle());
    }
  }
}

void WProcPlacementComponentManager::AddVisibleComponent(const WComponentHandle& hComponent, const WVec3& cameraPosition, const WVec3& cameraDirection) const
{
  if (!GetWorldSimulationEnabled())
    return;

  W_LOCK(m_VisibleComponentsMutex);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    if (visibleComponent.m_hComponent == hComponent &&
        visibleComponent.m_vCameraPosition.IsEqual(cameraPosition, WMath::LargeEpsilon<float>()) &&
        visibleComponent.m_vCameraDirection.IsEqual(cameraDirection, WMath::LargeEpsilon<float>()))
    {
      return;
    }
  }

  auto& visibleComponent = m_VisibleComponents.ExpandAndGetRef();
  visibleComponent.m_hComponent = hComponent;
  visibleComponent.m_vCameraPosition = cameraPosition;
  visibleComponent.m_vCameraDirection = cameraDirection;
}

void WProcPlacementComponentManager::ClearVisibleComponents()
{
  m_VisibleComponents.Clear();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WProcGenBoxExtents, WNoBase, 1, WRTTIDefaultAllocator<WProcGenBoxExtents>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Offset", m_vOffset),
    W_MEMBER_PROPERTY("Rotation", m_Rotation),
    W_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(10.0f)), new WClampValueAttribute(WVec3(0), WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WBoxManipulatorAttribute("Extents", 1.0f, false, "Offset", "Rotation"),
    new WBoxVisualizerAttribute("Extents", 1.0f, WColorScheme::LightUI(WColorScheme::Blue), nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "Offset", "Rotation"),
    new WTransformManipulatorAttribute("Offset", "Rotation"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WProcPlacementComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Resource", GetResource, SetResource)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph"), new WRequiredAttribute()),
    W_ARRAY_ACCESSOR_PROPERTY("BoxExtents", BoxExtents_GetCount, BoxExtents_GetValue, BoxExtents_SetValue, BoxExtents_Insert, BoxExtents_Remove),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Construction/Procedural Generation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WProcPlacementComponent::WProcPlacementComponent() = default;
WProcPlacementComponent::~WProcPlacementComponent() = default;
WProcPlacementComponent& WProcPlacementComponent::operator=(WProcPlacementComponent&& other) = default;

void WProcPlacementComponent::OnActivated()
{
  UpdateBoundsAndTiles();
}

void WProcPlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  m_Bounds.Clear();
  m_OutputContexts.Clear();

  auto pManager = static_cast<WProcPlacementComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);
}

void WProcPlacementComponent::SetResource(const WProcGenGraphResourceHandle& hResource)
{
  auto pManager = static_cast<WProcPlacementComponentManager*>(GetOwningManager());

  if (IsActiveAndInitialized())
  {
    pManager->RemoveComponent(this);
  }

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    pManager->AddComponent(this);
  }
}

void WProcPlacementComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg)
{
  if (m_BoxExtents.IsEmpty())
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  for (auto& boxExtent : m_BoxExtents)
  {
    WBoundingBoxSphere localBox = WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(-boxExtent.m_vExtents * 0.5f, boxExtent.m_vExtents * 0.5f));
    localBox.Transform(WTransform(boxExtent.m_vOffset, boxExtent.m_Rotation).GetAsMat4());

    bounds.ExpandToInclude(localBox);
  }

  ref_msg.AddBounds(bounds, GetOwner()->IsDynamic() ? WDefaultSpatialDataCategories::RenderDynamic : WDefaultSpatialDataCategories::RenderStatic);
}

void WProcPlacementComponent::OnMsgExtractRenderData(WMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::MainView &&
      ref_msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection or in shadow views.
  if (ref_msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  if (m_hResource.IsValid() == false)
    return;

  const WCamera* pCamera = ref_msg.m_pView->GetCullingCamera();
  const WVec3 cameraPosition = pCamera->GetCenterPosition();
  const WVec3 cameraDirection = pCamera->GetCenterDirForwards();

  auto pManager = static_cast<const WProcPlacementComponentManager*>(GetOwningManager());
  pManager->AddVisibleComponent(GetHandle(), cameraPosition, cameraDirection);
}

void WProcPlacementComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_BoxExtents).IgnoreResult();
}

void WProcPlacementComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s.ReadArray(m_BoxExtents).IgnoreResult();
}

WUInt32 WProcPlacementComponent::BoxExtents_GetCount() const
{
  return m_BoxExtents.GetCount();
}

const WProcGenBoxExtents& WProcPlacementComponent::BoxExtents_GetValue(WUInt32 uiIndex) const
{
  return m_BoxExtents[uiIndex];
}

void WProcPlacementComponent::BoxExtents_SetValue(WUInt32 uiIndex, const WProcGenBoxExtents& value)
{
  m_BoxExtents.EnsureCount(uiIndex + 1);
  m_BoxExtents[uiIndex] = value;

  UpdateBoundsAndTiles();
}

void WProcPlacementComponent::BoxExtents_Insert(WUInt32 uiIndex, const WProcGenBoxExtents& value)
{
  m_BoxExtents.InsertAt(uiIndex, value);

  UpdateBoundsAndTiles();
}

void WProcPlacementComponent::BoxExtents_Remove(WUInt32 uiIndex)
{
  m_BoxExtents.RemoveAtAndCopy(uiIndex);

  UpdateBoundsAndTiles();
}

void WProcPlacementComponent::UpdateBoundsAndTiles()
{
  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<WProcPlacementComponentManager*>(GetOwningManager());

    pManager->RemoveComponent(this);

    GetOwner()->UpdateLocalBounds();

    m_Bounds.Clear();
    m_OutputContexts.Clear();

    WSimdTransform ownerTransform = GetOwner()->GetGlobalTransformSimd();
    for (auto& boxExtent : m_BoxExtents)
    {
      WSimdTransform localBoxTransform;
      localBoxTransform.m_Position = WSimdConversion::ToVec3(boxExtent.m_vOffset);
      localBoxTransform.m_Rotation = WSimdConversion::ToQuat(boxExtent.m_Rotation);
      localBoxTransform.m_Scale = WSimdConversion::ToVec3(boxExtent.m_vExtents * 0.5f);

      WSimdTransform finalBoxTransform;
      finalBoxTransform = WSimdTransform::MakeGlobalTransform(ownerTransform, localBoxTransform);

      WSimdMat4f finalBoxMat = finalBoxTransform.GetAsMat4();

      WSimdBBox globalBox(WSimdVec4f(-1.0f), WSimdVec4f(1.0f));
      globalBox.Transform(finalBoxMat);

      auto& bounds = m_Bounds.ExpandAndGetRef();
      bounds.m_GlobalBoundingBox = globalBox;
      bounds.m_GlobalToLocalBoxTransform = finalBoxMat.GetInverse();
    }

    pManager->AddComponent(this);
  }
}

//////////////////////////////////////////////////////////////////////////

WResult WProcGenBoxExtents::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_vOffset;
  inout_stream << m_Rotation;
  inout_stream << m_vExtents;

  return W_SUCCESS;
}

WResult WProcGenBoxExtents::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_vOffset;
  inout_stream >> m_Rotation;
  inout_stream >> m_vExtents;

  return W_SUCCESS;
}


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcPlacementComponent);
