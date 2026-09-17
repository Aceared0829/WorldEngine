#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

W_IMPLEMENT_SINGLETON(WEditorEngineProcessApp);

WEditorEngineProcessApp::WEditorEngineProcessApp()
  : m_SingletonRegistrar(this)
{
}

WEditorEngineProcessApp::~WEditorEngineProcessApp()
{
  DestroyRemoteWindow();
}

void WEditorEngineProcessApp::SetRemoteMode()
{
  m_Mode = WEditorEngineProcessMode::Remote;

  CreateRemoteWindow();
}

void WEditorEngineProcessApp::CreateRemoteWindow()
{
  W_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  if (!m_hWindow.IsInvalidated())
    return;

  WUniquePtr<WWindow> pWindow = W_DEFAULT_NEW(WWindow);

  WWindowCreationDesc desc;
  desc.m_bClipMouseCursor = false;
  desc.m_bShowMouseCursor = true;
  desc.m_Resolution = WSizeU32(1024, 768);
  desc.m_WindowMode = WWindowMode::WindowFixedResolution;
  desc.m_Title = "Engine View";

  pWindow->Initialize(desc).IgnoreResult();

  m_hWindow = WWindowManager::GetSingleton()->Register("Engine View", this, std::move(pWindow));
}

void WEditorEngineProcessApp::DestroyRemoteWindow()
{
  if (!m_hRemoteView.IsInvalidated())
  {
    WRenderWorld::DeleteView(m_hRemoteView);
    m_hRemoteView.Invalidate();
  }

  if (WWindowManager::GetSingleton())
  {
    WWindowManager::GetSingleton()->CloseAll(this);
  }

  m_hWindow.Invalidate();
}

WRenderPipelineResourceHandle WEditorEngineProcessApp::CreateDefaultMainRenderPipeline()
{
  // EditorRenderPipeline.WRenderPipelineAsset
  return WResourceManager::LoadResource<WRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }");
}

WRenderPipelineResourceHandle WEditorEngineProcessApp::CreateDefaultDebugRenderPipeline()
{
  // DebugRenderPipeline.WRenderPipelineAsset
  return WResourceManager::LoadResource<WRenderPipelineResource>("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }");
}

WViewHandle WEditorEngineProcessApp::CreateRemoteWindowAndView(WCamera* pCamera)
{
  W_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  if (m_hRemoteView.IsInvalidated())
  {
    auto pWinMan = WWindowManager::GetSingleton();

    // create output target
    {
      WUniquePtr<WWindowOutputTargetGAL> pOutput = W_DEFAULT_NEW(WWindowOutputTargetGAL);

      WGALWindowSwapChainCreationDescription desc;
      desc.m_pWindow = pWinMan->GetWindow(m_hWindow);
      desc.m_BackBufferFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;

      pOutput->CreateSwapchain(desc);

      pWinMan->SetOutputTarget(m_hWindow, std::move(pOutput));
    }

    // get swapchain
    WGALSwapChainHandle hSwapChain;
    {
      WWindowOutputTargetGAL* pOutputTarget = static_cast<WWindowOutputTargetGAL*>(pWinMan->GetOutputTarget(m_hWindow));
      hSwapChain = pOutputTarget->m_hSwapChain;
    }

    // setup view
    {
      WView* pView = nullptr;
      m_hRemoteView = WRenderWorld::CreateView("Remote Process", pView);

      // EditorRenderPipeline.WRenderPipelineAsset
      pView->SetRenderPipelineResource(WResourceManager::LoadResource<WRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"));

      const WSizeU32 wndSize = pWinMan->GetWindow(m_hWindow)->GetClientAreaSize();

      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(WRectFloat(0.0f, 0.0f, (float)wndSize.width, (float)wndSize.height));
      pView->SetCamera(pCamera);
    }
  }

  return m_hRemoteView;
}
