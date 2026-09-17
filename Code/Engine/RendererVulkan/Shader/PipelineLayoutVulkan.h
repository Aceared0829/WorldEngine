#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/PipelineLayout.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class WGALPipelineLayoutVulkan : public WGALPipelineLayout
{
public:
  inline vk::PushConstantRange GetPushConstantRange() const { return m_PushConstants; }
  inline vk::PipelineLayout GetVkPipelineLayout() const { return m_PipelineLayout; }

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  WGALPipelineLayoutVulkan(const WGALPipelineLayoutCreationDescription& Description);

  virtual ~WGALPipelineLayoutVulkan();

private:
  vk::PushConstantRange m_PushConstants;
  vk::PipelineLayout m_PipelineLayout;
};
