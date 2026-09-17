#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

class W_ENGINEPLUGINJOLT_DLL WSceneExportModifier_JoltStaticMeshConversion : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_JoltStaticMeshConversion, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};
