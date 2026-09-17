#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>

WIPCObjectMirrorEditor::WIPCObjectMirrorEditor()
  : WDocumentObjectMirror()
{
  m_pIPC = nullptr;
}

WIPCObjectMirrorEditor::~WIPCObjectMirrorEditor() = default;

void WIPCObjectMirrorEditor::SetIPC(WEditorEngineConnection* pIPC)
{
  W_ASSERT_DEBUG(m_pContext == nullptr, "Need to call SetIPC before SetReceiver");
  m_pIPC = pIPC;
}

WEditorEngineConnection* WIPCObjectMirrorEditor::GetIPC()
{
  return m_pIPC;
}

void WIPCObjectMirrorEditor::ApplyOp(WObjectChange& ref_change)
{
  if (m_pManager)
  {
    SendOp(ref_change);
  }
  else
  {
    W_REPORT_FAILURE("WIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}

void WIPCObjectMirrorEditor::SendOp(WObjectChange& change)
{
  W_ASSERT_DEBUG(m_pIPC != nullptr, "Need to call SetIPC before SetReceiver");

  WEntityMsgToEngine msg;
  msg.m_change = std::move(change);

  m_pIPC->SendMessage(&msg);
}
