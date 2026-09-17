#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Enumerations.h>

VK_DEFINE_HANDLE(WVulkanAllocation)

class WGALBufferVulkan;
class WGALTextureVulkan;

struct WStagingBufferVulkan
{
  vk::Buffer m_buffer;
  vk::DeviceSize m_uiOffset = 0;
  WVulkanAllocation m_alloc;
  WByteArrayPtr m_Data;
};

struct WPendingBufferCopyVulkan
{
  WStagingBufferVulkan m_SrcBuffer = {};
  const WGALBufferVulkan* m_pDstBuffer = nullptr;
  vk::BufferCopy m_Region;
};

struct WPendingTextureCopyVulkan
{
  WStagingBufferVulkan m_SrcBuffer = {};
  const WGALTextureVulkan* m_pDstTexture = nullptr;
  vk::BufferImageCopy m_Region;
  vk::DeviceSize m_uiTotalSize;
};

/// Used as a key to descriptor allocators. Defines the resource usage of each type inside a bind group layout.
struct WBindGroupLayoutResourceUsageVulkan
{
  WUInt8 m_Usage[WGALShaderResourceType::COUNT] = {0};
};