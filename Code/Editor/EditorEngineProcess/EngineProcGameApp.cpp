#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#include <Core/Console/Console.h>
#include <EditorEngineProcess/EngineProcGameApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/TraceWriter.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#  include <shellscalingapi.h>
#endif

WCommandLineOptionPath opt_OutputDir("_EditorEngineProcess", "-outputDir", "Output directory", "");
WCommandLineOptionString opt_LogName("_EditorEngineProcess", "-logName", "Log File Prefix", "LogEngine");

// Will forward assert messages and crash handler messages to the log system and then to the editor.
// Note that this is unsafe as in some crash situation allocating memory will not be possible but it's better to have some logs compared to none.
void EditorPrintFunction(const char* szText)
{
  WStringBuilder sError = szText;
  sError.Trim();
  WLog::Error("{}", sError.GetData());
}

static bool g_bUnattended = false;
static WAssertHandler g_PreviousAssertHandler = nullptr;

WEngineProcessGameApplication::WEngineProcessGameApplication()
  : WGameApplication("WEditorEngineProcess", nullptr)
{
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif

  m_LongOpWorkerManager.Startup(&m_IPC);
}

WEngineProcessGameApplication::~WEngineProcessGameApplication() = default;

WResult WEngineProcessGameApplication::BeforeCoreSystemsStartup()
{
  m_pApp = CreateEngineProcessApp();
  WStartup::AddApplicationTag("editorengineprocess");

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)
  // Make sure to disable the fileserve plugin
  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_off");
#endif

  AddEditorAssertHandler();
  return SUPER::BeforeCoreSystemsStartup();
}

void WEngineProcessGameApplication::AfterCoreSystemsStartup()
{
  // skip project creation at this point
  // SUPER::AfterCoreSystemsStartup();

#if W_DISABLED(W_PLATFORM_WINDOWS_DESKTOP) && W_DISABLED(W_PLATFORM_LINUX)
  {
    // on all 'mobile' platforms, we assume we are in remote mode
    WEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#else
  if (WCommandLineUtils::GetGlobalInstance()->GetBoolOption("-remote", false))
  {
    WEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#endif

  WaitForDebugger();

  WCrashHandler::SetCrashHandler(&WCrashHandler_WriteMiniDump::g_Instance);

  DisableErrorReport();

  WTaskSystem::SetTargetFrameTime(WTime::MakeFromSeconds(1.0 / 20.0));

  ConnectToHost();
}


void WEngineProcessGameApplication::ConnectToHost()
{
  W_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(WMakeDelegate(&WEngineProcessGameApplication::EventHandlerIPC, this));

  // wait indefinitely (not necessary anymore, should work regardless)
  // m_IPC.WaitForMessage(WGetStaticRTTI<WSetupProjectMsgToEngine>(), WTime());
}

void WEngineProcessGameApplication::DisableErrorReport()
{
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
}

void WEngineProcessGameApplication::WaitForDebugger()
{
  if (WCommandLineUtils::GetGlobalInstance()->GetBoolOption("-WaitForDebugger"))
  {
    while (!WSystemInformation::IsDebuggerAttached())
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
  }
}

bool WEngineProcessGameApplication::EditorAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  WLog::Error("*** Assertion ***:\nFile: \"{}\",\nLine: \"{}\",\nFunction: \"{}\",\nExpression: \"{}\",\nMessage: \"{}\"", szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  void* pBuffer[64];
  WArrayPtr<void*> tempTrace(pBuffer);
  const WUInt32 uiNumTraces = WStackTracer::GetStackTrace(tempTrace, nullptr);
  WStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &WLog::Print);
  WLog::Flush();

  // Wait for flush of IPC messages
  WThreadUtils::Sleep(WTime::MakeFromMilliseconds(500));

  // Don't chain to the default handler, it would show a modal dialog that nobody is going to close.
  // Returning true still breaks into an attached debugger and otherwise crashes the process, which is
  // the same thing the default handler does once it is told to stay silent.
  if (g_PreviousAssertHandler && !g_bUnattended)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void WEngineProcessGameApplication::AddEditorAssertHandler()
{
  // '-headless' is what the editor passes when it runs as WEditorProcessor, which has no user either
  auto* pCmd = WCommandLineUtils::GetGlobalInstance();
  g_bUnattended = pCmd->GetBoolOption("-unattended", false) || pCmd->GetBoolOption("-headless", false);

  g_PreviousAssertHandler = WGetAssertHandler();
  WSetAssertHandler(EditorAssertHandler);
}

void WEngineProcessGameApplication::RemoveEditorAssertHandler()
{
  WSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
}

void WEngineProcessGameApplication::BeforeCoreSystemsShutdown()
{
  RemoveEditorAssertHandler();

  m_pApp = nullptr;

  m_LongOpWorkerManager.Shutdown();

  m_IPC.m_Events.RemoveEventHandler(WMakeDelegate(&WEngineProcessGameApplication::EventHandlerIPC, this));

  SUPER::BeforeCoreSystemsShutdown();
}

void WEngineProcessGameApplication::Run()
{
  bool bPendingOpInProgress = false;
  do
  {
    bPendingOpInProgress = WEngineProcessDocumentContext::PendingOperationsInProgress();
    if (ProcessIPCMessages(bPendingOpInProgress))
    {
      WEngineProcessDocumentContext::UpdateDocumentContexts();
    }

    SendQueuedLogMessages();
  } while (!bPendingOpInProgress && m_uiRedrawCountExecuted == m_uiRedrawCountReceived);

  // If the editor enqueues a frame to be rendered, the loop above will break due to m_uiRedrawCountExecuted != m_uiRedrawCountReceived, and we will render a frame.
  // Alternatively, a pending operations is active at which point we render as fast as we can, ignoring the editor lock step.
  // Note there are two other cases in which we render a frame: when "FreeGalResources" or "FreeAllResources" messages are sent we must render a frame to clear pending deletions and free memory.
  SUPER::Run();
  WRenderWorld::ClearMainViews();
  m_uiRedrawCountExecuted = m_uiRedrawCountReceived;
}

void WEngineProcessGameApplication::LogWriter(const WLoggingEventData& e)
{
  WLogEntry entry(e);

  // the editor does not care about flushing caches, so no need to send this over
  if (entry.m_Type == WLogMsgType::Flush)
    return;

  if (entry.m_sTag == "IPC")
    return;

  // Resolving the links requires access to the document contexts, which is only allowed from the main thread.
  // Messages that are logged from a worker thread and require link resolution are therefore queued and sent later.
  // Consequently such messages can arrive out of order, but it's not worth the overhead to fix this rare case.
  if (!WThreadUtils::IsMainThread() && entry.m_sMsg.FindSubString("[[") != nullptr)
  {
    W_LOCK(m_QueuedLogMsgMutex);
    m_QueuedLogMsgs.PushBack(std::move(entry));
    return;
  }

  SendLogMessage(entry);
}

void WEngineProcessGameApplication::SendLogMessage(WLogEntry& ref_entry)
{
  // Prevent infinite recursion by disabling logging until we are done resolving and sending the message
  W_LOG_BLOCK_MUTE();

  // Engine side code can only log object/component handles, translate those into links that the editor can navigate to.
  {
    WStringBuilder sMsg = ref_entry.m_sMsg;

    if (WEngineProcessDocumentContext::ResolveLogLinks(sMsg))
    {
      ref_entry.m_sMsg = sMsg;
    }
  }

  WLogMsgToEditor msg;
  msg.m_Entry = std::move(ref_entry);

  m_IPC.SendMessage(&msg);
}

void WEngineProcessGameApplication::SendQueuedLogMessages()
{
  WDeque<WLogEntry> queued;
  {
    W_LOCK(m_QueuedLogMsgMutex);

    if (m_QueuedLogMsgs.IsEmpty())
      return;

    queued.Swap(m_QueuedLogMsgs);
  }

  for (WLogEntry& entry : queued)
  {
    SendLogMessage(entry);
  }
}

static bool EmptyAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  return false;
}

bool WEngineProcessGameApplication::ProcessIPCMessages(bool bPendingOpInProgress)
{
  W_PROFILE_SCOPE("ProcessIPCMessages");

  if (!m_IPC.IsHostAlive()) // check whether the host crashed
  {
    // The problem here is, that the editor process crashed (or was terminated through Visual Studio),
    // but our process depends on it for cleanup!
    // That means, this process created rendering resources through a device that is bound to a window handle, which belonged to the editor
    // process. So now we can't clean up, and therefore we can only crash. Therefore we try to crash as silently as possible.

#if W_ENABLED(W_PLATFORM_WINDOWS)
    // Make sure that Windows doesn't show a default message box when we call abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
    TerminateProcess(GetCurrentProcess(), 0);
#endif

    WLog::SeriousWarning("Host process no longer alive, exiting engine process.");

    // The OS will still call destructors for our objects (even though we called abort ... what a pointless design).
    // Our code might assert on destruction, so make sure our assert handler doesn't show anything.
    WSetAssertHandler(EmptyAssertHandler);
    std::abort();
  }
  else
  {
    // if an operation is still pending or this process is a remote process, we do NOT want to block
    // remote processes shall run as fast as they can
    if (bPendingOpInProgress || WEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    {
      m_IPC.ProcessMessages();
    }
    else
    {
      W_PROFILE_SCOPE("WaitForMessages");

      // A plugin may have work that no IPC message is going to announce - see m_HasPendingExternalWork.
      // Waiting with a short timeout then keeps the frame loop turning, at the cost of a wake-up every
      // 20 ms while such work is outstanding. When nothing is pending this still blocks indefinitely,
      // so an idle engine process costs nothing.
      const bool bExternalWork = m_HasPendingExternalWork.IsValid() && m_HasPendingExternalWork();

      // Only suspend and wait if no more pending ops need to be done.
      m_IPC.WaitForMessages(bExternalWork ? WTime::MakeFromMilliseconds(20) : WTime::MakeZero());
    }
  }
  return true;
}

void WEngineProcessGameApplication::SendProjectReadyMessage()
{
  WProjectReadyMsgToEditor msg;
  m_IPC.SendMessage(&msg);
}

void WEngineProcessGameApplication::SendReflectionInformation()
{
  if (WEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  WSet<const WRTTI*> types;
  WRTTI::ForEachType(
    [&](const WRTTI* pRtti)
    {
      if (pRtti->GetTypeFlags().IsSet(WTypeFlags::StandardType) == false)
      {
        types.Insert(pRtti);
      }
    });

  WDynamicArray<const WRTTI*> sortedTypes;
  WReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes).AssertSuccess("Sorting failed");

  for (auto type : sortedTypes)
  {
    WUpdateReflectionTypeMsgToEditor TypeMsg;
    WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, TypeMsg.m_desc);
    m_IPC.SendMessage(&TypeMsg);
  }
}

void WEngineProcessGameApplication::EventHandlerIPC(const WEngineProcessCommunicationChannel::Event& e)
{
  if (const auto* pMsg = WDynamicCast<const WSyncWithProcessMsgToEngine*>(e.m_pMessage))
  {
    WStringBuilder sRedrawScope;
    sRedrawScope.SetFormat("Redraw {}", pMsg->m_uiRedrawCount);
    W_PROFILE_SCOPE(sRedrawScope.GetData());

    WSyncWithProcessMsgToEditor msg;
    msg.m_uiRedrawCount = pMsg->m_uiRedrawCount;
    m_uiRedrawCountReceived = msg.m_uiRedrawCount;

    // We must clear the main views after rendering so that if the editor runs in lock step with the engine we don't render a view twice or request update again without rendering being done.
    // As multiple WSyncWithProcessMsgToEngine messages could be enqueued we break out of the message processing loop to render this frame.
    // Previously re renderer int frame on the stack but this caused stuttering so WEngineProcessGameApplication::Run is now the only place we render in this class.
    e.m_bInterruptMessageProcessing = true;

    m_IPC.SendMessage(&msg);
    return;
  }

  if (const auto* pMsg = WDynamicCast<const WShutdownProcessMsgToEngine*>(e.m_pMessage))
  {
    // in non-remote mode, the process needs to be properly killed, to prevent error messages
    // this is taken care of by the editor process
    if (WEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    {
      QuitApplication();
    }

    return;
  }

  // Project Messages:
  if (const auto* pMsg = WDynamicCast<const WSetupProjectMsgToEngine*>(e.m_pMessage))
  {
    if (!m_sProjectDirectory.IsEmpty())
    {
      // ignore this message, if it is for the same project
      if (m_sProjectDirectory == pMsg->m_sProjectDir)
        return;

      WLog::Error("Engine Process must restart to switch to another project ('{0}' -> '{1}').", m_sProjectDirectory, pMsg->m_sProjectDir);
      return;
    }

    m_sProjectDirectory = pMsg->m_sProjectDir;
    m_CustomFileSystemConfig = pMsg->m_FileSystemConfig;
    m_CustomPluginConfig = pMsg->m_PluginConfig;

    if (!pMsg->m_sAssetProfile.IsEmpty())
    {
      WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-profile");
      WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sAssetProfile);
    }

    if (!pMsg->m_sFileserveAddress.IsEmpty())
    {
      // we have no link dependency on the fileserve plugin here, it might not be loaded (yet / at all)
      // but we can pass the address to the command line, then it will pick it up, if necessary
      WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_server");
      WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sFileserveAddress);
    }

    // now that we know which project to initialize, do the delayed project setup
    {
      ExecuteInitFunctions();

      WStartup::StartupHighLevelSystems();

      WRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(true);
      WDebugRenderer::SetTextScale(pMsg->m_fDevicePixelRatio);
    }

    // after the WSetupProjectMsgToEngine was processed, all dynamic plugins should be loaded and we can finally send the reflection
    // information over
    SendReflectionInformation();

    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (const auto* pMsg1 = WDynamicCast<const WReloadResourceMsgToEngine*>(e.m_pMessage))
  {
    W_PROFILE_SCOPE("ReloadResource");

    const WRTTI* pType = WResourceManager::FindResourceForAssetType(pMsg1->m_sResourceType);
    if (auto hResource = WResourceManager::GetExistingResourceByType(pType, pMsg1->m_sResourceID); hResource.IsValid())
    {
      // reload existing resources
      WResourceManager::ReloadResource(pType, hResource, false);

      // ReloadResource() only makes sure to UNLOAD a resource, but if it isn't directly polled for, it won't get LOADED again
      // for most resource types this is fine, but some types need to get LOADED again to set up data that sticks around even while they are unloaded
      // specifically this happens for WSurfaceResource, because that signals to the physics engine to set up materials,
      // which stick around even if the surface resource gets unloaded
      // this is an editor specific issue, and we only do this here to guarantee an up-to-date representation while editing
      WResourceManager::PreloadResource(hResource);
    }
  }
  else if (const auto* pMsg1 = WDynamicCast<const WSimpleConfigMsgToEngine*>(e.m_pMessage))
  {
    if (pMsg1->m_sWhatToDo == "ChangeActivePlatform")
    {
      WStringBuilder sRedirFile("AssetCache/", pMsg1->m_sPayload, ".WAidlt");

      WDataDirectory::FolderType::s_sRedirectionFile = sRedirFile;

      WFileSystem::ReloadAllExternalDataDirectoryConfigs();

      m_PlatformProfile.SetConfigName(pMsg1->m_sPayload);
      Init_PlatformProfile_LoadForRuntime();

      WResourceManager::ReloadAllResources(false);
      WRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadAssetLUT")
    {
      WFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg1->m_sWhatToDo == "FreeGalResources")
    {
      WRenderWorld::ClearMainViews();
      RunOneFrame();
      WGALDevice::GetDefaultDevice()->WaitIdle();
    }
    else if (pMsg1->m_sWhatToDo == "FreeAllResources")
    {
      WResourceManager::FreeAllUnusedResources();
      WRenderWorld::ClearMainViews();
      RunOneFrame();
      WGALDevice::GetDefaultDevice()->WaitIdle();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadResources")
    {
      if (pMsg1->m_sPayload == "ReloadAllResources")
      {
        WFileSystem::ReloadAllExternalDataDirectoryConfigs();
        WResourceManager::ReloadAllResources(false);
      }
      WRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "ForceNoFallbackAcquisition")
    {
      WResourceManager::ForceNoFallbackAcquisition(3);
    }
    else if (pMsg1->m_sWhatToDo == "SaveProfiling")
    {
      WProfilingUtils::SaveProfilingCapture(pMsg1->m_sPayload).IgnoreResult();

      WSaveProfilingResponseToEditor response;
      WStringBuilder sAbsPath;
      if (WFileSystem::ResolvePath(pMsg1->m_sPayload, &sAbsPath, nullptr).Succeeded())
      {
        response.m_sProfilingFile = sAbsPath;
      }
      m_IPC.SendMessage(&response);
    }
    else
      WLog::Warning("Unknown WSimpleConfigMsgToEngine '{0}'", pMsg1->m_sWhatToDo);
  }
  else if (const auto* pMsg2 = WDynamicCast<const WResourceUpdateMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceUpdateMsg(*pMsg2);
  }
  else if (const auto* pMsg2a = WDynamicCast<const WRestoreResourceMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceRestoreMsg(*pMsg2a);
  }
  else if (const auto* pMsg2b = WDynamicCast<const WGlobalSettingsMsgToEngine*>(e.m_pMessage))
  {
    WGizmoRenderer::s_fGizmoScale = pMsg2b->m_fGizmoScale;
    WSpriteRenderer::s_fShapeIconScale = pMsg2b->m_fShapeIconScale;
    WSpriteRenderer::s_fShapeIconFadeDistance = pMsg2b->m_fShapeIconFadeDistance;
  }
  else if (const auto* pMsg3 = WDynamicCast<const WChangeCVarMsgToEngine*>(e.m_pMessage))
  {
    if (WCVar* pCVar = WCVar::FindCVarByName(pMsg3->m_sCVarName))
    {
      if (pCVar->GetType() == WCVarType::Int && pMsg3->m_NewValue.CanConvertTo<WInt32>())
      {
        *static_cast<WCVarInt*>(pCVar) = pMsg3->m_NewValue.ConvertTo<WInt32>();
      }
      else if (pCVar->GetType() == WCVarType::Float && pMsg3->m_NewValue.CanConvertTo<float>())
      {
        *static_cast<WCVarFloat*>(pCVar) = pMsg3->m_NewValue.ConvertTo<float>();
      }
      else if (pCVar->GetType() == WCVarType::Bool && pMsg3->m_NewValue.CanConvertTo<bool>())
      {
        *static_cast<WCVarBool*>(pCVar) = pMsg3->m_NewValue.ConvertTo<bool>();
      }
      else if (pCVar->GetType() == WCVarType::String && pMsg3->m_NewValue.CanConvertTo<WString>())
      {
        *static_cast<WCVarString*>(pCVar) = pMsg3->m_NewValue.ConvertTo<WString>();
      }
      else
      {
        WLog::Warning("WChangeCVarMsgToEngine: New value for CVar '{0}' is incompatible with CVar type", pMsg3->m_sCVarName);
      }
    }
    else
      WLog::Warning("WChangeCVarMsgToEngine: Unknown CVar '{0}'", pMsg3->m_sCVarName);
  }
  else if (const auto* pMsg4 = WDynamicCast<const WConsoleCmdMsgToEngine*>(e.m_pMessage))
  {
    if (m_pConsole->GetCommandInterpreter())
    {
      WCommandInterpreterState s;
      s.m_sInput = pMsg4->m_sCommand;

      WStringBuilder tmp;

      if (pMsg4->m_iType == 1)
      {
        m_pConsole->GetCommandInterpreter()->AutoComplete(s);
        tmp.AppendFormat(";;00||<{}", s.m_sInput);
      }
      else
        m_pConsole->GetCommandInterpreter()->Interpret(s);

      for (auto l : s.m_sOutput)
      {
        tmp.AppendFormat(";;{}||{}", WArgI((int)l.m_Type, 2, true), l.m_sText);
      }

      WConsoleCmdResultMsgToEditor r;
      r.m_sResult = tmp;

      m_IPC.SendMessage(&r);
    }
  }

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<WEditorEngineDocumentMsg>())
    return;

  const WEditorEngineDocumentMsg* pDocMsg = (const WEditorEngineDocumentMsg*)e.m_pMessage;

  WEngineProcessDocumentContext* pDocumentContext = WEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (const auto* pMsg5 = WDynamicCast<const WDocumentOpenMsgToEngine*>(e.m_pMessage)) // Document was opened or closed
  {
    if (pMsg5->m_bDocumentOpen)
    {
      pDocumentContext = CreateDocumentContext(pMsg5);
      W_ASSERT_DEV(pDocumentContext != nullptr, "Could not create a document context for document type '{0}'", pMsg5->m_sDocumentType);
    }
    else
    {
      WEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  if (const auto* pMsg6 = WDynamicCast<const WDocumentClearMsgToEngine*>(e.m_pMessage))
  {
    pDocumentContext = WEngineProcessDocumentContext::GetDocumentContext(pMsg6->m_DocumentGuid);

    if (pDocumentContext)
    {
      pDocumentContext->ClearExistingObjects();
    }
    return;
  }

  // can be null if the asset was deleted on disk manually
  if (pDocumentContext)
  {
    pDocumentContext->HandleMessage(pDocMsg);
  }
}

WEngineProcessDocumentContext* WEngineProcessGameApplication::CreateDocumentContext(const WDocumentOpenMsgToEngine* pMsg)
{
  WDocumentOpenResponseMsgToEditor m;
  m.m_DocumentGuid = pMsg->m_DocumentGuid;
  WEngineProcessDocumentContext* pDocumentContext = WEngineProcessDocumentContext::GetDocumentContext(pMsg->m_DocumentGuid);

  if (pDocumentContext == nullptr)
  {
    WRTTI::ForEachDerivedType<WEngineProcessDocumentContext>(
      [&](const WRTTI* pRtti)
      {
        auto* pProp = pRtti->FindPropertyByName("DocumentType");
        if (pProp && pProp->GetCategory() == WPropertyCategory::Constant)
        {
          const WStringBuilder sDocTypes(";", static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<WString>(), ";");
          const WStringBuilder sRequestedType(";", pMsg->m_sDocumentType, ";");

          if (sDocTypes.FindSubString(sRequestedType) != nullptr)
          {
            WLog::Dev("Created Context of type '{0}' for '{1}'", pRtti->GetTypeName(), pMsg->m_sDocumentType);
            for (auto pFunc : pRtti->GetFunctions())
            {
              if (WStringUtils::IsEqual(pFunc->GetPropertyName(), "AllocateContext"))
              {
                WVariant res;
                WTempHybridArray<WVariant, 1> params;
                params.PushBack(pMsg);
                pFunc->Execute(nullptr, params, res);
                if (res.IsA<WEngineProcessDocumentContext*>())
                {
                  pDocumentContext = res.Get<WEngineProcessDocumentContext*>();
                }
                else
                {
                  WLog::Error("Failed to call custom allocator '{}::{}'.", pRtti->GetTypeName(), pFunc->GetPropertyName());
                }
              }
            }

            if (!pDocumentContext)
            {
              pDocumentContext = pRtti->GetAllocator()->Allocate<WEngineProcessDocumentContext>();
            }

            WEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pMsg->m_DocumentMetaData, pDocumentContext, &m_IPC, pMsg->m_sDocumentType);
          }
        }
      });
  }
  else
  {
    pDocumentContext->Reset();
  }

  m_IPC.SendMessage(&m);
  return pDocumentContext;
}

void WEngineProcessGameApplication::Init_LoadProjectPlugins()
{
  m_CustomPluginConfig.m_Plugins.Sort([](const WApplicationPluginConfig::PluginConfig& lhs, const WApplicationPluginConfig::PluginConfig& rhs) -> bool
    {
    const bool isEnginePluginLhs = lhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;
    const bool isEnginePluginRhs = rhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;

    if (isEnginePluginLhs != isEnginePluginRhs)
    {
      // make sure the "engine plugins" end up at the back of the list
      // the reason for this is, that the engine plugins often have a link dependency on runtime plugins and pull their reflection data in right away
      // but then the WPlugin system doesn't know that certain reflected types actually come from some runtime plugin
      // by loading the editor engine plugins last, this solves that problem
      return isEnginePluginRhs;
    }

    return lhs.m_sAppDirRelativePath.Compare_NoCase(rhs.m_sAppDirRelativePath) < 0; });

  m_CustomPluginConfig.Apply();
}

WString WEngineProcessGameApplication::FindProjectDirectory() const
{
  return m_sProjectDirectory;
}

void WEngineProcessGameApplication::Init_FileSystem_ConfigureDataDirs()
{
  WStringBuilder sAppDir = ">sdk/Data/Tools/EditorEngineProcess";
  WStringBuilder sUserData = ">user/WorldEngine Project/EditorEngineProcess";
  if (opt_OutputDir.IsOptionSpecified(nullptr))
  {
    sUserData = opt_OutputDir.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
  }


  // make sure these directories exist
  WFileSystem::CreateDirectoryStructure(sAppDir).AssertSuccess();
  WFileSystem::CreateDirectoryStructure(sUserData).AssertSuccess();
  WFileSystem::CreateDirectoryStructure(">sdk/Output/").AssertSuccess();

  WFileSystem::AddDataDirectory("", "EngineProcess", ":", WDataDirUsage::AllowWrites).AssertSuccess();                       // for absolute paths
  WFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "bin", WDataDirUsage::ReadOnly).AssertSuccess();                // writing to the binary directory
  WFileSystem::AddDataDirectory(">sdk/Output/", "EngineProcess", "shadercache", WDataDirUsage::AllowWrites).AssertSuccess(); // for shader files
  WFileSystem::AddDataDirectory(sAppDir.GetData(), "EngineProcess", "app").AssertSuccess();                                   // app specific data
  WFileSystem::AddDataDirectory(sUserData, "EngineProcess", "appdata", WDataDirUsage::AllowWrites).AssertSuccess();          // for writing app user data

  m_CustomFileSystemConfig.Apply();

  {
    // We need the file system before we can start the html logger.
    WStringBuilder sLogName = "LogEngine";
    if (opt_LogName.IsOptionSpecified(nullptr))
    {
      sLogName = opt_LogName.GetOptionValue(WCommandLineOption::LogMode::Never);
    }
    WOsProcessID uiProcessID = WProcess::GetCurrentProcessID();
    WStringBuilder sLogFile;
    sLogFile.SetFormat(":appdata/Logs/{0}_{1}.htm", sLogName, uiProcessID);
    m_LogHTML.BeginLog(sLogFile, "EditorEngineProcess");
  }
}

bool WEngineProcessGameApplication::Run_ProcessApplicationInput()
{
  // override the escape action to not shut down the app, but instead close the play-the-game window
  if (WInputManager::GetInputActionState("GameApp", "CloseApp") == WKeyState::Pressed)
  {
    if (m_pGameState)
    {
      m_pGameState->RequestQuit("editor-esc");
    }
  }
  else
  {
    return SUPER::Run_ProcessApplicationInput();
  }

  return true;
}

WUniquePtr<WEditorEngineProcessApp> WEngineProcessGameApplication::CreateEngineProcessApp()
{
  return W_DEFAULT_NEW(WEditorEngineProcessApp);
}

void WEngineProcessGameApplication::BaseInit_ConfigureLogging()
{
  SUPER::BaseInit_ConfigureLogging();

  WGlobalLog::AddLogWriter(WMakeDelegate(&WEngineProcessGameApplication::LogWriter, this));
  WGlobalLog::AddLogWriter(WLoggingEvent::Handler(&WLogWriter::HTML::LogMessageHandler, &m_LogHTML));

  WLog::SetCustomPrintFunction(&EditorPrintFunction);

  // used for sending CVar changes over to the editor
  WCVar::s_AllCVarEvents.AddEventHandler(WMakeDelegate(&WEngineProcessGameApplication::EventHandlerCVar, this));
  WPlugin::Events().AddEventHandler(WMakeDelegate(&WEngineProcessGameApplication::EventHandlerCVarPlugin, this));
}

void WEngineProcessGameApplication::Deinit_ShutdownLogging()
{
  SendQueuedLogMessages();

  WGlobalLog::RemoveLogWriter(WLoggingEvent::Handler(&WLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  WGlobalLog::RemoveLogWriter(WMakeDelegate(&WEngineProcessGameApplication::LogWriter, this));

  // used for sending CVar changes over to the editor
  WCVar::s_AllCVarEvents.RemoveEventHandler(WMakeDelegate(&WEngineProcessGameApplication::EventHandlerCVar, this));
  WPlugin::Events().RemoveEventHandler(WMakeDelegate(&WEngineProcessGameApplication::EventHandlerCVarPlugin, this));

  SUPER::Deinit_ShutdownLogging();
}

void WEngineProcessGameApplication::EventHandlerCVar(const WCVarEvent& e)
{
  if (e.m_EventType == WCVarEvent::ValueChanged)
  {
    TransmitCVar(e.m_pCVar);
  }

  if (e.m_EventType == WCVarEvent::ListOfVarsChanged)
  {
    // currently no way to remove CVars from the editor UI

    for (WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void WEngineProcessGameApplication::EventHandlerCVarPlugin(const WPluginEvent& e)
{
  if (e.m_EventType == WPluginEvent::Type::AfterLoadingBeforeInit)
  {
    for (WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void WEngineProcessGameApplication::TransmitCVar(const WCVar* pCVar)
{
  WCVarMsgToEditor msg;
  msg.m_sName = pCVar->GetName();
  msg.m_sPlugin = pCVar->GetPluginName();
  msg.m_sDescription = pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case WCVarType::Int:
      msg.m_Value = ((WCVarInt*)pCVar)->GetValue();
      break;
    case WCVarType::Float:
      msg.m_Value = ((WCVarFloat*)pCVar)->GetValue();
      break;
    case WCVarType::Bool:
      msg.m_Value = ((WCVarBool*)pCVar)->GetValue();
      break;
    case WCVarType::String:
      msg.m_Value = ((WCVarString*)pCVar)->GetValue();
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  m_IPC.SendMessage(&msg);
}

void WEngineProcessGameApplication::HandleResourceUpdateMsg(const WResourceUpdateMsgToEngine& msg)
{
  const WRTTI* pRtti = WResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    WLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  WTypelessResourceHandle hResource = WResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    WStringBuilder sResourceDesc;
    sResourceDesc.Set(msg.m_sResourceType, "LiveUpdate");

    WUniquePtr<WResourceLoaderFromMemory> loader(W_DEFAULT_NEW(WResourceLoaderFromMemory));
    loader->m_ModificationTimestamp = WTimestamp::CurrentTimestamp();
    loader->m_sResourceDescription = sResourceDesc;

    WMemoryStreamWriter memoryWriter(&loader->m_CustomData);
    memoryWriter.WriteBytes(msg.m_Data.GetData(), msg.m_Data.GetCount()).IgnoreResult();

    WResourceManager::UpdateResourceWithCustomLoader(hResource, std::move(loader));

    WResourceManager::ForceLoadResourceNow(hResource);
  }
}

void WEngineProcessGameApplication::HandleResourceRestoreMsg(const WRestoreResourceMsgToEngine& msg)
{
  const WRTTI* pRtti = WResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    WLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  WTypelessResourceHandle hResource = WResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    WResourceManager::RestoreResource(hResource);
  }
}
