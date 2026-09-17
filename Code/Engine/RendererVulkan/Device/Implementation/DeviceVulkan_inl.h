W_ALWAYS_INLINE vk::Device WGALDeviceVulkan::GetVulkanDevice() const
{
  return m_Device;
}

W_ALWAYS_INLINE const WGALDeviceVulkan::Queue& WGALDeviceVulkan::GetGraphicsQueue() const
{
  return m_GraphicsQueue;
}

W_ALWAYS_INLINE const WGALDeviceVulkan::Queue& WGALDeviceVulkan::GetTransferQueue() const
{
  return m_TransferQueue;
}

W_ALWAYS_INLINE vk::PhysicalDevice WGALDeviceVulkan::GetVulkanPhysicalDevice() const
{
  return m_PhysicalDevice;
}

W_ALWAYS_INLINE vk::Instance WGALDeviceVulkan::GetVulkanInstance() const
{
  return m_Instance;
}

W_ALWAYS_INLINE const WGALFormatLookupTableVulkan& WGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
