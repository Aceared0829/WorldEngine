#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

using WCommentComponentManager = WComponentManager<class WCommentComponent, WBlockStorageType::Compact>;

/// This component is for adding notes to objects in a scene.
///
/// These comments are solely to explain things to other people that look at the scene or prefab structure.
/// They are not meant for use at runtime. Therefore, all instances of WCommentComponent are automatically stripped from a scene during export.
class W_ENGINEPLUGINSCENE_DLL WCommentComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WCommentComponent, WComponent, WCommentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WCommentComponent

public:
  WCommentComponent();
  ~WCommentComponent();

  void SetComment(const char* szText);
  const char* GetComment() const;

private:
  WHashedString m_sComment;
};

//////////////////////////////////////////////////////////////////////////

class W_ENGINEPLUGINSCENE_DLL WSceneExportModifier_RemoveCommentComponents : public WSceneExportModifier
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier_RemoveCommentComponents, WSceneExportModifier);

public:
  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) override;
};
