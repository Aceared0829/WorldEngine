#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Foundation/Types/UniquePtr.h>

template <typename T>
class QList;
class QString;
using QStringList = QList<QString>;

class W_EDITORFRAMEWORK_DLL WEditorProcessCommunicationChannel : public WProcessCommunicationChannel
{
public:
  WResult StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const WRTTI* pFirstAllowedMessageType = nullptr,
    WUInt32 uiMemSize = 1024 * 1024 * 10);
  bool IsClientAlive() const;
  void CloseConnection();
  WString GetStdoutContents();
  WOsProcessID GetProcessId() const;

private:
  WUniquePtr<WProcessGroup> m_pClientProcessGroup;
};

class W_EDITORFRAMEWORK_DLL WEditorProcessRemoteCommunicationChannel : public WProcessCommunicationChannel
{
public:
  WResult ConnectToServer(const char* szAddress);

  bool IsConnected() const;

  void CloseConnection();

  void TryConnect();

private:
};
