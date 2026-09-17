#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Resources/ReadbackTexture.h>

class WGALBufferVulkan;
class WGALDeviceVulkan;

class WGALReadbackTextureVulkan : public WGALReadbackTexture
{
public:
  W_ALWAYS_INLINE vk::Buffer GetVkBuffer() const { return m_Buffer; }
  W_ALWAYS_INLINE WVulkanAllocation GetBufferAllocation() const { return m_pBufferAlloc; }

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALReadbackTextureVulkan(const WGALTextureCreationDescription& Description);
  ~WGALReadbackTextureVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Buffer m_Buffer = {};
  WVulkanAllocation m_pBufferAlloc;
  WVulkanAllocationInfo m_BufferAllocInfo;

  WGALDeviceVulkan* m_pDevice = nullptr;
};
