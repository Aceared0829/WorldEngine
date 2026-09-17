#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>
#include <RendererTest/Advanced/OffscreenRenderer.h>
#include <RendererTest/TestClass/TestClass.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WOffscreenTest_SharedTexture, WNoBase, 1, WRTTIDefaultAllocator<WOffscreenTest_SharedTexture>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CurrentTextureIndex", m_uiCurrentTextureIndex),
    W_MEMBER_PROPERTY("CurrentSemaphoreValue", m_uiCurrentSemaphoreValue),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WOffscreenTest_OpenMsg, 1, WRTTIDefaultAllocator<WOffscreenTest_OpenMsg>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("TextureDesc", m_TextureDesc),
      W_ARRAY_MEMBER_PROPERTY("TextureHandles", m_TextureHandles),
    }
    W_END_PROPERTIES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WOffscreenTest_CloseMsg, 1, WRTTIDefaultAllocator<WOffscreenTest_CloseMsg>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WOffscreenTest_RenderMsg, 1, WRTTIDefaultAllocator<WOffscreenTest_RenderMsg>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Texture", m_Texture),
    }
    W_END_PROPERTIES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WOffscreenTest_RenderResponseMsg, 1, WRTTIDefaultAllocator<WOffscreenTest_RenderResponseMsg>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Texture", m_Texture),
    }
    W_END_PROPERTIES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WOffscreenRendererTest::WOffscreenRendererTest()
  : WApplication("WOffscreenRendererTest")

{
}

WOffscreenRendererTest::~WOffscreenRendererTest() = default;

void WOffscreenRendererTest::Run()
{
  W_PROFILE_SCOPE("Run");

  WClock::GetGlobalClock()->Update();

  if (!m_pProtocol->ProcessMessages())
  {
    m_pProtocol->WaitForMessages(WTime::MakeFromMilliseconds(8)).IgnoreResult();
  }

  // do the rendering
  if (!m_RequestedFrames.IsEmpty())
  {
    WOffscreenTest_RenderMsg action = m_RequestedFrames[0];
    m_RequestedFrames.RemoveAtAndCopy(0);

    auto device = WGALDevice::GetDefaultDevice();

    auto pSwapChain = const_cast<WGALSharedTextureSwapChain*>(WGALDevice::GetDefaultDevice()->GetSwapChain<WGALSharedTextureSwapChain>(m_hSwapChain));
    W_ASSERT_DEBUG(pSwapChain, "SwapChain should have been created at this point");
    W_ANALYSIS_ASSUME(pSwapChain != nullptr);
    pSwapChain->Arm(action.m_Texture.m_uiCurrentTextureIndex, action.m_Texture.m_uiCurrentSemaphoreValue);

    WStringBuilder sTemp;
    sTemp.SetFormat("Render {}|{}", action.m_Texture.m_uiCurrentTextureIndex, action.m_Texture.m_uiCurrentSemaphoreValue);
    W_PROFILE_SCOPE(sTemp);

    m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
    device->BeginFrame();

    WGALCommandEncoder* pCommandEncoder = device->BeginCommands(sTemp);

    WGALRenderingSetup renderingSetup;
    WGALRenderTargetViewHandle hBackbufferRTV = device->GetDefaultRenderTargetView(pSwapChain->GetRenderTargets().m_hRTs[0]);
    renderingSetup.SetColorTarget(0, hBackbufferRTV);
    renderingSetup.SetClearColor(0, WColor::Pink);

    WRectFloat viewport = WRectFloat(0, 0, 8, 8);
    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    WGraphicsTest::SetClipSpace();

    WRenderContext::GetDefaultInstance()->BindShader(m_hScreenShader);
    WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    WRenderContext::GetDefaultInstance()->EndRendering();

    device->EndCommands(pCommandEncoder);

    device->EndFrame();
  }

  if (m_RequestedFrames.IsEmpty() && m_bExiting)
  {
    SetReturnCode(0);
    QuitApplication();
  }

  // needs to be called once per frame
  WResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  WTaskSystem::FinishFrameTasks();
}

void WOffscreenRendererTest::OnPresent(WUInt32 uiCurrentTexture, WUInt64 uiCurrentSemaphoreValue)
{
  WStringBuilder sTemp;
  sTemp.SetFormat("Response {}|{}", uiCurrentTexture, uiCurrentSemaphoreValue);
  W_PROFILE_SCOPE(sTemp);

  WOffscreenTest_RenderResponseMsg msg = {};
  msg.m_Texture.m_uiCurrentSemaphoreValue = uiCurrentSemaphoreValue;
  msg.m_Texture.m_uiCurrentTextureIndex = uiCurrentTexture;
  m_pProtocol->Send(&msg);
}

void WOffscreenRendererTest::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  WGraphicsTest::CreateRenderer(m_pDevice).AssertSuccess();

  WGlobalLog::AddLogWriter(WLoggingEvent::Handler(&WLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  WStringBuilder sLogFile;
  sLogFile.SetFormat(":imgout/OffscreenLog.htm");
  m_LogHTML.BeginLog(sLogFile, "OffscreenRenderer"_wsv);

  // Setup Shaders and Materials
  {
    m_hScreenShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/UVColor.WShader");
  }

  if (WCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC").IsEmpty())
  {
    W_REPORT_FAILURE("Command Line does not contain -IPC parameter");
    SetReturnCode(-1);
    QuitApplication();
    return;
  }

  if (WCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID").IsEmpty())
  {
    W_REPORT_FAILURE("Command Line does not contain -PID parameter");
    SetReturnCode(-2);
    QuitApplication();
    return;
  }

  m_iHostPID = 0;
  if (WConversionUtils::StringToInt64(WCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID"), m_iHostPID).Failed())
  {
    W_REPORT_FAILURE("Command Line -PID parameter could not be converted to int");
    SetReturnCode(-3);
    QuitApplication();
    return;
  }

  WLog::Debug("Host Process ID: {0}", m_iHostPID);

  m_pChannel = WIpcChannel::CreatePipeChannel(WCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), WIpcChannel::Mode::Client);
  m_pProtocol = W_DEFAULT_NEW(WIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(WMakeDelegate(&WOffscreenRendererTest::MessageFunc, this));
  W_TEST_RESULT(m_pChannel->Connect());

  while (m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Connecting)
  {
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(16));
  }

  if (m_pChannel->GetConnectionState() != WIpcChannel::ConnectionState::Connected)
  {
    WLog::Error("Failed to connect to host process");
    SetReturnCode(-4);
    QuitApplication();
    return;
  }

  WStartup::StartupHighLevelSystems();
}

void WOffscreenRendererTest::BeforeHighLevelSystemsShutdown()
{
  WStringView sPath = ":imgout/Profiling/offscreenProfiling.json"_wsv;
  W_TEST_RESULT(WProfilingUtils::SaveProfilingCapture(sPath));

  auto pDevice = WGALDevice::GetDefaultDevice();

  pDevice->DestroySwapChain(m_hSwapChain);
  // This guarantees that when the process exits no shared textures are still being modified by the GPU so the main process can savely delete the resources.
  pDevice->WaitIdle();

  m_hSwapChain.Invalidate();

  m_hScreenShader.Invalidate();

  m_pProtocol = nullptr;
  m_pChannel = nullptr;

  WGlobalLog::RemoveLogWriter(WLoggingEvent::Handler(&WLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  SUPER::BeforeHighLevelSystemsShutdown();
}

void WOffscreenRendererTest::BeforeCoreSystemsShutdown()
{
  WResourceManager::FreeAllUnusedResources();

  if (m_pDevice)
  {
    m_pDevice->Shutdown().IgnoreResult();
    W_DEFAULT_DELETE(m_pDevice);
  }

  SUPER::BeforeCoreSystemsShutdown();
}

void WOffscreenRendererTest::MessageFunc(const WIpcProcessMessageProtocol::Event& msg)
{
  if (const auto* pAction = WDynamicCast<const WOffscreenTest_OpenMsg*>(msg.m_pMessage))
  {
    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
    W_ASSERT_DEBUG(m_hSwapChain.IsInvalidated(), "SwapChain creation should only happen once");

    WGALSharedTextureSwapChainCreationDescription desc;
    desc.m_TextureDesc = pAction->m_TextureDesc;
    desc.m_Textures = pAction->m_TextureHandles;
    desc.m_OnPresent = WMakeDelegate(&WOffscreenRendererTest::OnPresent, this);

    m_hSwapChain = WGALSharedTextureSwapChain::Create(desc);
    if (m_hSwapChain.IsInvalidated())
    {
      W_REPORT_FAILURE("Failed to create shared texture swapchain");
      SetReturnCode(-4);
      QuitApplication();
    }
  }
  else if (const auto* pAction = WDynamicCast<const WOffscreenTest_CloseMsg*>(msg.m_pMessage))
  {
    m_bExiting = true;
  }
  else if (const auto* pAction = WDynamicCast<const WOffscreenTest_RenderMsg*>(msg.m_pMessage))
  {
    W_ASSERT_DEBUG(m_bExiting == false, "No new frame requests should come in at this point.");
    m_RequestedFrames.PushBack(*pAction);
  }
}
