#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Configuration/SubSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

W_IMPLEMENT_SINGLETON(WAssetProcessor);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetProcessor)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WAssetProcessor);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WAssetProcessor* pDummy = WAssetProcessor::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WCuratorLog
////////////////////////////////////////////////////////////////////////

void WAssetProcessorLog::HandleLogMessage(const WLoggingEventData& le)
{
  m_LoggingEvent.Broadcast(le);
}

void WAssetProcessorLog::AddLogWriter(WLoggingEvent::Handler handler)
{
  m_LoggingEvent.AddEventHandler(handler);
}

void WAssetProcessorLog::RemoveLogWriter(WLoggingEvent::Handler handler)
{
  m_LoggingEvent.RemoveEventHandler(handler);
}


////////////////////////////////////////////////////////////////////////
// WAssetProcessor
////////////////////////////////////////////////////////////////////////

WAssetProcessor::WAssetProcessor()
  : m_SingletonRegistrar(this)
{
}


WAssetProcessor::~WAssetProcessor()
{
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }
  W_ASSERT_DEV(m_ProcessorState == ProcessorState::Stopped, "Call StopProcessor first before destroying the WAssetProcessor.");
}

void WAssetProcessor::StartProcessor()
{
  W_LOCK(m_ProcessorMutex);
  if (m_ProcessorState != ProcessorState::Stopped)
  {
    return;
  }

  // Join old thread.
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }

  m_ProcessorState = ProcessorState::Running;

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();

  const WUInt32 uiWorkerCount = WMath::Min<WUInt32>(WTaskSystem::GetWorkerThreadCount(WWorkerThreadType::LongTasks), pPreferences->m_uiMaxAssetProcessors);
  m_Processes.SetCount(uiWorkerCount);
  m_EditorProcessorStates.SetCount(uiWorkerCount);
  m_RestartRequests.SetCount(uiWorkerCount);

  for (WUInt32 idx = 0; idx < uiWorkerCount; ++idx)
  {
    m_Processes[idx].m_uiProcessorID = idx;
    m_Processes[idx].m_pNewWorkSignal = &m_NewWorkSignal;
  }

  m_pThread = W_DEFAULT_NEW(WAssetProcessorThread);
  m_pThread->Start();

  {
    WAssetProcessorEvent e;
    e.m_Type = WAssetProcessorEvent::Type::AssetProcessorStateChanged;
    e.m_uiProcessCount = m_EditorProcessorStates.GetCount();
    m_Events.Broadcast(e);
  }
}

void WAssetProcessor::StopProcessor(bool bForce)
{
  {
    W_LOCK(m_ProcessorMutex);
    switch (m_ProcessorState)
    {
      case ProcessorState::Running:
      {
        m_ProcessorState = ProcessorState::Stopping;
        {
          WAssetProcessorEvent e;
          e.m_Type = WAssetProcessorEvent::Type::AssetProcessorStateChanged;
          e.m_uiProcessCount = m_EditorProcessorStates.GetCount();
          m_Events.Broadcast(e);

          // Make sure worker thread is woken up.
          m_NewWorkSignal.RaiseSignal();
        }
      }
      break;
      case ProcessorState::Stopping:
        if (!bForce)
          return;
        break;
      default:
      case ProcessorState::Stopped:
        return;
    }
  }

  if (bForce)
  {
    m_bForceStop = true;
    m_pThread->Join();
    m_pThread.Clear();
    W_ASSERT_DEV(m_ProcessorState == ProcessorState::Stopped, "Process task should have set the state to stopped.");
  }
}

WUInt32 WAssetProcessor::GetProcessCount() const
{
  W_LOCK(m_ProcessorMutex);
  return m_EditorProcessorStates.GetCount();
}

WEditorProcessorState WAssetProcessor::GetProcessState(WUInt32 uiProcessIndex) const
{
  W_LOCK(m_ProcessorMutex);
  if (uiProcessIndex < m_EditorProcessorStates.GetCount())
  {
    return m_EditorProcessorStates[uiProcessIndex];
  }
  return {};
}

void WAssetProcessor::RequestRestartProcess(WUInt32 uiProcessIndex)
{
  W_LOCK(m_ProcessorMutex);
  if (uiProcessIndex < m_RestartRequests.GetCount())
  {
    m_RestartRequests[uiProcessIndex].Set(true);
    m_NewWorkSignal.RaiseSignal();
  }
}

void WAssetProcessor::AddLogWriter(WLoggingEvent::Handler handler)
{
  m_CuratorLog.AddLogWriter(handler);
}

void WAssetProcessor::RemoveLogWriter(WLoggingEvent::Handler handler)
{
  m_CuratorLog.RemoveLogWriter(handler);
}

void WAssetProcessor::UpdateProcessStates()
{
  WTempHybridArray<WUInt8, 8> changedProcesses;
  {
    W_LOCK(m_ProcessorMutex);
    for (WUInt32 i = 0; i < m_Processes.GetCount(); i++)
    {
      WEditorProcessorState state;
      state.m_bConnected = m_Processes[i].IsConnected();
      state.m_bRunning = m_Processes[i].IsRunning();
      state.m_bCrashed = m_Processes[i].IsCrashed();
      state.m_uiProcessID = m_Processes[i].GetProcessId();

      if (m_EditorProcessorStates[i] != state)
      {
        m_EditorProcessorStates[i] = state;
        changedProcesses.PushBack(i);
      }
    }
  }
  for (WUInt8 uiProcessId : changedProcesses)
  {
    WAssetProcessorEvent e;
    e.m_Type = WAssetProcessorEvent::Type::ProcessStateChanged;
    e.m_uiProcessorID = uiProcessId;
    m_Events.Broadcast(e);
  }
}

void WAssetProcessor::Run()
{
  QEventLoop loop;
  while (m_ProcessorState == ProcessorState::Running)
  {
    loop.processEvents(QEventLoop::AllEvents);
    if (m_iPauseProcessing == 0)
    {
      W_PROFILE_SCOPE("WAssetProcessor::Run");

      // Check for restart requests
      for (WUInt32 i = 0; i < m_Processes.GetCount(); i++)
      {
        if (m_RestartRequests[i].TestAndSet(true, false))
        {
          m_Processes[i].RequestRestart();
        }
      }

      for (WUInt32 i = 0; i < m_Processes.GetCount(); i++)
      {
        m_Processes[i].Tick(true);
      }
      UpdateProcessStates();
    }
    m_NewWorkSignal.WaitForSignal(WTime::MakeFromSeconds(1));
  }

  while (true)
  {
    bool bAnyRunning = false;
    loop.processEvents(QEventLoop::AllEvents);
    for (WUInt32 i = 0; i < m_Processes.GetCount(); i++)
    {
      if (m_bForceStop)
        m_Processes[i].ShutdownProcess();

      bAnyRunning |= m_Processes[i].Tick(false);
    }

    if (bAnyRunning)
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    else
      break;
  }

  WUInt32 uiProcesses = 0;
  {
    W_LOCK(m_ProcessorMutex);
    uiProcesses = m_Processes.GetCount();
    m_Processes.Clear();
    m_EditorProcessorStates.Clear();
    m_ProcessorState = ProcessorState::Stopped;
    m_bForceStop = false;
  }

  for (WUInt8 uiProcessId = 0; uiProcessId < uiProcesses; uiProcessId++)
  {
    WAssetProcessorEvent e;
    e.m_Type = WAssetProcessorEvent::Type::ProcessStateChanged;
    e.m_uiProcessorID = uiProcessId;
    m_Events.Broadcast(e);
  }

  {
    WAssetProcessorEvent e;
    e.m_Type = WAssetProcessorEvent::Type::AssetProcessorStateChanged;
    e.m_uiProcessCount = m_EditorProcessorStates.GetCount();
    m_Events.Broadcast(e);
  }
}


////////////////////////////////////////////////////////////////////////
// WEditorProcessorProcess
////////////////////////////////////////////////////////////////////////

WEditorProcessorProcess::WEditorProcessorProcess()
  : m_Status(W_SUCCESS)
{
  m_pIPC = W_DEFAULT_NEW(WEditorProcessCommunicationChannel);
  m_pIPC->m_Events.AddEventHandler(WMakeDelegate(&WEditorProcessorProcess::EventHandlerIPC, this));
  m_pIPC->m_IpcChannelEvents.AddEventHandler(WMakeDelegate(&WEditorProcessorProcess::ChannelEventHandler, this));
}

WEditorProcessorProcess::~WEditorProcessorProcess()
{
  ShutdownProcess();
  m_pIPC->m_IpcChannelEvents.RemoveEventHandler(WMakeDelegate(&WEditorProcessorProcess::ChannelEventHandler, this));
  m_pIPC->m_Events.RemoveEventHandler(WMakeDelegate(&WEditorProcessorProcess::EventHandlerIPC, this));
  W_DEFAULT_DELETE(m_pIPC);
}


WResult WEditorProcessorProcess::StartProcess()
{
  const WRTTI* pFirstAllowedMessageType = nullptr;

  WStringBuilder tmp;

  QStringList args;
  args << "-appname";
  args << WApplication::GetApplicationInstance()->GetApplicationName().GetData();
  args << "-appid";
  args << QString::number(m_uiProcessorID);
  args << "-project";
  args << WToolsProject::GetSingleton()->GetProjectFile().GetData();
  args << "-renderer";
  args << WGameApplication::GetActiveRenderer().GetData(tmp);
  {
    WStringBuilder sRelativeData;
    sRelativeData = ":APPDATA";

    WStringBuilder sAbsoluteData;
    WFileSystem::ResolvePath(sRelativeData, &sAbsoluteData, nullptr).AssertSuccess("Failed to resolve APPDATA dir!");

    args << "-outputDir";
    args << sAbsoluteData.GetData();
  }

#if W_ENABLED(W_PLATFORM_WINDOWS)
  const char* EditorProcessorExecutable = "WEditorProcessor.exe";
#else
  const char* EditorProcessorExecutable = "WEditorProcessor";
#endif

  if (m_pIPC->StartClientProcess(EditorProcessorExecutable, args, false, pFirstAllowedMessageType).Failed())
  {
    return W_FAILURE;
  }
  m_bProcessShouldBeRunning = true;
  m_CurrentProcessID = m_pIPC->GetProcessId();
  return W_SUCCESS;
}

void WEditorProcessorProcess::ShutdownProcess()
{
  m_bProcessShouldBeRunning = false;
  m_pIPC->CloseConnection();
}

void WEditorProcessorProcess::EventHandlerIPC(const WProcessCommunicationChannel::Event& e)
{
  if (const WProcessAssetResponseMsg* pMsg = WDynamicCast<const WProcessAssetResponseMsg*>(e.m_pMessage))
  {
    W_ASSERT_DEV(m_State == State::Processing, "Message handling should only happen when currently processing");
    m_State = State::ReportResult;
    m_Status = pMsg->m_Status;
    m_LogEntries.Swap(pMsg->m_LogEntries);
    m_MissmatchTransformDependencies.Swap(pMsg->m_MissmatchTransformDependencies);
    m_MissmatchThumbnailDependencies.Swap(pMsg->m_MissmatchThumbnailDependencies);
    m_uiMissmatchAssetHash = pMsg->m_uiMissmatchAssetHash;
    m_uiMissmatchThumbHash = pMsg->m_uiMissmatchThumbHash;
    m_StartedProcessing = pMsg->m_StartedProcessing;
    m_StartedTransform = pMsg->m_StartedTransform;
    m_FinishedProcessing = pMsg->m_FinishedProcessing;
  }
}

void WEditorProcessorProcess::ChannelEventHandler(const WIpcChannelEvent& e)
{
  // We explicitly do not handle the event as it is called in a separate thread. All handling of state changes happens in WEditorProcessorProcess::Tick.
  m_pNewWorkSignal->RaiseSignal();
}

bool WEditorProcessorProcess::GetNextAssetToProcess(WAssetInfo* pInfo, WUuid& out_guid, WDataDirPath& out_path, WAssetInfo::TransformState& out_transformState)
{
  bool bComplete = true;

  const WDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (WDocumentManager::FindDocumentTypeFromPath(pInfo->m_Path, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<const WAssetDocumentTypeDescriptor*>(pTypeDesc)->m_AssetDocumentFlags;

    if (flags.IsAnySet(WAssetDocumentFlags::OnlyTransformManually | WAssetDocumentFlags::DisableTransform))
      return false;
  }

  auto TestFunc = [this, &bComplete](const WSet<WString>& files) -> WAssetInfo*
  {
    for (const auto& sFile : files)
    {
      if (WAssetInfo* pFileInfo = WAssetCurator::GetSingleton()->GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
          case WAssetInfo::TransformState::Unknown:
          case WAssetInfo::TransformState::TransformError:
          case WAssetInfo::TransformState::MissingTransformDependency:
          case WAssetInfo::TransformState::MissingPackageDependency:
          case WAssetInfo::TransformState::MissingThumbnailDependency:
          case WAssetInfo::TransformState::CircularDependency:
          {
            bComplete = false;
            continue;
          }
          case WAssetInfo::TransformState::NeedsTransform:
          case WAssetInfo::TransformState::NeedsThumbnail:
          {
            bComplete = false;
            return pFileInfo;
          }
          case WAssetInfo::TransformState::UpToDate:
            continue;

          case WAssetInfo::TransformState::NeedsImport:
            // the main processor has to do this itself
            continue;

            W_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    return nullptr;
  };

  if (WAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_TransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path, out_transformState);
  }

  if (WAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_ThumbnailDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path, out_transformState);
  }

  // not needed to go through package dependencies here

  if (bComplete && !WAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) &&
      !WAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
  {
    WAssetCurator::GetSingleton()->m_Updating.Insert(pInfo->m_Info->m_DocumentID);
    out_guid = pInfo->m_Info->m_DocumentID;
    out_path = pInfo->m_Path;
    out_transformState = pInfo->m_TransformState;
    return true;
  }

  return false;
}

bool WEditorProcessorProcess::GetNextAssetToProcess(WUuid& out_guid, WDataDirPath& out_path, WAssetInfo::TransformState& out_transformState)
{
  W_PROFILE_SCOPE("WEditorProcessorProcess::GetNextAssetToProcess");
  W_LOCK(WAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = WAssetCurator::GetSingleton()->m_TransformState[WAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    WAssetInfo* pInfo = WAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path, out_transformState);
      if (bRes)
        return true;
    }
  }

  for (auto it = WAssetCurator::GetSingleton()->m_TransformState[WAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    WAssetInfo* pInfo = WAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path, out_transformState);
      if (bRes)
        return true;
    }
  }

  return false;
}


void WEditorProcessorProcess::OnProcessCrashed(WStringView message)
{
  ShutdownProcess();
  m_State = (m_State == State::Processing || m_State == State::ReadyForProcessing) ? State::ReportResult : State::Crashed;
  m_Status = WStatus(message);
  WLogEntryDelegate logger([this](WLogEntry& ref_entry)
    { m_LogEntries.PushBack(std::move(ref_entry)); });
  WLog::Error(&logger, message);
  WLog::Error(&WAssetProcessor::GetSingleton()->m_CuratorLog, message);
  WLog::Error("EditorProcessor with pid '{}' crashed. Right-click on the crashed instance in the curator panel to go to the crashdumps.", m_CurrentProcessID);
}

void WEditorProcessorProcess::RequestRestart()
{
  if (m_State == State::Crashed)
  {
    WLog::Info(&WAssetProcessor::GetSingleton()->m_CuratorLog, "Restarting crashed processor {}", m_uiProcessorID);
    m_State = State::StartClient;
  }
}

bool WEditorProcessorProcess::IsConnected() const
{
  return m_pIPC->IsConnected();
}

bool WEditorProcessorProcess::IsRunning() const
{
  return m_State == State::Processing;
}

bool WEditorProcessorProcess::IsCrashed() const
{
  return m_State == State::Crashed;
}

WOsProcessID WEditorProcessorProcess::GetProcessId() const
{
  return m_CurrentProcessID;
}

bool WEditorProcessorProcess::HasProcessCrashed()
{
  return !m_pIPC->IsClientAlive();
}

void WEditorProcessorProcess::HandleHashMissmatch()
{
  W_SCOPE_EXIT(
    {
      m_MissmatchTransformDependencies.Clear();
      m_MissmatchThumbnailDependencies.Clear();
      m_uiMissmatchAssetHash = 0;
      m_uiMissmatchThumbHash = 0;
    });

  WUInt64 uiAssetHash = 0;
  WUInt64 uiThumbHash = 0;
  WUInt64 uiPackageHash = 0;
  WMap<WString, WUInt64> TransformDependencies;
  WMap<WString, WUInt64> ThumbnailDependencies;
  {
    // Check the asset hash again but force evaluation to detect cases where the file system watcher did not trigger.
    const WUInt32 uiPlatform = WAssetCurator::GetSingleton()->FindAssetProfileByName(m_sPlatform);
    WAssetCurator::GetSingleton()->IsAssetUpToDate(m_AssetGuid, WAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash, uiPackageHash, true);
    if ((uiAssetHash == m_uiMissmatchAssetHash) || (uiThumbHash == m_uiMissmatchThumbHash))
    {
      // The newly computed hash now matches the one computed by the WEditorProcessor.
      return;
    }

    WSet<WString> dependencies;
    WAssetCurator::GetSingleton()->GenerateTransitiveHull(m_AssetPath.GetAbsolutePath(), dependencies, WDependencyFlags::Transform);
    WAssetCurator::GetSingleton()->GenerateSettingsHashMap(dependencies, WDependencyFlags::Transform, TransformDependencies);

    dependencies.Clear();
    WAssetCurator::GetSingleton()->GenerateTransitiveHull(m_AssetPath.GetAbsolutePath(), dependencies, WDependencyFlags::Thumbnail);
    WAssetCurator::GetSingleton()->GenerateSettingsHashMap(dependencies, WDependencyFlags::Thumbnail, ThumbnailDependencies);
  }

  auto AddLogMessage = [&](WStringView sMsg)
  {
    WLogEntry le;
    le.m_sMsg = sMsg;
    le.m_Type = WLogMsgType::WarningMsg;
    m_LogEntries.PushBack(le);
  };

  auto CompareDependencies = [&](const WMap<WString, WUInt64>& dependencies, const WMap<WString, WUInt64>& missmatchDependencies, WStringView sDependencyType)
  {
    WStringBuilder sMsg;
    for (auto it = TransformDependencies.GetIterator(); it.IsValid(); ++it)
    {
      const WString& sKey = it.Key();
      const WUInt64 uiExpected = it.Value();

      auto itOld = m_MissmatchTransformDependencies.Find(sKey);
      if (itOld.IsValid())
      {
        const WUInt64 uiOld = itOld.Value();
        if (uiOld != uiExpected)
        {
          sMsg.SetFormat("{} dependency hash mismatch for '{}': expected {} != recorded {}", sDependencyType, sKey, uiExpected, uiOld);
          AddLogMessage(sMsg);
        }
      }
      else
      {
        sMsg.SetFormat("{} dependency missing in records: '{0}' (hash {1})", sDependencyType, sKey, uiExpected);
        AddLogMessage(sMsg);
      }
    }

    // Check for dependencies that used to be recorded but are no longer expected
    for (auto it = m_MissmatchTransformDependencies.GetIterator(); it.IsValid(); ++it)
    {
      if (!TransformDependencies.Contains(it.Key()))
      {
        sMsg.SetFormat("{} dependency no longer present: '{0}' (old hash {1})", sDependencyType, it.Key(), it.Value());
        AddLogMessage(sMsg);
      }
    }
  };
  CompareDependencies(TransformDependencies, m_MissmatchTransformDependencies, "Transform");
  CompareDependencies(ThumbnailDependencies, m_MissmatchThumbnailDependencies, "Thumbnail");
}

bool WEditorProcessorProcess::Tick(bool bStartNewWork)
{
  W_PROFILE_SCOPE("WEditorProcessorProcess::Tick");
  if (m_State != State::StartClient && m_State != State::Crashed)
  {
    if (!m_pIPC->IsClientAlive())
    {
      // Will transition to Crashed or ReportResult
      OnProcessCrashed("Asset processor crashed while waiting for connection");
    }
  }

  if (m_State >= State::LookingForWork && m_State <= State::Processing)
  {
    if (!m_pIPC->IsConnected())
    {
      // Will transition to Crashed or ReportResult
      OnProcessCrashed("Asset processor IPC channel is not connected");
    }
  }

  while (true)
  {
    switch (m_State)
    {
      case State::StartClient:
      {
        if (StartProcess().Failed())
        {
          OnProcessCrashed("Asset processor did not launch");
          m_State = State::Crashed;
          return false; // don't call later
        }
        else
        {
          m_State = State::WaitingForConnection;
          return bStartNewWork;
        }
      }
      break;

      case State::WaitingForConnection:
      {
        if (m_pIPC->IsConnected())
        {
          m_State = State::LookingForWork;
          break;
        }
        return bStartNewWork;
      }
      break;
      case State::LookingForWork:
      {
        if (!bStartNewWork)
        {
          return false; // don't call later
        }
        m_ProcessingStartTime = WTime::MakeZero();
        // Clear asset to process
        m_AssetGuid = {};
        m_AssetPath.Clear();
        m_TransformState = WAssetInfo::TransformState::Unknown;
        m_sPlatform.Clear();
        m_uiAssetHash = 0;
        m_uiThumbHash = 0;
        m_uiPackageHash = 0;
        m_TransitiveHull.Clear();

        // Clear transform result
        m_Status = WStatus(W_SUCCESS);
        m_LogEntries.Clear();
        m_MissmatchTransformDependencies.Clear();
        m_MissmatchThumbnailDependencies.Clear();
        m_uiMissmatchAssetHash = 0;
        m_uiMissmatchThumbHash = 0;
        m_StartedProcessing = {};
        m_StartedTransform = {};
        m_FinishedProcessing = {};

        {
          auto pCurator = WAssetCurator::GetSingleton();

          W_LOCK(pCurator->m_CuratorMutex);

          if (!GetNextAssetToProcess(m_AssetGuid, m_AssetPath, m_TransformState))
          {
            m_AssetGuid = WUuid();
            m_AssetPath.Clear();
            if (m_pIPC->IsClientAlive() && m_pIPC->IsConnected() && !m_bIsIdle)
            {
              m_bIsIdle = true;
              // If we have nothing else to do, we might as well free some resource memory the process holds.
              WFreeAllResourcesMsg msg;
              m_pIPC->SendMessage(&msg);
            }

            return bStartNewWork; // call again if we should be looking for new work
          }
          m_bIsIdle = false;


          WAssetInfo::TransformState state;
          state = pCurator->IsAssetUpToDate(m_AssetGuid, nullptr, nullptr, m_uiAssetHash, m_uiThumbHash, m_uiPackageHash);
          W_ASSERT_DEV(state == WAssetInfo::TransformState::NeedsTransform || state == WAssetInfo::TransformState::NeedsThumbnail, "An asset was selected that is already up to date.");
          WSet<WString> dependencies;
          WStringBuilder sTemp;
          pCurator->GenerateTransitiveHull(WConversionUtils::ToString(m_AssetGuid, sTemp), dependencies, WDependencyFlags::Transform | WDependencyFlags::Thumbnail);
          m_sPlatform = WAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();
          m_TransitiveHull.Reserve(dependencies.GetCount());
          for (const WString& str : dependencies)
          {
            if (WConversionUtils::IsStringUuid(str))
            {
              if (auto pAsset = pCurator->FindSubAsset(str))
              {
                m_TransitiveHull.PushBack(pAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
              }
            }
            else
            {
              m_TransitiveHull.PushBack(str);
            }
          }
        }

        m_State = State::ReadyForProcessing;
      }
      break;

      case State::ReadyForProcessing:
      {
        WLog::Info(&WAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", m_AssetPath.GetDataDirRelativePath());

        // Fire progress event for processing started
        m_ProcessingStartTime = WTime::Now();
        {
          WAssetProcessorProgressEvent e;
          e.m_Type = WAssetProcessorProgressEvent::Type::ProcessingStarted;
          e.m_TransformState = m_TransformState;
          e.m_uiProcessorID = (WUInt8)m_uiProcessorID;
          e.m_AssetGuid = m_AssetGuid;
          e.m_sAssetPath = m_AssetPath.GetDataDirRelativePath();
          e.m_StartTime = m_ProcessingStartTime;
          WAssetProcessor::GetSingleton()->m_ProgressEvents.Broadcast(e);
        }

        // Send and wait
        WProcessAssetMsg msg;
        msg.m_AssetGuid = m_AssetGuid;
        msg.m_AssetHash = m_uiAssetHash;
        msg.m_ThumbHash = m_uiThumbHash;
        msg.m_PackageHash = m_uiPackageHash;
        msg.m_sAssetPath = m_AssetPath;
        msg.m_DepRefHull.Swap(m_TransitiveHull);
        msg.m_sPlatform = m_sPlatform;

        if (m_pIPC->SendMessage(&msg))
        {
          m_State = State::Processing;
          return true; // call again later
        }
        else
        {
          OnProcessCrashed("Asset processor crashed, failed to send message");
          break;
        }
      }
      break;
      case State::Processing:
      {
        W_PROFILE_SCOPE("WEditorProcessorProcess::Processing");
        m_pIPC->ProcessMessages();
        return true; // call again later
      }
      break;
      case State::ReportResult:
      {
        const bool bProcessCrashed = !m_pIPC->IsClientAlive();

        if (!m_MissmatchTransformDependencies.IsEmpty())
        {
          HandleHashMissmatch();
        }

        // Fire progress event only if we actually started processing.
        const bool bDidStartWork = !m_ProcessingStartTime.IsZero();
        if (bDidStartWork)
        {
          // The actual start times are only available if we receive a response message. If we crash, fall back to the range of [m_ProcessingStartTime, now()] as an estimate of the work time.
          WTime processingEndTime = WTime::Now();

          WAssetProcessorProgressEvent e;
          e.m_Type = WAssetProcessorProgressEvent::Type::ProcessingFinished;
          e.m_uiProcessorID = m_uiProcessorID;
          e.m_AssetGuid = m_AssetGuid;
          e.m_sAssetPath = m_AssetPath.GetDataDirRelativePath();
          e.m_StartTime = bProcessCrashed ? m_ProcessingStartTime : m_StartedProcessing;
          e.m_TransformStartTime = bProcessCrashed ? m_ProcessingStartTime : m_StartedTransform;
          e.m_EndTime = bProcessCrashed ? processingEndTime : m_FinishedProcessing;
          e.m_Result = m_Status;
          WAssetProcessor::GetSingleton()->m_ProgressEvents.Broadcast(e);
        }

        if (m_Status.Succeeded())
        {
          WAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
          WAssetCurator::GetSingleton()->NeedsReloadResources(m_AssetGuid);
          WLog::Info(&WAssetProcessor::GetSingleton()->m_CuratorLog, "Finished '{0}'", m_AssetPath.GetDataDirRelativePath());
        }
        else
        {
          if (m_Status.m_Result == WTransformResult::NeedsImport)
          {
            WAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, WAssetInfo::TransformState::NeedsImport);
            WLog::Warning(&WAssetProcessor::GetSingleton()->m_CuratorLog, "Needs Import '{0}'", m_AssetPath.GetDataDirRelativePath());
          }
          else
          {
            WAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_AssetGuid, m_LogEntries);
            WAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, WAssetInfo::TransformState::TransformError);
            if (bProcessCrashed)
            {
              WLog::Error(&WAssetProcessor::GetSingleton()->m_CuratorLog, "Failed '{0}' (process crashed)", m_AssetPath.GetDataDirRelativePath());
            }
            else
            {
              WLog::Error(&WAssetProcessor::GetSingleton()->m_CuratorLog, "Failed '{0}'", m_AssetPath.GetDataDirRelativePath());
            }
          }
        }

        {
          W_LOCK(WAssetCurator::GetSingleton()->m_CuratorMutex);
          WAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
        }

        if (bProcessCrashed)
        {
          m_State = State::Crashed;
        }
        else
        {
          m_State = State::LookingForWork;
        }
      }
      break;
      case State::Crashed:
      {
        return false;
      }
      break;
    }
  }
}

WUInt32 WAssetProcessorThread::Run()
{
  WAssetProcessor::GetSingleton()->Run();
  return 0;
}
