#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class W_ENGINEPLUGINASSETS_DLL WStateMachineContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineContext, WEngineProcessDocumentContext);

public:
  WStateMachineContext();

protected:
  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;

  virtual WStatus ExportDocument(const WExportDocumentMsgToEngine* pMsg) override;
};
