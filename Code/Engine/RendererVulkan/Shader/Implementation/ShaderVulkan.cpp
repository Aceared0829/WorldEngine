#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/PipelineLayoutVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

WGALShaderVulkan::WGALShaderVulkan(const WGALShaderCreationDescription& Description)
  : WGALShader(Description)
{
}

WGALShaderVulkan::~WGALShaderVulkan() = default;

void WGALShaderVulkan::SetDebugName(WStringView sName) const
{
  WStringBuilder tmp;
  for (WUInt32 i = 0; i < WGALShaderStage::ENUM_COUNT; i++)
  {
    static_cast<WGALDeviceVulkan*>(m_pDevice)->SetDebugName(sName.GetData(tmp), m_Shaders[i]);
  }
}

WResult WGALShaderVulkan::InitPlatform(WGALDevice* pDevice)
{
  m_pDevice = pDevice;
  W_SUCCEED_OR_RETURN(CreateBindingMapping(false));
  W_SUCCEED_OR_RETURN(CreateLayouts(pDevice, true));

  auto pDeviceVulkan = static_cast<WGALDeviceVulkan*>(pDevice);

  // Build shaders
  vk::ShaderModuleCreateInfo createInfo;
  for (WUInt32 i = 0; i < WGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((WGALShaderStage::Enum)i))
    {
      createInfo.codeSize = m_Description.m_ByteCodes[i]->m_ByteCode.GetCount();
      W_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
      createInfo.pCode = reinterpret_cast<const WUInt32*>(m_Description.m_ByteCodes[i]->m_ByteCode.GetData());
      VK_SUCCEED_OR_RETURN_W_FAILURE(pDeviceVulkan->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
    }
  }

  return W_SUCCESS;
}

WResult WGALShaderVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  DestroyBindingMapping();
  DestroyLayouts(pDevice);

  auto* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
  for (auto& m_Shader : m_Shaders)
  {
    pVulkanDevice->DeleteLater(m_Shader);
  }
  return W_SUCCESS;
}

vk::PipelineLayout WGALShaderVulkan::GetVkPipelineLayout() const
{
  return static_cast<const WGALPipelineLayoutVulkan*>(m_pDevice->GetPipelineLayout(m_hPipelineLayout))->GetVkPipelineLayout();
}

vk::DescriptorSetLayout WGALShaderVulkan::GetDescriptorSetLayout(WUInt32 uiSet) const
{
  W_ASSERT_DEBUG(uiSet < GetBindGroupCount(), "Bind group index out of range.");
  return static_cast<const WGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(m_BindGroupLayouts[uiSet]))->GetDescriptorSetLayout();
}

vk::PushConstantRange WGALShaderVulkan::GetPushConstantRange() const
{
  return static_cast<const WGALPipelineLayoutVulkan*>(m_pDevice->GetPipelineLayout(m_hPipelineLayout))->GetPushConstantRange();
}
