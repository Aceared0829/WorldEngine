#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#include <RendererFoundation/Utils/RingBufferTracker.h>

class WGALDeviceVulkan;
class WGALBufferVulkan;

/// `WGALBufferVulkan` created with `WGALBufferUsageFlags::Transient` will allocate scratch memory from this pool which will last until the end of the frame.
class W_RENDERERVULKAN_DLL WUniformBufferPoolVulkan
{
public:
  enum class BufferUpdateResult
  {
    OffsetChanged,        ///< The offset of the current buffer was moved forward.
    DynamicBufferChanged, ///< The current buffer was depleted and a new buffer was started.
  };

  WUniformBufferPoolVulkan(WGALDeviceVulkan* pDevice);

  void Initialize();
  void DeInitialize();

  void EndFrame();
  void BeforeCommandBufferSubmit();

  /// Needs to be called each from for a buffer before it can be used.
  /// Can be called multiple times per frame. GetBuffer will always return a reference to the last UpdateBuffer content.
  /// \param pBuffer The buffer to allocate scratch memory for.
  /// \param data The data of the buffer. Must be the size of the entire buffer.
  /// \return Returns whether a new pool needed to be created.
  BufferUpdateResult UpdateBuffer(const WGALBufferVulkan* pBuffer, WArrayPtr<const WUInt8> data);

  /// Access the descriptor info for the given buffer.
  /// \param pBuffer The buffer for which previously scratch memory was allocated for.
  /// \return Pointer to the descriptor info.
  const vk::DescriptorBufferInfo* GetBuffer(const WGALBufferVulkan* pBuffer) const;

private:
  struct UniformBufferPool
  {
    UniformBufferPool(WUInt32 uiAlignment, WUInt32 uiTotalSize);
    ~UniformBufferPool();
    WResult CanAllocate(WUInt32 uiSize) const { return m_Tracker.CanAllocate(uiSize); }
    WResult Allocate(WUInt32 uiSize, WUInt64 uiCurrentFrame, WUInt32& out_uiStartOffset, WByteArrayPtr& out_allocation);
    void Free(WUInt64 uiUpToFrame);
    void Submit(WGALDeviceVulkan* pDevice, WUInt64 uiFrame);
    WUInt32 GetFreeMemory() const { return m_Tracker.GetFreeMemory(); }

    WRingBufferTracker m_Tracker;
    WArrayPtr<WUInt8> m_Data;
    vk::Buffer m_Buffer;
    vk::Buffer m_StagingBuffer;
    WVulkanAllocation m_Alloc;
    WVulkanAllocation m_StagingAlloc;
    WVulkanAllocationInfo m_AllocInfo;
    WVulkanAllocationInfo m_StagingAllocInfo;
  };

private:
  UniformBufferPool* GetFreePool(WUInt32 uiSize);
  WUInt32 GetBufferSize(WUInt32 uiSize);

private:
  WGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_Device;
  WUInt32 m_uiAlignment = 0;
  WUInt32 m_uiBufferSize = 0;

  WMap<const WGALBufferVulkan*, vk::DescriptorBufferInfo> m_Buffer;

  UniformBufferPool* m_pCurrentPool = nullptr;
  WDeque<UniformBufferPool*> m_PendingPools;
  WHybridArray<UniformBufferPool*, 8> m_FreePools;
};
