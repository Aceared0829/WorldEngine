#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/TransientDescriptorSetPoolVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

//////////////////////////////////////////////////////////////////////

WGALDeviceVulkan* WDescriptorSetPoolVulkan::s_pDevice = nullptr;
WMutex WDescriptorSetPoolVulkan::s_Mutex;
WHashTable<WBindGroupLayoutResourceUsageVulkan, WSharedPtr<WDescriptorSetPoolVulkan>, WDescriptorSetPoolVulkan::ResourceUsageHash> WDescriptorSetPoolVulkan::s_Pools;
WHashSet<WDescriptorSetPoolVulkan*> WDescriptorSetPoolVulkan::s_DirtyPools;

WUInt32 WDescriptorSetPoolVulkan::ResourceUsageHash::Hash(const WBindGroupLayoutResourceUsageVulkan& a)
{
  return WHashingUtils::xxHash32(&a.m_Usage[0], WGALShaderResourceType::COUNT);
}

bool WDescriptorSetPoolVulkan::ResourceUsageHash::Equal(const WBindGroupLayoutResourceUsageVulkan& a, const WBindGroupLayoutResourceUsageVulkan& b)
{
  for (WUInt32 i = 0; i < WGALShaderResourceType::COUNT; ++i)
  {
    if (a.m_Usage[i] != b.m_Usage[i])
      return false;
  }

  return true;
}

void WDescriptorSetPoolVulkan::Initialize(WGALDeviceVulkan* pDevice)
{
  s_pDevice = pDevice;
}

void WDescriptorSetPoolVulkan::DeInitialize()
{
  // Destroy all pools, make sure ref count is zero.
  for (auto it : s_Pools)
  {
    W_ASSERT_DEBUG(it.Value()->GetRefCount() == 1, "Pool still referenced. A bind group layout probably leaked");
  }
  s_Pools.Clear();
  s_Pools.Compact();
  s_DirtyPools.Clear();
  s_DirtyPools.Compact();
  s_pDevice = nullptr;
}

WSharedPtr<WDescriptorSetPoolVulkan> WDescriptorSetPoolVulkan::GetPool(const WBindGroupLayoutResourceUsageVulkan& resourceUsage)
{
  W_LOCK(s_Mutex);
  auto it = s_Pools.Find(resourceUsage);
  if (it.IsValid())
  {
    return it.Value();
  }

  WSharedPtr<WDescriptorSetPoolVulkan> pPool = W_DEFAULT_NEW(WDescriptorSetPoolVulkan, s_pDevice, resourceUsage);
  s_Pools[resourceUsage] = pPool;

  return pPool;
}

WUInt32 WDescriptorSetPoolVulkan::GetPoolCount()
{
  return s_Pools.GetCount();
}

void WDescriptorSetPoolVulkan::MarkPoolDirty(WDescriptorSetPoolVulkan* pPool)
{
  W_LOCK(s_Mutex);
  s_DirtyPools.Insert(pPool);
}

void WDescriptorSetPoolVulkan::BeginFrame()
{
  W_LOCK(s_Mutex);
  for (WDescriptorSetPoolVulkan* pPool : s_DirtyPools)
  {
    if (pPool->ReclaimResources())
    {
      // #TODO_VULKAN: We should delete the pool to free resources.
    }
  }
  s_DirtyPools.Clear();
}

WDescriptorSetPoolVulkan::WDescriptorSetPoolVulkan(WGALDeviceVulkan* pDevice, const WBindGroupLayoutResourceUsageVulkan& resourceUsage)
  : m_pDevice(pDevice)
  , m_ResourceUsage(resourceUsage)
{
}

WDescriptorSetPoolVulkan::~WDescriptorSetPoolVulkan()
{
  ReclaimResources();
  W_ASSERT_DEBUG(m_uiTotalAllocations == 0, "A descriptor set has leaked, probably caused by a bind group not being freed before shutting down the GAL device");
  for (WUniquePtr<DescriptorSubPool>& pool : m_SubPools)
  {
    m_pDevice->GetVulkanDevice().destroyDescriptorPool(pool->m_DescriptorPool);
  }
  m_SubPools.Clear();
}

vk::DescriptorSet WDescriptorSetPoolVulkan::CreateDescriptorSet(WGALBindGroupLayoutHandle hBindGroupLayout, WDescriptorSetPoolVulkan::Allocation& out_allocation)
{
  W_LOCK(m_Mutex);
  WUInt32 uiPoolIndex = GetFreePoolIndex();
  DescriptorSubPool* pSubPool = m_SubPools[uiPoolIndex].Borrow();

  const WGALBindGroupLayoutVulkan* pLayout = static_cast<const WGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(hBindGroupLayout));

  vk::DescriptorSet set;
  vk::DescriptorSetAllocateInfo allocateInfo;
  allocateInfo.pSetLayouts = &pLayout->GetDescriptorSetLayout();
  allocateInfo.descriptorPool = pSubPool->m_DescriptorPool;
  allocateInfo.descriptorSetCount = 1;

  VK_ASSERT_DEV(m_pDevice->GetVulkanDevice().allocateDescriptorSets(&allocateInfo, &set));

  ++m_uiTotalAllocations;
  ++pSubPool->m_uiAllocatedSets;

  out_allocation.m_uiPoolIndex = uiPoolIndex;

  return set;
}

void WDescriptorSetPoolVulkan::ReclaimDescriptorSet(vk::DescriptorSet descriptorSet, Allocation allocation)
{
  bool bMarkPoolDirty = false;
  {
    W_LOCK(m_Mutex);
    DescriptorSubPool* pPool = m_SubPools[allocation.m_uiPoolIndex].Borrow();
    pPool->m_Reclaim.PushBack(descriptorSet);
    if (!m_DirtySubPools.Contains(pPool))
    {
      m_DirtySubPools.Insert(pPool);
      bMarkPoolDirty = true;
    }
  }

  if (bMarkPoolDirty)
  {
    WDescriptorSetPoolVulkan::MarkPoolDirty(this);
  }
}

WUInt32 WDescriptorSetPoolVulkan::GetFreePoolIndex()
{
  if (m_uiActivePool != WInvalidIndex && m_SubPools[m_uiActivePool]->m_uiAllocatedSets < m_SubPools[m_uiActivePool]->m_uiPoolSize)
  {
    return m_uiActivePool;
  }

  for (WInt32 poolIndex = (WInt32)m_SubPools.GetCount() - 1; poolIndex >= 0; --poolIndex)
  {
    if (m_SubPools[poolIndex]->m_uiAllocatedSets < m_SubPools[poolIndex]->m_uiPoolSize)
    {
      m_uiActivePool = poolIndex;
      return m_uiActivePool;
    }
  }

  WUniquePtr<DescriptorSubPool> pSubPool = W_DEFAULT_NEW(DescriptorSubPool);
  pSubPool->m_uiPoolSize = m_uiNextPoolSize;
  {
    // Create Vulkan descriptor pool
    WHybridArray<vk::DescriptorPoolSize, WGALShaderResourceType::COUNT> poolSizes;
    for (WUInt32 i = 0; i < WGALShaderResourceType::COUNT; ++i)
    {
      if (m_ResourceUsage.m_Usage[i] > 0)
        poolSizes.PushBack(vk::DescriptorPoolSize(WConversionUtilsVulkan::GetDescriptorType((WGALShaderResourceType::Enum)i), m_ResourceUsage.m_Usage[i] * m_uiNextPoolSize));
    }

    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolCreateInfo.maxSets = m_uiNextPoolSize;
    poolCreateInfo.poolSizeCount = poolSizes.GetCount();
    poolCreateInfo.pPoolSizes = poolSizes.GetData();

    VK_ASSERT_DEV(m_pDevice->GetVulkanDevice().createDescriptorPool(&poolCreateInfo, nullptr, &pSubPool->m_DescriptorPool));
  }
  m_SubPools.PushBack(std::move(pSubPool));
  m_uiActivePool = m_SubPools.GetCount() - 1;

  m_uiNextPoolSize *= 2;

  return m_uiActivePool;
}

bool WDescriptorSetPoolVulkan::ReclaimResources()
{
  W_LOCK(m_Mutex);

  for (DescriptorSubPool* pSubPool : m_DirtySubPools)
  {
    const WUInt32 uiReclaimCount = pSubPool->m_Reclaim.GetCount();
    m_pDevice->GetVulkanDevice().freeDescriptorSets(pSubPool->m_DescriptorPool, uiReclaimCount, pSubPool->m_Reclaim.GetData());
    pSubPool->m_uiAllocatedSets -= uiReclaimCount;
    m_uiTotalAllocations -= uiReclaimCount;
    pSubPool->m_Reclaim.Clear();
  }
  m_DirtySubPools.Clear();

  return m_uiTotalAllocations == 0;
}
