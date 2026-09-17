
#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <GameEngine/MouseCursor/MouseCursorRenderer.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/XRInterface.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

WCommandLineOptionPath opt_Window("GameState", "-wnd", "Path to the window configuration file to use.", "");

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameState, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
// clang-format on

WGameState* WGameState::s_pActiveGameState = nullptr;

WGameState::WGameState()
{
  // initialize camera to default values
  m_MainCamera.SetCameraMode(WCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 1000.0f);
  m_MainCamera.LookAt(WVec3::MakeZero(), WVec3(1, 0, 0), WVec3(0, 0, 1));
}

WGameState::~WGameState() = default;

WGameState* WGameState::GetActiveGameState()
{
  return s_pActiveGameState;
}

void WGameState::OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  s_pActiveGameState = this;

  CreateWindows();
  ConfigureInputActions();

  if (pWorld)
  {
    ChangeMainWorld(pWorld, sStartPosition, startPositionOffset);
  }
  else
  {
    WString sSceneFile;
    WString sPreloadCollection;
    GetStartupOptions(sSceneFile, sPreloadCollection);

    if (!sSceneFile.IsEmpty())
    {
      LoadScene(sSceneFile, sPreloadCollection, sStartPosition, startPositionOffset);
    }
  }
}

void WGameState::OnDeactivation()
{
  CancelBackgroundSceneLoading();

  if (m_bXREnabled)
  {
    m_bXREnabled = false;
    WXRInterface* pXRInterface = WSingletonRegistry::GetSingletonInstance<WXRInterface>();
    WWindowManager::GetSingleton()->CloseAll(pXRInterface); // maybe do this inside Deinitialize ?
    pXRInterface->Deinitialize();

    m_pDummyXR = nullptr;
  }

  WRenderWorld::DeleteView(m_hMainView);

  s_pActiveGameState = nullptr;
}

void WGameState::AddMainViewsToRender()
{
  if (!m_hMainView.IsInvalidated())
  {
    WRenderWorld::AddMainView(m_hMainView);
  }
}

void WGameState::RequestQuit(WStringView sRequestedBy)
{
  m_bStateWantsToQuit = true;
}

bool WGameState::WasQuitRequested() const
{
  return m_bStateWantsToQuit;
}


void WGameState::ProcessInput()
{
  UpdateBackgroundSceneLoading();
}

WView* WGameState::GetMainView()
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hMainView, pView))
  {
    return pView;
  }

  return nullptr;
}

bool WGameState::IsLoadingSceneInBackground(float* out_pProgress) const
{
  if (out_pProgress)
  {
    *out_pProgress = 0.0f;

    if (m_pBackgroundSceneLoad != nullptr)
    {
      *out_pProgress = m_pBackgroundSceneLoad->GetLoadingProgress();

      auto state = m_pBackgroundSceneLoad->GetLoadingState();
      if (state != WSceneLoadUtility::LoadingState::FinishedSuccessfully)
      {
        *out_pProgress = WMath::Min(*out_pProgress, 0.99f);
      }
    }
  }

  return m_pBackgroundSceneLoad != nullptr;
}

bool WGameState::IsInLoadingScreen() const
{
  return m_pMainWorld == m_pLoadingScreenWorld;
}

WRegisteredWndHandle WGameState::CreateXRWindow()
{
  W_LOG_BLOCK("CreateXRActor");
  // Init XR
  const WXRConfig* pConfig = WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<WXRConfig>();
  if (!pConfig)
    return {};

  if (!pConfig->m_bEnableXR)
    return {};

  WXRInterface* pXRInterface = WSingletonRegistry::GetSingletonInstance<WXRInterface>();
  if (!pXRInterface)
  {
    WLog::Warning("No WXRInterface interface found. Please load a XR plugin to enable XR. Loading dummyXR interface.");
    m_pDummyXR = W_DEFAULT_NEW(WDummyXR);
    pXRInterface = WSingletonRegistry::GetSingletonInstance<WXRInterface>();
    W_ASSERT_DEV(pXRInterface, "Creating dummyXR did not register the WXRInterface.");
  }

  if (pXRInterface->Initialize().Failed())
  {
    WLog::Error("WXRInterface could not be initialized. See log for details.");
    {
      return {};
    }
  }
  m_bXREnabled = true;

  WUniquePtr<WWindow> pMainWindow;
  WUniquePtr<WWindowOutputTargetGAL> pOutput;

  if (pXRInterface->SupportsCompanionView())
  {
    // XR Window with added companion window (allows keyboard / mouse input).
    pMainWindow = CreateMainWindow();
    W_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override WGameState::CreateActors().");
    pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
    ConfigureMainWindowInputDevices(pMainWindow.Borrow());
    CreateMainView();
    SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());
  }
  else
  {
    // XR Window (no companion window)
    CreateMainView();
    SetupMainView({}, {});
  }

  WView* pView = nullptr;
  W_VERIFY(WRenderWorld::TryGetView(m_hMainView, pView), "");
  return pXRInterface->CreateXRWindow(pView, WGALMSAASampleCount::Default, std::move(pMainWindow), std::move(pOutput));
}

void WGameState::CreateWindows()
{
  W_LOG_BLOCK("CreateActors");
  WRegisteredWndHandle windowId = CreateXRWindow();
  if (!windowId.IsInvalidated())
    return;

  WUniquePtr<WWindow> pMainWindow = CreateMainWindow();
  W_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override WGameState::CreateActors().");
  WUniquePtr<WWindowOutputTargetGAL> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
  ConfigureMainWindowInputDevices(pMainWindow.Borrow());
  CreateMainView();
  SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());


  // Default flat window
  auto pWinMan = WWindowManager::GetSingleton();
  WRegisteredWndHandle id = pWinMan->Register("Game", this, std::move(pMainWindow));
  pWinMan->SetOutputTarget(id, std::move(pOutput));
}

void WGameState::ConfigureMainWindowInputDevices(WWindow* pWindow) {}

void WGameState::ConfigureInputActions()
{
  if (auto pApp = WGameApplication::GetGameApplicationInstance())
  {
    // In shipping builds none of the developer shortcuts (F1 console, F5 stats, ESC to quit, ...) are registered,
    // since a finished game shouldn't react to those keys at all.
    //
    // If you want a different split than "everything in development builds, nothing in shipping builds",
    // override this function in your own game state and call WGameApplication::RegisterGameApplicationInputActions()
    // with the exact flags that you need.
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    pApp->RegisterGameApplicationInputActions(WGameApplicationInputFlags::All);
#else
    pApp->RegisterGameApplicationInputActions(WGameApplicationInputFlags::Regular);
#endif
  }
}

void WGameState::SetupMainView(WGALSwapChainHandle hSwapChain, WSizeU32 viewportSize)
{
  // Render the custom mouse cursor (if one is set) into the same swap-chain as the main view.
  // Done here, rather than in CreateWindows(), so that it also follows swap-chain re-creation.
  if (auto pCursorRenderer = WMouseCursorRenderer::GetSingleton())
  {
    pCursorRenderer->SetSwapChain(hSwapChain);
  }

  WView* pView = nullptr;
  if (!WRenderWorld::TryGetView(m_hMainView, pView))
  {
    WLog::Error("Main view is invalid, SetupMainView canceled.");
    return;
  }

  if (m_bXREnabled)
  {
    const WXRConfig* pConfig = WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<WXRConfig>();

    auto renderPipeline = WResourceManager::LoadResource<WRenderPipelineResource>(pConfig->m_sXRRenderPipeline);
    pView->SetRenderPipelineResource(renderPipeline);
    // Render target setup is done by WXRInterface::CreateActor
  }
  else
  {
    // Render target setup
    {
      const auto* pConfig = WGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<WRenderPipelineProfileConfig>();
      auto renderPipeline = WResourceManager::LoadResource<WRenderPipelineResource>(pConfig->m_sMainRenderPipeline);
      pView->SetRenderPipelineResource(renderPipeline);
      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(WRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
      pView->ForceUpdate();
    }
  }
}

WView* WGameState::CreateMainView()
{
  W_ASSERT_DEV(m_hMainView.IsInvalidated(), "CreateMainView was already called.");

  W_LOG_BLOCK("CreateMainView");
  WView* pView = nullptr;
  m_hMainView = WRenderWorld::CreateView("MainView", pView);
  pView->SetCameraUsageHint(WCameraUsageHint::MainView);
  pView->SetWorld(m_pMainWorld);
  pView->SetCamera(&m_MainCamera);
  WRenderWorld::AddMainView(m_hMainView);

  const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  // exclude all editor objects from rendering in proper game views
  pView->m_ExcludeTags.Set(tagEditor);
  return pView;
}

WResult WGameState::SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset)
{
  if (m_pMainWorld == nullptr)
    return W_FAILURE;

  W_LOCK(m_pMainWorld->GetWriteMarker());

  WPlayerStartPointComponentManager* pMan = m_pMainWorld->GetComponentManager<WPlayerStartPointComponentManager>();
  if (pMan == nullptr)
    return W_FAILURE;

  WPlayerStartPointComponent* pBestComp = nullptr;

  for (auto it = pMan->GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActive() && it->GetPlayerPrefab().IsValid())
    {
      if (pBestComp == nullptr)
      {
        // take the first one, no matter what
        pBestComp = it;
      }
      else if (it->GetOwner()->GetName().IsEqual_NoCase(sStartPosition))
      {
        // if we find one by exact name match, take that one
        pBestComp = it;
      }
      else if (!pBestComp->GetOwner()->GetName().IsEqual_NoCase(sStartPosition) && it->GetOwner()->GetName().IsEmpty())
      {
        // if the name of the best one isn't identical to the searched name, yet
        // and this one is nameless, prefer the nameless one
        pBestComp = it;
      }
    }
  }

  if (pBestComp)
  {
    WResourceLock<WPrefabResource> pPrefab(pBestComp->GetPlayerPrefab(), WResourceAcquireMode::BlockTillLoaded);

    if (pPrefab.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      const WUInt16 uiTeamID = pBestComp->GetOwner()->GetTeamID();
      WTransform startPos = WTransform::MakeGlobalTransform(pBestComp->GetOwner()->GetGlobalTransform(), startPositionOffset);

      if (sStartPosition.IsEqual_NoCase("GlobalOverride"))
      {
        startPos = startPositionOffset;
      }

      startPos.m_vScale.Set(1.0f);

      WPrefabInstantiationOptions options;
      options.m_pOverrideTeamID = &uiTeamID;

      pPrefab->InstantiatePrefab(*m_pMainWorld, startPos, options, &(pBestComp->m_Parameters));

      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

void WGameState::ChangeMainWorld(WWorld* pNewMainWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  if (m_pMainWorld == pNewMainWorld)
    return;

  WWorld* pPrevWorld = m_pMainWorld;

  m_pMainWorld = pNewMainWorld;

  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetWorld(m_pMainWorld);
  }

  OnChangedMainWorld(pPrevWorld, pNewMainWorld, sStartPosition, startPositionOffset);

  // make sure the camera gets re-initialized for the new world
  ConfigureMainCamera();
}

void WGameState::OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  if (pNewWorld != m_pLoadingScreenWorld)
  {
    // can get rid of the loading screen world, or we could also keep it around for later, if that has any use
    m_pLoadingScreenWorld.Clear();

    SpawnPlayer(sStartPosition, startPositionOffset).IgnoreResult();
  }
}

void WGameState::ConfigureMainCamera()
{
  if (m_MainCamera.GetCameraMode() == WCameraMode::Stereo)
  {
    // if the camera is already set to be in 'Stereo' mode, its parameters are set from the outside
    return;
  }


  if (const WWorld* pConstWorld = m_pMainWorld)
  {
    W_LOCK(pConstWorld->GetReadMarker());

    const WCameraComponentManager* pManager = pConstWorld->GetComponentManager<WCameraComponentManager>();
    if (pManager != nullptr)
    {
      for (auto itComp = pManager->GetComponents(); itComp.IsValid(); itComp.Next())
      {
        const WCameraComponent* pComp = itComp;

        if (pComp->IsActive() && pComp->GetUsageHint() == WCameraUsageHint::MainView)
        {
          WVec3 vCameraPos = pComp->GetOwner()->GetGlobalPosition();

          WCoordinateSystem coordSys;
          coordSys.m_vForwardDir = pComp->GetOwner()->GetGlobalDirForwards();
          coordSys.m_vRightDir = pComp->GetOwner()->GetGlobalDirRight();
          coordSys.m_vUpDir = pComp->GetOwner()->GetGlobalDirUp();

          // update the camera position
          // camera options (FOV etc) are already set by WCameraComponentManager on demand
          m_MainCamera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
          return;
        }
      }
    }
  }
}

WUniquePtr<WWindow> WGameState::CreateMainWindow()
{
  if (false)
  {
    WTempHybridArray<WScreenInfo, 2> screens;
    WScreen::EnumerateScreens(screens).IgnoreResult();
    WScreen::PrintScreenInfo(screens);
  }

  WStringBuilder sWndCfg = opt_Window.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sWndCfg.IsEmpty() && !WFileSystem::ExistsFile(sWndCfg))
  {
    WLog::Dev("Window Config file does not exist: '{0}'", sWndCfg);
    sWndCfg.Clear();
  }

  if (sWndCfg.IsEmpty())
  {
    const WStringView sCfgAppData = ":appdata/RuntimeConfigs/Window.ddl";
    const WStringView sCfgProject = ":project/RuntimeConfigs/Window.ddl";

    if (WFileSystem::ExistsFile(sCfgAppData))
      sWndCfg = sCfgAppData;
    else
      sWndCfg = sCfgProject;
  }

  WWindowCreationDesc wndDesc;
  wndDesc.LoadFromDDL(sWndCfg).IgnoreResult();
  wndDesc.AdjustWindowSizeAndPosition().IgnoreResult();

  WUniquePtr<WWindow> pWindow = W_DEFAULT_NEW(WWindow);
  pWindow->Initialize(wndDesc).AssertSuccess("Window creation failed");

  pWindow->WindowEvents().AddEventHandler(WMakeDelegate(&WGameState::OnWindowEvent, this));

  if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard*>(pWindow->GetInputDevice()))
  {
    pInput->SetMouseSpeed(WVec2(0.02f));
  }

  return pWindow;
}

WUniquePtr<WWindowOutputTargetGAL> WGameState::CreateMainOutputTarget(WWindow* pMainWindow)
{
  WUniquePtr<WWindowOutputTargetGAL> pOutput = W_DEFAULT_NEW(WWindowOutputTargetGAL, [this](WGALSwapChainHandle hSwapChain, WSizeU32 size)
    { SetupMainView(hSwapChain, size); });

  WGALWindowSwapChainCreationDescription desc;
  desc.m_pWindow = pMainWindow;
  desc.m_BackBufferFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;

  pOutput->CreateSwapchain(desc);

  return pOutput;
}

void WGameState::GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection)
{
  out_sScene = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");

  WStringBuilder sPreloadCollection = out_sScene;
  sPreloadCollection.ChangeFileExtension("WBinCollection");
  if (WFileSystem::ExistsFile(sPreloadCollection))
  {
    out_sPreloadCollection = sPreloadCollection;
  }
}

void WGameState::LoadScene(WStringView sSceneFile, WStringView sPreloadCollection, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  m_sTargetSceneSpawnPoint = sStartPosition;
  m_TargetSceneSpawnOffset = startPositionOffset;

  StartBackgroundSceneLoading(sSceneFile, sPreloadCollection);
  m_bTransitionWhenReady = true;

  auto state = m_pBackgroundSceneLoad->GetLoadingState();
  W_ASSERT_DEBUG(state != WSceneLoadUtility::LoadingState::FinishedAndRetrieved, "Scene already loaded and retrieved.");

  if (state != WSceneLoadUtility::LoadingState::FinishedSuccessfully)
  {
    // switch to loading screen only if we can't immediately switch to the target scene
    SwitchToLoadingScreen(sSceneFile);
  }
}

void WGameState::SwitchToLoadingScreen(WStringView sTargetSceneFile)
{
  m_pLoadingScreenWorld = CreateLoadingScreenWorld(sTargetSceneFile);

  ChangeMainWorld(m_pLoadingScreenWorld.Borrow(), {}, WTransform::MakeIdentity());
}

WUniquePtr<WWorld> WGameState::CreateLoadingScreenWorld(WStringView sTargetSceneFile)
{
  WWorldDesc desc("LoadingScreen");
  return W_DEFAULT_NEW(WWorld, desc);
}

void WGameState::StartBackgroundSceneLoading(WStringView sSceneFile, WStringView sPreloadCollection)
{
  m_bTransitionWhenReady = false;

  if ((m_pBackgroundSceneLoad != nullptr) && (m_pBackgroundSceneLoad->GetRequestedScene() == sSceneFile))
  {
    // already being loaded
    return;
  }

  CancelBackgroundSceneLoading();

  m_pBackgroundSceneLoad = W_DEFAULT_NEW(WSceneLoadUtility);
  m_pBackgroundSceneLoad->StartSceneLoading(sSceneFile, sPreloadCollection);
}

void WGameState::CancelBackgroundSceneLoading()
{
  if (m_pBackgroundSceneLoad)
  {
    OnBackgroundSceneLoadingCanceled();
    m_pBackgroundSceneLoad.Clear();
  }
}

void WGameState::UpdateBackgroundSceneLoading()
{
  if (m_pBackgroundSceneLoad)
  {
    WSceneLoadUtility::LoadingState state = m_pBackgroundSceneLoad->GetLoadingState();

    switch (state)
    {
      case WSceneLoadUtility::LoadingState::FinishedAndRetrieved:
        return;

      case WSceneLoadUtility::LoadingState::NotStarted:
      case WSceneLoadUtility::LoadingState::Ongoing:
        m_pBackgroundSceneLoad->TickSceneLoading();
        break;

      case WSceneLoadUtility::LoadingState::FinishedSuccessfully:
        if (m_bTransitionWhenReady)
        {
          OnBackgroundSceneLoadingFinished(m_pBackgroundSceneLoad->RetrieveLoadedScene());
          m_pBackgroundSceneLoad.Clear();
        }
        break;

      case WSceneLoadUtility::LoadingState::Failed:
        OnBackgroundSceneLoadingFailed(m_pBackgroundSceneLoad->GetLoadingFailureReason());
        m_pBackgroundSceneLoad.Clear();
        break;
    }
  }
}

void WGameState::OnBackgroundSceneLoadingFinished(WUniquePtr<WWorld>&& pWorld)
{
  WLog::Success("Finished loading scene '{}'.", m_pBackgroundSceneLoad->GetRequestedScene());

  m_pLoadedWorld = std::move(pWorld);
  ChangeMainWorld(m_pLoadedWorld.Borrow(), m_sTargetSceneSpawnPoint, m_TargetSceneSpawnOffset);
  m_sTargetSceneSpawnPoint.Clear();
  m_TargetSceneSpawnOffset = WTransform::MakeIdentity();
}

void WGameState::OnBackgroundSceneLoadingFailed(WStringView sReason)
{
  WLog::Error("Scene loading failed: {}", sReason);
}

void WGameState::OnBackgroundSceneLoadingCanceled()
{
  WLog::Dev("Canceled background loading of scene '{}'.", m_pBackgroundSceneLoad->GetRequestedScene());
}

void WGameState::OnWindowEvent(const WWindowEvent& e)
{
  if (e.m_Type == WWindowEvent::Type::CloseButtonClicked)
  {
    // forward the close button click to the game state
    RequestQuit("window");
  }
}
