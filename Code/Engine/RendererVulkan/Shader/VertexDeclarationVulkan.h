
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>

class WGALVertexDeclarationVulkan : public WGALVertexDeclaration
{
public:
  W_ALWAYS_INLINE const vk::PipelineVertexInputStateCreateInfo& GetCreateInfo() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  WGALVertexDeclarationVulkan(const WGALVertexDeclarationCreationDescription& Description);

  virtual ~WGALVertexDeclarationVulkan();

  vk::PipelineVertexInputStateCreateInfo m_CreateInfo;
  WHybridArray<vk::VertexInputAttributeDescription, W_GAL_MAX_VERTEX_ATTRIBUTE_COUNT> m_Attributes;
  WHybridArray<vk::VertexInputBindingDescription, W_GAL_MAX_VERTEX_BUFFER_COUNT> m_Bindings;
};

#include <RendererVulkan/Shader/Implementation/VertexDeclarationVulkan_inl.h>
