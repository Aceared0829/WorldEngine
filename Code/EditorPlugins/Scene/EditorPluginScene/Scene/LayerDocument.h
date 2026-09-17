#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>

class WScene2Document;

class W_EDITORPLUGINSCENE_DLL WLayerDocument : public WSceneDocument
{
  W_ADD_DYNAMIC_REFLECTION(WLayerDocument, WSceneDocument);

public:
  WLayerDocument(WStringView sDocumentPath, WScene2Document* pParentScene);
  ~WLayerDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual WVariant GetCreateEngineMetaData() const override;
};
