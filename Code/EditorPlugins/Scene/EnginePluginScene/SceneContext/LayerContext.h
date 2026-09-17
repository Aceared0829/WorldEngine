#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class WDocumentOpenMsgToEngine;

/// Layers that are loaded as sub-documents of a scene share the WWorld with their main document scene. Thus, this context attaches itself to its parent WSceneContext.
class W_ENGINEPLUGINSCENE_DLL WLayerContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WLayerContext, WEngineProcessDocumentContext);

public:
  static WEngineProcessDocumentContext* AllocateContext(const WDocumentOpenMsgToEngine* pMsg);
  WLayerContext();
  ~WLayerContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;
  void SceneDeinitialized();
  const WTag& GetLayerTag() const;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual WStatus ExportDocument(const WExportDocumentMsgToEngine* pMsg) override;

  virtual void UpdateDocumentContext() override;

private:
  WSceneContext* m_pParentSceneContext = nullptr;
  WTag m_LayerTag;
};
