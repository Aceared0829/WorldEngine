
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/State/State.h>

class W_RENDERERVULKAN_DLL WGALBlendStateVulkan : public WGALBlendState
{
public:
  W_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* GetBlendState() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALBlendStateVulkan(const WGALBlendStateCreationDescription& Description);

  ~WGALBlendStateVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  vk::PipelineColorBlendStateCreateInfo m_BlendState = {};
  vk::PipelineColorBlendAttachmentState m_blendAttachmentState[8] = {};
};

class W_RENDERERVULKAN_DLL WGALDepthStencilStateVulkan : public WGALDepthStencilState
{
public:
  W_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALDepthStencilStateVulkan(const WGALDepthStencilStateCreationDescription& Description);

  ~WGALDepthStencilStateVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  vk::PipelineDepthStencilStateCreateInfo m_DepthStencilState = {};
};

class W_RENDERERVULKAN_DLL WGALRasterizerStateVulkan : public WGALRasterizerState
{
public:
  W_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALRasterizerStateVulkan(const WGALRasterizerStateCreationDescription& Description);

  ~WGALRasterizerStateVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  vk::PipelineRasterizationStateCreateInfo m_RasterizerState = {};
  vk::PipelineRasterizationConservativeStateCreateInfoEXT m_ConservativeRasterState = {}; ///< Only chained into m_RasterizerState::pNext when conservative rasterization is requested.
};

class W_RENDERERVULKAN_DLL WGALSamplerStateVulkan : public WGALSamplerState
{
public:
  W_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALSamplerStateVulkan(const WGALSamplerStateCreationDescription& Description);
  ~WGALSamplerStateVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  vk::DescriptorImageInfo m_ResourceImageInfo;
};


#include <RendererVulkan/State/Implementation/StateVulkan_inl.h>
