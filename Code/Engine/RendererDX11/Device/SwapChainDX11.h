
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

struct IDXGISwapChain;

class WGALSwapChainDX11 : public WGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(WGALDevice* pDevice) override;
  virtual void PresentRenderTarget(WGALDevice* pDevice) override;
  virtual WResult UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode) override;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALSwapChainDX11(const WGALWindowSwapChainCreationDescription& Description);

  virtual ~WGALSwapChainDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  WResult CreateBackBufferInternal(WGALDeviceDX11* pDXDevice);
  void DestroyBackBufferInternal(WGALDeviceDX11* pDXDevice);
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  WGALDevice* m_pDevice = nullptr;
  IDXGISwapChain* m_pDXSwapChain = nullptr;

  WGALTextureHandle m_hBackBufferTexture;

  WEnum<WGALPresentMode> m_CurrentPresentMode;
  bool m_bCanMakeDirectScreenshots = true;
  // We can't do screenshots if we're using any of the FLIP swap effects.
  // If the user requests screenshots anyways, we need to put another buffer in between.
  // For ease of use, this is m_hBackBufferTexture and the actual "OS backbuffer" is this texture.
  // In any other case this handle is unused.
  WGALTextureHandle m_hActualBackBufferTexture;
};

#include <RendererDX11/Device/Implementation/SwapChainDX11_inl.h>
