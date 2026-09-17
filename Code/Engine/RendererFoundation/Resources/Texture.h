
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class W_RENDERERFOUNDATION_DLL WGALTexture : public WGALResource<WGALTextureCreationDescription>
{
public:
  WVec3U32 GetMipMapSize(WUInt32 uiMipLevel) const;
  /// Replaced W_GAL_ALL_MIP_LEVELS and W_GAL_ALL_ARRAY_SLICES with the correct upper bounds for this texture.
  WGALTextureRange ClampRange(WGALTextureRange range) const;

protected:
  friend class WGALDevice;

  WGALTexture(const WGALTextureCreationDescription& Description);
  virtual ~WGALTexture();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;

protected:
  WGALRenderTargetViewHandle m_hDefaultRenderTargetView;
  WHashTable<WUInt32, WGALRenderTargetViewHandle> m_RenderTargetViews;
};

/// Optional interface for WGALTexture if it was created via WGALDevice::CreateSharedTexture.
/// A WGALTexture can be a shared texture, but doesn't have to be. Access through WGALDevice::GetSharedTexture.
class W_RENDERERFOUNDATION_DLL WGALSharedTexture
{
public:
  /// Returns the handle that can be used to open this texture on another device / process. Call  WGALDevice::OpenSharedTexture to do so.
  virtual WGALPlatformSharedHandle GetSharedHandle() const = 0;
  /// Before the current render pipeline is executed, the GPU will wait for the semaphore to have the given value.
  /// \param iValue Value the semaphore needs to have before the texture can be used.
  virtual void WaitSemaphoreGPU(WUInt64 uiValue) const = 0;
  /// Once the current render pipeline is done on the GPU, the semaphore will be signaled with the given value.
  /// \param iValue Value the semaphore is set to once we are done using the texture (after the current render pipeline).
  virtual void SignalSemaphoreGPU(WUInt64 uiValue) const = 0;
};
