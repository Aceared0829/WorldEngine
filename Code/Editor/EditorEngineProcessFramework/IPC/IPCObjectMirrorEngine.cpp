#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

WIPCObjectMirrorEngine::WIPCObjectMirrorEngine()
  : WDocumentObjectMirror()
{
}

WIPCObjectMirrorEngine::~WIPCObjectMirrorEngine() = default;

void WIPCObjectMirrorEngine::ApplyOp(WObjectChange& inout_change)
{
  if (m_pContext)
  {
    WDocumentObjectMirror::ApplyOp(inout_change);
  }
  else
  {
    W_REPORT_FAILURE("WIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}
