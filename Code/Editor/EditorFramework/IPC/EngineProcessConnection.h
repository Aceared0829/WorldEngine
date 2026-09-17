#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

class WEditorEngineConnection;
class WDocument;
class WDocumentObject;
struct WDocumentObjectPropertyEvent;
struct WDocumentObjectStructureEvent;
class WQtEngineDocumentWindow;
class WAssetDocument;

class W_EDITORFRAMEWORK_DLL WEditorEngineProcessConnection
{
  W_DECLARE_SINGLETON(WEditorEngineProcessConnection);

public:
  WEditorEngineProcessConnection();
  ~WEditorEngineProcessConnection();

  /// The given file system configuration will be used by the engine process to setup the runtime data directories.
  ///        This only takes effect if the editor process is restarted.
  void SetFileSystemConfig(const WApplicationFileSystemConfig& cfg) { m_FileSystemConfig = cfg; }

  /// The given plugin configuration will be used by the engine process to load runtime plugins.
  ///        This only takes effect if the editor process is restarted.
  void SetPluginConfig(const WApplicationPluginConfig& cfg) { m_PluginConfig = cfg; }

  void Update();
  WResult RestartProcess();
  void ShutdownProcess();
  bool IsProcessCrashed() const { return m_bProcessCrashed; }

  WEditorEngineConnection* CreateEngineConnection(WAssetDocument* pDocument);
  void DestroyEngineConnection(WAssetDocument* pDocument);

  bool SendMessage(WProcessMessage* pMessage);

  /// Waits for a message of type pMessageType. If tTimeout is zero, the function will not timeout. If the timeout is valid
  /// and is it, W_FAILURE is returned. If the message type matches and pCallback is valid, the function will be called
  /// and the return values decides whether the message is to be accepted and the waiting has ended.
  WResult WaitForMessage(const WRTTI* pMessageType, WTime timeout, WProcessCommunicationChannel ::WaitForMessageCallback* pCallback = nullptr);
  /// Same as WaitForMessage but the message must be to a specific document. Therefore,
  /// pMessageType must be derived from WEditorEngineDocumentMsg and the function will only return if the received
  /// message matches both type, document and is accepted by pCallback.
  WResult WaitForDocumentMessage(const WUuid& assetGuid, const WRTTI* pMessageType, WTime timeout, WProcessCommunicationChannel::WaitForMessageCallback* pCallback = nullptr);

  bool IsEngineSetup() const { return m_bClientIsConfigured; }

  WOsProcessID GetEngineProcessID() const { return m_IPC.GetProcessId(); }

  void ActivateRemoteProcess(const WAssetDocument* pDocument, WUInt32 uiViewID);

  WProcessCommunicationChannel& GetCommunicationChannel() { return m_IPC; }

  struct Event
  {
    enum class Type
    {
      Invalid,
      ProcessStarted,
      ProcessCrashed,
      ProcessShutdown,
      ProcessMessage,
      ProcessRestarted,
      ProcessStuck,   ///< Engine process is not responding
      ProcessUnstuck, ///< Engine process started responding again after ProcessStuck was triggered
    };

    Event()
    {
      m_Type = Type::Invalid;
      m_pMsg = nullptr;
    }

    Type m_Type;
    const WProcessMessage* m_pMsg;
  };

  static WEvent<const Event&> s_Events;

private:
  void Initialize(const WRTTI* pFirstAllowedMessageType);
  void HandleIPCEvent(const WProcessCommunicationChannel::Event& e);
  void UIServicesTickEventHandler(const WQtUiServices::TickEvent& e);
  bool ConnectToRemoteProcess();
  void ShutdownRemoteProcess();

  static constexpr WUInt32 s_uiMaxFailedRedrawCount = 5 * 60;
  bool m_bProcessShouldBeRunning = false;
  bool m_bProcessCrashed = false;
  bool m_bClientIsConfigured = false;
  WEventSubscriptionID m_TickEventSubscriptionID = 0;
  WUInt32 m_uiRedrawCountSent = 0;
  WUInt32 m_uiRedrawCountReceived = 0;
  WUInt32 m_uiFailedRedrawCount = 0;

  WEditorProcessCommunicationChannel m_IPC;
  WUniquePtr<WEditorProcessRemoteCommunicationChannel> m_pRemoteProcess;
  WApplicationFileSystemConfig m_FileSystemConfig;
  WApplicationPluginConfig m_PluginConfig;
  WHashTable<WUuid, WAssetDocument*> m_DocumentByGuid;
};

class W_EDITORFRAMEWORK_DLL WEditorEngineConnection
{
public:
  bool SendMessage(WEditorEngineDocumentMsg* pMessage);
  void SendHighlightObjectMessage(WViewHighlightMsgToEngine* pMessage);

  WDocument* GetDocument() const { return m_pDocument; }

private:
  friend class WEditorEngineProcessConnection;
  WEditorEngineConnection(WDocument* pDocument) { m_pDocument = pDocument; }
  ~WEditorEngineConnection() = default;

  WDocument* m_pDocument;
};
