#include <ShaderExplorer/ShaderExplorer.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Core/Input/VirtualThumbStick.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

static bool g_bWindowResized = false;

WShaderExplorerApp::WShaderExplorerApp()
  : WApplication("Shader Explorer")
{
}

void WShaderExplorerApp::Run()
{
  m_pWindow->ProcessWindowMessages();
  if (!m_pWindow->IsVisible())
  {
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(16));
    return;
  }

  if (g_bWindowResized)
  {
    g_bWindowResized = false;
    UpdateSwapChain();
  }

  if (WInputManager::GetInputActionState("Main", "CloseApp") == WKeyState::Pressed)
  {
    QuitApplication();
    return;
  }

  // make sure time goes on
  WClock::GetGlobalClock()->Update();

  // update all input state
  WInputManager::Update(WClock::GetGlobalClock()->GetTimeDiff());

  // mouse look
  if (WInputManager::GetInputActionState("Main", "Look") == WKeyState::Down)
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard*>(m_pWindow->GetInputDevice()))
    {
      pInput->SetShowMouseCursor(false);
      pInput->SetClipMouseCursor(WMouseCursorClipMode::ClipToPosition);
    }

    float fInputValue = 0.0f;
    const float fMouseSpeed = 0.01f;

    WVec3 mouseMotion(0.0f);

    if (WInputManager::GetInputActionState("Main", "LookPosX", &fInputValue) != WKeyState::Up)
      mouseMotion.x += fInputValue * fMouseSpeed;
    if (WInputManager::GetInputActionState("Main", "LookNegX", &fInputValue) != WKeyState::Up)
      mouseMotion.x -= fInputValue * fMouseSpeed;
    if (WInputManager::GetInputActionState("Main", "LookPosY", &fInputValue) != WKeyState::Up)
      mouseMotion.y -= fInputValue * fMouseSpeed;
    if (WInputManager::GetInputActionState("Main", "LookNegY", &fInputValue) != WKeyState::Up)
      mouseMotion.y += fInputValue * fMouseSpeed;

    m_pCamera->RotateLocally(WAngle::MakeFromRadian(0.0), WAngle::MakeFromRadian(mouseMotion.y), WAngle::MakeFromRadian(0.0));
    m_pCamera->RotateGlobally(WAngle::MakeFromRadian(0.0), WAngle::MakeFromRadian(mouseMotion.x), WAngle::MakeFromRadian(0.0));
  }
  else
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard*>(m_pWindow->GetInputDevice()))
    {
      pInput->SetShowMouseCursor(true);
      pInput->SetClipMouseCursor(WMouseCursorClipMode::NoClip);
    }
  }

  // turn camera with keys
  {
    float fInputValue = 0.0f;
    const float fTurnSpeed = 1.0f;

    WVec3 mouseMotion(0.0f);

    if (WInputManager::GetInputActionState("Main", "TurnPosX", &fInputValue) != WKeyState::Up)
      mouseMotion.x += fInputValue * fTurnSpeed;
    if (WInputManager::GetInputActionState("Main", "TurnNegX", &fInputValue) != WKeyState::Up)
      mouseMotion.x -= fInputValue * fTurnSpeed;
    if (WInputManager::GetInputActionState("Main", "TurnPosY", &fInputValue) != WKeyState::Up)
      mouseMotion.y += fInputValue * fTurnSpeed;
    if (WInputManager::GetInputActionState("Main", "TurnNegY", &fInputValue) != WKeyState::Up)
      mouseMotion.y -= fInputValue * fTurnSpeed;

    m_pCamera->RotateLocally(WAngle::MakeFromRadian(0.0), WAngle::MakeFromRadian(mouseMotion.y), WAngle::MakeFromRadian(0.0));
    m_pCamera->RotateGlobally(WAngle::MakeFromRadian(0.0), WAngle::MakeFromRadian(mouseMotion.x), WAngle::MakeFromRadian(0.0));
  }

  // movement
  {
    float fInputValue = 0.0f;
    WVec3 cameraMotion(0.0f);

    if (WInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != WKeyState::Up)
      cameraMotion.x += fInputValue;
    if (WInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != WKeyState::Up)
      cameraMotion.x -= fInputValue;
    if (WInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != WKeyState::Up)
      cameraMotion.y += fInputValue;
    if (WInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != WKeyState::Up)
      cameraMotion.y -= fInputValue;

    m_pCamera->MoveLocally(cameraMotion.y, cameraMotion.x, 0.0f);
  }

#if W_ENABLED(USE_DIRECTORY_WATCHER)
  {
    m_bStuffChanged = false;
    m_pDirectoryWatcher->EnumerateChanges(WMakeDelegate(&WShaderExplorerApp::OnFileChanged, this));

    if (m_bStuffChanged)
    {
      WResourceManager::ReloadAllResources(false);
    }
  }
#endif

  // do the rendering
  {
    // Before starting to render in a frame call this function
    m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
    m_pDevice->BeginFrame();

    WGALCommandEncoder* pCommandEncoder = m_pDevice->BeginCommands("WShaderExplorerMainPass");
    const WGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
    WGALRenderTargetViewHandle hBBRTV = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetRenderTargets().m_hRTs[0]);
    WGALRenderTargetViewHandle hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
    renderingSetup.SetClearColor(0).SetClearDepth().SetClearStencil();

    const float fWindowWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fWindowHeight = (float)m_pWindow->GetClientAreaSize().height;

    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, WRectFloat(0.0f, 0.0f, fWindowWidth, fWindowHeight));

    auto& gc = WRenderContext::GetDefaultInstance()->WriteGlobalConstants();
    WMemoryUtils::ZeroFill(&gc, 1);

    WMat4 m0, m1;
    m0 = m_pCamera->GetViewMatrix(WCameraEye::Left);
    m1 = m_pCamera->GetViewMatrix(WCameraEye::Right);
    gc.WorldToCameraMatrix[0] = m0;
    gc.WorldToCameraMatrix[1] = m1;
    gc.CameraToWorldMatrix[0] = m0.GetInverse();
    gc.CameraToWorldMatrix[1] = m1.GetInverse();
    gc.ViewportSize = WVec4(fWindowWidth, fWindowHeight, 1.0f / fWindowWidth, 1.0f / fWindowHeight);
    // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
    gc.GlobalTime = (float)WMath::Mod(WClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
    gc.WorldTime = gc.GlobalTime;

    WRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);
    WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMeshBuffer);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();

    WRenderContext::GetDefaultInstance()->EndRendering();
    m_pDevice->EndCommands(pCommandEncoder);

    m_pDevice->EndFrame();
  }

  // needs to be called once per frame
  WResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  WTaskSystem::FinishFrameTasks();

  // for plugins (like FileServe) that need to hook into the game update
  W_BROADCAST_EVENT(GameApp_UpdatePlugins);
}

void WShaderExplorerApp::AfterCoreSystemsStartup()
{
#if W_ENABLED(USE_FILESERVE)
  WPlugin::LoadPlugin("WFileservePlugin").AssertSuccess("Failed to load FileServe plugin");
#endif

  m_pCamera = W_DEFAULT_NEW(WCamera);
  m_pCamera->LookAt(WVec3(3, 3, 1.5), WVec3(0, 0, 0), WVec3(0, 1, 0));

  WStringBuilder sProjectDir = ">sdk/Data/Samples/ShaderExplorer";
  WStringBuilder sProjectDirResolved;
  WFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();
  WFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

#if W_ENABLED(USE_DIRECTORY_WATCHER)
  m_pDirectoryWatcher = W_DEFAULT_NEW(WDirectoryWatcher);
  m_pDirectoryWatcher->OpenDirectory(sProjectDirResolved, WDirectoryWatcher::Watch::Writes | WDirectoryWatcher::Watch::Subdirectories).AssertSuccess("Failed to watch project directory");
#endif

  WFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", WDataDirUsage::AllowWrites).AssertSuccess();
  WFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
  WFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

  WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

  WTelemetry::CreateServer();
  WPlugin::LoadPlugin("WInspectorPlugin", WPluginLoadFlags::PluginIsOptional).IgnoreResult();

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultRenderer = "Vulkan";
#else
  constexpr const char* szDefaultRenderer = "DX11";
#endif

  WStringView sRendererName = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  WGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

  WShaderManager::Configure(szShaderModel, true);
  WPlugin::LoadPlugin(szShaderCompiler).IgnoreResult();

  // Register Input
  {
    {
      m_pLeftStick = W_DEFAULT_NEW(WVirtualThumbStick);
      m_pLeftStick->SetInputArea(WVec2(0, 0), WVec2(0.5, 1), 0.25f, 1.0f, WVirtualThumbStick::CenterMode::ActivationPoint);
      m_pLeftStick->SetTriggerInputSlot(WVirtualThumbStick::Input::Touchpoint);
      m_pLeftStick->SetThumbstickOutput(WVirtualThumbStick::Output::Controller0_LeftStick);
      m_pLeftStick->SetAreaFocusMode(WInputActionConfig::OnEnterArea::ActivateImmediately, WInputActionConfig::OnLeaveArea::KeepFocus);
      m_pLeftStick->SetEnabled(true);
    }
    {
      m_pRightStick = W_DEFAULT_NEW(WVirtualThumbStick);
      m_pRightStick->SetInputArea(WVec2(0.5, 0), WVec2(1, 1), 0.25f, 1.0f, WVirtualThumbStick::CenterMode::ActivationPoint);
      m_pRightStick->SetTriggerInputSlot(WVirtualThumbStick::Input::Touchpoint);
      m_pRightStick->SetThumbstickOutput(WVirtualThumbStick::Output::Controller0_RightStick);
      m_pRightStick->SetAreaFocusMode(WInputActionConfig::OnEnterArea::ActivateImmediately, WInputActionConfig::OnLeaveArea::KeepFocus);
      m_pRightStick->SetEnabled(true);
    }


    WInputActionConfig cfg;

    cfg = WInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyEscape;
    WInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "LookPosX");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMovePosX;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "LookPosX", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "LookNegX");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMoveNegX;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "LookNegX", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "LookPosY");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMovePosY;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "LookPosY", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "LookNegY");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMoveNegY;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "LookNegY", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "TurnPosX");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyRight;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_RightStick_PosX;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "TurnPosX", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "TurnNegX");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyLeft;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_RightStick_NegX;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "TurnNegX", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "TurnPosY");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyDown;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_RightStick_PosY;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "TurnPosY", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "TurnNegY");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyUp;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_RightStick_NegY;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "TurnNegY", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "Look");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseButton0;
    cfg.m_bApplyTimeScaling = false;
    WInputManager::SetInputActionConfig("Main", "Look", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "MovePosX");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyD;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_LeftStick_PosX;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "MoveNegX");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyA;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_LeftStick_NegX;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "MovePosY");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyW;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_LeftStick_PosY;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

    cfg = WInputManager::GetInputActionConfig("Main", "MoveNegY");
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyS;
    cfg.m_sInputSlotTrigger[1] = WInputSlot_Controller0_LeftStick_NegY;
    cfg.m_bApplyTimeScaling = true;
    WInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);
  }

  // Create a window for rendering
  {
    WWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = 1024;
    WindowCreationDesc.m_Resolution.height = 768;
    WindowCreationDesc.m_Title = "Shader Explorer";
    WindowCreationDesc.m_bShowMouseCursor = true;
    WindowCreationDesc.m_bClipMouseCursor = false;
    WindowCreationDesc.m_WindowMode = WWindowMode::WindowResizable;
    m_pWindow = W_DEFAULT_NEW(WWindow);
    m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();

    m_pWindow->WindowEvents().AddEventHandler([this](const WWindowEvent& e)
      {
        if (e.m_Type == WWindowEvent::Type::CloseButtonClicked)
        {
          this->QuitApplication();
        }
        if (e.m_Type == WWindowEvent::Type::SizeChanged)
        {
          g_bWindowResized = true;
        }
        //
      });
  }

  // Create a device
  {
    WGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = true;

    m_pDevice = WGALDeviceFactory::CreateDevice(sRendererName, WFoundation::GetDefaultAllocator(), DeviceInit);
    W_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sRendererName);
    W_VERIFY(m_pDevice->Init() == W_SUCCESS, "Device init failed!");

    WGALDevice::SetDefaultDevice(m_pDevice);
  }

  // now that we have a window and device, tell the engine to initialize the rendering infrastructure
  WStartup::StartupHighLevelSystems();

  UpdateSwapChain();

  // Setup Shaders and Materials
  {
    m_hMaterial = WResourceManager::LoadResource<WMaterialResource>("Materials/screen.WMaterial");

    // Create the mesh that we use for rendering
    CreateScreenQuad();
  }
}

void WShaderExplorerApp::BeforeHighLevelSystemsShutdown()
{
  WTelemetry::CloseConnection();
  m_pLeftStick.Clear();
  m_pRightStick.Clear();
#if W_ENABLED(USE_DIRECTORY_WATCHER)
  m_pDirectoryWatcher->CloseDirectory();
  m_pDirectoryWatcher.Clear();
#endif
  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_hMaterial.Invalidate();
  m_hQuadMeshBuffer.Invalidate();
  m_pDevice->DestroySwapChain(m_hSwapChain);

  // tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  WStartup::ShutdownHighLevelSystems();

  // now we can destroy the graphics device
  m_pDevice->Shutdown().IgnoreResult();

  W_DEFAULT_DELETE(m_pDevice);

  // finally destroy the window
  m_pWindow->DestroyWindow();
  W_DEFAULT_DELETE(m_pWindow);

  m_pCamera.Clear();
}

void WShaderExplorerApp::UpdateSwapChain()
{
  // Create a Swapchain
  if (m_hSwapChain.IsInvalidated())
  {
    WGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = WGALMSAASampleCount::None;
    swapChainDesc.m_InitialPresentMode = WGALPresentMode::VSync;
    m_hSwapChain = WGALWindowSwapChain::Create(swapChainDesc);
  }
  else
  {
    m_pDevice->UpdateSwapChain(m_hSwapChain, WGALPresentMode::VSync).IgnoreResult();
  }

  if (!m_hSwapChain.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();
  }
  // Create depth texture
  {
    WGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = m_pWindow->GetClientAreaSize().width;
    texDesc.m_uiHeight = m_pWindow->GetClientAreaSize().height;
    texDesc.m_Format = WGALResourceFormat::D24S8;
    texDesc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }
}

void WShaderExplorerApp::CreateScreenQuad()
{
  WGeometry geom;
  WGeometry::GeoOptions opt;
  opt.m_Color = WColor::Black;
  geom.AddRect(WVec2(2, 2), 1, 1, opt);

  WMeshBufferResourceDescriptor desc;
  desc.AddStream(WMeshVertexStreamType::Position);
  desc.AllocateStreamsFromGeometry(geom);

  m_hQuadMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}");

  if (!m_hQuadMeshBuffer.IsValid())
    m_hQuadMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}", std::move(desc));
}

#if W_ENABLED(USE_DIRECTORY_WATCHER)
void WShaderExplorerApp::OnFileChanged(WStringView sFilename, WDirectoryWatcherAction action, WDirectoryWatcherType type)
{
  if (action == WDirectoryWatcherAction::Modified && type == WDirectoryWatcherType::File)
  {
    WLog::Info("The file {0} was modified", sFilename);
    m_bStuffChanged = true;
  }
}
#endif

W_APPLICATION_ENTRY_POINT(WShaderExplorerApp);
