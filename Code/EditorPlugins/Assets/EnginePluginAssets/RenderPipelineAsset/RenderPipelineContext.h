#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class W_ENGINEPLUGINASSETS_DLL WRenderPipelineContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelineContext, WEngineProcessDocumentContext);

public:
  WRenderPipelineContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;

  virtual WStatus ExportDocument(const WExportDocumentMsgToEngine* pMsg) override;
};
