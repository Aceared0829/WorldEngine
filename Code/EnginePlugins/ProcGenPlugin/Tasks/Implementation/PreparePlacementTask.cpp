#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>

using namespace WProcGenInternal;

PreparePlacementTask::PreparePlacementTask(PlacementData* pData, const char* szName)
  : m_pData(pData)
{
  ConfigureTask(szName, WTaskNesting::Maybe);
}

PreparePlacementTask::~PreparePlacementTask() = default;

void PreparePlacementTask::Execute()
{
  const WWorld& world = *m_pData->m_pWorld;
  const WBoundingBox& box = m_pData->m_TileBoundingBox;
  const Output& output = *m_pData->m_pOutput;

  WProcGenGlobalData::ExtractVolumeCollections(world, box, output, m_pData->m_VolumeCollections, m_pData->m_GlobalData);
  WProcGenGlobalData::SetInstanceSeed(m_pData->m_uiTileSeed, m_pData->m_GlobalData);
  WProcGenGlobalData::SetCurves(output, m_pData->m_GlobalData);
}
