
vk::Buffer WGALBufferVulkan::GetVkBuffer() const
{
  return m_Buffer;
}

vk::IndexType WGALBufferVulkan::GetIndexType() const
{
  return m_IndexType;
}

WVulkanAllocation WGALBufferVulkan::GetAllocation() const
{
  return m_pAlloc;
}

const WVulkanAllocationInfo& WGALBufferVulkan::GetAllocationInfo() const
{
  return m_AllocInfo;
}

vk::PipelineStageFlags WGALBufferVulkan::GetUsedByPipelineStage() const
{
  return m_Stages;
}

vk::AccessFlags WGALBufferVulkan::GetAccessMask() const
{
  return m_Access;
}
