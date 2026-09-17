
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

#include <RendererCore/Shader/ShaderStageBinary.h>

class W_RENDERERVULKAN_DLL WGALShaderVulkan : public WGALShader
{
public:
  virtual void SetDebugName(WStringView sName) const override;

  W_ALWAYS_INLINE vk::ShaderModule GetShader(WGALShaderStage::Enum stage) const;
  vk::PipelineLayout GetVkPipelineLayout() const;
  vk::DescriptorSetLayout GetDescriptorSetLayout(WUInt32 uiSet = 0) const;
  vk::PushConstantRange GetPushConstantRange() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALShaderVulkan(const WGALShaderCreationDescription& description);
  virtual ~WGALShaderVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

private:
  vk::ShaderModule m_Shaders[WGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
