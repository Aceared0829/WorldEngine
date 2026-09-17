#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/Console/ConsoleActions.h>
#include <GameEngine/Console/QuakeConsole.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Formats/TgaFileFormat.h>
#include <Texture/Image/Image.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
#  include <GameEngine/Console/ImGuiConsole.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  define USE_IMGUI_CONSOLE 1
#endif

WGameApplication* WGameApplication::s_pGameApplicationInstance = nullptr;
WDelegate<WGALDevice*(const WGALDeviceCreationDescription&)> WGameApplication::s_DefaultDeviceCreator;

WCVarBool WGameApplication::cvar_AppVSync("App.VSync", true, WCVarFlags::Save, "Enables V-Sync");
WCVarBool WGameApplication::cvar_AppShowFPS("App.ShowFPS", false, WCVarFlags::Save, "Show frames per second counter");
WCVarBool WGameApplication::cvar_WorldShowObjectOrigins("World.ShowObjectOrigins", false, WCVarFlags::Default, "Render debug geometry at every game object position");

WGameApplication::WGameApplication(const char* szAppName, const char* szProjectPath /*= nullptr*/)
  : WGameApplicationBase(szAppName)
  , m_sAppProjectPath(szProjectPath)
{
  m_pUpdateTask = W_DEFAULT_NEW(WDelegateTask<void>, "UpdateWorldsAndExtractViews", WTaskNesting::Never, WMakeDelegate(&WGameApplication::UpdateWorldsAndExtractViews, this));
  m_pUpdateTask->ConfigureTask("GameApplication.Update", WTaskNesting::Maybe);

  s_pGameApplicationInstance = this;

#if USE_IMGUI_CONSOLE
  m_pConsole = W_DEFAULT_NEW(WImGuiConsole);
#else
  m_pConsole = W_DEFAULT_NEW(WQuakeConsole);
#endif

  if (m_pConsole)
  {
    WConsole::SetMainConsole(m_pConsole.Borrow());
  }
}

WGameApplication::~WGameApplication()
{
  s_pGameApplicationInstance = nullptr;
}

// static
void WGameApplication::SetOverrideDefaultDeviceCreator(WDelegate<WGALDevice*(const WGALDeviceCreationDescription&)> creator)
{
  s_DefaultDeviceCreator = creator;
}

WResult WGameApplication::BeforeCoreSystemsStartup()
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // before anything else may log, so that '-logfile' contains the entire startup
  Unattended_Setup();
#endif

  return SUPER::BeforeCoreSystemsStartup();
}

void WGameApplication::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // the base class returns early when initialization went wrong, in which case there is no game state
  // and no world to set up
  if (!ShouldApplicationQuit())
  {
    // after the game state, because seeding the random number generators needs the worlds to exist
    Unattended_Start();
  }
#endif
}

void WGameApplication::BeforeCoreSystemsShutdown()
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // while the log writers are still attached, so that the summary ends up in '-logfile'
  Unattended_Finish();
#endif

  SUPER::BeforeCoreSystemsShutdown();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // after the base class, which shuts the logging down
  Unattended_DetachLog();
#endif
}

void WGameApplication::Run()
{
  SUPER::Run();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  Unattended_CheckTimeout();
#endif
}

void WGameApplication::StoreScreenshot(WImage&& image, WStringView sContext /*= {}*/)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // '-screenshot' names one specific file, so it takes precedence over the path the base class generates
  if (Unattended_StoreScreenshot(image))
    return;
#endif

  SUPER::StoreScreenshot(std::move(image), sContext);
}

//////////////////////////////////////////////////////////////////////////

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)

// These options exist to run an application unattended, e.g. as a smoke test from a script.
// They are implemented here, rather than in WPlayer, so that every application built with the engine
// has them - an exported game as much as the player.
WCommandLineOptionInt opt_RunFrames("_App", "-runframes", "Quit automatically after this many rendered frames.\nUse this to check that a project starts up and renders at all.", -1, -1);
WCommandLineOptionFloat opt_Timeout("_App", "-timeout", "Quit automatically after this many seconds, no matter what.\nSafety net in case startup hangs. Sets the return code to 2 when it triggers.", 0.0f, 0.0f);
WCommandLineOptionPath opt_Screenshot("_App", "-screenshot", "Absolute path to a PNG file to write a screenshot to, right before quitting.\nOnly useful together with -runframes or -timeout.", "");
WCommandLineOptionPath opt_LogFile("_App", "-logfile", "Absolute path to a text file to write the full log to.", "");
WCommandLineOptionBool opt_FailOnError("_App", "-failonerror", "Set the return code to 1 if any error was logged during the run.", false);
WCommandLineOptionFloat opt_FixedTimeStep("_App", "-fixedtimestep",
  "Advance the clock by a fixed 1/N seconds per frame, instead of by the time that really elapsed.\n\
\n\
Together with -seed this makes consecutive runs produce identical frames, which is what a screenshot\n\
has to be for comparing it against a reference image. The application then no longer runs in real time.\n\
\n\
Example:\n\
  -fixedtimestep 30\n",
  0.0f, 0.0f);
WCommandLineOptionInt opt_Seed("_App", "-seed",
  "Seed for the random number generator of every world, so that random behavior repeats between runs.\n\
Only useful together with -fixedtimestep.",
  -1, -1);

void WGameApplication::Unattended_Setup()
{
  const WStringBuilder sLogFile = opt_LogFile.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sLogFile.IsEmpty())
  {
    // uses WOSFile, so this works before the WFileSystem is configured
    if (m_UnattendedLogFile.BeginLog(sLogFile).Succeeded())
    {
      m_UnattendedLogFile.SetTimestampMode(WLog::TimestampMode::TimeOnly);
      m_UnattendedLogToFileID = WGlobalLog::AddLogWriter(WMakeDelegate(&WLogWriter::TextFile::LogMessageHandler, &m_UnattendedLogFile));
    }
    else
    {
      WLog::Error("Could not open log file '{}' for writing.", sLogFile);
    }
  }

  m_bFailOnError = opt_FailOnError.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  if (m_bFailOnError)
  {
    m_UnattendedLogErrorCounterID = WGlobalLog::AddLogWriter(WMakeDelegate(&WGameApplication::Unattended_OnLogEvent, this));
  }

  m_iRunFrames = opt_RunFrames.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
  m_UnattendedTimeout = WTime::MakeFromSeconds(opt_Timeout.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));
  m_sScreenshotPath = opt_Screenshot.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
  m_iRandomSeed = opt_Seed.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  const float fFixedTimeStepHz = opt_FixedTimeStep.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  if (fFixedTimeStepHz > 0.0f)
  {
    m_FixedTimeStep = WTime::MakeFromSeconds(1.0 / fFixedTimeStepHz);
  }

  m_bUnattended = m_iRunFrames >= 0 || m_UnattendedTimeout.IsPositive() || !m_sScreenshotPath.IsEmpty() ||
                  m_bFailOnError || !sLogFile.IsEmpty() || m_FixedTimeStep.IsPositive() || m_iRandomSeed >= 0;
}

void WGameApplication::Unattended_Start()
{
  // WTime is only usable once the core systems are up, so the timeout can't start any earlier than this
  m_UnattendedStartTime = WTime::Now();

  if (m_FixedTimeStep.IsPositive())
  {
    WClock::GetGlobalClock()->SetFixedTimeStep(m_FixedTimeStep);
  }

  if (m_iRandomSeed >= 0)
  {
    // the worlds that exist at this point are the ones the game state created; a world created later
    // is not covered, seeding it is then up to that code
    for (WUInt32 i = 0; i < WWorld::GetWorldCount(); ++i)
    {
      if (WWorld* pWorld = WWorld::GetWorld(static_cast<WUInt8>(i)))
      {
        W_LOCK(pWorld->GetWriteMarker());
        pWorld->GetRandomNumberGenerator().Initialize(static_cast<WUInt64>(m_iRandomSeed));
        pWorld->GetClock().SetFixedTimeStep(m_FixedTimeStep);
      }
    }
  }

  if (m_iRunFrames >= 0 || !m_sScreenshotPath.IsEmpty())
  {
    m_UnattendedExecutionEventsID = m_ExecutionEvents.AddEventHandler(WMakeDelegate(&WGameApplication::Unattended_OnExecutionEvent, this));
  }
}

void WGameApplication::Unattended_Finish()
{
  if (m_UnattendedExecutionEventsID != 0)
  {
    m_ExecutionEvents.RemoveEventHandler(m_UnattendedExecutionEventsID);
    m_UnattendedExecutionEventsID = 0;
  }

  if (m_bFailOnError && m_iLoggedErrors > 0)
  {
    WLog::Info("{} errors were logged.", (WInt32)m_iLoggedErrors);

    // a more specific failure that was already reported wins
    if (GetReturnCode() == 0)
    {
      SetReturnCode(1);
    }
  }
}

void WGameApplication::Unattended_DetachLog()
{
  if (m_UnattendedLogErrorCounterID != 0)
  {
    WGlobalLog::RemoveLogWriter(m_UnattendedLogErrorCounterID);
    m_UnattendedLogErrorCounterID = 0;
  }

  if (m_UnattendedLogToFileID != 0)
  {
    WGlobalLog::RemoveLogWriter(m_UnattendedLogToFileID);
    m_UnattendedLogToFileID = 0;
    m_UnattendedLogFile.EndLog();
  }
}

void WGameApplication::Unattended_CheckTimeout()
{
  if (m_UnattendedTimeout.IsPositive() && WTime::Now() - m_UnattendedStartTime > m_UnattendedTimeout)
  {
    WLog::Error("Timeout of {} seconds reached, quitting.", m_UnattendedTimeout.GetSeconds());
    SetReturnCode(2);
    QuitApplication();
  }
}

void WGameApplication::Unattended_OnLogEvent(const WLoggingEventData& e)
{
  if (e.m_EventType == WLogMsgType::ErrorMsg || e.m_EventType == WLogMsgType::SeriousWarningMsg)
  {
    m_iLoggedErrors.Increment();
  }
}

void WGameApplication::Unattended_OnExecutionEvent(const WGameApplicationExecutionEvent& e)
{
  // BeforePresent, not AfterPresent: Run_FinishFrame() resets the 'take screenshot' flag at the end of each
  // frame, so the request has to be made before the frame is presented
  if (e.m_Type != WGameApplicationExecutionEvent::Type::BeforePresent)
    return;

  ++m_uiRenderedFrames;

  if (m_bScreenshotRequested)
  {
    // the capture is started during the present of the frame in which it was requested and can only be
    // retrieved one or more frames later, so wait for it, rather than quitting with no (or a broken) file
    if (m_bScreenshotDone)
    {
      QuitApplication();
    }

    return;
  }

  if (m_iRunFrames < 0 || m_uiRenderedFrames < (WUInt32)m_iRunFrames)
    return;

  if (m_sScreenshotPath.IsEmpty())
  {
    WLog::Info("Rendered {} frames, quitting.", m_uiRenderedFrames);
    QuitApplication();
    return;
  }

  m_bScreenshotRequested = true;
  TakeScreenshot();
}

bool WGameApplication::Unattended_StoreScreenshot(WImage& ref_image)
{
  if (m_sScreenshotPath.IsEmpty())
    return false;

  m_bScreenshotDone = true;

  if (ref_image.Convert(WImageFormat::R8G8B8_UNORM_SRGB).Failed())
  {
    WLog::Error("Could not convert the screenshot to RGB8.");
    return true;
  }

  const WStringView sExtension = WPathUtils::GetFileExtension(m_sScreenshotPath);
  const WImageFileFormat* pFormat = WImageFileFormat::GetWriterFormat(sExtension);

  if (pFormat == nullptr)
  {
    WLog::Error("No image file format is available to write '{}'.", m_sScreenshotPath);
    return true;
  }

  WDefaultMemoryStreamStorage storage;
  WMemoryStreamWriter memoryWriter(&storage);

  if (pFormat->WriteImage(memoryWriter, ref_image, sExtension).Failed())
  {
    WLog::Error("Could not encode the screenshot as '{}'.", sExtension);
    return true;
  }

  WStringBuilder sFolder = m_sScreenshotPath;
  sFolder.PathParentDirectory();

  if (!sFolder.IsEmpty() && WOSFile::CreateDirectoryStructure(sFolder).Failed())
  {
    WLog::Error("Could not create the folder for screenshot '{}'.", m_sScreenshotPath);
    return true;
  }

  WOSFile file;
  if (file.Open(m_sScreenshotPath, WFileOpenMode::Write).Failed())
  {
    WLog::Error("Could not open screenshot file '{}' for writing.", m_sScreenshotPath);
    return true;
  }

  WMemoryStreamReader memoryReader(&storage);
  WHybridArray<WUInt8, 4096> chunk;
  chunk.SetCountUninitialized(4096);

  while (const WUInt64 uiRead = memoryReader.ReadBytes(chunk.GetData(), chunk.GetCount()))
  {
    if (file.Write(chunk.GetData(), uiRead).Failed())
    {
      WLog::Error("Could not write screenshot file '{}'.", m_sScreenshotPath);
      return true;
    }
  }

  WLog::Success("Screenshot: '{}'", m_sScreenshotPath);
  return true;
}

#endif

//////////////////////////////////////////////////////////////////////////

namespace
{
  const char* s_szInputSet = "GameApp";
  const char* s_szCloseAppAction = "CloseApp";
  const char* s_szShowConsole = "ShowConsole";
  const char* s_szShowFpsAction = "ShowFps";
  const char* s_szReloadResourcesAction = "ReloadResources";
  const char* s_szCaptureProfilingAction = "CaptureProfiling";
  const char* s_szCaptureFrame = "CaptureFrame";
  const char* s_szTakeScreenshot = "TakeScreenshot";
  const char* s_szOpenInspector = "OpenInspector";
} // namespace


void WGameApplication::RegisterGameApplicationInputActions(WBitflags<WGameApplicationInputFlags> flags)
{
  WInputActionConfig config;

  if (flags.IsSet(WGameApplicationInputFlags::Dev_EscapeToClose))
  {
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyEscape;
    WInputManager::SetInputActionConfig(s_szInputSet, s_szCloseAppAction, config, true);
  }

  if (flags.IsSet(WGameApplicationInputFlags::Dev_Console))
  {
    // the tilde has problematic behavior on keyboards where it is a hat (^)
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyF1;
    WInputManager::SetInputActionConfig("Console", s_szShowConsole, config, true);

    if (m_pConsole)
    {
      m_pConsole->LoadInputHistory(":appdata/ConsoleInputHistory.cfg");
    }
  }

  if (flags.IsSet(WGameApplicationInputFlags::Dev_ReloadResources))
  {
    // in the editor we cannot use F5, because that is already 'run application'
    // so we use F4 there, and it should be consistent here
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyF4;
    WInputManager::SetInputActionConfig(s_szInputSet, s_szReloadResourcesAction, config, true);
    WConsoleActions::AddAction(s_szInputSet, s_szReloadResourcesAction, "Engine", []()
      { WResourceManager::ReloadAllResources(false); });
  }

  if (flags.IsSet(WGameApplicationInputFlags::Dev_ShowStats))
  {
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyF5;
    WInputManager::SetInputActionConfig(s_szInputSet, s_szShowFpsAction, config, true);
    WConsoleActions::AddAction(s_szInputSet, s_szShowFpsAction, "Engine", []()
      { cvar_AppShowFPS = !cvar_AppShowFPS; });
  }

  if (flags.IsSet(WGameApplicationInputFlags::Dev_CaptureProfilingInfo))
  {
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyF8;
    WInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureProfilingAction, config, true);
    WConsoleActions::AddAction(s_szInputSet, s_szCaptureProfilingAction, "Engine", [this]()
      { TakeProfilingCapture(); });
  }

  if (flags.IsSet(WGameApplicationInputFlags::Dev_CaptureFrame))
  {
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyF11;
    WInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureFrame, config, true);
    WConsoleActions::AddAction(s_szInputSet, s_szCaptureFrame, "Engine", [this]()
      { CaptureFrame(); });
  }

  if (flags.IsSet(WGameApplicationInputFlags::Dev_Screenshot))
  {
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyF12;
    WInputManager::SetInputActionConfig(s_szInputSet, s_szTakeScreenshot, config, true);
    WConsoleActions::AddAction(s_szInputSet, s_szTakeScreenshot, "Engine", [this]()
      { TakeScreenshot(); });
  }

  if (flags.IsSet(WGameApplicationInputFlags::Dev_OpenInspector))
  {
    config.m_sInputSlotTrigger[0] = WInputSlot_KeyF10;
    WInputManager::SetInputActionConfig(s_szInputSet, s_szOpenInspector, config, true);
    WConsoleActions::AddAction(s_szInputSet, s_szOpenInspector, "Engine", [this]()
      { OpenInspector(); });
  }

  if (flags.IsSet(WGameApplicationInputFlags::LoadInputConfig))
  {
    WStringView sConfigFile = WGameAppInputConfig::s_sConfigFile;

    WFileReader file;
    if (file.Open(sConfigFile).Succeeded())
    {
      WTempHybridArray<WGameAppInputConfig, 32> InputActions;

      WGameAppInputConfig::ReadFromDDL(file, InputActions);
      WGameAppInputConfig::ApplyAll(InputActions);
    }
  }
}

WString WGameApplication::FindProjectDirectory() const
{
  W_ASSERT_RELEASE(!m_sAppProjectPath.IsEmpty(), "Either the project must have a built-in project directory passed to the WGameApplication constructor, or m_sAppProjectPath must be set manually before doing project setup, or WGameApplication::FindProjectDirectory() must be overridden.");

  if (WPathUtils::IsAbsolutePath(m_sAppProjectPath))
    return m_sAppProjectPath;

  // first check if the path is relative to the SDK special directory
  {
    WStringBuilder relToSdk(m_sAppProjectPath);

    if (!relToSdk.StartsWith_NoCase(">sdk/"))
    {
      relToSdk.Prepend(">sdk/");
    }

    WStringBuilder absToSdk;
    if (WFileSystem::ResolveSpecialDirectory(relToSdk, absToSdk).Succeeded())
    {
      if (WOSFile::ExistsDirectory(absToSdk))
        return absToSdk;
    }
  }

  WStringBuilder result;
  if (WFileSystem::FindFolderWithSubPath(result, WOSFile::GetApplicationDirectory(), m_sAppProjectPath).Failed())
  {
    WLog::Error("Could not find the project directory.");
  }

  return result;
}

void WGameApplication::OpenInspector()
{
#if W_ENABLED(W_SUPPORTS_PROCESSES)
  WProcessOptions opt;

  WStringBuilder sInspectorPath = WOSFile::GetApplicationDirectory();
#  if W_ENABLED(W_PLATFORM_WINDOWS)
  sInspectorPath.AppendPath("WInspector.exe");
#  else
  sInspectorPath.AppendPath("WInspector");
#  endif

  opt.m_sProcess = sInspectorPath;

  WProcess process;
  if (process.Launch(opt, WProcessLaunchFlags::Detached).Failed())
  {
    WLog::Warning("Failed to launch WInspector.");
  }
#endif
}

WGameUpdateMode WGameApplication::GetGameUpdateMode() const
{
  const bool bViewsScheduled = !WRenderWorld::GetMainViews().IsEmpty();
  const bool bRenderingScheduled = WRenderWorld::IsRenderingScheduled();
  if (bViewsScheduled)
  {
    return WGameUpdateMode::UpdateInputAndRender;
  }
  return bRenderingScheduled ? WGameUpdateMode::Render : WGameUpdateMode::Skip;
}

void WGameApplication::Run_WorldUpdateAndRender()
{
  W_PROFILE_SCOPE("Run_WorldUpdateAndRender");
  // If multi-threaded rendering is disabled, the same content is updated/extracted and rendered in the same frame.
  // As WRenderWorld::BeginFrame applies the render pipeline properties that were set during the update phase, it needs to be done after update/extraction but before rendering.
  if (!WRenderWorld::GetUseMultithreadedRendering())
  {
    UpdateWorldsAndExtractViews();
  }

  WRenderWorld::BeginFrame();

  WTaskGroupID updateTaskID;
  if (WRenderWorld::GetUseMultithreadedRendering())
  {
    updateTaskID = WTaskSystem::StartSingleTask(m_pUpdateTask, WTaskPriority::EarlyThisFrame);
  }

  WRenderWorld::Render(WRenderContext::GetDefaultInstance());

  if (WRenderWorld::GetUseMultithreadedRendering())
  {
    W_PROFILE_SCOPE("Wait for UpdateWorldsAndExtractViews");
    WTaskSystem::WaitForGroup(updateTaskID);
  }
}

void WGameApplication::Run_AcquireImage()
{
  auto pWinMan = WWindowManager::GetSingleton();

  WTempHybridArray<WRegisteredWndHandle, 8> windows;
  pWinMan->GetRegistered(windows);

  for (auto id : windows)
  {
    if (auto pOutput = pWinMan->GetOutputTarget(id))
    {
      // We could call `ExecuteTakeScreenshot` here, which would improve the screenshot latency by one frame. However, all unit test image comparisons would fail.
      W_PROFILE_SCOPE("AcquireImage");
      pOutput->AcquireImage();
    }
  }
}

void WGameApplication::Run_PresentImage()
{
  auto pWinMan = WWindowManager::GetSingleton();

  WTempHybridArray<WRegisteredWndHandle, 8> windows;
  pWinMan->GetRegistered(windows);

  bool bExecutedFrameCapture = false;
  for (auto id : windows)
  {
    if (auto pOutput = pWinMan->GetOutputTarget(id))
    {
      // if we have multiple actors, append the actor name to each screenshot
      WStringBuilder ctxt;
      if (windows.GetCount() > 1)
      {
        ctxt.Append(" - ", pWinMan->GetName(id));
      }

      ExecuteTakeScreenshot(pOutput, ctxt);

      auto pWindow = pWinMan->GetWindow(id);
      if (pWindow && !bExecutedFrameCapture)
      {
        ExecuteFrameCapture(pWindow->GetNativeWindowHandle(), ctxt);
        bExecutedFrameCapture = true;
      }

      W_PROFILE_SCOPE("PresentImage");
      pOutput->PresentImage(cvar_AppVSync);
    }
  }
}

void WGameApplication::Run_FinishFrame()
{
  WRenderWorld::EndFrame();

  SUPER::Run_FinishFrame();
}

void WGameApplication::UpdateWorldsAndExtractViews()
{
  WStringBuilder sb;
  sb.SetFormat("UPDATE FRAME {}", WRenderWorld::GetFrameCounter());
  W_PROFILE_SCOPE(sb.GetData());

  Run_BeforeWorldUpdate();

  WTempHybridArray<WWorld*, 16> worldsToUpdate;

  auto mainViews = WRenderWorld::GetMainViews();
  for (auto hView : mainViews)
  {
    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(hView, pView))
    {
      WWorld* pWorld = pView->GetWorld();

      if (pWorld != nullptr && !worldsToUpdate.Contains(pWorld))
      {
        worldsToUpdate.PushBack(pWorld);
      }
    }
  }

  if (WRenderWorld::GetUseMultithreadedRendering())
  {
    WTaskGroupID updateWorldsTaskID = WTaskSystem::CreateTaskGroup(WTaskPriority::EarlyThisFrame);
    for (WUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      WTaskSystem::AddTaskToGroup(updateWorldsTaskID, worldsToUpdate[i]->GetUpdateTask());
    }
    WTaskSystem::StartTaskGroup(updateWorldsTaskID);
    WTaskSystem::WaitForGroup(updateWorldsTaskID);
  }
  else
  {
    for (WUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      WWorld* pWorld = worldsToUpdate[i];
      W_LOCK(pWorld->GetWriteMarker());

      pWorld->Update();
    }
  }

  for (WUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
  {
    WWorld* pWorld = worldsToUpdate[i];
    W_LOCK(pWorld->GetReadMarker());
    RenderWorldDebugInfos(*pWorld);
  }

  Run_AfterWorldUpdate();

  RenderFps();
  RenderConsole();

  // do this now, in parallel to the view extraction
  Run_UpdatePlugins();

  WRenderWorld::ExtractMainViews();
}

void WGameApplication::RenderWorldDebugInfos(const WWorld& world)
{
  if (cvar_WorldShowObjectOrigins)
  {
    WUInt32 uiInactive = 0;
    WUInt32 uiStatic = 0;
    WUInt32 uiDynamic = 0;

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      WTransform tObj = it->GetGlobalTransform();
      tObj.m_vScale.Set(1.0f);

      if (!it->IsActive())
      {
        ++uiInactive;
        WDebugRenderer::DrawCross(&world, WVec3::MakeZero(), 0.25f, WColor::DarkGrey, tObj);
      }
      else if (it->IsDynamic())
      {
        ++uiDynamic;
        WDebugRenderer::DrawCross(&world, WVec3::MakeZero(), 0.25f, WColor::DeepPink, tObj);
      }
      else
      {
        ++uiStatic;
        WDebugRenderer::DrawCross(&world, WVec3::MakeZero(), 0.4f, WColor::DeepSkyBlue, tObj);
      }
    }

    WDebugRenderer::DrawInfoText(&world, WDebugTextPlacement::BottomLeft, "WorldStats", WFmt("Num Objects: {} - {} static / {} dynamic / {} inactive", uiStatic + uiDynamic + uiInactive, uiStatic, uiDynamic, uiInactive));
  }
}

void WGameApplication::RenderFps()
{
  W_PROFILE_SCOPE("RenderFps");
  // Do not use WClock for this, it smooths and clamps the timestep

  static WTime tAccumTime;
  static WTime tDisplayedFrameTime = m_FrameTime;
  static WUInt32 uiFrames = 0;
  static WUInt32 uiFPS = 0;

  ++uiFrames;
  tAccumTime += m_FrameTime;

  if (tAccumTime >= WTime::MakeFromSeconds(0.5))
  {
    tAccumTime -= WTime::MakeFromSeconds(0.5);
    tDisplayedFrameTime = m_FrameTime;

    uiFPS = uiFrames * 2;
    uiFrames = 0;
  }

  if (cvar_AppShowFPS)
  {
    if (const WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView))
    {
      WDebugRenderer::DrawInfoText(pView->GetHandle(), WDebugTextPlacement::BottomLeft, "FPS", WFmt("{0} fps, {1} ms", uiFPS, WArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)));
    }
  }
}

void WGameApplication::RenderConsole()
{
  if (!m_pConsole)
    return;

  W_PROFILE_SCOPE("RenderConsole");

  m_pConsole->RenderConsole(m_bShowConsole);
}

bool WGameApplication::Run_ProcessApplicationInput()
{
  // the show console command must be in the "Console" input set, because we are using that for exclusive input when the console is open
  if (WInputManager::GetInputActionState("Console", s_szShowConsole) == WKeyState::Pressed)
  {
    m_bShowConsole = !m_bShowConsole;

    if (m_bShowConsole)
    {
      WInputManager::SetExclusiveInputSet("Console");
    }
    else
    {
      WInputManager::SetExclusiveInputSet("");

      if (m_pConsole)
      {
        m_pConsole->SaveInputHistory(":appdata/ConsoleInputHistory.cfg").IgnoreResult();
      }
    }
  }

  WConsoleActions::HandleInput();

  if (m_pConsole)
  {
    m_pConsole->HandleInput(m_bShowConsole);

    if (m_bShowConsole)
      return false;
  }

  if (WInputManager::GetInputActionState(s_szInputSet, s_szCloseAppAction) == WKeyState::Pressed)
  {
    if (m_pGameState)
    {
      m_pGameState->RequestQuit("dev-esc");
    }
  }

  return SUPER::Run_ProcessApplicationInput();
}



W_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplication);
