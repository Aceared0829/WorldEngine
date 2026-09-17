#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

/// Simple pool for command buffers
///
/// Do not call ReclaimCommandBuffer manually, instead call WGALDeviceVulkan::ReclaimLater which will make sure to reclaim the command buffer once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::CommandBuffer c = pPool->RequestCommandBuffer();
///   c.begin();
///   ...
///   c.end();
///   WGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(c);
/// \endcode
class W_RENDERERVULKAN_DLL WCommandBufferPoolVulkan
{
public:
  WCommandBufferPoolVulkan(WAllocator* pAllocator);
  ~WCommandBufferPoolVulkan();

  void Initialize(vk::Device device, WUInt32 uiGraphicsFamilyIndex);
  void DeInitialize();

  vk::CommandBuffer RequestCommandBuffer();
  void ReclaimCommandBuffer(vk::CommandBuffer& ref_commandBuffer);

private:
  vk::Device m_Device;
  vk::CommandPool m_CommandPool;
  WHybridArray<vk::CommandBuffer, 4> m_CommandBuffers;
};
