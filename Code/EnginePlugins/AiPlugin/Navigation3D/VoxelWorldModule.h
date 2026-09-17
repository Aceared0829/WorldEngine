#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>

class WAiVoxelGridComponent;

/// World module that manages the voxel grids used for 3D navigation.
///
/// Access it via GetWorld()->GetOrCreateModule<WAiVoxelWorldModule>().
class W_AIPLUGIN_DLL WAiVoxelWorldModule final : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WAiVoxelWorldModule, WWorldModule);

public:
  WAiVoxelWorldModule(WWorld* pWorld);
  ~WAiVoxelWorldModule();

  virtual void Initialize() override;

  /// Returns true once the grid has been voxelized and is ready for pathfinding.
  bool IsReady() const { return m_bIsReady; }

  /// Finds all WAiVoxelGridComponent instances whose bounds overlap the given AABB.
  void FindGridsInBox(const WBoundingBox& box, WDynamicArray<WAiVoxelGridComponent*>& out_grids) const;

private:
  void Update(const UpdateContext& ctxt);

  bool m_bIsReady = false;

  /// Number of PostTransform updates to wait after simulation start before the first voxelization
  /// (and thus before IsReady() returns true). Gives the collision geometry - physics bodies and
  /// spawned/streamed objects - time to settle so the static grid rasterizes the final world state.
  WUInt32 m_uiUpdateDelay = 10;
};
