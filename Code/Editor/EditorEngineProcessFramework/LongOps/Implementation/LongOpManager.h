#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

/// Base class with shared functionality for WLongOpControllerManager and WLongOpWorkerManager
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpManager
{
public:
  /// Needs to be called early to initialize the IPC channel to use.
  void Startup(WProcessCommunicationChannel* pCommunicationChannel);

  /// Call this to shut down the IPC communication.
  void Shutdown();

  /// Publicly exposed mutex for some special cases.
  mutable WMutex m_Mutex;

protected:
  virtual void ProcessCommunicationChannelEventHandler(const WProcessCommunicationChannel::Event& e) = 0;

  WProcessCommunicationChannel* m_pCommunicationChannel = nullptr;
  WEvent<const WProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;
};
