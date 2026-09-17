

#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WGALBindGroupLayoutVulkan::WGALBindGroupLayoutVulkan(const WGALBindGroupLayoutCreationDescription& Description)
  : WGALBindGroupLayout(Description)
{
}

WGALBindGroupLayoutVulkan::~WGALBindGroupLayoutVulkan() = default;

WResult WGALBindGroupLayoutVulkan::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  WHybridArray<vk::DescriptorSetLayoutBinding, 6> bindings;
  bindings.Reserve(m_Description.m_ResourceBindings.GetCount() + m_Description.m_ImmutableSamplers.GetCount());

  auto ConvertBinding = [](const WShaderResourceBinding& WBinding, vk::DescriptorSetLayoutBinding& out_Binding)
  {
    out_Binding.binding = WBinding.m_iSlot;
    out_Binding.descriptorType = WConversionUtilsVulkan::GetDescriptorType(WBinding.m_ResourceType);
    out_Binding.descriptorCount = WBinding.m_uiArraySize;
    out_Binding.stageFlags = WConversionUtilsVulkan::GetShaderStages(WBinding.m_Stages);
  };

  // Build Vulkan descriptor set layout
  for (WUInt32 i = 0; i < m_Description.m_ResourceBindings.GetCount(); i++)
  {
    const WShaderResourceBinding& WBinding = m_Description.m_ResourceBindings[i];
    vk::DescriptorSetLayoutBinding& binding = bindings.ExpandAndGetRef();
    ConvertBinding(WBinding, binding);
  }

  // Build m_ResourceUsage
  // Immutable samplers are counted as well, they occupy a descriptor in the pool just like any other binding.
  for (WUInt32 i = 0; i < m_Description.m_ResourceBindings.GetCount(); i++)
  {
    const WShaderResourceBinding& WBinding = m_Description.m_ResourceBindings[i];
    m_ResourceUsage.m_Usage[WBinding.m_ResourceType.GetValue()]++;
    W_ASSERT_DEV(WBinding.m_uiArraySize == 1, "Descriptor arrays are not supported, binding '{}' requests {} elements.", WBinding.m_sName, WBinding.m_uiArraySize);
  }
  for (WUInt32 i = 0; i < m_Description.m_ImmutableSamplers.GetCount(); i++)
  {
    const WShaderResourceBinding& WBinding = m_Description.m_ImmutableSamplers[i];
    m_ResourceUsage.m_Usage[WBinding.m_ResourceType.GetValue()]++;
    W_ASSERT_DEV(WBinding.m_uiArraySize == 1, "Descriptor arrays are not supported, binding '{}' requests {} elements.", WBinding.m_sName, WBinding.m_uiArraySize);
  }

  const WGALImmutableSamplers::ImmutableSamplers& immutableSamplers = WGALImmutableSamplers::GetImmutableSamplers();

  for (WUInt32 i = 0; i < m_Description.m_ImmutableSamplers.GetCount(); i++)
  {
    const WShaderResourceBinding& WBinding = m_Description.m_ImmutableSamplers[i];
    vk::DescriptorSetLayoutBinding& binding = bindings.ExpandAndGetRef();
    ConvertBinding(WBinding, binding);
    if (const WGALSamplerStateHandle* hSampler = immutableSamplers.GetValue(WBinding.m_sName))
    {
      const auto* pSampler = static_cast<const WGALSamplerStateVulkan*>(pVulkanDevice->GetSamplerState(*hSampler));
      binding.pImmutableSamplers = &pSampler->GetImageInfo().sampler;
    }
    else
    {
      WLog::Error("Immutable sampler '{}' not found, failed to create bind group layout", WBinding.m_sName.GetData());
      return W_FAILURE;
    }
  }

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayout;
  descriptorSetLayout.bindingCount = bindings.GetCount();
  descriptorSetLayout.pBindings = bindings.GetData();
  VK_SUCCEED_OR_RETURN_W_FAILURE(pVulkanDevice->GetVulkanDevice().createDescriptorSetLayout(&descriptorSetLayout, nullptr, &m_DescriptorSetLayout));

  m_pDescriptorSetPool = WDescriptorSetPoolVulkan::GetPool(m_ResourceUsage);

  return W_SUCCESS;
}

WResult WGALBindGroupLayoutVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  m_pDescriptorSetPool = nullptr;
  auto* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_DescriptorSetLayout);
  return W_SUCCESS;
}

WDescriptorSetPoolVulkan* WGALBindGroupLayoutVulkan::GetDescriptorSetPool() const
{
  return m_pDescriptorSetPool.Borrow();
}
