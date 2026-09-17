#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Logging/Log.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

WWindowOutputTargetGAL::WWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged)
  : m_OnSwapChainChanged(onSwapChainChanged)
{
  WGALDevice::GetDefaultDevice()->s_SwapChainUpdatedEvent.AddEventHandler(WMakeDelegate(&WWindowOutputTargetGAL::SwapChainUpdatedEventHandler, this));
  WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WWindowOutputTargetGAL::OnRenderEvent, this));
  m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("CaptureImage", WRenderGraphPhase::PostRender);
}

WWindowOutputTargetGAL::~WWindowOutputTargetGAL()
{
  WGALDevice::GetDefaultDevice()->s_SwapChainUpdatedEvent.RemoveEventHandler(WMakeDelegate(&WWindowOutputTargetGAL::SwapChainUpdatedEventHandler, this));
  WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WWindowOutputTargetGAL::OnRenderEvent, this));

  WGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();
  // After the swapchain is destroyed it can still be used in the renderer. As right after this usually the window is destroyed we must ensure that nothing still renders to it.
  WGALDevice::GetDefaultDevice()->WaitIdle();
}

void WWindowOutputTargetGAL::CreateSwapchain(const WGALWindowSwapChainCreationDescription& desc)
{
  m_CurrentDesc = desc;
  // WWindowOutputTargetGAL takes over the present mode and keeps it up to date with cvar_AppVSync.
  m_CurrentDesc.m_InitialPresentMode = WGameApplication::cvar_AppVSync ? WGALPresentMode::VSync : WGALPresentMode::Immediate;
  const bool bSwapChainExisted = !m_hSwapChain.IsInvalidated();
  if (bSwapChainExisted)
  {
    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
    pDevice->UpdateSwapChain(m_hSwapChain, WGameApplication::cvar_AppVSync ? WGALPresentMode::VSync : WGALPresentMode::Immediate).AssertSuccess("");
  }
  else
  {
    m_Size = desc.m_pWindow->GetClientAreaSize();
    m_hSwapChain = WGALWindowSwapChain::Create(m_CurrentDesc);
  }
}

void WWindowOutputTargetGAL::PresentImage(bool bEnableVSync)
{
  // For now, the actual present call is done during WGALDevice::EndFrame by calling WGALDevice::EnqueueFrameSwapChain before the render loop.
}

void WWindowOutputTargetGAL::AcquireImage()
{
  // For now, the actual acquire call is done during WGALDevice::BeginFrame by calling WGALDevice::EnqueueFrameSwapChain before the render loop.
  // This call is only used to recreate the swapchain at a safe location.

  WEnum<WGALPresentMode> presentMode = WGameApplication::cvar_AppVSync ? WGALPresentMode::VSync : WGALPresentMode::Immediate;

  if (m_Size != m_CurrentDesc.m_pWindow->GetClientAreaSize() || presentMode != m_CurrentDesc.m_InitialPresentMode)
  {
    CreateSwapchain(m_CurrentDesc);
  }
}

WResult WWindowOutputTargetGAL::StartCaptureImage()
{
  if (m_bCaptureInFlight || m_bCaptureRequested)
    return W_FAILURE;

  m_bCaptureRequested = true;
  return W_SUCCESS;
}

void WWindowOutputTargetGAL::OnRenderEvent(const WGALDeviceEvent& e)
{
  if (e.m_Type != WGALDeviceEvent::AfterBeginFrame)
    return;

  if (!m_bCaptureRequested)
    return;

  m_bCaptureRequested = false;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  const WGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
  WGALTextureHandle hBackbuffer = pSwapChain ? pSwapChain->GetRenderTargets().m_hRTs[0] : WGALTextureHandle();
  if (hBackbuffer.IsInvalidated())
    return;

  const WGALTexture* pBackbuffer = pDevice->GetTexture(hBackbuffer);
  m_CaptureBackbufferDesc = pBackbuffer->GetDescription();

  m_pRenderGraph->Reset();

  WRenderGraphTextureHandle hTex = m_pRenderGraph->ImportTexture(hBackbuffer);

  {
    auto pass = m_pRenderGraph->AddTransferPass("CaptureImage");
    pass.ReadTexture(hTex, {}, WGALResourceState::CopySource);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hTex](const WRenderGraphContext& ctx)
      { m_Readback.ReadbackTexture(*ctx.GetCommandEncoder(), ctx.ResolveTexture(hTex)); });
  }
  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  m_bCaptureInFlight = true;
}

WEnum<WCaptureImageResult> WWindowOutputTargetGAL::WaitCaptureImage(WImage& out_image)
{
  if (!m_bCaptureInFlight)
    return m_bCaptureRequested ? WCaptureImageResult::Pending : WCaptureImageResult::NotStarted;

  WGALDevice::GetDefaultDevice()->Flush();
  WEnum<WGALAsyncResult> res = m_Readback.GetReadbackResult(WTime::MakeFromHours(1));
  if (res == WGALAsyncResult::Pending)
    return WCaptureImageResult::Pending;

  if (res == WGALAsyncResult::Expired)
  {
    m_bCaptureInFlight = false;
    return WCaptureImageResult::NotStarted;
  }

  // Ready
  WGALTextureSubresource sourceSubResource;
  WArrayPtr<WGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  WTempHybridArray<WGALSystemMemoryDescription, 1> memory;
  WReadbackTextureLock lock = m_Readback.LockTexture(sourceSubResources, memory);
  if (!lock)
  {
    m_bCaptureInFlight = false;
    return WCaptureImageResult::NotStarted;
  }

  WTextureUtils::CopySubResourceToImage(m_CaptureBackbufferDesc, sourceSubResource, memory[0], out_image, true);

  m_bCaptureInFlight = false;
  return WCaptureImageResult::Ready;
}

void WWindowOutputTargetGAL::SwapChainUpdatedEventHandler(const WGALSwapChain* pSwapChain)
{
  if (m_hSwapChain.IsInvalidated())
    return;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  auto* pMySwapChain = pDevice->GetSwapChain<WGALWindowSwapChain>(m_hSwapChain);

  if (pSwapChain != pMySwapChain || !m_OnSwapChainChanged.IsValid())
    return;

  WSizeU32 currentSize = pSwapChain->GetCurrentSize();
  if (m_Size == currentSize)
    return;

  m_Size = currentSize;
  m_OnSwapChainChanged(m_hSwapChain, currentSize);
}
