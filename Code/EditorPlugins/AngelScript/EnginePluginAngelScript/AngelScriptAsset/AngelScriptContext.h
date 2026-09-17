#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAngelScript/EnginePluginAngelScriptDLL.h>

class W_ENGINEPLUGINAS_DLL WAngelScriptDocumentContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WAngelScriptDocumentContext, WEngineProcessDocumentContext);

public:
  WAngelScriptDocumentContext();
  ~WAngelScriptDocumentContext();

protected:
  virtual WStatus ExportDocument(const WExportDocumentMsgToEngine* pMsg) override;
  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  WEngineProcessViewContext* CreateViewContext() override;
  void DestroyViewContext(WEngineProcessViewContext* pContext) override;

  void SyncExposedParameters();
  asIScriptModule* CompileModule(WStringBuilder& out_sCode, WSet<WString>* out_pDependencies);
  void RetrieveScriptInfos(WStringView sBasePath);

  WString m_sInputFile;
  WString m_sClass;
  WString m_sCode;
};
