#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ReadbackTextureVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>

WGALReadbackTextureVulkan::WGALReadbackTextureVulkan(const WGALTextureCreationDescription& Description)
  : WGALReadbackTexture(Description)
{
}

WGALReadbackTextureVulkan::~WGALReadbackTextureVulkan() = default;

WResult WGALReadbackTextureVulkan::InitPlatform(WGALDevice* pDevice)
{
  m_pDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;
  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

  WHybridArray<WGALTextureVulkan::SubResourceOffset, 8> subResourceSizes;
  bufferCreateInfo.size = WGALTextureVulkan::ComputeSubResourceOffsets(m_pDevice, m_Description, subResourceSizes);

  WVulkanAllocationCreateInfo allocCreateInfo;
  allocCreateInfo.m_usage = WVulkanMemoryUsage::Auto;
  allocCreateInfo.m_flags = WVulkanAllocationCreateFlags::HostAccessRandom /*| WVulkanAllocationCreateFlags::Mapped*/;

  VK_ASSERT_DEV(WMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_Buffer, m_pBufferAlloc, &m_BufferAllocInfo));
  return W_SUCCESS;
}

WResult WGALReadbackTextureVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  if (m_Buffer)
  {
    WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
    pVulkanDevice->DeleteLater(m_Buffer, m_pBufferAlloc);
    m_BufferAllocInfo = {};
    m_Buffer = nullptr;
  }

  return W_SUCCESS;
}

void WGALReadbackTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_Buffer, m_pBufferAlloc);
}
