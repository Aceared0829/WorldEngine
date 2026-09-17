#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

void WStagingBufferPoolVulkan::Initialize(WGALDeviceVulkan* pDevice, WUInt64 uiStartingPoolSize)
{
  m_pDevice = pDevice;
  m_Device = pDevice->GetVulkanDevice();

  const vk::PhysicalDeviceProperties& properties = m_pDevice->GetPhysicalDeviceProperties();
  m_uiAlignment = WMath::Max<WUInt64>(16ull, m_uiAlignment, (WUInt64)properties.limits.nonCoherentAtomSize);
  m_uiAlignment = WMath::Max(m_uiAlignment, (WUInt64)properties.limits.optimalBufferCopyOffsetAlignment);
  W_ASSERT_DEBUG(WMath::IsPowerOf2(m_uiAlignment), "Non-power of two alignment not supported");
  m_uiStartingPoolSize = WMemoryUtils::AlignSize(uiStartingPoolSize, m_uiAlignment);
}

void WStagingBufferPoolVulkan::DeInitialize()
{
  // We assume the device makes sure no buffers are still in use before calling this.
  m_Device = nullptr;
  for (StagingBufferPool* pPool : m_Pools)
  {
    W_DELETE(m_pDevice->GetAllocator(), pPool);
  }
  m_Pools.Clear();

  WLog::Info("StagingBufferPoolVulkan#{}: high water mark: {}", WArgP(this), WArgHumanReadable((WInt64)m_uiHighWatermark));
}

void WStagingBufferPoolVulkan::AfterBeginFrame()
{
  WUInt64 m_uiTotalAllocatedSize = 0;
  const WUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();
  if (uiSafeFrame >= 1)
  {
    for (StagingBufferPool* pPool : m_Pools)
    {
      m_uiTotalAllocatedSize += pPool->m_Tracker.GetUsedMemory();
      // The -1 is necessary because we use some staging buffers for two frames, see comments in WGALDeviceVulkan::UpdateBufferForNextFramePlatform and UpdateTextureForNextFramePlatform.
      pPool->Free(uiSafeFrame - 1);
    }
  }
  m_uiHighWatermark = WMath::Max<WUInt64>(m_uiTotalAllocatedSize, m_uiHighWatermark);

  // Keep one pool around at all times.
  while (m_Pools.GetCount() > 1 && m_Pools.PeekBack()->m_uiFramesWithoutAllocations > s_uiNumberOfFramesToKeepUnusedPoolsAlive)
  {
    // It is safe to delete the pool here as there can't be any usage on the GPU of the buffer if the pool is unused.
    StagingBufferPool* pPool = m_Pools.PeekBack();
    m_Pools.PopBack();
    W_DELETE(m_pDevice->GetAllocator(), pPool);
  }
}

void WStagingBufferPoolVulkan::BeforeCommandBufferSubmit()
{
  for (StagingBufferPool* pPool : m_Pools)
  {
    pPool->Submit(m_pDevice, m_pDevice->GetCurrentFrame());
  }
}

WStagingBufferVulkan WStagingBufferPoolVulkan::AllocateBuffer(WUInt64 uiSize)
{
  W_ASSERT_DEBUG(m_Device, "WStagingBufferPoolVulkan::Initialize not called");
  WStagingBufferVulkan buffer;

  WUInt32 uiOffset = 0;
  WByteArrayPtr allocation;
  for (StagingBufferPool* pPool : m_Pools)
  {
    if (pPool->Allocate((WUInt32)uiSize, m_pDevice->GetCurrentFrame(), uiOffset, allocation).Succeeded())
    {
      buffer.m_alloc = pPool->m_Alloc;
      buffer.m_buffer = pPool->m_Buffer;
      break;
    }
  }

  if (allocation.IsEmpty())
  {
    StagingBufferPool* pPool = GetFreePool(uiSize);
    pPool->Allocate((WUInt32)uiSize, m_pDevice->GetCurrentFrame(), uiOffset, allocation).AssertSuccess("Newly created pool should be able to allocate requested size");
    buffer.m_alloc = pPool->m_Alloc;
    buffer.m_buffer = pPool->m_Buffer;
  }

  buffer.m_Data = allocation;
  buffer.m_uiOffset = uiOffset;
  return buffer;
}

WStagingBufferPoolVulkan::StagingBufferPool* WStagingBufferPoolVulkan::GetFreePool(WUInt64 uiSize)
{
  WUInt64 uiNewPoolSize = m_Pools.IsEmpty() ? m_uiStartingPoolSize : m_Pools.PeekBack()->m_Tracker.GetTotalMemory() * 2;

  m_uiStartingPoolSize = WMemoryUtils::AlignSize(WMath::Max(uiNewPoolSize, uiSize), m_uiAlignment);

  StagingBufferPool* pPool = W_NEW(m_pDevice->GetAllocator(), StagingBufferPool, (WUInt32)m_uiAlignment, (WUInt32)m_uiStartingPoolSize);
  m_Pools.PushBack(pPool);
  return pPool;
}


WStagingBufferPoolVulkan::StagingBufferPool::StagingBufferPool(WUInt32 uiAlignment, WUInt32 uiTotalSize)
  : m_Tracker(uiAlignment, uiTotalSize)
{
  vk::BufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.size = uiTotalSize;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

  WVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = WVulkanMemoryUsage::Auto;
  allocInfo.m_flags = WVulkanAllocationCreateFlags::HostAccessSequentialWrite | WVulkanAllocationCreateFlags::Mapped;

  VK_ASSERT_DEV(WMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, m_Buffer, m_Alloc, &m_AllocInfo));
  m_Data = WMakeArrayPtr(reinterpret_cast<WUInt8*>(m_AllocInfo.m_pMappedData), uiTotalSize);
}

WStagingBufferPoolVulkan::StagingBufferPool::~StagingBufferPool()
{
  WMemoryAllocatorVulkan::DestroyBuffer(m_Buffer, m_Alloc);
}

WResult WStagingBufferPoolVulkan::StagingBufferPool::Allocate(WUInt32 uiSize, WUInt64 uiCurrentFrame, WUInt32& out_uiStartOffset, WByteArrayPtr& out_allocation)
{
  if (m_Tracker.Allocate(uiSize, uiCurrentFrame, out_uiStartOffset).Failed())
    return W_FAILURE;

  out_allocation = m_Data.GetSubArray(out_uiStartOffset, uiSize);
  return W_SUCCESS;
}

void WStagingBufferPoolVulkan::StagingBufferPool::Free(WUInt64 uiUpToFrame)
{
  m_uiFramesWithoutAllocations = m_Tracker.GetUsedMemory() == 0 ? ++m_uiFramesWithoutAllocations : 0;
  m_Tracker.Free(uiUpToFrame);
}

void WStagingBufferPoolVulkan::StagingBufferPool::Submit(WGALDeviceVulkan* pDevice, WUInt64 uiFrame)
{
  WHybridArray<WRingBufferTracker::FrameData, 4> frameData;
  if (m_Tracker.SubmitFrame(uiFrame, frameData).Failed())
    return;

  for (WRingBufferTracker::FrameData& frame : frameData)
  {
    VK_ASSERT_DEBUG(WMemoryAllocatorVulkan::FlushAllocation(m_Alloc, frame.m_uiStartOffset, frame.m_uiSize));
  }
}
