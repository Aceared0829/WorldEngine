#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class WGALBindGroupVulkan : public WGALBindGroup
{
public:
  inline vk::DescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
  inline WArrayPtr<const WUInt32> GetOffsets() const { return m_Offsets.GetArrayPtr(); }

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void Invalidate(WGALDevice* pDevice) override;
  virtual bool IsInvalidated() const override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  WGALBindGroupVulkan(const WGALBindGroupCreationDescription& Description);

  virtual ~WGALBindGroupVulkan();

private:
  vk::DescriptorSet m_DescriptorSet;
  WHybridArray<WUInt32, 1> m_Offsets;
  WDescriptorSetPoolVulkan::Allocation m_Allocation;
};
