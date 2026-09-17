#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

class W_ENGINEPLUGINSCENE_DLL WSceneExportModifier_RemoveShapeIconComponents : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_RemoveShapeIconComponents, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class W_ENGINEPLUGINSCENE_DLL WSceneExportModifier_RemovePathNodeComponents : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_RemovePathNodeComponents, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class W_ENGINEPLUGINSCENE_DLL WSceneExportModifier_GenericExport : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_GenericExport, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};
