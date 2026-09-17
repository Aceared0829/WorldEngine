#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

/// An object mirror that mirrors across IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another
/// one on the engine side as receiver.
class W_EDITORFRAMEWORK_DLL WIPCObjectMirrorEditor : public WDocumentObjectMirror
{
public:
  WIPCObjectMirrorEditor();
  ~WIPCObjectMirrorEditor();

  void SetIPC(WEditorEngineConnection* pIPC);
  WEditorEngineConnection* GetIPC();
  virtual void ApplyOp(WObjectChange& ref_change) override;

private:
  void SendOp(WObjectChange& change);

  WEditorEngineConnection* m_pIPC;
};
