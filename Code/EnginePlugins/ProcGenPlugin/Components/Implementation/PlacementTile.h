#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Declarations.h>

class WPhysicsWorldModuleInterface;

namespace WProcGenInternal
{
  class PlacementTile
  {
  public:
    PlacementTile();
    PlacementTile(PlacementTile&& other);
    ~PlacementTile();

    void Initialize(const PlacementTileDesc& desc, WSharedPtr<const PlacementOutput>& ref_pOutput);
    void Deinitialize(WWorld& ref_world);

    bool IsValid() const;

    const PlacementTileDesc& GetDesc() const;
    const PlacementOutput* GetOutput() const;
    WArrayPtr<const WGameObjectHandle> GetPlacedObjects() const;
    WBoundingBox GetBoundingBox() const;
    WColor GetDebugColor() const;

    void PreparePlacementData(const WWorld* pWorld, const WPhysicsWorldModuleInterface* pPhysicsModule, bool bDebugVisualization, PlacementData& ref_placementData);

    WUInt32 PlaceObjects(WWorld& ref_world, WArrayPtr<const PlacementTransform> objectTransforms);

  private:
    PlacementTileDesc m_Desc;
    WSharedPtr<const PlacementOutput> m_pOutput;

    struct State
    {
      enum Enum
      {
        Invalid,
        Initialized,
        Scheduled,
        Finished
      };
    };

    State::Enum m_State = State::Invalid;
    WDynamicArray<WGameObjectHandle> m_PlacedObjects;
  };
} // namespace WProcGenInternal
