#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

/// An object mirror that mirrors across IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another
/// one on the engine side as receiver.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WIPCObjectMirrorEngine : public WDocumentObjectMirror
{
public:
  WIPCObjectMirrorEngine();
  ~WIPCObjectMirrorEngine();

  virtual void ApplyOp(WObjectChange& inout_change) override;
};
