#pragma once

#include <RendererFoundation/Utils/RingBufferTracker.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>


class WGALDeviceVulkan;



/// Allocates temporary staging buffers from a large pool. Allocations will automatically be freed at the end of the frame.
/// New (larger) pools will be created if the existing ones run out of space. Pools will be deleted after a certain time of no usage.
class W_RENDERERVULKAN_DLL WStagingBufferPoolVulkan
{
public:
  /// Initializes the pool.
  /// \param pDevice GAL device.
  /// \param uiStartingPoolSize Size of the first pool. If depleted, a new one with twice the size of the previous one is created.
  void Initialize(WGALDeviceVulkan* pDevice, WUInt64 uiStartingPoolSize);
  /// Needs to be called before destroying this instance. Ensure that the GPU is idle before calling.
  void DeInitialize();

  /// Needs to be called after begin frame to free memory.
  void AfterBeginFrame();
  /// Needs to be called before submitting work to the GPU to flush GPU caches.
  void BeforeCommandBufferSubmit();

  /// Allocates a temp buffer of the given size.
  /// \param size The size of the temp buffer.
  /// \return Allocated temp buffer.
  WStagingBufferVulkan AllocateBuffer(WUInt64 uiSize);

private:
  static constexpr WUInt32 s_uiNumberOfFramesToKeepUnusedPoolsAlive = 600;

  struct StagingBufferPool
  {
    StagingBufferPool(WUInt32 uiAlignment, WUInt32 uiTotalSize);
    ~StagingBufferPool();
    WResult Allocate(WUInt32 uiSize, WUInt64 uiCurrentFrame, WUInt32& out_uiStartOffset, WByteArrayPtr& out_allocation);
    void Free(WUInt64 uiUpToFrame);
    void Submit(WGALDeviceVulkan* pDevice, WUInt64 uiFrame);

    WRingBufferTracker m_Tracker;
    WArrayPtr<WUInt8> m_Data;
    vk::Buffer m_Buffer;
    WVulkanAllocation m_Alloc;
    WVulkanAllocationInfo m_AllocInfo;
    WUInt32 m_uiFramesWithoutAllocations = 0;
  };

private:
  StagingBufferPool* GetFreePool(WUInt64 uiSize);

private:
  WUInt64 m_uiAlignment = 0;
  WUInt64 m_uiStartingPoolSize = 10 * 1024u * 1024u;
  WGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_Device;

  WHybridArray<StagingBufferPool*, 8> m_Pools;
  WUInt64 m_uiHighWatermark = 0;
};
