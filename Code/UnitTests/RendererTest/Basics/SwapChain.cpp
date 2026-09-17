#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererTest/Basics/SwapChain.h>

WResult WRendererTestSwapChain::InitializeTest()
{
  WStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WResult WRendererTestSwapChain::DeInitializeTest()
{
  ShutdownRenderer();
  WStartup::ShutdownCoreSystems();
  WMemoryTracker::DumpMemoryLeaks();

  return W_SUCCESS;
}

WResult WRendererTestSwapChain::InitializeSubTest(WInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  m_CurrentWindowSize = WSizeU32(320, 240);

  // Window
  {
    WWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = m_CurrentWindowSize.width;
    WindowCreationDesc.m_Resolution.height = m_CurrentWindowSize.height;
    WindowCreationDesc.m_bClipMouseCursor = false;
    WindowCreationDesc.m_bShowMouseCursor = true;
    WindowCreationDesc.m_WindowMode = (iIdentifier == SubTests::ST_ResizeWindow) ? WWindowMode::WindowResizable : WWindowMode::WindowFixedResolution;
    // WWindow writes any window size changes into the config.
    m_pWindow = W_DEFAULT_NEW(WWindow);
    m_pWindow->Initialize(WindowCreationDesc).AssertSuccess("Window creation failed");
  }

  {
    WGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = WGALMSAASampleCount::None;
    swapChainDesc.m_InitialPresentMode = (iIdentifier == SubTests::ST_VSync) ? WGALPresentMode::VSync : WGALPresentMode::Immediate;
    m_hSwapChain = WGALWindowSwapChain::Create(swapChainDesc);
  }

  // Depth Texture
  if (iIdentifier != SubTests::ST_ColorOnly)
  {
    WGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = m_CurrentWindowSize.width;
    texDesc.m_uiHeight = m_CurrentWindowSize.height;
    texDesc.m_ResourceAccess.m_bImmutable = false;
    switch (iIdentifier)
    {
      case SubTests::ST_D16:
        texDesc.m_Format = WGALResourceFormat::D16;
        break;
      case SubTests::ST_D24S8:
        texDesc.m_Format = WGALResourceFormat::D24S8;
        break;
      default:
      case SubTests::ST_D32:
        texDesc.m_Format = WGALResourceFormat::DFloat;
        break;
    }

    texDesc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);
    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }

  return W_SUCCESS;
}

WResult WRendererTestSwapChain::DeInitializeSubTest(WInt32 iIdentifier)
{
  DestroyWindow();

  // Don't call parent's DeInitializeSubTest - renderer shutdown happens in DeInitializeTest

  return W_SUCCESS;
}


void WRendererTestSwapChain::ResizeTest(WUInt32 uiInvocationCount)
{
  if (uiInvocationCount == 4)
  {
    // Not implemented on all platforms,  so we ignore the result here.
    m_pWindow->Resize(WSizeU32(640, 480)).IgnoreResult();
  }

  if (m_pWindow->GetClientAreaSize() != m_CurrentWindowSize)
  {
    m_CurrentWindowSize = m_pWindow->GetClientAreaSize();
    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    // Swap Chain
    {
      auto presentMode = m_pDevice->GetSwapChain<WGALWindowSwapChain>(m_hSwapChain)->GetWindowDescription().m_InitialPresentMode;
      W_TEST_RESULT(m_pDevice->UpdateSwapChain(m_hSwapChain, presentMode));
    }

    // Depth Texture
    {
      WGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth = m_CurrentWindowSize.width;
      texDesc.m_uiHeight = m_CurrentWindowSize.height;
      texDesc.m_Format = WGALResourceFormat::DFloat;
      texDesc.m_ResourceAccess.m_bImmutable = false;
      texDesc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);
      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    }
  }
}

WTestAppRun WRendererTestSwapChain::BasicRenderLoop(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  BeginFrame();
  BeginCommands("SwapChainTest");
  {
    const WGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
    TransitionTexture(pPrimarySwapChain->GetBackBufferTexture(), WGALResourceState::RenderTarget);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
    renderingSetup.SetClearColor(0, WColor::CornflowerBlue);
    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      TransitionTexture(m_hDepthStencilTexture, WGALResourceState::DepthStencilWrite);
      renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
      renderingSetup.SetClearDepth().SetClearStencil();
    }
    WRectFloat viewport = WRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    m_pWindow->ProcessWindowMessages();

    WRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();
  EndFrame();

  return m_iFrame < 120 ? WTestAppRun::Continue : WTestAppRun::Quit;
}

static WRendererTestSwapChain g_SwapChainTest;
