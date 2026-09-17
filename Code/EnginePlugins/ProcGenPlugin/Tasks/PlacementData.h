#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <ProcGenPlugin/Declarations.h>

class WPhysicsWorldModuleInterface;
class WVolumeCollection;

namespace WProcGenInternal
{
  struct PlacementData
  {
    PlacementData();
    ~PlacementData();

    void Clear();

    const WPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;
    const WWorld* m_pWorld = nullptr;

    WSharedPtr<const PlacementOutput> m_pOutput;
    WUInt32 m_uiTileSeed = 0;
    WBoundingBox m_TileBoundingBox;
    bool m_bDebugVisualization = false;

    WDynamicArray<WSimdMat4f, WAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;

    WDeque<WVolumeCollection> m_VolumeCollections;
    WExpression::GlobalData m_GlobalData;
  };
} // namespace WProcGenInternal
