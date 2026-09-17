#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <Foundation/Platform/Win/Utils/HResultUtils.h>
#include <d3d11.h>

void WGALSwapChainDX11::AcquireNextRenderTarget(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);
}

void WGALSwapChainDX11::PresentRenderTarget(WGALDevice* pDevice)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);

  // If there is a "actual backbuffer" (see it's documentation for detailed explanation), copy to it.
  if (!this->m_hActualBackBufferTexture.IsInvalidated())
  {
    pDXDevice->GetCommandEncoder()->CopyTexture(this->m_hActualBackBufferTexture, this->m_hBackBufferTexture);
  }

  HRESULT result = m_pDXSwapChain->Present(m_CurrentPresentMode == WGALPresentMode::VSync ? 1 : 0, 0);
  if (FAILED(result))
  {
    WLog::Error("Swap chain PresentImage failed with {0}", (WUInt32)result);
    return;
  }
}

WResult WGALSwapChainDX11::UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);
  m_CurrentPresentMode = newPresentMode;
  DestroyBackBufferInternal(pDXDevice);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDXDevice->FlushDeadObjects();

  HRESULT result = m_pDXSwapChain->ResizeBuffers(m_WindowDesc.m_bDoubleBuffered ? 2 : 1,
    m_WindowDesc.m_pWindow->GetClientAreaSize().width,
    m_WindowDesc.m_pWindow->GetClientAreaSize().height,
    pDXDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget,
    DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

  if (FAILED(result))
  {
    WLog::Error("UpdateSwapChain: ResizeBuffers call failed: {}", WArgErrorCode(result));
    return W_FAILURE;
  }

  return CreateBackBufferInternal(pDXDevice);
}

WGALSwapChainDX11::WGALSwapChainDX11(const WGALWindowSwapChainCreationDescription& Description)
  : WGALWindowSwapChain(Description)

{
}

WGALSwapChainDX11::~WGALSwapChainDX11() = default;


WResult WGALSwapChainDX11::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);
  m_pDevice = pDevice;
  m_CurrentPresentMode = m_WindowDesc.m_InitialPresentMode;

  DXGI_SWAP_CHAIN_DESC SwapChainDesc;
  SwapChainDesc.BufferCount = m_WindowDesc.m_bDoubleBuffered ? 2 : 1;
  SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; /// \todo The mode switch needs to be handled (ResizeBuffers + communication with engine)
  SwapChainDesc.SampleDesc.Count = m_WindowDesc.m_SampleCount;
  SwapChainDesc.SampleDesc.Quality = 0;                         /// \todo Get from MSAA value of the m_WindowDesc
  SwapChainDesc.OutputWindow = WMinWindows::ToNative(m_WindowDesc.m_pWindow->GetNativeWindowHandle());
  SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;          // The FLIP models are more efficient but only supported in Win8+. See
                                                                // https://msdn.microsoft.com/en-us/library/windows/desktop/bb173077(v=vs.85).aspx#DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
  SwapChainDesc.Windowed = m_WindowDesc.m_pWindow->IsFullscreenWindow(true) ? FALSE : TRUE;

  /// \todo Get from enumeration of available modes
  SwapChainDesc.BufferDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  SwapChainDesc.BufferDesc.Width = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  SwapChainDesc.BufferDesc.Height = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

  SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;

  if (FAILED(pDXDevice->GetDXGIFactory()->CreateSwapChain(pDXDevice->GetDXDevice(), &SwapChainDesc, &m_pDXSwapChain)))
  {
    return W_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_WindowDesc.m_pWindow->AddReference();

  m_bCanMakeDirectScreenshots = (SwapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL);
  return CreateBackBufferInternal(pDXDevice);
}

WResult WGALSwapChainDX11::CreateBackBufferInternal(WGALDeviceDX11* pDXDevice)
{
  // Get texture of the swap chain
  ID3D11Texture2D* pNativeBackBufferTexture = nullptr;
  HRESULT result = m_pDXSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pNativeBackBufferTexture));
  if (FAILED(result))
  {
    WLog::Error("Couldn't access backbuffer texture of swapchain: {0}", WHRESULTtoString(result));
    W_GAL_DX11_RELEASE(m_pDXSwapChain);

    return W_FAILURE;
  }

  WGALTextureCreationDescription TexDesc;
  TexDesc.m_uiWidth = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  TexDesc.m_uiHeight = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  TexDesc.m_SampleCount = m_WindowDesc.m_SampleCount;
  TexDesc.m_pExisitingNativeObject = pNativeBackBufferTexture;
  TexDesc.m_TextureFlags = WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::Presentable;
  TexDesc.m_Format = m_WindowDesc.m_BackBufferFormat;

  TexDesc.m_ResourceAccess.m_bImmutable = false;

  // And create the W texture object wrapping the backbuffer texture
  m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
  W_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  // Create extra texture to be used as "practical backbuffer" if we can't do the screenshots the user wants.
  if (!m_bCanMakeDirectScreenshots)
  {
    TexDesc.m_pExisitingNativeObject = nullptr;

    m_hActualBackBufferTexture = m_hBackBufferTexture;
    m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    W_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create non-native backbuffer texture object!");
  }

  m_RenderTargets.m_hRTs[0] = m_hBackBufferTexture;
  m_CurrentSize = WSizeU32(TexDesc.m_uiWidth, TexDesc.m_uiHeight);
  m_pDevice->s_SwapChainUpdatedEvent.Broadcast(this);
  return W_SUCCESS;
}

void WGALSwapChainDX11::DestroyBackBufferInternal(WGALDeviceDX11* pDXDevice)
{
  pDXDevice->DestroyTexture(m_hBackBufferTexture);
  m_hBackBufferTexture.Invalidate();

  if (!m_hActualBackBufferTexture.IsInvalidated())
  {
    pDXDevice->DestroyTexture(m_hActualBackBufferTexture);
    m_hActualBackBufferTexture.Invalidate();
  }

  m_RenderTargets.m_hRTs[0].Invalidate();
}

WResult WGALSwapChainDX11::DeInitPlatform(WGALDevice* pDevice)
{
  DestroyBackBufferInternal(static_cast<WGALDeviceDX11*>(pDevice));

  if (m_pDXSwapChain)
  {
    // Full screen swap chains must be switched to windowed mode before destruction.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    m_pDXSwapChain->SetFullscreenState(FALSE, NULL);

    W_GAL_DX11_RELEASE(m_pDXSwapChain);

    m_WindowDesc.m_pWindow->RemoveReference();
  }
  return W_SUCCESS;
}
