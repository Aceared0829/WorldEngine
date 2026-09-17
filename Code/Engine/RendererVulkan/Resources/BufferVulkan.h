
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Resources/Buffer.h>
#include <RendererVulkan/Device/DeviceVulkan.h>

class W_RENDERERVULKAN_DLL WGALBufferVulkan : public WGALBuffer
{
public:
  W_ALWAYS_INLINE vk::Buffer GetVkBuffer() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;

  W_ALWAYS_INLINE vk::IndexType GetIndexType() const;
  W_ALWAYS_INLINE WVulkanAllocation GetAllocation() const;
  W_ALWAYS_INLINE const WVulkanAllocationInfo& GetAllocationInfo() const;
  W_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  W_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;
  vk::BufferView GetTexelBufferView(WGALBufferRange bufferRange, WEnum<WGALResourceFormat> overrideTexelBufferFormat) const;
  static vk::DeviceSize GetAlignment(const WGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage);

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALBufferVulkan(const WGALBufferCreationDescription& Description);

  virtual ~WGALBufferVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<const WUInt8> pInitialData) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;
  WResult CreateBuffer();

protected:
  vk::Buffer m_Buffer = {};
  WVulkanAllocation m_pAlloc = {};
  WVulkanAllocationInfo m_AllocInfo = {};
  vk::DescriptorBufferInfo m_ResourceBufferInfo = {};

  // Data for memory barriers and access
  vk::PipelineStageFlags m_Stages = {};
  vk::AccessFlags m_Access = {};
  vk::IndexType m_IndexType = vk::IndexType::eUint16; // Only applicable for index buffers
  vk::BufferUsageFlags m_Usage = {};
  vk::DeviceSize m_Size = 0;

  WGALDeviceVulkan* m_pDeviceVulkan = nullptr;
  vk::Device m_Device = {};

  // Views
  struct View : WHashableStruct<View>
  {
    WGALBufferRange m_BufferRange;
    WEnum<WGALResourceFormat> m_OverrideTexelBufferFormat;

    W_ALWAYS_INLINE static WUInt32 Hash(const View& value) { return value.CalculateHash(); }
    W_ALWAYS_INLINE static bool Equal(const View& a, const View& b) { return a == b; }
  };
  mutable WHashTable<View, vk::BufferView, View> m_TexelBufferViews;

  mutable WString m_sDebugName;
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
