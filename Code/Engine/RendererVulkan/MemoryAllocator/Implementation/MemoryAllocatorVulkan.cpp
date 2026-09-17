#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Types/UniquePtr.h>

#define VMA_VULKAN_VERSION 1001000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATS_STRING_ENABLED 1


//
// #define VMA_DEBUG_LOG(format, ...)   \
//  do                                 \
//  {                                  \
//    WStringBuilder tmp;             \
//    tmp.Printf(format, __VA_ARGS__); \
//    WLog::Error("{}", tmp);         \
//  } while (false)

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#define VMA_IMPLEMENTATION

#ifndef VA_IGNORE_THIS_FILE
#  define VA_INCLUDE_HIDDEN <vk_mem_alloc.h>
#else
#  define VA_INCLUDE_HIDDEN ""
#endif

#include VA_INCLUDE_HIDDEN

static_assert(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT == (WUInt32)WVulkanAllocationCreateFlags::DedicatedMemory);
static_assert(VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT == (WUInt32)WVulkanAllocationCreateFlags::NeverAllocate);
static_assert(VMA_ALLOCATION_CREATE_MAPPED_BIT == (WUInt32)WVulkanAllocationCreateFlags::Mapped);
static_assert(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT == (WUInt32)WVulkanAllocationCreateFlags::CanAlias);
static_assert(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT == (WUInt32)WVulkanAllocationCreateFlags::HostAccessSequentialWrite);
static_assert(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT == (WUInt32)WVulkanAllocationCreateFlags::HostAccessRandom);
static_assert(VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT == (WUInt32)WVulkanAllocationCreateFlags::AllowTransferInstead);
static_assert(VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT == (WUInt32)WVulkanAllocationCreateFlags::StrategyMinMemory);
static_assert(VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT == (WUInt32)WVulkanAllocationCreateFlags::StrategyMinTime);

static_assert(VMA_MEMORY_USAGE_UNKNOWN == (WUInt32)WVulkanMemoryUsage::Unknown);
static_assert(VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED == (WUInt32)WVulkanMemoryUsage::GpuLazilyAllocated);
static_assert(VMA_MEMORY_USAGE_AUTO == (WUInt32)WVulkanMemoryUsage::Auto);
static_assert(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE == (WUInt32)WVulkanMemoryUsage::AutoPreferDevice);
static_assert(VMA_MEMORY_USAGE_AUTO_PREFER_HOST == (WUInt32)WVulkanMemoryUsage::AutoPreferHost);

static_assert(sizeof(WVulkanAllocation) == sizeof(VmaAllocation));

static_assert(sizeof(WVulkanAllocationInfo) == sizeof(VmaAllocationInfo));

W_DEFINE_AS_POD_TYPE(VkExportMemoryAllocateInfo);

namespace WMemoryAllocatorVulkanInternal
{
  struct ExportedSharedPool
  {
    VmaPool m_pool = nullptr;
    WUniquePtr<vk::ExportMemoryAllocateInfo> m_exportInfo; // must outlive the pool and remain at the same address.
#if W_ENABLED(W_PLATFORM_WINDOWS)
    WUniquePtr<vk::ExportMemoryWin32HandleInfoKHR> m_exportInfoWin32;
#endif
  };
} // namespace

struct WMemoryAllocatorVulkan::Impl
{
  VmaAllocator m_allocator;
  WMutex m_exportedSharedPoolsMutex;
  WHashTable<uint32_t, WMemoryAllocatorVulkanInternal::ExportedSharedPool> m_exportedSharedPools;
};

using ExportedSharedPool = WMemoryAllocatorVulkanInternal::ExportedSharedPool;

WMemoryAllocatorVulkan::Impl* WMemoryAllocatorVulkan::s_pImpl = nullptr;

vk::Result WMemoryAllocatorVulkan::Initialize(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Instance instance, PFN_vkGetInstanceProcAddr instanceProcAddr, PFN_vkGetDeviceProcAddr deviceProcAddr)
{
  W_ASSERT_DEV(s_pImpl == nullptr, "WMemoryAllocatorVulkan::Initialize was already called");
  s_pImpl = W_DEFAULT_NEW(Impl);

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = instanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = deviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
  allocatorCreateInfo.physicalDevice = physicalDevice;
  allocatorCreateInfo.device = device;
  allocatorCreateInfo.instance = instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  vk::Result res = (vk::Result)vmaCreateAllocator(&allocatorCreateInfo, &s_pImpl->m_allocator);
  if (res != vk::Result::eSuccess)
  {
    W_DEFAULT_DELETE(s_pImpl);
  }

  return res;
}

void WMemoryAllocatorVulkan::DeInitialize()
{
  W_ASSERT_DEV(s_pImpl != nullptr, "WMemoryAllocatorVulkan is not initialized.");

  for (auto it : s_pImpl->m_exportedSharedPools)
  {
    vmaDestroyPool(s_pImpl->m_allocator, it.Value().m_pool);
  }
  s_pImpl->m_exportedSharedPools.Clear();

  // Uncomment below to debug leaks in VMA.

  char* pStats = nullptr;
  vmaBuildStatsString(s_pImpl->m_allocator, &pStats, true);

  vmaDestroyAllocator(s_pImpl->m_allocator);
  W_DEFAULT_DELETE(s_pImpl);
}

vk::Result WMemoryAllocatorVulkan::CreateImage(const vk::ImageCreateInfo& imageCreateInfo, const WVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, WVulkanAllocation& out_pAlloc, WVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  if (allocationCreateInfo.m_bExportSharedAllocation)
  {
    allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    W_LOCK(s_pImpl->m_exportedSharedPoolsMutex);

    uint32_t memoryTypeIndex = 0;
    if (auto res = vmaFindMemoryTypeIndexForImageInfo(s_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, &memoryTypeIndex); res != VK_SUCCESS)
    {
      return (vk::Result)res;
    }

    ExportedSharedPool* pool = s_pImpl->m_exportedSharedPools.GetValue(memoryTypeIndex);
    if (pool == nullptr)
    {
      ExportedSharedPool newPool;
      {
        newPool.m_exportInfo = W_DEFAULT_NEW(vk::ExportMemoryAllocateInfo);
        vk::ExportMemoryAllocateInfo& exportInfo = *newPool.m_exportInfo.Borrow();
#if W_ENABLED(W_PLATFORM_LINUX)
        exportInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif W_ENABLED(W_PLATFORM_WINDOWS)
        newPool.m_exportInfoWin32 = W_DEFAULT_NEW(vk::ExportMemoryWin32HandleInfoKHR);
        vk::ExportMemoryWin32HandleInfoKHR& exportInfoWin = *newPool.m_exportInfoWin32.Borrow();
        exportInfoWin.dwAccess = GENERIC_ALL;

        exportInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
        exportInfo.pNext = &exportInfoWin;
#else
        W_ASSERT_NOT_IMPLEMENTED
#endif
      }

      VmaPoolCreateInfo poolCreateInfo = {};
      poolCreateInfo.memoryTypeIndex = memoryTypeIndex;
      poolCreateInfo.pMemoryAllocateNext = newPool.m_exportInfo.Borrow();

      if (auto res = vmaCreatePool(s_pImpl->m_allocator, &poolCreateInfo, &newPool.m_pool); res != VK_SUCCESS)
      {
        return (vk::Result)res;
      }
      s_pImpl->m_exportedSharedPools.Insert(memoryTypeIndex, std::move(newPool));
      pool = s_pImpl->m_exportedSharedPools.GetValue(memoryTypeIndex);
    }

    allocCreateInfo.pool = pool->m_pool;
  }

  return (vk::Result)vmaCreateImage(s_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&out_image), reinterpret_cast<VmaAllocation*>(&out_pAlloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void WMemoryAllocatorVulkan::DestroyImage(vk::Image& ref_image, WVulkanAllocation& ref_pAlloc)
{
  vmaSetAllocationUserData(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(ref_pAlloc), nullptr);
  vmaDestroyImage(s_pImpl->m_allocator, reinterpret_cast<VkImage&>(ref_image), reinterpret_cast<VmaAllocation&>(ref_pAlloc));
  ref_image = nullptr;
  ref_pAlloc = nullptr;
}

vk::Result WMemoryAllocatorVulkan::CreateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const WVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, WVulkanAllocation& out_pAlloc, WVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  return (vk::Result)vmaCreateBuffer(s_pImpl->m_allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), &allocCreateInfo, reinterpret_cast<VkBuffer*>(&out_buffer), reinterpret_cast<VmaAllocation*>(&out_pAlloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void WMemoryAllocatorVulkan::DestroyBuffer(vk::Buffer& ref_buffer, WVulkanAllocation& ref_pAlloc)
{
  vmaSetAllocationUserData(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(ref_pAlloc), nullptr);
  vmaDestroyBuffer(s_pImpl->m_allocator, reinterpret_cast<VkBuffer&>(ref_buffer), reinterpret_cast<VmaAllocation&>(ref_pAlloc));
  ref_buffer = nullptr;
  ref_pAlloc = nullptr;
}

WVulkanAllocationInfo WMemoryAllocatorVulkan::GetAllocationInfo(WVulkanAllocation pAlloc)
{
  VmaAllocationInfo info;
  vmaGetAllocationInfo(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(pAlloc), &info);

  return reinterpret_cast<WVulkanAllocationInfo&>(info);
}

vk::MemoryPropertyFlags WMemoryAllocatorVulkan::GetAllocationFlags(WVulkanAllocation pAlloc)
{
  VkMemoryPropertyFlags memPropFlags;
  vmaGetAllocationMemoryProperties(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(pAlloc), &memPropFlags);
  return reinterpret_cast<vk::MemoryPropertyFlags&>(memPropFlags);
}

void WMemoryAllocatorVulkan::SetAllocationUserData(WVulkanAllocation pAlloc, const char* pUserData)
{
  vmaSetAllocationUserData(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(pAlloc), (void*)pUserData);
}

vk::Result WMemoryAllocatorVulkan::MapMemory(WVulkanAllocation pAlloc, void** pData)
{
  return (vk::Result)vmaMapMemory(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(pAlloc), pData);
}

void WMemoryAllocatorVulkan::UnmapMemory(WVulkanAllocation pAlloc)
{
  vmaUnmapMemory(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(pAlloc));
}

vk::Result WMemoryAllocatorVulkan::FlushAllocation(WVulkanAllocation pAlloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaFlushAllocation(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(pAlloc), offset, size);
}

vk::Result WMemoryAllocatorVulkan::InvalidateAllocation(WVulkanAllocation pAlloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaInvalidateAllocation(s_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(pAlloc), offset, size);
}

W_DEFINE_AS_POD_TYPE(VmaBudget);

WVulkanMemoryStatistics WMemoryAllocatorVulkan::GetStats()
{
  WVulkanMemoryStatistics stats;
  const WUInt32 uiHeapCount = s_pImpl->m_allocator->GetMemoryHeapCount();
  WHybridArray<VmaBudget, 4> budgets;
  budgets.SetCount(uiHeapCount);
  vmaGetHeapBudgets(s_pImpl->m_allocator, budgets.GetData());
  for (WUInt32 i = 0; i < uiHeapCount; ++i)
  {
    const VmaBudget& budget = budgets[i];
    stats.m_uiBlockCount += budget.statistics.blockCount;
    stats.m_uiAllocationCount += budget.statistics.allocationCount;
    stats.m_uiBlockBytes += (WUInt64)budget.statistics.blockBytes;
    stats.m_uiAllocationBytes += (WUInt64)budget.statistics.allocationBytes;
  }
  return stats;
}
