#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>

/// Description of a shared texture swap chain. Use WGALSharedTextureSwapChain::Create to create instance.
struct WGALSharedTextureSwapChainCreationDescription : public WHashableStruct<WGALSharedTextureSwapChainCreationDescription>
{
  WGALTextureCreationDescription m_TextureDesc;
  WHybridArray<WGALPlatformSharedHandle, 3> m_Textures;
  /// Called when rendering to a swap chain texture has been submitted to the GPU queue. Use this to get the new semaphore value of the previously armed texture.
  WDelegate<void(WUInt32 uiTextureIndex, WUInt64 uiSemaphoreValue)> m_OnPresent;
};

/// Use to render to a set of shared textures.
/// To use it, it needs to be armed with the next shared texture index and its current semaphore value.
class W_RENDERERFOUNDATION_DLL WGALSharedTextureSwapChain : public WGALSwapChain
{
  friend class WGALDevice;

public:
  using Functor = WDelegate<WGALSwapChainHandle(const WGALSharedTextureSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  /// Creates an instance of a WGALSharedTextureSwapChain.
  static WGALSwapChainHandle Create(const WGALSharedTextureSwapChainCreationDescription& desc);

public:
  /// Call this before rendering.
  /// \param uiTextureIndex Texture to render into.
  /// \param uiCurrentSemaphoreValue Current semaphore value of the texture.
  void Arm(WUInt32 uiTextureIndex, WUInt64 uiCurrentSemaphoreValue);

protected:
  WGALSharedTextureSwapChain(const WGALSharedTextureSwapChainCreationDescription& desc);
  virtual void AcquireNextRenderTarget(WGALDevice* pDevice) override;
  virtual void PresentRenderTarget(WGALDevice* pDevice) override;
  virtual WResult UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode) override;
  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

protected:
  static Functor s_Factory;

protected:
  WUInt32 m_uiCurrentTexture = WMath::MaxValue<WUInt32>();
  WUInt64 m_uiCurrentSemaphoreValue = 0;
  WHybridArray<WGALTextureHandle, 3> m_SharedTextureHandles;
  WHybridArray<const WGALSharedTexture*, 3> m_SharedTextureInterfaces;
  WHybridArray<WUInt64, 3> m_CurrentSemaphoreValue;
  WGALSharedTextureSwapChainCreationDescription m_Desc = {};
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERFOUNDATION_DLL, WGALSharedTextureSwapChain);
