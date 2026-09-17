#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/State/GraphicsPipeline.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class WGALDeviceVulkan;

class W_RENDERERVULKAN_DLL WGALGraphicsPipelineVulkan : public WGALGraphicsPipeline
{
public:
  WGALGraphicsPipelineVulkan(const WGALGraphicsPipelineCreationDescription& description);
  ~WGALGraphicsPipelineVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  const vk::Pipeline& GetPipeline() const { return m_Pipeline; }
  bool HasStencilTest() const { return m_bStencilTest; }

  virtual void SetDebugName(const char* szName) override;

private:
  vk::Pipeline m_Pipeline;
  bool m_bStencilTest = false;
};