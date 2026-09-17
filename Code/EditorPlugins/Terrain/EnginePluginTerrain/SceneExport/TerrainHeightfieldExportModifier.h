#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginTerrain/EnginePluginTerrainDLL.h>

/// Scene export modifier that bakes terrain heightfield colliders and occluders from the GPU height data.
///
/// For each WTerrainPatchComponent with a non-None collider mode, it reads back the GPU-baked height data,
/// cooks a Jolt heightfield shape, and writes a .WBinJoltHeightfield file to AssetCache/Generated/. A child
/// game object named "HeightfieldCollider" is then created (or reused) with an
/// WJoltHeightfieldColliderComponent pointing to that file.
///
/// For each patch with a non-zero occlusion cell size, it additionally computes a coarse, conservative mesh and
/// stores it on the component, from where the patch builds its CPU occlusion culling geometry. Both use the
/// same readback, which is why they live in one modifier: a readback forces a full re-bake of the patch.
class W_ENGINEPLUGINTERRAIN_DLL WSceneExportModifier_TerrainHeightfieldCollision : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_TerrainHeightfieldCollision, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};
