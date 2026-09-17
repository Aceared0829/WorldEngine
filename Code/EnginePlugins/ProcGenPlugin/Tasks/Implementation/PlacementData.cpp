#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>

namespace WProcGenInternal
{
  PlacementData::PlacementData() = default;
  PlacementData::~PlacementData() = default;

  void PlacementData::Clear()
  {
    m_pPhysicsModule = nullptr;
    m_pWorld = nullptr;

    m_pOutput = nullptr;
    m_uiTileSeed = 0;
    m_TileBoundingBox = WBoundingBox::MakeInvalid();
    m_bDebugVisualization = false;
    m_GlobalToLocalBoxTransforms.Clear();

    m_VolumeCollections.Clear();
    m_GlobalData.Clear();
  }
} // namespace WProcGenInternal
