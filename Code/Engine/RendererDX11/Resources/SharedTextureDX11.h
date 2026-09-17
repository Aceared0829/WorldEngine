#pragma once

#include <RendererDX11/Resources/TextureDX11.h>

struct IDXGIKeyedMutex;

class WGALSharedTextureDX11 : public WGALTextureDX11, public WGALSharedTexture
{
  using SUPER = WGALTextureDX11;

protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  WGALSharedTextureDX11(const WGALTextureCreationDescription& Description, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle hSharedHandle);
  ~WGALSharedTextureDX11();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  virtual WGALPlatformSharedHandle GetSharedHandle() const override;
  virtual void WaitSemaphoreGPU(WUInt64 uiValue) const override;
  virtual void SignalSemaphoreGPU(WUInt64 uiValue) const override;

protected:
  WEnum<WGALSharedTextureType> m_SharedType = WGALSharedTextureType::None;
  WGALPlatformSharedHandle m_hSharedHandle;
  IDXGIKeyedMutex* m_pKeyedMutex = nullptr;
};
