#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackBufferVulkan.h>

WGALReadbackBufferVulkan::WGALReadbackBufferVulkan(const WGALBufferCreationDescription& Description)
  : WGALReadbackBuffer(Description)
{
}

WGALReadbackBufferVulkan::~WGALReadbackBufferVulkan() = default;

WResult WGALReadbackBufferVulkan::InitPlatform(WGALDevice* pDevice)
{
  m_pDeviceVulkan = static_cast<WGALDeviceVulkan*>(pDevice);

  vk::DeviceSize alignment = WGALBufferVulkan::GetAlignment(m_pDeviceVulkan, vk::BufferUsageFlagBits::eTransferDst);
  m_Size = WMemoryUtils::AlignSize((vk::DeviceSize)m_Description.m_uiTotalSize, alignment);

  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst;
  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  bufferCreateInfo.size = m_Size;

  WVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = WVulkanMemoryUsage::Auto;
  allocCreateInfo.m_flags = WVulkanAllocationCreateFlags::HostAccessRandom | WVulkanAllocationCreateFlags::Mapped;

  VK_ASSERT_DEV(WMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_Buffer, m_pAlloc, &m_AllocInfo));

  return W_SUCCESS;
}

WResult WGALReadbackBufferVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  if (m_Buffer)
  {
    m_pDeviceVulkan->DeleteLater(m_Buffer, m_pAlloc);
    m_AllocInfo = {};
  }
  m_Size = 0;
  m_pDeviceVulkan = nullptr;
  return W_SUCCESS;
}

void WGALReadbackBufferVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDeviceVulkan->SetDebugName(szName, m_Buffer, m_pAlloc);
}
