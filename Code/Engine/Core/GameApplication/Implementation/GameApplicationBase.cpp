#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/Image.h>

WGameApplicationBase* WGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

WGameApplicationBase::WGameApplicationBase(WStringView sAppName)
  : WApplication(sAppName)
  , m_ConFunc_TakeScreenshot("TakeScreenshot", "()", WMakeDelegate(&WGameApplicationBase::TakeScreenshot, this))
  , m_ConFunc_CaptureFrame("CaptureFrame", "()", WMakeDelegate(&WGameApplicationBase::CaptureFrame, this))
{
  s_pGameApplicationBaseInstance = this;
}

WGameApplicationBase::~WGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void AppendCurrentTimestamp(WStringBuilder& out_sString)
{
  const WDateTime dt = WDateTime::MakeFromTimestamp(WTimestamp::CurrentTimestamp());

  out_sString.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), WArgU(dt.GetMonth(), 2, true), WArgU(dt.GetDay(), 2, true), WArgU(dt.GetHour(), 2, true), WArgU(dt.GetMinute(), 2, true), WArgU(dt.GetSecond(), 2, true), WArgU(dt.GetMicroseconds() / 1000, 3, true));
}

void WGameApplicationBase::TakeProfilingCapture()
{
  class WriteProfilingDataTask final : public WTask
  {
  public:
    WProfilingSystem::ProfilingData m_profilingData;

    WriteProfilingDataTask() = default;
    ~WriteProfilingDataTask() = default;

  private:
    virtual void Execute() override
    {
      WStringBuilder sPath(":appdata/Profiling/", WApplication::GetApplicationInstance()->GetApplicationName());
      AppendCurrentTimestamp(sPath);
      sPath.Append(".json");

      WFileWriter fileWriter;
      if (fileWriter.Open(sPath) == W_SUCCESS)
      {
        m_profilingData.Write(fileWriter).IgnoreResult();
        WLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
      else
      {
        WLog::Error("Could not write profiling capture to '{0}'.", sPath);
      }
    }
  };

  WSharedPtr<WriteProfilingDataTask> pWriteProfilingDataTask = W_DEFAULT_NEW(WriteProfilingDataTask);
  pWriteProfilingDataTask->ConfigureTask("Write Profiling Data", WTaskNesting::Never);
  WProfilingSystem::Capture(pWriteProfilingDataTask->m_profilingData);

  WTaskSystem::StartSingleTask(pWriteProfilingDataTask, WTaskPriority::LongRunning);
}

//////////////////////////////////////////////////////////////////////////

void WGameApplicationBase::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void WGameApplicationBase::StoreScreenshot(WImage&& image, WStringView sContext /*= {} */)
{
  class WriteFileTask final : public WTask
  {
  public:
    WImage m_Image;
    WStringBuilder m_sPath;

    WriteFileTask() = default;
    ~WriteFileTask() = default;

  private:
    virtual void Execute() override
    {
      // get rid of Alpha channel before saving
      m_Image.Convert(WImageFormat::R8G8B8_UNORM_SRGB).IgnoreResult();

      if (m_Image.SaveTo(m_sPath).Succeeded())
      {
        WLog::Info("Screenshot: '{0}'", m_sPath);
      }
    }
  };

  WSharedPtr<WriteFileTask> pWriteTask = W_DEFAULT_NEW(WriteFileTask);
  pWriteTask->ConfigureTask("Write Screenshot", WTaskNesting::Never);
  pWriteTask->m_Image.ResetAndMove(std::move(image));

  pWriteTask->m_sPath.SetFormat(":appdata/Screenshots/{0}", WApplication::GetApplicationInstance()->GetApplicationName());
  AppendCurrentTimestamp(pWriteTask->m_sPath);
  pWriteTask->m_sPath.Append(sContext);
  pWriteTask->m_sPath.Append(".png");

  // we move the file writing off to another thread to save some time
  // if we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  WTaskSystem::StartSingleTask(pWriteTask, WTaskPriority::LongRunning);
}

void WGameApplicationBase::ExecuteTakeScreenshot(WWindowOutputTargetBase* pOutputTarget, WStringView sContext /* = {} */)
{
  // Poll a previously started capture first.
  if (m_bScreenshotPending)
  {
    WImage img;
    WEnum<WCaptureImageResult> res = pOutputTarget->WaitCaptureImage(img);
    if (res == WCaptureImageResult::Ready)
    {
      StoreScreenshot(std::move(img), sContext);
      m_bScreenshotPending = false;
    }
  }

  // Start a new capture if requested and no operation is already in flight.
  if (m_bTakeScreenshot)
  {
    W_PROFILE_SCOPE("ExecuteTakeScreenshot");
    m_bScreenshotPending = true;
    pOutputTarget->StartCaptureImage().IgnoreResult();
  }
}

//////////////////////////////////////////////////////////////////////////

void WGameApplicationBase::CaptureFrame()
{
  m_bCaptureFrame = true;
}

void WGameApplicationBase::SetContinuousFrameCapture(bool bEnable)
{
  m_bContinuousFrameCapture = bEnable;
}

bool WGameApplicationBase::GetContinousFrameCapture() const
{
  return m_bContinuousFrameCapture;
}


WResult WGameApplicationBase::GetAbsFrameCaptureOutputPath(WStringBuilder& ref_sOutputPath)
{
  WStringBuilder sPath = ":appdata/FrameCaptures/Capture_";
  AppendCurrentTimestamp(sPath);
  return WFileSystem::ResolvePath(sPath, &ref_sOutputPath, nullptr);
}

void WGameApplicationBase::ExecuteFrameCapture(WWindowHandle targetWindowHandle, WStringView sContext /*= {} */)
{
  WFrameCaptureInterface* pCaptureInterface = WSingletonRegistry::GetSingletonInstance<WFrameCaptureInterface>();
  if (!pCaptureInterface)
  {
    return;
  }

  W_PROFILE_SCOPE("ExecuteFrameCapture");
  // If we still have a running capture (i.e., if no one else has taken the capture so far), finish it
  if (pCaptureInterface->IsFrameCapturing())
  {
    if (m_bCaptureFrame)
    {
      WStringBuilder sOutputPath;
      if (GetAbsFrameCaptureOutputPath(sOutputPath).Succeeded())
      {
        sOutputPath.Append(sContext);
        pCaptureInterface->SetAbsCaptureFilePathTemplate(sOutputPath);
      }

      pCaptureInterface->EndFrameCaptureAndWriteOutput(targetWindowHandle);

      WStringBuilder stringBuilder;
      if (pCaptureInterface->GetLastAbsCaptureFileName(stringBuilder).Succeeded())
      {
        WLog::Info("Frame captured: '{}'", stringBuilder);
      }
      else
      {
        WLog::Warning("Frame capture failed!");
      }
      m_bCaptureFrame = false;
    }
    else
    {
      pCaptureInterface->EndFrameCaptureAndDiscardResult(targetWindowHandle);
    }
  }

  // Start capturing the next frame if
  // (a) we want to capture the very next frame, or
  // (b) we capture every frame and later decide if we want to persist or discard it.
  if (m_bCaptureFrame || m_bContinuousFrameCapture)
  {
    pCaptureInterface->StartFrameCapture(targetWindowHandle);
  }
}

//////////////////////////////////////////////////////////////////////////

void WGameApplicationBase::ActivateGameState(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  W_ASSERT_DEBUG(m_pGameState == nullptr, "ActivateGameState cannot be called when another GameState is already active");

  m_pGameState = CreateGameState();

  W_ASSERT_ALWAYS(m_pGameState != nullptr, "Failed to create a game state.");

  m_pGameState->OnActivation(pWorld, sStartPosition, startPositionOffset);

  WGameApplicationStaticEvent e;
  e.m_Type = WGameApplicationStaticEvent::Type::AfterGameStateActivated;
  m_StaticEvents.Broadcast(e);

  W_BROADCAST_EVENT(AfterGameStateActivation, m_pGameState.Borrow());
}

void WGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  W_BROADCAST_EVENT(BeforeGameStateDeactivation, m_pGameState.Borrow());

  WGameApplicationStaticEvent e;
  e.m_Type = WGameApplicationStaticEvent::Type::BeforeGameStateDeactivated;
  m_StaticEvents.Broadcast(e);

  m_pGameState->OnDeactivation();

  WWindowManager::GetSingleton()->CloseAll(m_pGameState.Borrow());

  m_pGameState = nullptr;
}

WUniquePtr<WGameStateBase> WGameApplicationBase::CreateGameState()
{
  W_LOG_BLOCK("Create Game State");

  WUniquePtr<WGameStateBase> pCurState;

  WRTTI::ForEachDerivedType<WGameStateBase>(
    [&](const WRTTI* pRtti)
    {
      WUniquePtr<WGameStateBase> pNewState = pRtti->GetAllocator()->Allocate<WGameStateBase>();

      if (pCurState == nullptr)

      {
        pCurState = std::move(pNewState);
        return;
      }

      if (pCurState->IsFallbackGameState() && !pNewState->IsFallbackGameState())
      {
        pCurState = std::move(pNewState);
        return;
      }

      if (pCurState->IsFallbackGameState() && pNewState->IsFallbackGameState())
      {
        if (pNewState->GetDynamicRTTI()->IsDerivedFrom(pCurState->GetDynamicRTTI()))
        {
          pCurState = std::move(pNewState);
          return;
        }

        WLog::Warning("Multiple fallback game states found: '{}' and '{}'", pNewState->GetDynamicRTTI()->GetTypeName(), pCurState->GetDynamicRTTI()->GetTypeName());
        return;
      }

      if (!pCurState->IsFallbackGameState() && !pNewState->IsFallbackGameState())
      {
        WLog::Warning("Multiple game state implementations found: '{}' and '{}'", pNewState->GetDynamicRTTI()->GetTypeName(), pCurState->GetDynamicRTTI()->GetTypeName());
        return;
      }
    },
    WRTTI::ForEachOptions::ExcludeNotConcrete);

  return pCurState;
}

void WGameApplicationBase::ActivateGameStateAtStartup()
{
  ActivateGameState(nullptr, {}, WTransform::MakeIdentity());
}

WResult WGameApplicationBase::BeforeCoreSystemsStartup()
{
  WStartup::AddApplicationTag("runtime");

  ExecuteBaseInitFunctions();

  return SUPER::BeforeCoreSystemsStartup();
}

void WGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  // If one of the init functions already requested the application to quit,
  // something must have gone wrong. Don't continue initialization and let the
  // application exit.
  if (ShouldApplicationQuit())
  {
    return;
  }

  WStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void WGameApplicationBase::ExecuteBaseInitFunctions()
{
  BaseInit_ConfigureLogging();
}

void WGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // make sure that no resources continue to be streamed in, while the engine shuts down
    WResourceManager::EngineAboutToShutdown();
    WResourceManager::ExecuteAllResourceCleanupCallbacks();
    WResourceManager::FreeAllUnusedResources();
  }
}

void WGameApplicationBase::BeforeCoreSystemsShutdown()
{
  if (WWindowManager::GetSingleton() != nullptr)
  {
    WWindowManager::GetSingleton()->CloseAll(nullptr);
  }

  {
    WFrameAllocator::Reset();
    WResourceManager::FreeAllUnusedResources();
  }

  {
    Deinit_ShutdownGraphicsDevice();
    WResourceManager::FreeAllUnusedResources();
  }

  WTaskSystem::BroadcastClearThreadLocalsEvent();

  Deinit_UnloadPlugins();

  // shut down telemetry if it was set up
  {
    WTelemetry::CloseConnection();
  }

  Deinit_ShutdownLogging();

  SUPER::BeforeCoreSystemsShutdown();
}

static bool s_bUpdatePluginsExecuted = false;

W_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  W_IGNORE_UNUSED(param0);
  W_IGNORE_UNUSED(param1);
  W_IGNORE_UNUSED(param2);
  W_IGNORE_UNUSED(param3);

  s_bUpdatePluginsExecuted = true;
}

void WGameApplicationBase::Run()
{
  RunOneFrame();
}

void WGameApplicationBase::RunOneFrame()
{
  WProfilingSystem::StartNewFrame();

  W_PROFILE_SCOPE("Run");
  s_bUpdatePluginsExecuted = false;

  WWindowManager::GetSingleton()->Update();

  const WGameUpdateMode state = GetGameUpdateMode();
  if (state == WGameUpdateMode::Skip)
    return;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    W_PROFILE_SCOPE("GameApp_BeginAppTick");
    W_BROADCAST_EVENT(GameApp_BeginAppTick);
    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::BeginAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  if (state == WGameUpdateMode::UpdateInputAndRender)
  {
    Run_InputUpdate();
  }

  Run_AcquireImage();

  Run_WorldUpdateAndRender();

  if (!s_bUpdatePluginsExecuted)
  {
    Run_UpdatePlugins();

    W_ASSERT_DEV(s_bUpdatePluginsExecuted, "WGameApplicationBase::Run_UpdatePlugins has been overridden, but it does not broadcast the "
                                            "global event 'GameApp_UpdatePlugins' anymore.");
  }

  {
    // for plugins that need to hook into this without a link dependency on this lib
    W_PROFILE_SCOPE("GameApp_EndAppTick");
    W_BROADCAST_EVENT(GameApp_EndAppTick);

    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    W_PROFILE_SCOPE("BeforePresent");
    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    W_PROFILE_SCOPE("Run_PresentImage");
    Run_PresentImage();
  }
  WClock::GetGlobalClock()->Update();
  UpdateFrameTime();

  {
    W_PROFILE_SCOPE("AfterPresent");
    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    W_PROFILE_SCOPE("Run_FinishFrame");
    Run_FinishFrame();
  }
}

bool WGameApplicationBase::ShouldApplicationQuit() const
{
  if (m_pGameState && m_pGameState->WasQuitRequested())
  {
    return true;
  }

  return WApplication::ShouldApplicationQuit();
}

void WGameApplicationBase::Run_InputUpdate()
{
  W_PROFILE_SCOPE("Run_InputUpdate");
  WInputManager::Update(WClock::GetGlobalClock()->GetTimeDiff());

  if (!Run_ProcessApplicationInput())
    return;

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}

bool WGameApplicationBase::Run_ProcessApplicationInput()
{
  return true;
}

void WGameApplicationBase::Run_AcquireImage()
{
}

void WGameApplicationBase::Run_BeforeWorldUpdate()
{
  W_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->BeforeWorldUpdate();
  }

  {
    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::BeforeWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void WGameApplicationBase::Run_AfterWorldUpdate()
{
  W_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->AfterWorldUpdate();

    m_pGameState->ConfigureMainCamera();
  }

  {
    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::AfterWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void WGameApplicationBase::Run_UpdatePlugins()
{
  W_PROFILE_SCOPE("Run_UpdatePlugins");
  {
    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::BeforeUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }

  // for plugins that need to hook into this without a link dependency on this lib
  W_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    WGameApplicationExecutionEvent e;
    e.m_Type = WGameApplicationExecutionEvent::Type::AfterUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }
}

void WGameApplicationBase::Run_PresentImage() {}

void WGameApplicationBase::Run_FinishFrame()
{
  WTelemetry::PerFrameUpdate();
  WResourceManager::PerFrameUpdate();
  WTaskSystem::FinishFrameTasks();
  WFrameAllocator::Swap();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // if many messages have been logged, make sure they get written to disk
  WLog::Flush(100, WTime::MakeFromSeconds(10));
#endif

  // reset this state
  m_bTakeScreenshot = false;
}

void WGameApplicationBase::UpdateFrameTime()
{
  // Do not use WClock for this, it smooths and clamps the timestep
  const WTime tNow = WClock::GetGlobalClock()->GetLastUpdateTime();

  static WTime tLast = tNow;
  m_FrameTime = tNow - tLast;
  tLast = tNow;
}



W_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBase);
