#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

/// Subset of VmaAllocationCreateFlagBits. Duplicated for abstraction purposes.
struct WVulkanAllocationCreateFlags
{
  using StorageType = WUInt32;
  enum Enum
  {
    DedicatedMemory = 0x00000001,
    NeverAllocate = 0x00000002,
    Mapped = 0x00000004,
    CanAlias = 0x00000200,
    HostAccessSequentialWrite = 0x00000400,
    HostAccessRandom = 0x00000800,
    AllowTransferInstead = 0x00001000,
    StrategyMinMemory = 0x00010000,
    StrategyMinTime = 0x00020000,

    Default = 0,
  };

  struct Bits
  {
    StorageType DedicatedMemory : 1;
    StorageType NeverAllocate : 1;
    StorageType Mapped : 1;
    StorageType UnusedBit3 : 1;

    StorageType UnusedBit4 : 1;
    StorageType UnusedBit5 : 1;
    StorageType UnusedBit6 : 1;
    StorageType UnusedBit7 : 1;

    StorageType UnusedBit8 : 1;
    StorageType CanAlias : 1;
    StorageType HostAccessSequentialWrite : 1;
    StorageType HostAccessRandom : 1;

    StorageType UnusedBit12 : 1;
    StorageType UnusedBit13 : 1;
    StorageType UnusedBit14 : 1;
    StorageType StrategyMinMemory : 1;

    StorageType StrategyMinTime : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WVulkanAllocationCreateFlags);

/// Subset of VmaMemoryUsage. Duplicated for abstraction purposes.
struct WVulkanMemoryUsage
{
  using StorageType = WUInt8;
  enum Enum
  {
    Unknown = 0,
    GpuLazilyAllocated = 6,
    Auto = 7,
    AutoPreferDevice = 8,
    AutoPreferHost = 9,
    Default = Unknown,
  };
};

/// Subset of VmaAllocationCreateInfo. Duplicated for abstraction purposes.
struct WVulkanAllocationCreateInfo
{
  WBitflags<WVulkanAllocationCreateFlags> m_flags;
  WEnum<WVulkanMemoryUsage> m_usage;
  const char* m_pUserData = nullptr;
  bool m_bExportSharedAllocation = false; // If this allocation should be exported so other processes can access it.
};

/// Subset of VmaAllocationInfo. Duplicated for abstraction purposes.
struct WVulkanAllocationInfo
{
  uint32_t m_memoryType;
  vk::DeviceMemory m_deviceMemory;
  vk::DeviceSize m_offset;
  vk::DeviceSize m_size;
  void* m_pMappedData;
  void* m_pUserData;
  const char* m_pName;
};

/// Copy of VmaStatistics. Duplicated for abstraction purposes.
struct WVulkanMemoryStatistics
{
  WUInt32 m_uiBlockCount = 0;
  WUInt32 m_uiAllocationCount = 0;
  WUInt64 m_uiBlockBytes = 0;
  WUInt64 m_uiAllocationBytes = 0;
};


/// Thin abstraction layer over VulkanMemoryAllocator to allow for abstraction and prevent pulling in its massive header into other files.
/// Functions are a subset of VMA's. To be extended once a use-case comes up.
class W_RENDERERVULKAN_DLL WMemoryAllocatorVulkan
{
public:
  static vk::Result Initialize(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Instance instance, PFN_vkGetInstanceProcAddr instanceProcAddr, PFN_vkGetDeviceProcAddr deviceProcAddr);
  static void DeInitialize();

  static vk::Result CreateImage(const vk::ImageCreateInfo& imageCreateInfo, const WVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, WVulkanAllocation& out_pAlloc, WVulkanAllocationInfo* pAllocInfo = nullptr);
  static void DestroyImage(vk::Image& ref_image, WVulkanAllocation& ref_pAlloc);

  static vk::Result CreateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const WVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, WVulkanAllocation& out_pAlloc, WVulkanAllocationInfo* pAllocInfo = nullptr);
  static void DestroyBuffer(vk::Buffer& ref_buffer, WVulkanAllocation& ref_pAlloc);

  static WVulkanAllocationInfo GetAllocationInfo(WVulkanAllocation pAlloc);
  static vk::MemoryPropertyFlags GetAllocationFlags(WVulkanAllocation pAlloc);
  static void SetAllocationUserData(WVulkanAllocation pAlloc, const char* pUserData);

  static vk::Result MapMemory(WVulkanAllocation pAlloc, void** pData);
  static void UnmapMemory(WVulkanAllocation pAlloc);
  static vk::Result FlushAllocation(WVulkanAllocation pAlloc, vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);
  static vk::Result InvalidateAllocation(WVulkanAllocation pAlloc, vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);

  static WVulkanMemoryStatistics GetStats();

private:
  struct Impl;
  static Impl* s_pImpl;
};
