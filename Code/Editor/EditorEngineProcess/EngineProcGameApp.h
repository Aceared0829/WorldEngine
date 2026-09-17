#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>

class WEditorEngineProcessApp;
class WDocumentOpenMsgToEngine;
class WEngineProcessDocumentContext;
class WResourceUpdateMsgToEngine;
class WRestoreResourceMsgToEngine;

class WEngineProcessGameApplication : public WGameApplication
{
public:
  using SUPER = WGameApplication;

  WEngineProcessGameApplication();
  ~WEngineProcessGameApplication();

  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Run() override;

  void LogWriter(const WLoggingEventData& e);

  virtual bool ShouldApplicationQuit() const override
  {
    // override the behavior of WGameApplicationGase
    // so ignore what the game-state does
    return WApplication::ShouldApplicationQuit();
  }

protected:
  virtual void BaseInit_ConfigureLogging() override;
  virtual void Deinit_ShutdownLogging() override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual bool Run_ProcessApplicationInput() override;
  virtual WUniquePtr<WEditorEngineProcessApp> CreateEngineProcessApp();

  virtual void ActivateGameStateAtStartup() override
  {
    /* do nothing */
  }

private:
  void ConnectToHost();
  void DisableErrorReport();
  void WaitForDebugger();
  static bool EditorAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);
  void AddEditorAssertHandler();
  void RemoveEditorAssertHandler();

  bool ProcessIPCMessages(bool bPendingOpInProgress);
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const WEngineProcessCommunicationChannel::Event& e);
  void EventHandlerCVar(const WCVarEvent& e);
  void EventHandlerCVarPlugin(const WPluginEvent& e);
  void TransmitCVar(const WCVar* pCVar);

  void HandleResourceUpdateMsg(const WResourceUpdateMsgToEngine& msg);
  void HandleResourceRestoreMsg(const WRestoreResourceMsgToEngine& msg);

  WEngineProcessDocumentContext* CreateDocumentContext(const WDocumentOpenMsgToEngine* pMsg);

  virtual void Init_LoadProjectPlugins() override;

  virtual WString FindProjectDirectory() const override;

  WString m_sProjectDirectory;
  WApplicationFileSystemConfig m_CustomFileSystemConfig;
  WApplicationPluginConfig m_CustomPluginConfig;
  WEngineProcessCommunicationChannel m_IPC;
  WUniquePtr<WEditorEngineProcessApp> m_pApp;
  WLongOpWorkerManager m_LongOpWorkerManager;
  WLogWriter::HTML m_LogHTML;

  WUInt32 m_uiRedrawCountReceived = 0;
  WUInt32 m_uiRedrawCountExecuted = 0;

  /// Sends log messages that were queued up by LogWriter() from non-main threads.
  void SendQueuedLogMessages();

  /// Resolves the log links in the message and sends it to the editor. Must be called from the main thread.
  void SendLogMessage(WLogEntry& ref_entry);

  WMutex m_QueuedLogMsgMutex;
  WDeque<WLogEntry> m_QueuedLogMsgs;
};
