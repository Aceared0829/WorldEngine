#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

class WGALDeviceVulkan;
class WUniformBufferPoolVulkan;
struct WShaderResourceBinding;
struct WGALBindGroupItem;

/// A thread-safe pool for batching Vulkan descriptor sets writes.
///
/// This class manages the creation and batching of Vulkan descriptor set writes, supporting both transient and persistent descriptor set updates.
/// All writes are batched and flushed together to minimize API calls before the next bindDescriptorSets call.
class W_RENDERERVULKAN_DLL WDescriptorWritePoolVulkan
{
public:
  WDescriptorWritePoolVulkan(WGALDeviceVulkan* pDevice);

  /// Writes a transient descriptor set with resources that may include dynamic uniform buffers.
  ///
  /// This method is used for transient descriptor sets that contain resources which may be allocated from a uniform buffer pool. Transient constant buffers are handled specially by using the uniform buffer pool to obtain their actual buffer info. Offsets are handled separately in WGALCommandEncoderImplVulkan::UpdateDynamicUniformBufferOffsets.
  ///
  /// \param descriptorSet The Vulkan descriptor set to write to.
  /// \param desc The bind group creation description containing the resources to write.
  /// \param pUniformBufferPool Pointer to the uniform buffer pool for resolving transient buffers.
  void WriteTransientDescriptor(vk::DescriptorSet descriptorSet, const WGALBindGroupCreationDescription& desc, WUniformBufferPoolVulkan* pUniformBufferPool);

  /// Writes a persistent descriptor set and extracts dynamic buffer offsets.
  ///
  /// This method is used for persistent descriptor sets used by bind group resources. We tag every constant buffer as dynamic in the layout, so we need to extract the offsets for each constant buffer here. The offsets are extracted and stored in the output array for later use during command buffer recording.
  ///
  /// \param descriptorSet The Vulkan descriptor set to write to.
  /// \param desc The bind group creation description containing the resources to write.
  /// \param out_Offsets Output array that will be filled with dynamic buffer offsets for constant buffers. See WGALBindGroupVulkan::GetOffsets.
  void WriteDescriptor(vk::DescriptorSet descriptorSet, const WGALBindGroupCreationDescription& desc, WDynamicArray<WUInt32>& out_offsets);

  /// Flushes all pending descriptor writes to the GPU.
  ///
  /// Right now, this method must be executed before each call to bindDescriptorSets until VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT is used (Vulkan 1.2). https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorSetLayoutBindingFlagsCreateInfo.html. As we can't guarantee that the API is available, only the inefficient version is implemented for now.
  ///
  /// \return The number of descriptor writes that were flushed
  WUInt32 FlushWrites();

private:
  vk::WriteDescriptorSet& WriteBindGroupItem(vk::DescriptorSet descriptorSet, const WShaderResourceBinding& binding, const WGALBindGroupItem& item, WUniformBufferPoolVulkan* pUniformBufferPool);

private:
  WGALDeviceVulkan* m_pDevice = nullptr;
  WMutex m_Mutex;
  WDynamicArray<vk::WriteDescriptorSet> m_DescriptorWrites;
  WDeque<vk::DescriptorImageInfo> m_DescriptorImageInfos;
  WDeque<vk::DescriptorBufferInfo> m_DescriptorBufferInfos;
  WDeque<vk::BufferView> m_DescriptorBufferViews;
};