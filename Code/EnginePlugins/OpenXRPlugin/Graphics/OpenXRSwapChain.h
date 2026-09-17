#pragma once

#include <GameEngine/XR/XRSwapChain.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class WOpenXR;

class W_OPENXRPLUGIN_DLL WGALOpenXRSwapChain : public WGALXRSwapChain
{
public:
  WSizeU32 GetRenderTargetSize() const { return m_CurrentSize; }
  XrSwapchain GetColorSwapchain() const { return m_ColorSwapchain.handle; }
  XrSwapchain GetDepthSwapchain() const { return m_DepthSwapchain.handle; }

  virtual void AcquireNextRenderTarget(WGALDevice* pDevice) override;
  virtual void PresentRenderTarget(WGALDevice* pDevice) override;
  void PresentRenderTarget() const;

protected:
  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

private:
  friend class WOpenXR;
  struct Swapchain
  {
    XrSwapchain handle = 0;
    int64_t format = 0;
    WUInt32 imageCount = 0;
    uint32_t imageIndex = 0;
  };

private:
  WGALOpenXRSwapChain(WOpenXR* pXrInterface, WGALMSAASampleCount::Enum msaaCount);
  XrResult InitSwapChain(WGALMSAASampleCount::Enum msaaCount);
  void DeinitSwapChain();

private:
  XrInstance m_pInstance = XR_NULL_HANDLE;
  uint64_t m_SystemId = XR_NULL_SYSTEM_ID;
  XrSession m_pSession = XR_NULL_HANDLE;
  WEnum<WGALMSAASampleCount> m_MsaaCount;

  // Swapchain
  XrViewConfigurationView m_PrimaryConfigView;
  Swapchain m_ColorSwapchain;
  Swapchain m_DepthSwapchain;

  // Render targets - created by the graphics binding
  WHybridArray<WGALTextureHandle, 3> m_ColorRTs;
  WHybridArray<WGALTextureHandle, 3> m_DepthRTs;

  bool m_bImageAcquired = false;
  WGALTextureHandle m_hColorRT;
  WGALTextureHandle m_hDepthRT;
};
