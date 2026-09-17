#pragma once

#include <RendererVulkan/Resources/TextureVulkan.h>

class WGALSharedTextureVulkan : public WGALTextureVulkan, public WGALSharedTexture
{
  using SUPER = WGALTextureVulkan;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALSharedTextureVulkan(const WGALTextureCreationDescription& Description, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle hSharedHandle);
  ~WGALSharedTextureVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  virtual WGALPlatformSharedHandle GetSharedHandle() const override;
  virtual void WaitSemaphoreGPU(WUInt64 uiValue) const override;
  virtual void SignalSemaphoreGPU(WUInt64 uiValue) const override;

protected:
  WEnum<WGALSharedTextureType> m_SharedType = WGALSharedTextureType::None;
  WGALPlatformSharedHandle m_hSharedHandle;
  vk::Semaphore m_SharedSemaphore;
};
