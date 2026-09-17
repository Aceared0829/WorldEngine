#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Resources/ReadbackBuffer.h>

class WGALDeviceVulkan;

class WGALReadbackBufferVulkan : public WGALReadbackBuffer
{
public:
  W_ALWAYS_INLINE vk::Buffer GetVkBuffer() const { return m_Buffer; }
  W_ALWAYS_INLINE WVulkanAllocation GetAllocation() const { return m_pAlloc; }
  W_ALWAYS_INLINE const WVulkanAllocationInfo& GetAllocationInfo() const { return m_AllocInfo; }

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALReadbackBufferVulkan(const WGALBufferCreationDescription& Description);
  virtual ~WGALReadbackBufferVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Buffer m_Buffer = {};
  vk::DeviceSize m_Size = 0;
  WVulkanAllocation m_pAlloc = {};
  WVulkanAllocationInfo m_AllocInfo = {};

  WGALDeviceVulkan* m_pDeviceVulkan = nullptr;
};
