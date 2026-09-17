#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/World/World.h>
#include <DetourNavMesh.h>
#include <Foundation/Configuration/CVar.h>

WCVarInt cvar_NavMeshVisualize("AI.Navmesh.Visualize", -1, WCVarFlags::None, "Visualize the n-th navmesh.");

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WAiNavMeshWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAiNavMeshWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAiNavMeshWorldModule::WAiNavMeshWorldModule(WWorld* pWorld)
  : WWorldModule(pWorld)
{
  m_Config.Load().IgnoreResult();

  {
    // add a default filter
    auto& cfg = m_Config.m_PathSearchConfigs.ExpandAndGetRef();
  }

  for (const auto& cfg : m_Config.m_PathSearchConfigs)
  {
    auto& filter = m_PathSearchFilters[cfg.m_sName];

    WUInt32 groundMask = 0;
    for (WUInt32 gt = 0; gt < WAiNumGroundTypes; ++gt)
    {
      if (cfg.m_bGroundTypeAllowed[gt])
        groundMask |= (1 << gt);

      filter.setAreaCost((int)gt, cfg.m_fGroundTypeCost[gt]);
    }

    filter.setIncludeAreaBits(groundMask);
  }

  if (m_Config.m_NavmeshConfigs.IsEmpty())
  {
    // insert a default navmesh config
    m_Config.m_NavmeshConfigs.ExpandAndGetRef();
  }
}

WAiNavMeshWorldModule::~WAiNavMeshWorldModule()
{
  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    W_DEFAULT_DELETE(m_WorldNavMeshes[cfg.m_sName]);
  }
}

void WAiNavMeshWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WAiNavMeshWorldModule::Update, this);
    updateDesc.m_Phase = WWorldUpdatePhase::PostTransform;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  m_WorldNavMeshes.Clear();

  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    // TODO: make tile size etc configurable
    m_WorldNavMeshes[cfg.m_sName] = W_DEFAULT_NEW(WAiNavMesh, cfg);
  }

  m_pGenerateSectorTask = W_DEFAULT_NEW(WNavMeshSectorGenerationTask);
  m_pGenerateSectorTask->ConfigureTask("Generate Navmesh Sector", WTaskNesting::Maybe);
}

void WAiNavMeshWorldModule::Deinitialize()
{
  m_pGenerateSectorTask = nullptr;
  WTaskSystem::CancelGroup(m_GenerateSectorTaskID).IgnoreResult();
  WTaskSystem::WaitForGroup(m_GenerateSectorTaskID);
}

WAiNavMesh* WAiNavMeshWorldModule::GetNavMesh(WStringView sName)
{
  auto it = m_WorldNavMeshes.Find(sName);
  if (it.IsValid())
    return it.Value();

  return nullptr;
}

const WAiNavMesh* WAiNavMeshWorldModule::GetNavMesh(WStringView sName) const
{
  auto it = m_WorldNavMeshes.Find(sName);
  if (it.IsValid())
    return it.Value();

  return nullptr;
}

void WAiNavMeshWorldModule::Update(const UpdateContext& ctxt)
{
  if (m_uiUpdateDelay > 0)
  {
    --m_uiUpdateDelay;
    return;
  }

  for (auto& nm : m_WorldNavMeshes)
  {
    nm.Value()->FinalizeSectorUpdates();
  }

  if (cvar_NavMeshVisualize >= 0)
  {
    WInt32 i = cvar_NavMeshVisualize;
    for (auto it = m_WorldNavMeshes.GetIterator(); it.IsValid(); ++it)
    {
      if (i-- == 0)
      {
        it.Value()->DebugDraw(GetWorld(), m_Config);
        break;
      }
    }
  }

  if (!WTaskSystem::IsTaskGroupFinished(m_GenerateSectorTaskID))
    return;

  auto pNavGeo = GetWorld()->GetOrCreateModule<WNavmeshGeoWorldModuleInterface>();
  if (pNavGeo == nullptr)
    return;

  for (auto& nm : m_WorldNavMeshes)
  {
    auto sectorID = nm.Value()->RetrieveRequestedSector();
    if (sectorID == WInvalidIndex)
      continue;

    m_pGenerateSectorTask->m_pWorldNavMesh = nm.Value();
    m_pGenerateSectorTask->m_SectorID = sectorID;
    m_pGenerateSectorTask->m_pNavGeo = pNavGeo;

    m_GenerateSectorTaskID = WTaskSystem::StartSingleTask(m_pGenerateSectorTask, WTaskPriority::LongRunning);

    break;
  }
}

const dtQueryFilter& WAiNavMeshWorldModule::GetPathSearchFilter(WStringView sName) const
{
  auto it = m_PathSearchFilters.Find(sName);
  if (it.IsValid())
    return it.Value();

  it = m_PathSearchFilters.Find("");
  WLog::Warning("Ai Path Search Filter '{}' does not exist.", sName);
  return it.Value();
}


W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Implementation_NavMeshWorldModule);
