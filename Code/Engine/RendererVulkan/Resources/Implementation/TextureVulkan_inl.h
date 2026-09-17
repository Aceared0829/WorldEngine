vk::Image WGALTextureVulkan::GetImage() const
{
  return m_Image;
}

WVulkanAllocation WGALTextureVulkan::GetAllocation() const
{
  return m_pAlloc;
}

const WVulkanAllocationInfo& WGALTextureVulkan::GetAllocationInfo() const
{
  return m_AllocInfo;
}
