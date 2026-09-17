#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>

WUniformBufferPoolVulkan::WUniformBufferPoolVulkan(WGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
  , m_Device(pDevice->GetVulkanDevice())
  , m_Buffer(pDevice->GetAllocator())
  , m_PendingPools(pDevice->GetAllocator())
  , m_FreePools(pDevice->GetAllocator())
{
}

void WUniformBufferPoolVulkan::Initialize()
{
  m_uiAlignment = (WUInt32)WGALBufferVulkan::GetAlignment(m_pDevice, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
  m_uiBufferSize = GetBufferSize(2u * 1024u * 1024u);
}

void WUniformBufferPoolVulkan::DeInitialize()
{
  if (m_pCurrentPool)
  {
    m_FreePools.PushBack(m_pCurrentPool);
    m_pCurrentPool = nullptr;
  }

  for (UniformBufferPool* pPool : m_PendingPools)
  {
    m_FreePools.PushBack(pPool);
  }
  m_PendingPools.Clear();

  for (UniformBufferPool* pPool : m_FreePools)
  {
    W_DELETE(m_pDevice->GetAllocator(), pPool);
  }
  m_FreePools.Clear();
}

void WUniformBufferPoolVulkan::EndFrame()
{
  m_Buffer.Clear();
  WUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();

  for (UniformBufferPool* pPool : m_PendingPools)
  {
    m_FreePools.PushBack(pPool);
  }
  m_PendingPools.Clear();

  for (UniformBufferPool* pPool : m_FreePools)
  {
    pPool->Free(uiSafeFrame);
  }

  m_FreePools.Sort([](const UniformBufferPool* pLhs, const UniformBufferPool* pRhs) -> bool
    { return pLhs->GetFreeMemory() < pRhs->GetFreeMemory(); });

  // Don't switch the current pool as that would cause descriptor re-creation later on.
  if (m_pCurrentPool)
  {
    m_pCurrentPool->Free(uiSafeFrame);
  }
}

void WUniformBufferPoolVulkan::BeforeCommandBufferSubmit()
{
  if (m_pCurrentPool)
  {
    m_pCurrentPool->Submit(m_pDevice, m_pDevice->GetCurrentFrame());
  }
}

WUniformBufferPoolVulkan::BufferUpdateResult WUniformBufferPoolVulkan::UpdateBuffer(const WGALBufferVulkan* pBuffer, WArrayPtr<const WUInt8> data)
{
  const WUInt64 uiCurrentFrame = m_pDevice->GetCurrentFrame();
  BufferUpdateResult res = BufferUpdateResult::OffsetChanged;
  const WUInt32 uiSize = WMemoryUtils::AlignSize(data.GetCount(), (WUInt32)m_uiAlignment);
  if (!m_pCurrentPool)
  {
    m_pCurrentPool = GetFreePool(uiSize);
    res = BufferUpdateResult::DynamicBufferChanged;
  }

  WUInt32 uiOffset = 0;
  WByteArrayPtr allocation;
  if (m_pCurrentPool->Allocate(uiSize, uiCurrentFrame, uiOffset, allocation).Failed())
  {
    m_pCurrentPool->Submit(m_pDevice, uiCurrentFrame);
    m_PendingPools.PushBack(m_pCurrentPool);
    // GetFreePool needs to check for conteguous memory block
    m_pCurrentPool = GetFreePool(uiSize);
    m_pCurrentPool->Allocate(uiSize, uiCurrentFrame, uiOffset, allocation).AssertSuccess("implementation error");
    res = BufferUpdateResult::DynamicBufferChanged;
  }

  W_ASSERT_DEBUG(allocation.GetCount() >= data.GetCount(), "implementation error");
  WMemoryUtils::RawByteCopy(allocation.GetPtr(), data.GetPtr(), data.GetCount());

  vk::DescriptorBufferInfo info;
  info.buffer = m_pCurrentPool->m_Buffer;
  info.range = uiSize;
  info.offset = uiOffset;
  m_Buffer.Insert(pBuffer, info);

  return res;
}

const vk::DescriptorBufferInfo* WUniformBufferPoolVulkan::GetBuffer(const WGALBufferVulkan* pBuffer) const
{
  vk::DescriptorBufferInfo* info = nullptr;
  bool bFound = m_Buffer.TryGetValue(pBuffer, info);
  W_ASSERT_DEBUG(bFound, "Dynamic buffer not found. Dynamic buffers must be updated every frame before use.");
  return info;
}

WUniformBufferPoolVulkan::UniformBufferPool* WUniformBufferPoolVulkan::GetFreePool(WUInt32 uiSize)
{
  if (!m_FreePools.IsEmpty())
  {
    // The back has the pool with the highest available memory. If this fails, it is unlikely the other pools can allocate (could only happen due to fragmentation at the buffer wrap-around_.
    WUniformBufferPoolVulkan::UniformBufferPool* pPool = m_FreePools.PeekBack();
    if (pPool->CanAllocate(uiSize).Succeeded())
    {
      m_FreePools.PopBack();
      return pPool;
    }
  }

  // Create new pool. This time larger in the hopes that it won't run out.
  m_uiBufferSize = GetBufferSize(m_uiBufferSize * 2);
  UniformBufferPool* pPool(W_NEW(m_pDevice->GetAllocator(), UniformBufferPool, m_uiAlignment, m_uiBufferSize));
  return pPool;
}


WUInt32 WUniformBufferPoolVulkan::GetBufferSize(WUInt32 uiSize)
{
  const WUInt32 uiMaxBufferSize = m_pDevice->GetPhysicalDeviceProperties().limits.maxUniformBufferRange;
  WUInt32 uiBufferSize = WMath::Min(uiSize, uiMaxBufferSize);
  uiBufferSize = WMemoryUtils::AlignSize(uiBufferSize, m_uiAlignment);
  while (uiBufferSize >= uiMaxBufferSize)
  {
    uiBufferSize -= m_uiAlignment;
  }
  return uiBufferSize;
}

WUniformBufferPoolVulkan::UniformBufferPool::UniformBufferPool(WUInt32 uiAlignment, WUInt32 uiBufferSize)
  : m_Tracker(uiAlignment, uiBufferSize)
{
  {
    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.size = uiBufferSize;

    WVulkanAllocationCreateInfo allocCreateInfo;
    allocCreateInfo.m_usage = WVulkanMemoryUsage::Auto;
    allocCreateInfo.m_flags = WVulkanAllocationCreateFlags::HostAccessSequentialWrite | WVulkanAllocationCreateFlags::Mapped | WVulkanAllocationCreateFlags::AllowTransferInstead;
    VK_ASSERT_DEV(WMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_Buffer, m_Alloc, &m_AllocInfo));
  }

  vk::MemoryPropertyFlags memFlags = WMemoryAllocatorVulkan::GetAllocationFlags(m_Alloc);
  if (memFlags & vk::MemoryPropertyFlagBits::eHostVisible)
  {
    m_Data = WMakeArrayPtr(reinterpret_cast<WUInt8*>(m_AllocInfo.m_pMappedData), uiBufferSize);
  }
  else
  {
    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.size = uiBufferSize;

    WVulkanAllocationCreateInfo allocCreateInfo;
    allocCreateInfo.m_usage = WVulkanMemoryUsage::Auto;
    allocCreateInfo.m_flags = WVulkanAllocationCreateFlags::HostAccessSequentialWrite | WVulkanAllocationCreateFlags::Mapped;
    VK_ASSERT_DEV(WMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocCreateInfo, m_StagingBuffer, m_StagingAlloc, &m_StagingAllocInfo));
    m_Data = WMakeArrayPtr(reinterpret_cast<WUInt8*>(m_StagingAllocInfo.m_pMappedData), uiBufferSize);
  }
}


WUniformBufferPoolVulkan::UniformBufferPool::~UniformBufferPool()
{
  m_Data.Clear();
  WMemoryAllocatorVulkan::DestroyBuffer(m_Buffer, m_Alloc);
  if (m_StagingBuffer)
  {
    WMemoryAllocatorVulkan::DestroyBuffer(m_StagingBuffer, m_StagingAlloc);
  }
}

WResult WUniformBufferPoolVulkan::UniformBufferPool::Allocate(WUInt32 uiSize, WUInt64 uiCurrentFrame, WUInt32& out_uiStartOffset, WByteArrayPtr& out_allocation)
{
  if (m_Tracker.Allocate(uiSize, uiCurrentFrame, out_uiStartOffset).Failed())
    return W_FAILURE;

  out_allocation = m_Data.GetSubArray(out_uiStartOffset, uiSize);
  return W_SUCCESS;
}

void WUniformBufferPoolVulkan::UniformBufferPool::Free(WUInt64 uiUpToFrame)
{
  m_Tracker.Free(uiUpToFrame);
}

void WUniformBufferPoolVulkan::UniformBufferPool::Submit(WGALDeviceVulkan* pDevice, WUInt64 uiFrame)
{
  WHybridArray<WRingBufferTracker::FrameData, 4> frameData;
  if (m_Tracker.SubmitFrame(uiFrame, frameData).Failed())
    return;

  for (WRingBufferTracker::FrameData& frame : frameData)
  {
    if (m_StagingBuffer)
    {
      VK_ASSERT_DEBUG(WMemoryAllocatorVulkan::FlushAllocation(m_StagingAlloc, frame.m_uiStartOffset, frame.m_uiSize));
      W_ASSERT_DEBUG(frame.m_uiStartOffset + frame.m_uiSize <= m_Tracker.GetTotalMemory(), "Buffer overrun");
      pDevice->GetInitContext().UpdateDynamicUniformBuffer(m_Buffer, m_StagingBuffer, frame.m_uiStartOffset, frame.m_uiSize);
    }
    else
    {
      VK_ASSERT_DEBUG(WMemoryAllocatorVulkan::FlushAllocation(m_Alloc, frame.m_uiStartOffset, frame.m_uiSize));
      W_ASSERT_DEBUG(frame.m_uiStartOffset + frame.m_uiSize <= m_Tracker.GetTotalMemory(), "Buffer overrun");
      pDevice->GetInitContext().UpdateDynamicUniformBuffer(m_Buffer, m_StagingBuffer, frame.m_uiStartOffset, frame.m_uiSize);
    }
  }
}
