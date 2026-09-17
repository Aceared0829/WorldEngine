#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

/// A export modifier that finalizes the collision mesh generation from WJoltGenerateCollisionComponents.
///
/// A static mesh actor which references the generated collision mesh and the generate component is removed from scenes (not prefabs though).
class W_ENGINEPLUGINJOLT_DLL WSceneExportModifier_JoltFinalizeGeneratedCollision : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_JoltFinalizeGeneratedCollision, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};
