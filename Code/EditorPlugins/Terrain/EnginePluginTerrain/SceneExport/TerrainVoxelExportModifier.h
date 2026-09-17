#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginTerrain/EnginePluginTerrainDLL.h>

/// Scene export modifier that bakes voxel volume colliders to disk.
///
/// For each active WTerrainVolumeComponent with EnableCollider = true, it reads back
/// the GPU-baked mesh data, cooks a Jolt triangle mesh, and writes a .WBinJoltTriangleMesh file to
/// AssetCache/Generated/. A child game object named "VoxelCollider" is then created (or
/// reused) with an WJoltStaticActorComponent pointing to that file.
class W_ENGINEPLUGINTERRAIN_DLL WSceneExportModifier_TerrainVoxelCollision : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_TerrainVoxelCollision, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};
