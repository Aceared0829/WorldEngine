#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/ComputePipelineVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WGALComputePipelineVulkan::WGALComputePipelineVulkan(const WGALComputePipelineCreationDescription& description)
  : WGALComputePipeline(description)
{
}

WGALComputePipelineVulkan::~WGALComputePipelineVulkan() = default;

WResult WGALComputePipelineVulkan::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pDeviceVulkan = static_cast<WGALDeviceVulkan*>(pDevice);

  const WGALShaderVulkan* pShader = static_cast<const WGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));
  if (pShader == nullptr)
  {
    WLog::Error("Failed to create Vulkan Compute pipeline: Invalid shader handle.");
    return W_FAILURE;
  }

  vk::ComputePipelineCreateInfo pipe;
  pipe.layout = pShader->GetVkPipelineLayout();
  {
    vk::ShaderModule shader = pShader->GetShader(WGALShaderStage::ComputeShader);
    W_ASSERT_DEV(shader != nullptr, "No compute shader stage present in the bound shader");
    pipe.stage.stage = WConversionUtilsVulkan::GetShaderStage(WGALShaderStage::ComputeShader);
    pipe.stage.module = shader;
    pipe.stage.pName = "main";
  }

  VK_SUCCEED_OR_RETURN_W_FAILURE(pDeviceVulkan->GetVulkanDevice().createComputePipelines(WResourceCacheVulkan::GetPipelineCache(), 1, &pipe, nullptr, &m_Pipeline));

  return W_SUCCESS;
}

WResult WGALComputePipelineVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pDeviceVulkan = static_cast<WGALDeviceVulkan*>(pDevice);

  if (m_Pipeline)
  {
    pDeviceVulkan->DeleteLater(m_Pipeline);
    m_Pipeline = nullptr;
  }
  return W_SUCCESS;
}

void WGALComputePipelineVulkan::SetDebugName(const char*)
{
}
