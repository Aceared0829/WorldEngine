#include <GameEngine/GameEnginePCH.h>

#include "../../../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

//////////////////////////////////////////////////////////////////////////

WWindowXR::WWindowXR(WXRInterface* pVrInterface, WUniquePtr<WWindowBase> pCompanionWindow)
  : m_pVrInterface(pVrInterface)
  , m_pCompanionWindow(std::move(pCompanionWindow))
{
}

WWindowXR::~WWindowXR()
{
  W_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call WGALDevice::WaitIdle before destroying a window.");
}

WSizeU32 WWindowXR::GetClientAreaSize() const
{
  return m_pVrInterface->GetHmdInfo().m_vEyeRenderTargetSize;
}

WWindowHandle WWindowXR::GetNativeWindowHandle() const
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->GetNativeWindowHandle();
  }
  return WWindowHandle();
}

bool WWindowXR::IsFullscreenWindow(bool bOnlyProperFullscreenMode) const
{
  return true;
}

void WWindowXR::ProcessWindowMessages()
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->ProcessWindowMessages();
  }
}

const WWindowBase* WWindowXR::GetCompanionWindow() const
{
  return m_pCompanionWindow.Borrow();
}

//////////////////////////////////////////////////////////////////////////

WWindowOutputTargetXR::WWindowOutputTargetXR(WXRInterface* pXrInterface, WUniquePtr<WWindowOutputTargetGAL> pCompanionWindowOutputTarget)
  : m_pXrInterface(pXrInterface)
  , m_pCompanionWindowOutputTarget(std::move(pCompanionWindowOutputTarget))
{
  if (m_pCompanionWindowOutputTarget)
  {
    // Create companion resources.
    m_hCompanionShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/VRCompanionView.WShader");
    W_ASSERT_DEV(m_hCompanionShader.IsValid(), "Could not load VR companion view shader!");
    m_hCompanionConstantBuffer = WRenderContext::CreateConstantBufferStorage<WVRCompanionViewConstants>();
    m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("XR CompanionView", WRenderGraphPhase::PostRender);
    WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WWindowOutputTargetXR::OnGALEvent, this));
  }
}

WWindowOutputTargetXR::~WWindowOutputTargetXR()
{
  if (m_pCompanionWindowOutputTarget)
  {
    WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WWindowOutputTargetXR::OnGALEvent, this));
  }
  // Delete companion resources.
  WRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
}

void WWindowOutputTargetXR::PresentImage(bool bEnableVSync)
{
  // Swapchain present is handled by the rendering of the view automatically and RenderCompanionView is called by the WXRInterface now.
}

void WWindowOutputTargetXR::CompanionViewBeginFrame(bool bThrottleCompanionView)
{
  WTime currentTime = WTime::Now();
  if (bThrottleCompanionView && currentTime < (m_LastPresent + WTime::MakeFromMilliseconds(16)))
    return;

  m_LastPresent = currentTime;
  WGALDevice::GetDefaultDevice()->EnqueueFrameSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
  m_bRender = true;
}

void WWindowOutputTargetXR::RenderCompanionView()
{
  if (!m_bRender)
    return;

  m_bRender = false;

  W_PROFILE_SCOPE("RenderCompanionView");
  WGALTextureHandle hColorRT = m_pXrInterface->GetCurrentTexture();
  if (hColorRT.IsInvalidated() || !m_pCompanionWindowOutputTarget)
    return;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  const WGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
  WGALTextureHandle hCompanionRenderTarget = pSwapChain->GetBackBufferTexture();
  const WGALTexture* tex = pDevice->GetTexture(hCompanionRenderTarget);
  WVec2 targetSize = WVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

  m_pRenderGraph->Reset();

  WRenderGraphTextureHandle hTarget = m_pRenderGraph->ImportTexture(hCompanionRenderTarget);
  WRenderGraphTextureHandle hVRSource = m_pRenderGraph->ImportTexture(hColorRT);

  {
    auto pass = m_pRenderGraph->AddGraphicsPass("Blit CompanionView");
    pass.AddColorTarget(hTarget);
    pass.ReadTexture(hVRSource);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hVRSource, targetSize](const WRenderGraphContext& ctx)
      {
      auto* pRenderContext = ctx.GetRenderContext();

      pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

      pRenderContext->BindShader(m_hCompanionShader);

      auto* constants = WRenderContext::GetConstantBufferData<WVRCompanionViewConstants>(m_hCompanionConstantBuffer);
      constants->TargetSize = targetSize;

      WBindGroupBuilder& bindGroup = WRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroup.BindBuffer("WVRCompanionViewConstants", m_hCompanionConstantBuffer);
      bindGroup.BindTexture("VRTexture", ctx.ResolveTexture(hVRSource));

      pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  // If a capture was requested, add a readback pass to the same graph.
  if (m_bCaptureRequested)
  {
    m_bCaptureRequested = false;

    const WGALTexture* pBackbuffer = pDevice->GetTexture(hCompanionRenderTarget);
    m_CaptureBackbufferDesc = pBackbuffer->GetDescription();

    auto capturePass = m_pRenderGraph->AddTransferPass("CaptureImage");
    capturePass.ReadTexture(hTarget, {}, WGALResourceState::CopySource);
    capturePass.HasSideEffects();
    capturePass.SetExecuteCallback([this, hTarget](const WRenderGraphContext& ctx)
      { m_Readback.ReadbackTexture(*ctx.GetCommandEncoder(), ctx.ResolveTexture(hTarget)); });

    m_bCaptureInFlight = true;
  }

  WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
}

void WWindowOutputTargetXR::OnGALEvent(const WGALDeviceEvent& e)
{
  if (e.m_Type != WGALDeviceEvent::AfterBeginFrame)
    return;

  RenderCompanionView();
}

WResult WWindowOutputTargetXR::StartCaptureImage()
{
  if (!m_pCompanionWindowOutputTarget)
    return W_FAILURE;

  if (m_bCaptureInFlight || m_bCaptureRequested)
    return W_FAILURE;

  m_bCaptureRequested = true;
  return W_SUCCESS;
}

WEnum<WCaptureImageResult> WWindowOutputTargetXR::WaitCaptureImage(WImage& out_image)
{
  if (!m_bCaptureInFlight)
    return WCaptureImageResult::NotStarted;

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

const WWindowOutputTargetBase* WWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}
