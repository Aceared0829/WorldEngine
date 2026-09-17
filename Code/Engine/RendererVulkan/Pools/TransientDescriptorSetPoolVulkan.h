#pragma once

#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

W_DEFINE_AS_POD_TYPE(vk::DescriptorType);

template <>
struct WHashHelper<vk::DescriptorType>
{
  W_ALWAYS_INLINE static WUInt32 Hash(vk::DescriptorType value) { return WHashHelper<WUInt32>::Hash(WUInt32(value)); }
  W_ALWAYS_INLINE static bool Equal(vk::DescriptorType a, vk::DescriptorType b) { return a == b; }
};

/// Creates descriptor sets that are only valid for a single frame.
/// Descriptors are created from pools that are put in the frame deletion queue once full. The frame deletion queue on the device will then call ReclaimPool and the entire pool is reset and reused. Pools are only destroyed on shutdown.
class W_RENDERERVULKAN_DLL WTransientDescriptorSetPoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();
  static WHashTable<vk::DescriptorType, float>& AccessDescriptorPoolWeights();

  static vk::DescriptorSet CreateTransientDescriptorSet(vk::DescriptorSetLayout layout);
  static void UpdateDescriptorSet(vk::DescriptorSet descriptorSet, WArrayPtr<vk::WriteDescriptorSet> update);
  static void ReclaimPool(vk::DescriptorPool& ref_descriptorPool);

private:
  static constexpr WUInt32 s_uiPoolBaseSize = 1024;

  static vk::DescriptorPool GetNewTransientPool();

  static vk::DescriptorPool s_CurrentTransientPool;
  static WHybridArray<vk::DescriptorPool, 4> s_FreeTransientPools;

  static vk::Device s_Device;
  static WHashTable<vk::DescriptorType, float> s_DescriptorWeights;
};