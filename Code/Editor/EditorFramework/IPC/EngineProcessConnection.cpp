#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Dialogs/RemoteConnectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


W_IMPLEMENT_SINGLETON(WEditorEngineProcessConnection);

WEvent<const WEditorEngineProcessConnection::Event&> WEditorEngineProcessConnection::s_Events;

WEditorEngineProcessConnection::WEditorEngineProcessConnection()
  : m_SingletonRegistrar(this)
{
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  m_IPC.m_Events.AddEventHandler(WMakeDelegate(&WEditorEngineProcessConnection::HandleIPCEvent, this));
}

WEditorEngineProcessConnection::~WEditorEngineProcessConnection()
{
  m_IPC.m_Events.RemoveEventHandler(WMakeDelegate(&WEditorEngineProcessConnection::HandleIPCEvent, this));
}

void WEditorEngineProcessConnection::HandleIPCEvent(const WProcessCommunicationChannel::Event& e)
{
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<WSyncWithProcessMsgToEditor>())
  {
    const WSyncWithProcessMsgToEditor* msg = static_cast<const WSyncWithProcessMsgToEditor*>(e.m_pMessage);
    m_uiRedrawCountReceived = msg->m_uiRedrawCount;
    if (m_uiFailedRedrawCount >= s_uiMaxFailedRedrawCount)
    {
      Event e;
      e.m_Type = Event::Type::ProcessUnstuck;
      s_Events.Broadcast(e);
    }
    m_uiFailedRedrawCount = 0;
  }
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<WEditorEngineDocumentMsg>())
  {
    const WEditorEngineDocumentMsg* pMsg = static_cast<const WEditorEngineDocumentMsg*>(e.m_pMessage);

    WAssetDocument* pDocument = nullptr;
    if (m_DocumentByGuid.TryGetValue(pMsg->m_DocumentGuid, pDocument))
    {
      pDocument->HandleEngineMessage(pMsg);
    }
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<WEditorEngineMsg>())
  {
    Event ee;
    ee.m_pMsg = static_cast<const WEditorEngineMsg*>(e.m_pMessage);
    ee.m_Type = Event::Type::ProcessMessage;

    s_Events.Broadcast(ee);
  }
}

void WEditorEngineProcessConnection::UIServicesTickEventHandler(const WQtUiServices::TickEvent& e)
{
  if (e.m_Type == WQtUiServices::TickEvent::Type::BeforeFrame)
  {
    if (IsProcessCrashed())
    {
      e.m_uiForceCancelFrame++;
      return;
    }

    m_IPC.ProcessMessages();
    // We still didn't get confirmation from the engine that the frame was drawn, so don't render another frame yet.
    if (m_uiRedrawCountSent > m_uiRedrawCountReceived)
    {
      m_uiFailedRedrawCount++;
      e.m_uiForceCancelFrame++;
      if (m_uiFailedRedrawCount == s_uiMaxFailedRedrawCount)
      {
        Event e;
        e.m_Type = Event::Type::ProcessStuck;
        s_Events.Broadcast(e);
      }
      return;
    }
  }
  else if (e.m_Type == WQtUiServices::TickEvent::Type::EndFrame)
  {
    if (!IsProcessCrashed())
    {
      WSyncWithProcessMsgToEngine sm;
      sm.m_uiRedrawCount = m_uiRedrawCountSent + 1;
      SendMessage(&sm);
      ++m_uiRedrawCountSent;
    }
  }
}

WEditorEngineConnection* WEditorEngineProcessConnection::CreateEngineConnection(WAssetDocument* pDocument)
{
  WEditorEngineConnection* pConnection = new WEditorEngineConnection(pDocument);

  m_DocumentByGuid[pDocument->GetGuid()] = pDocument;

  pDocument->SendDocumentOpenMessage(true);

  return pConnection;
}

void WEditorEngineProcessConnection::DestroyEngineConnection(WAssetDocument* pDocument)
{
  pDocument->SendDocumentOpenMessage(false);

  m_DocumentByGuid.Remove(pDocument->GetGuid());

  delete pDocument->GetEditorEngineConnection();
}

void WEditorEngineProcessConnection::Initialize(const WRTTI* pFirstAllowedMessageType)
{
  W_PROFILE_SCOPE("Initialize");
  if (m_IPC.IsClientAlive())
    return;

  WLog::Dev("Starting Client Engine Process");

  W_ASSERT_DEBUG(m_TickEventSubscriptionID == 0, "A previous subscription is still in place. ShutdownProcess not called?");
  m_TickEventSubscriptionID = WQtUiServices::s_TickEvent.AddEventHandler(WMakeDelegate(&WEditorEngineProcessConnection::UIServicesTickEventHandler, this));

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  WStringBuilder tmp;

  QStringList args = QCoreApplication::arguments();
  args.pop_front(); // Remove first argument which is the name of the path to the editor executable

  {
    WStringBuilder sWndCfgPath = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sWndCfgPath.AppendPath("RuntimeConfigs/Window.ddl");

    if (WFileSystem::ExistsFile(sWndCfgPath))
    {
      args << "-wnd";
      args << sWndCfgPath.GetData();
    }
  }

  // set up the EditorEngineProcess telemetry server on a different port
  {
    args << "-TelemetryPort";
    args << WCommandLineUtils::GetGlobalInstance()->GetStringOption("-TelemetryPort", 0, "1050").GetData(tmp);
  }

  {
    WStringBuilder sRelativeData;
    sRelativeData = ":APPDATA";

    WStringBuilder sAbsoluteData;
    WFileSystem::ResolvePath(sRelativeData, &sAbsoluteData, nullptr).AssertSuccess("Failed to resolve APPDATA dir!");

    args << "-outputDir";
    args << sAbsoluteData.GetData();
    args << "-logName";

    if (WQtEditorApp::GetSingleton()->IsInHeadlessMode())
    {
      tmp.SetFormat("LogEditorProcessor_{}_Engine", WCommandLineUtils::GetGlobalInstance()->GetIntOption("-appid", 0));
      args << tmp.GetData();
    }
    else
    {
      tmp.SetFormat("LogEditor_{}_Engine", WCommandLineUtils::GetGlobalInstance()->GetIntOption("-appid", 0));
      args << tmp.GetData();
    }
  }


#if W_ENABLED(W_PLATFORM_WINDOWS)
  const char* EditorEngineProcessExecutableName = "WEditorEngineProcess.exe";
#elif W_ENABLED(W_PLATFORM_LINUX)
  const char* EditorEngineProcessExecutableName = "WEditorEngineProcess";
#else
#  error Platform not supported
#endif


  if (m_IPC.StartClientProcess(EditorEngineProcessExecutableName, args, false, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
    WLog::Error("EngineProcess crashed on startup");
  }
  else
  {
    Event e;
    e.m_Type = Event::Type::ProcessStarted;
    s_Events.Broadcast(e);
  }
}

void WEditorEngineProcessConnection::ActivateRemoteProcess(const WAssetDocument* pDocument, WUInt32 uiViewID)
{
  // make sure process is started
  if (!ConnectToRemoteProcess())
    return;

  // resend entire document
  {
    // open document message
    {
      WDocumentOpenMsgToEngine msg;
      msg.m_DocumentGuid = pDocument->GetGuid();
      msg.m_bDocumentOpen = true;
      msg.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;
      m_pRemoteProcess->SendMessage(&msg);
    }

    if (pDocument->GetDynamicRTTI()->IsDerivedFrom<WAssetDocument>())
    {
      WAssetDocument* pAssetDoc = (WAssetDocument*)pDocument;
      WDocumentOpenResponseMsgToEditor response;
      response.m_DocumentGuid = pDocument->GetGuid();
      pAssetDoc->HandleEngineMessage(&response);
    }
  }

  // send activation message
  {
    WActivateRemoteViewMsgToEngine msg;
    msg.m_DocumentGuid = pDocument->GetGuid();
    msg.m_uiViewID = uiViewID;
    m_pRemoteProcess->SendMessage(&msg);
  }
}

bool WEditorEngineProcessConnection::ConnectToRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    if (m_pRemoteProcess->IsConnected())
      return true;

    ShutdownRemoteProcess();
  }

  WQtRemoteConnectionDlg dlg(QApplication::activeWindow());

  if (dlg.exec() == QDialog::Rejected)
    return false;

  m_pRemoteProcess = W_DEFAULT_NEW(WEditorProcessRemoteCommunicationChannel);
  m_pRemoteProcess->ConnectToServer(dlg.GetResultingAddress().toUtf8().data()).AssertSuccess();

  WQtWaitForOperationDlg waitDialog(QApplication::activeWindow());
  waitDialog.m_OnIdle = [this]() -> bool
  {
    if (m_pRemoteProcess->IsConnected())
      return false;

    m_pRemoteProcess->TryConnect();
    return true;
  };

  const int iRet = waitDialog.exec();

  if (iRet == QDialog::Accepted)
  {
    // Send project setup.
    WSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = WToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
    msg.m_sFileserveAddress = dlg.GetResultingFsAddress().toUtf8().data();
    msg.m_sAssetProfile = WAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    m_pRemoteProcess->SendMessage(&msg);
  }

  return iRet == QDialog::Accepted;
}


void WEditorEngineProcessConnection::ShutdownRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    WLog::Info("Shutting down Remote Engine Process");
    m_pRemoteProcess->CloseConnection();

    m_pRemoteProcess = nullptr;
  }
}

void WEditorEngineProcessConnection::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  ShutdownRemoteProcess();

  WLog::Info("Shutting down Engine Process");

  if (m_TickEventSubscriptionID != 0)
    WQtUiServices::s_TickEvent.RemoveEventHandler(m_TickEventSubscriptionID);

  m_bClientIsConfigured = false;
  m_bProcessShouldBeRunning = false;
  m_uiRedrawCountReceived = m_uiRedrawCountSent;
  m_IPC.CloseConnection();

  Event e;
  e.m_Type = Event::Type::ProcessShutdown;
  s_Events.Broadcast(e);
}

bool WEditorEngineProcessConnection::SendMessage(WProcessMessage* pMessage)
{
  bool res = m_IPC.SendMessage(pMessage);

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->SendMessage(pMessage);
  }
  return res;
}

WResult WEditorEngineProcessConnection::WaitForMessage(const WRTTI* pMessageType, WTime timeout, WProcessCommunicationChannel::WaitForMessageCallback* pCallback)
{
  W_PROFILE_SCOPE(pMessageType->GetTypeName());
  return m_IPC.WaitForMessage(pMessageType, timeout, pCallback);
}

WResult WEditorEngineProcessConnection::WaitForDocumentMessage(const WUuid& assetGuid, const WRTTI* pMessageType, WTime timeout, WProcessCommunicationChannel::WaitForMessageCallback* pCallback /*= nullptr*/)
{
  if (!m_bProcessShouldBeRunning)
  {
    return W_FAILURE; // if the process is not running, we can't wait for a message
  }
  W_ASSERT_DEBUG(pMessageType->IsDerivedFrom(WGetStaticRTTI<WEditorEngineDocumentMsg>()), "The type of the message to wait for must be a document message.");
  struct WaitData
  {
    WUuid m_AssetGuid;
    WProcessCommunicationChannel::WaitForMessageCallback* m_pCallback;
  };

  WaitData data;
  data.m_AssetGuid = assetGuid;
  data.m_pCallback = pCallback;

  WProcessCommunicationChannel::WaitForMessageCallback callback = [&data](WProcessMessage* pMsg) -> bool
  {
    WEditorEngineDocumentMsg* pMsg2 = WDynamicCast<WEditorEngineDocumentMsg*>(pMsg);
    if (pMsg2 && data.m_AssetGuid == pMsg2->m_DocumentGuid)
    {
      if (data.m_pCallback && data.m_pCallback->IsValid() && !(*data.m_pCallback)(pMsg))
      {
        return false;
      }
      return true;
    }
    return false;
  };

  return m_IPC.WaitForMessage(pMessageType, timeout, &callback);
}

WResult WEditorEngineProcessConnection::RestartProcess()
{
  W_PROFILE_SCOPE("RestartProcess");
  W_LOG_BLOCK("Restarting Engine Process");

  WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Engine Process...", WTime::MakeFromSeconds(5));

  ShutdownProcess();

  Initialize(WGetStaticRTTI<WSetupProjectMsgToEngine>());

  if (m_bProcessCrashed)
  {
    WLog::Error("Engine process crashed during startup.");
    ShutdownProcess();
    return W_FAILURE;
  }

  WLog::Dev("Waiting for IPC connection");

  if (m_IPC.WaitForConnection(WTime()).Failed())
  {
    WLog::Error("Engine process did not connect. Engine process output:\n{}", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return W_FAILURE;
  }

  {
    // Send project setup.
    WSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = WToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
    msg.m_sAssetProfile = WAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();
    msg.m_fDevicePixelRatio = QApplication::activeWindow() != nullptr ? QApplication::activeWindow()->devicePixelRatio() : QGuiApplication::primaryScreen()->devicePixelRatio();

    SendMessage(&msg);
  }

  WLog::Dev("Waiting for Engine Process response");

  if (WaitForMessage(WGetStaticRTTI<WProjectReadyMsgToEditor>(), WTime()).Failed())
  {
    WLog::Error("Failed to restart the engine process. Engine Process Output:\n", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return W_FAILURE;
  }

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  pPreferences->SyncGlobalSettingsToEngine();

  WLog::Dev("Transmitting open documents to Engine Process");

  WTempHybridArray<WAssetDocument*, 6> docs;
  docs.Reserve(m_DocumentByGuid.GetCount());

  // Resend all open documents. Make sure to send main documents before child documents.
  for (auto it = m_DocumentByGuid.GetIterator(); it.IsValid(); ++it)
  {
    docs.PushBack(it.Value());
  }
  docs.Sort([](const WAssetDocument* a, const WAssetDocument* b)
    {
    if (a->IsMainDocument() != b->IsMainDocument())
      return a->IsMainDocument();
    return a < b; });

  for (WAssetDocument* pDoc : docs)
  {
    pDoc->SendDocumentOpenMessage(true);
  }

  WAssetCurator::GetSingleton()->InvalidateAssetsWithTransformState(WAssetInfo::TransformState::TransformError);

  WLog::Success("Engine Process is running");

  m_bClientIsConfigured = true;
  // We could have crashed while a render was in flight which will now never complete so reset the received counter.
  m_uiRedrawCountReceived = m_uiRedrawCountSent;

  Event e;
  e.m_Type = Event::Type::ProcessRestarted;
  s_Events.Broadcast(e);

  return W_SUCCESS;
}

void WEditorEngineProcessConnection::Update()
{
  if (!m_bProcessShouldBeRunning)
    return;

  if (!m_IPC.IsClientAlive())
  {
    ShutdownProcess();
    m_bProcessCrashed = true;

    Event e;
    e.m_Type = Event::Type::ProcessCrashed;
    s_Events.Broadcast(e);

    return;
  }

  m_IPC.ProcessMessages();

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->ProcessMessages();
  }
}

bool WEditorEngineConnection::SendMessage(WEditorEngineDocumentMsg* pMessage)
{
  W_WARNING_PUSH()
  W_WARNING_DISABLE_GCC("-Wtautological-undefined-compare")
  W_WARNING_DISABLE_CLANG("-Wtautological-undefined-compare")

  W_ASSERT_DEV(this != nullptr, "No connection between editor and engine was created. This typically happens when an asset document does "
                                 "not enable the engine-connection through the constructor of WAssetDocument."); // NOLINT

  W_WARNING_POP()
  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  return WEditorEngineProcessConnection::GetSingleton()->SendMessage(pMessage);
}

void WEditorEngineConnection::SendHighlightObjectMessage(WViewHighlightMsgToEngine* pMessage)
{
  // without this check there will be so many messages, that the editor comes to a crawl (< 10 FPS)
  // This happens because Qt sends hundreds of mouse-move events and since each 'SendMessageToEngine'
  // requires a round-trip to the engine process, doing this too often will be sloooow

  static WUuid LastHighlightGuid;

  if (LastHighlightGuid == pMessage->m_HighlightObject)
    return;

  LastHighlightGuid = pMessage->m_HighlightObject;
  SendMessage(pMessage);
}
