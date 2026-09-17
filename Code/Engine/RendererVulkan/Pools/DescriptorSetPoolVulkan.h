#pragma once

#include <Foundation/Types/SharedPtr.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class WGALDeviceVulkan;

/// Creates persistent descriptor sets.
/// Each bind group layout requests a pool via GetPool. There is one pool for each resource usage distribution. This is efficient because the spec says: "Additionally, if all sets allocated from the pool since it was created or most recently reset use the same number of descriptors (of each type) and the requested allocation also uses that same number of descriptors (of each type), then fragmentation must not cause an allocation failure."
/// Each pool will create sub-pools in increasing size once all current pools are full.
class W_RENDERERVULKAN_DLL WDescriptorSetPoolVulkan : public WRefCounted
{
public:
  static void Initialize(WGALDeviceVulkan* pDevice);
  static void DeInitialize();
  static WSharedPtr<WDescriptorSetPoolVulkan> GetPool(const WBindGroupLayoutResourceUsageVulkan& resourceUsage);
  static WUInt32 GetPoolCount();
  static void MarkPoolDirty(WDescriptorSetPoolVulkan* pPool);
  static void BeginFrame();

public:
  ~WDescriptorSetPoolVulkan();

  struct Allocation
  {
    WUInt32 m_uiPoolIndex = 0;
  };

  /// Creates a descriptor set for the given layout.
  /// \param hBindGroupLayout The layout for which to create the descriptor set.
  /// \param out_Allocation Filled out by the call. Needs to be passed in ReclaimDescriptorSet once the descriptor set is no longer used by the GPU.
  vk::DescriptorSet CreateDescriptorSet(WGALBindGroupLayoutHandle hBindGroupLayout, Allocation& out_allocation);

  /// Reclaims a descriptor set for reuse.
  /// This function should only be called by WGALDeviceVulkan when it is safe to reuse the descriptor set. To reclaim a descriptor set created via CreateDescriptorSet, call WGALDeviceVulkan->ReclaimLater(vk::DescriptorSet, WDescriptorSetPoolVulkan*, Allocation::m_uiPoolIndex);
  void ReclaimDescriptorSet(vk::DescriptorSet descriptorSet, Allocation allocation);

private:
  struct ResourceUsageHash
  {
    W_ALWAYS_INLINE static WUInt32 Hash(const WBindGroupLayoutResourceUsageVulkan& a);
    W_ALWAYS_INLINE static bool Equal(const WBindGroupLayoutResourceUsageVulkan& a, const WBindGroupLayoutResourceUsageVulkan& b);
  };

  static WGALDeviceVulkan* s_pDevice;
  static WMutex s_Mutex;
  static WHashTable<WBindGroupLayoutResourceUsageVulkan, WSharedPtr<WDescriptorSetPoolVulkan>, ResourceUsageHash> s_Pools; ///< Cache of all pools
  static WHashSet<WDescriptorSetPoolVulkan*> s_DirtyPools;                                                                   ///< Pools that need to free resources and can potentially be destroyed.

private:
  struct DescriptorSubPool
  {
    vk::DescriptorPool m_DescriptorPool;
    WUInt32 m_uiPoolSize = 0;
    WUInt32 m_uiAllocatedSets = 0;
    WDynamicArray<vk::DescriptorSet> m_Reclaim;
  };

  WDescriptorSetPoolVulkan(WGALDeviceVulkan* pDevice, const WBindGroupLayoutResourceUsageVulkan& resourceUsage);
  WUInt32 GetFreePoolIndex();

  /// Reclaims resources previously added via ReclaimDescriptorSet.
  /// \return Returns true if the pool no longer holds any descriptor sets and can thus be safely deleted.
  bool ReclaimResources();

private:
  WMutex m_Mutex;
  WGALDeviceVulkan* m_pDevice = nullptr;
  WBindGroupLayoutResourceUsageVulkan m_ResourceUsage;        ///< How many resources of each type each descriptor set uses.
  WUInt32 m_uiNextPoolSize = 64;                              ///< When the current pool runs out of data, allocate next pool with this size and double the value.
  WUInt32 m_uiTotalAllocations = 0;                           ///< Total allocations across all sub-pools.

  WUInt32 m_uiActivePool = WInvalidIndex;                    ///< The current pool with free allocations.
  WHybridArray<WUniquePtr<DescriptorSubPool>, 1> m_SubPools; ///< Available descriptor sub-pools.
  WHashSet<DescriptorSubPool*> m_DirtySubPools;               ///< Descriptor sub-pools that have sets that need to be freed.
};