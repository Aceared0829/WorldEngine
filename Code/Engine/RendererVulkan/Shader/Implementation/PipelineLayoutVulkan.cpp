#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/PipelineLayoutVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WGALPipelineLayoutVulkan::WGALPipelineLayoutVulkan(const WGALPipelineLayoutCreationDescription& Description)
  : WGALPipelineLayout(Description)
{
}

WGALPipelineLayoutVulkan::~WGALPipelineLayoutVulkan() = default;

WResult WGALPipelineLayoutVulkan::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  // There must always be at least one empty set.
  WUInt32 uiMaxSets = 1;
  for (WUInt32 i = 1; i < W_GAL_MAX_BIND_GROUPS; ++i)
  {
    if (!m_Description.m_BindGroups[i].IsInvalidated())
    {
      uiMaxSets = i + 1;
    }
  }

  WHybridArray<vk::DescriptorSetLayout, W_GAL_MAX_BIND_GROUPS> descriptorSetLayouts;
  descriptorSetLayouts.SetCount(uiMaxSets);
  for (WUInt32 i = 0; i < uiMaxSets; ++i)
  {
    const WGALBindGroupLayout* pBindGroupLayout = pVulkanDevice->GetBindGroupLayout(m_Description.m_BindGroups[i]);
    if (pBindGroupLayout == nullptr)
    {
      // Vulkan needs a valid (possibly empty) layout for every set in the range, gaps cannot be expressed.
      WLog::Error("Failed to create Vulkan pipeline layout: no bind group layout for set {} of {}.", i, uiMaxSets);
      return W_FAILURE;
    }
    descriptorSetLayouts[i] = static_cast<const WGALBindGroupLayoutVulkan*>(pBindGroupLayout)->GetDescriptorSetLayout();
  }

  m_PushConstants.size = m_Description.m_PushConstants.m_uiSize;
  m_PushConstants.offset = m_Description.m_PushConstants.m_uiOffset;
  m_PushConstants.stageFlags = WConversionUtilsVulkan::GetShaderStages(m_Description.m_PushConstants.m_Stages);

  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.setLayoutCount = descriptorSetLayouts.GetCount();
  layoutInfo.pSetLayouts = descriptorSetLayouts.GetData();
  if (m_PushConstants.size != 0)
  {
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &m_PushConstants;
  }

  VK_SUCCEED_OR_RETURN_W_FAILURE(pVulkanDevice->GetVulkanDevice().createPipelineLayout(&layoutInfo, nullptr, &m_PipelineLayout));

  return W_SUCCESS;
}

WResult WGALPipelineLayoutVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  auto* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_PipelineLayout);
  return W_SUCCESS;
}
