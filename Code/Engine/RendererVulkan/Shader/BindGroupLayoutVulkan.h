
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>

class WDescriptorSetPoolVulkan;

class WGALBindGroupLayoutVulkan : public WGALBindGroupLayout
{
public:
  inline const vk::DescriptorSetLayout& GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
  WDescriptorSetPoolVulkan* GetDescriptorSetPool() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  WGALBindGroupLayoutVulkan(const WGALBindGroupLayoutCreationDescription& Description);

  virtual ~WGALBindGroupLayoutVulkan();

private:
  vk::DescriptorSetLayout m_DescriptorSetLayout;
  WBindGroupLayoutResourceUsageVulkan m_ResourceUsage; // How many resources of each type each descriptor set uses.
  WSharedPtr<WDescriptorSetPoolVulkan> m_pDescriptorSetPool;
};
