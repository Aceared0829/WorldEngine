
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Size.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class W_RENDERERFOUNDATION_DLL WGALSwapChain : public WGALObject<WGALSwapChainCreationDescription>
{
public:
  const WGALRenderTargets& GetRenderTargets() const { return m_RenderTargets; }
  WGALTextureHandle GetBackBufferTexture() const { return m_RenderTargets.m_hRTs[0]; }
  WSizeU32 GetCurrentSize() const { return m_CurrentSize; }

  virtual void AcquireNextRenderTarget(WGALDevice* pDevice) = 0;
  virtual void PresentRenderTarget(WGALDevice* pDevice) = 0;
  virtual WResult UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode) = 0;

  virtual ~WGALSwapChain();

protected:
  friend class WGALDevice;

  WGALSwapChain(const WRTTI* pSwapChainType);

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;

  WGALRenderTargets m_RenderTargets;
  WSizeU32 m_CurrentSize = {};
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERFOUNDATION_DLL, WGALSwapChain);


class W_RENDERERFOUNDATION_DLL WGALWindowSwapChain : public WGALSwapChain
{
public:
  using Functor = WDelegate<WGALSwapChainHandle(const WGALWindowSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  static WGALSwapChainHandle Create(const WGALWindowSwapChainCreationDescription& desc);

public:
  const WGALWindowSwapChainCreationDescription& GetWindowDescription() const { return m_WindowDesc; }

protected:
  WGALWindowSwapChain(const WGALWindowSwapChainCreationDescription& Description);

protected:
  static Functor s_Factory;

protected:
  WGALWindowSwapChainCreationDescription m_WindowDesc;
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERFOUNDATION_DLL, WGALWindowSwapChain);

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>
