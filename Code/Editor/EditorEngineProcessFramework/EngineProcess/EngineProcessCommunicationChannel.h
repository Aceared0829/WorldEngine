#pragma once

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEngineProcessCommunicationChannel : public WProcessCommunicationChannel
{
public:
  WResult ConnectToHostProcess();

  bool IsHostAlive() const;

private:
  WInt64 m_iHostPID = 0;
};
