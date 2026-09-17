
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Device/SwapChain.h>

class WGALDeviceVulkan;
struct WGALWindowSwapChainCreationDescription;

class WGALSwapChainVulkan : public WGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(WGALDevice* pDevice) override;
  virtual void PresentRenderTarget(WGALDevice* pDevice) override;
  virtual WResult UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode) override;

  W_ALWAYS_INLINE vk::SwapchainKHR GetVulkanSwapChain() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALSwapChainVulkan(const WGALWindowSwapChainCreationDescription& Description);

  virtual ~WGALSwapChainVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  WResult CreateSwapChainInternal();
  void DestroySwapChainInternal(WGALDeviceVulkan* pVulkanDevice);
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

protected:
  WGALDeviceVulkan* m_pVulkanDevice = nullptr;
  WEnum<WGALPresentMode> m_CurrentPresentMode;

  vk::SurfaceKHR m_VulkanSurface;
  vk::SwapchainKHR m_VulkanSwapChain;
  WHybridArray<vk::Image, 4> m_SwapChainImages;
  WHybridArray<WGALTextureHandle, 4> m_SwapChainTextures;
  WHybridBitfield<32> m_DefaultLayoutApplied;
  WUInt32 m_uiCurrentSwapChainImage = 0;

  vk::Semaphore m_CurrentPipelineImageAvailableSemaphore;
  WHybridArray<vk::Semaphore, 4> m_ImageRenderFinishedSemaphores;
};

#include <RendererVulkan/Device/Implementation/SwapChainVulkan_inl.h>
