#pragma once

#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WRemoteEngineProcessViewContext : public WEngineProcessViewContext
{
public:
  WRemoteEngineProcessViewContext(WEngineProcessDocumentContext* pContext);
  ~WRemoteEngineProcessViewContext();

protected:
  virtual void HandleViewMessage(const WEditorEngineViewMsg* pMsg) override;
  virtual WViewHandle CreateView() override;

  static WUInt32 s_uiActiveViewID;
  static WRemoteEngineProcessViewContext* s_pActiveRemoteViewContext;
};
