#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

enum
{
  EmptyTileIndex = WInvalidIndex,
  NewTileIndex = EmptyTileIndex - 1
};

class WProcPlacementComponent;

namespace WProcGenInternal
{
  class FindPlacementTilesTask final : public WTask
  {
  public:
    FindPlacementTilesTask(WProcPlacementComponent* pComponent, WUInt32 uiOutputIndex);
    ~FindPlacementTilesTask();

    void AddCameraPosition(const WVec3& vCameraPosition) { m_CameraPositions.PushBack(vCameraPosition); }

    WArrayPtr<const PlacementTileDesc> GetNewTiles() const { return m_NewTiles; }
    WArrayPtr<const WUInt64> GetOldTiles() const { return m_OldTileKeys; }

  private:
    virtual void Execute() override;

    WProcPlacementComponent* m_pComponent = nullptr;
    WUInt32 m_uiOutputIndex = 0;

    WHybridArray<WVec3, 2> m_CameraPositions;

    WDynamicArray<PlacementTileDesc, WAlignedAllocatorWrapper> m_NewTiles;
    WDynamicArray<WUInt64> m_OldTileKeys;

    struct TileByAge
    {
      W_DECLARE_POD_TYPE();

      WUInt64 m_uiTileKey;
      WUInt64 m_uiLastSeenFrame;
    };

    WDynamicArray<TileByAge> m_TilesByAge;
  };

  W_ALWAYS_INLINE WUInt64 GetTileKey(WInt32 x, WInt32 y)
  {
    WUInt64 sx = (WUInt32)x;
    WUInt64 sy = (WUInt32)y;

    return (sx << 32) | sy;
  }
} // namespace WProcGenInternal
