#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/State/ComputePipeline.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class WGALDeviceVulkan;

class W_RENDERERVULKAN_DLL WGALComputePipelineVulkan : public WGALComputePipeline
{
public:
  WGALComputePipelineVulkan(const WGALComputePipelineCreationDescription& description);
  ~WGALComputePipelineVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  const vk::Pipeline& GetPipeline() const { return m_Pipeline; }

  virtual void SetDebugName(const char* szName) override;

private:
  vk::Pipeline m_Pipeline;
};