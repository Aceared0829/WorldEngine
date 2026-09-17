#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGridComponent.h>
#include <AiPlugin/Navigation3D/VoxelWorldModule.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>

WCVarBool cvar_VoxelGridVisualize("AI.VoxelGrid.Visualize", false, WCVarFlags::None, "Visualize the voxel grid. Use AI.VoxelGrid.Visualize=true in the console to enable.");

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WAiVoxelWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAiVoxelWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAiVoxelWorldModule::WAiVoxelWorldModule(WWorld* pWorld)
  : WWorldModule(pWorld)
{
}

WAiVoxelWorldModule::~WAiVoxelWorldModule() = default;

void WAiVoxelWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WAiVoxelWorldModule::Update, this);
    updateDesc.m_Phase = WWorldUpdatePhase::PostTransform;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }
}

void WAiVoxelWorldModule::Update(const UpdateContext& ctxt)
{
  if (m_uiUpdateDelay > 0)
  {
    --m_uiUpdateDelay;
    return;
  }

  auto* pGridManager = GetWorld()->GetComponentManager<WAiVoxelGridComponentManager>();
  if (pGridManager == nullptr)
    return;

  for (auto it = pGridManager->GetComponents(); it.IsValid(); it.Next())
  {
    it->VoxelizeWorld();
  }

  m_bIsReady = true;

  for (auto it = pGridManager->GetComponents(); it.IsValid(); it.Next())
  {
    if (it->m_bVisualize || cvar_VoxelGridVisualize)
    {
      const WVoxelGrid& grid = it->GetStaticVoxelGrid();
      grid.DebugDraw(GetWorld(), WColor::LimeGreen.WithAlpha(0.1f));

      const WVec3U32 dim = grid.GetDimensions();
      const WUInt64 uiMemUsage = grid.GetHeapMemoryUsage();

      WDebugRenderer::Draw3DText(GetWorld(), WFmt("Voxel Grid\nCells: {} x {} x {}\nMemory: {}", dim.x, dim.y, dim.z, WArgFileSize(uiMemUsage)), grid.GetCenter(), WColor::White);
    }
  }
}

void WAiVoxelWorldModule::FindGridsInBox(const WBoundingBox& box, WDynamicArray<WAiVoxelGridComponent*>& out_grids) const
{
  out_grids.Clear();

  const WSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
  if (pSpatialSystem == nullptr)
    return;

  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = WAiVoxelGridComponent::SpatialDataCategory.GetBitmask();

  WDynamicArray<WGameObject*> objects;
  pSpatialSystem->FindObjectsInBox(box, queryParams, objects);

  for (WGameObject* pObject : objects)
  {
    WAiVoxelGridComponent* pGridComponent = nullptr;
    if (pObject->TryGetComponentOfBaseType(pGridComponent))
    {
      out_grids.PushBack(pGridComponent);
    }
  }
}


W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation3D_Implementation_VoxelWorldModule);
