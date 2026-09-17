#include <RendererVulkan/Shader/BindGroupVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorWritePoolVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>

WGALBindGroupVulkan::WGALBindGroupVulkan(const WGALBindGroupCreationDescription& Description)
  : WGALBindGroup(Description)
{
}

WGALBindGroupVulkan::~WGALBindGroupVulkan() = default;

WResult WGALBindGroupVulkan::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pDeviceVulkan = static_cast<WGALDeviceVulkan*>(pDevice);
  const WGALBindGroupLayoutVulkan* pLayout = static_cast<const WGALBindGroupLayoutVulkan*>(pDeviceVulkan->GetBindGroupLayout(m_Description.m_hBindGroupLayout));
  if (pLayout == nullptr)
  {
    WLog::Error("Invalid bind group layout handle passed into bind group");
    return W_FAILURE;
  }

  m_DescriptorSet = pLayout->GetDescriptorSetPool()->CreateDescriptorSet(m_Description.m_hBindGroupLayout, m_Allocation);
  pDeviceVulkan->GetDescriptorWritePool().WriteDescriptor(m_DescriptorSet, m_Description, m_Offsets);
  return W_SUCCESS;
}

WResult WGALBindGroupVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  Invalidate(pDevice);
  return W_SUCCESS;
}

void WGALBindGroupVulkan::Invalidate(WGALDevice* pDevice)
{
  if (m_DescriptorSet != nullptr)
  {
    WGALDeviceVulkan* pDeviceVulkan = static_cast<WGALDeviceVulkan*>(pDevice);
    const WGALBindGroupLayoutVulkan* pLayout = static_cast<const WGALBindGroupLayoutVulkan*>(pDeviceVulkan->GetBindGroupLayout(m_Description.m_hBindGroupLayout));
    pDeviceVulkan->ReclaimLater(m_DescriptorSet, pLayout->GetDescriptorSetPool(), m_Allocation.m_uiPoolIndex);
    m_DescriptorSet = nullptr;
    m_Allocation = {};
  }
}

bool WGALBindGroupVulkan::IsInvalidated() const
{
  return m_DescriptorSet == nullptr;
}

void WGALBindGroupVulkan::SetDebugNamePlatform(const char* szName) const
{
}
