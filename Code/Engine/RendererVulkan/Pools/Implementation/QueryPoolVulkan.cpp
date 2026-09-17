#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/QueryPoolVulkan.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererVulkan/Device/DeviceVulkan.h>

WQueryPoolVulkan::WQueryPoolVulkan(WGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
  , m_TimestampPool(pDevice->GetAllocator())
  , m_OcclusionPool(pDevice->GetAllocator())
{
}

void WQueryPoolVulkan::Initialize(WUInt32 uiValidBits)
{
  m_TimestampPool.Initialize(m_pDevice->GetVulkanDevice(), 256, vk::QueryType::eTimestamp);
  m_OcclusionPool.Initialize(m_pDevice->GetVulkanDevice(), 32, vk::QueryType::eOcclusion);

  W_ASSERT_DEV(m_pDevice->GetPhysicalDeviceProperties().limits.timestampComputeAndGraphics, "Timestamps not supported by hardware.");
  m_fNanoSecondsPerTick = m_pDevice->GetPhysicalDeviceProperties().limits.timestampPeriod;

  m_uiValidBitsMask = uiValidBits == 64 ? WMath::MaxValue<WUInt64>() : ((1ull << uiValidBits) - 1);
}

void WQueryPoolVulkan::DeInitialize()
{
  m_TimestampPool.DeInitialize();
  m_OcclusionPool.DeInitialize();
}

WQueryPoolVulkan::Pool::Pool(WAllocator* pAllocator)
  : m_pendingFrames(pAllocator)
  , m_resetPools(pAllocator)
  , m_freePools(pAllocator)
{
}

void WQueryPoolVulkan::Pool::Initialize(vk::Device device, WUInt32 uiPoolSize, vk::QueryType queryType)
{
  m_device = device;
  m_uiPoolSize = uiPoolSize;
  m_QueryType = queryType;
}

void WQueryPoolVulkan::Pool::DeInitialize()
{
  for (FramePool& framePool : m_pendingFrames)
  {
    for (WUInt32 i = 0; i < framePool.m_pools.GetCount(); i++)
    {
      m_freePools.PushBack(framePool.m_pools[i]);
    }
  }
  m_pendingFrames.Clear();
  for (QueryPool* pPool : m_freePools)
  {
    m_device.destroyQueryPool(pPool->m_pool);
    W_DEFAULT_DELETE(pPool);
  }
  m_freePools.Clear();
}

void WQueryPoolVulkan::Calibrate()
{
  W_PROFILE_SCOPE("Calibrate");
  // #TODO_VULKAN Replace with VK_KHR_calibrated_timestamps
  // Prefer a host-signaled timeline semaphore so the GPU can wait on the host.
  if (m_pDevice->GetExtensions().m_bTimelineSemaphore)
  {
    vk::SemaphoreTypeCreateInfo timelineTypeInfo;
    timelineTypeInfo.semaphoreType = vk::SemaphoreType::eTimeline;
    timelineTypeInfo.initialValue = 0;

    vk::SemaphoreCreateInfo semCreateInfo;
    semCreateInfo.pNext = &timelineTypeInfo;

    vk::Semaphore timelineSemaphore;
    VK_ASSERT_DEV(m_TimestampPool.m_device.createSemaphore(&semCreateInfo, nullptr, &timelineSemaphore));

    m_TimestampPool.m_device.waitIdle();
    vk::CommandBuffer cb = m_pDevice->GetCurrentCommandBuffer();

    // Insert the timestamp into the command buffer which will wait for the
    // timeline semaphore to be signaled by the host before executing.
    auto hTimestamp = InsertTimestamp(cb, vk::PipelineStageFlagBits::eTopOfPipe);

    m_pDevice->AddWaitSemaphore(WGALDeviceVulkan::SemaphoreInfo::MakeWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eTopOfPipe, vk::SemaphoreType::eTimeline, 1));

    vk::Fence fence = m_pDevice->Submit();

    // Signal the timeline semaphore from the host to let the GPU continue.
    WThreadUtils::Sleep(WTime::Milliseconds(10));

    vk::SemaphoreSignalInfo signalInfo;
    signalInfo.semaphore = timelineSemaphore;
    signalInfo.value = 1;
    VK_ASSERT_DEV(m_TimestampPool.m_device.signalSemaphore(&signalInfo, m_pDevice->GetDispatchContext()));
    const WTime systemTS = WTime::Now();

    VK_ASSERT_DEV(m_TimestampPool.m_device.waitForFences(1, &fence, true, WMath::MaxValue<WUInt64>()));

    WTime gpuTS;
    W_VERIFY(GetTimestampResult(hTimestamp, gpuTS, true) == WGALAsyncResult::Ready, "");
    m_GpuToCpuDelta = systemTS - gpuTS;

    m_TimestampPool.m_device.destroySemaphore(timelineSemaphore);
  }
  else
  {
    // Fallback for hardware without timeline semaphore support:
    // Insert a timestamp, submit and wait for the fence, then sample host time.
    // This provides a usable (though possibly less precise) GPU->CPU delta.
    m_TimestampPool.m_device.waitIdle();
    vk::CommandBuffer cb = m_pDevice->GetCurrentCommandBuffer();
    auto hTimestamp = InsertTimestamp(cb, vk::PipelineStageFlagBits::eTopOfPipe);
    vk::Fence fence = m_pDevice->Submit();

    VK_ASSERT_DEV(m_TimestampPool.m_device.waitForFences(1, &fence, true, WMath::MaxValue<WUInt64>()));

    const WTime systemTS = WTime::Now();
    WTime gpuTS;
    W_VERIFY(GetTimestampResult(hTimestamp, gpuTS, true) == WGALAsyncResult::Ready, "");
    m_GpuToCpuDelta = systemTS - gpuTS;
  }
}

void WQueryPoolVulkan::AfterBeginFrame(vk::CommandBuffer commandBuffer)
{
  WUInt64 uiCurrentFrame = m_pDevice->GetCurrentFrame();
  WUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();
  {
    W_PROFILE_SCOPE("TimestampPool");
    m_TimestampPool.BeginFrame(commandBuffer, uiCurrentFrame, uiSafeFrame);
  }
  {
    W_PROFILE_SCOPE("OcclusionPool");
    m_OcclusionPool.BeginFrame(commandBuffer, uiCurrentFrame, uiSafeFrame);
  }
  if (m_GpuToCpuDelta.IsZero())
  {
    Calibrate();
  }
}

void WQueryPoolVulkan::BeginRenderPass(vk::CommandBuffer commandBuffer)
{
  m_bInsideRenderPass = true;
  m_OcclusionPool.EnsureFreeQueryPoolSize(commandBuffer, 1);
  m_TimestampPool.EnsureFreeQueryPoolSize(commandBuffer, 3);
}

void WQueryPoolVulkan::Pool::BeginFrame(vk::CommandBuffer commandBuffer, WUInt64 uiCurrentFrame, WUInt64 uiSafeFrame)
{
  m_pCurrentFrame = &m_pendingFrames.ExpandAndGetRef();
  m_pCurrentFrame->m_uiFrameCounter = uiCurrentFrame;
  m_pCurrentFrame->m_uiNextIndex = 0;

  // Get results
  for (FramePool& framePool : m_pendingFrames)
  {
    // Skip checking if the corresponding frame fence has not been reached.
    if (framePool.m_uiFrameCounter > uiSafeFrame)
      break;

    bool bAllCompleted = true;
    for (WUInt32 i = 0; i < framePool.m_pools.GetCount(); i++)
    {
      QueryPool* pPool = framePool.m_pools[i];

      if (!pPool->m_bReady)
      {
        bAllCompleted = false;
        WUInt32 uiQueryCount = m_uiPoolSize;
        // The last pool may not be full, so we only query the number of queries that were actually used.
        const bool bLastPool = i + 1 == framePool.m_pools.GetCount();
        const WUInt64 uiPoolEndIndex = m_uiPoolSize * (i + 1);
        const bool bPoolFull = framePool.m_uiNextIndex >= uiPoolEndIndex;
        if (bLastPool && !bPoolFull)
          uiQueryCount = framePool.m_uiNextIndex % m_uiPoolSize;

        W_ASSERT_DEV(uiQueryCount != 0, "Query pools are created on demand, so they should never have zero queries in them!");
        W_PROFILE_SCOPE("getQueryPoolResults");
        vk::Result res = m_device.getQueryPoolResults(pPool->m_pool, 0, uiQueryCount, m_uiPoolSize * sizeof(WUInt64), pPool->m_queryResults.GetData(), sizeof(WUInt64), vk::QueryResultFlagBits::e64);
        if (res == vk::Result::eSuccess)
        {
          pPool->m_bReady = true;
        }
      }
    }

    if (bAllCompleted)
    {
      framePool.m_uiReadyFrames++;
    }
  }

  // Clear out old frames
  while (m_pendingFrames[0].m_uiReadyFrames > s_uiRetainFrames)
  {
    for (QueryPool* pPool : m_pendingFrames[0].m_pools)
    {
      pPool->m_bReady = false;
      pPool->m_queryResults.Clear();
      pPool->m_queryResults.SetCount(m_uiPoolSize, 0);
      m_freePools.PushBack(pPool);
      m_resetPools.PushBack(pPool->m_pool);
    }
    m_pendingFrames.PopFront();
  }

  m_uiFirstFrameIndex = m_pendingFrames[0].m_uiFrameCounter;

  for (vk::QueryPool pool : m_resetPools)
  {
    commandBuffer.resetQueryPool(pool, 0, m_uiPoolSize);
  }
  m_resetPools.Clear();
}

void WQueryPoolVulkan::Pool::EnsureFreeQueryPoolSize(vk::CommandBuffer commandBuffer, WUInt32 uiFreePools)
{
  while (m_freePools.GetCount() < uiFreePools)
  {
    m_freePools.PushBack(CreatePool());
  }
  for (vk::QueryPool pool : m_resetPools)
  {
    commandBuffer.resetQueryPool(pool, 0, m_uiPoolSize);
  }
  m_resetPools.Clear();
}

WGALTimestampHandle WQueryPoolVulkan::InsertTimestamp(vk::CommandBuffer commandBuffer, vk::PipelineStageFlagBits pipelineStage)
{
  WGALTimestampHandle hTimestamp = m_TimestampPool.CreateQuery(commandBuffer, m_bInsideRenderPass);
  Query query = m_TimestampPool.GetQuery(hTimestamp);
  commandBuffer.writeTimestamp(pipelineStage, query.m_pool, query.uiQueryIndex);

  return hTimestamp;
}

WGALPoolHandle WQueryPoolVulkan::Pool::CreateQuery(vk::CommandBuffer commandBuffer, bool bInsideRenderPass)
{
  const WUInt64 uiPoolIndex = m_pCurrentFrame->m_uiNextIndex / m_uiPoolSize;
  if (uiPoolIndex == m_pCurrentFrame->m_pools.GetCount())
  {
    W_ASSERT_DEV(!bInsideRenderPass || !m_freePools.IsEmpty(), "Ran out of pre-allocated query pools inside a render pass. A fresh pool cannot be reset here, EnsureFreeQueryPoolSize has to reserve more.");
    m_pCurrentFrame->m_pools.PushBack(GetFreePool());
  }

  WGALPoolHandle hPool = {m_pCurrentFrame->m_uiNextIndex, m_pCurrentFrame->m_uiFrameCounter};
  m_pCurrentFrame->m_uiNextIndex++;

  if (!bInsideRenderPass)
  {
    for (vk::QueryPool pool : m_resetPools)
    {
      commandBuffer.resetQueryPool(pool, 0, m_uiPoolSize);
    }
    m_resetPools.Clear();
  }

  return hPool;
}

WQueryPoolVulkan::Query WQueryPoolVulkan::Pool::GetQuery(WGALPoolHandle hPool)
{
  W_ASSERT_DEBUG(hPool.m_Generation == m_pCurrentFrame->m_uiFrameCounter, "Timestamps must be created and used in the same frame!");
  const WUInt32 uiPoolIndex = (WUInt32)hPool.m_InstanceIndex / m_uiPoolSize;
  const WUInt32 uiQueryIndex = (WUInt32)hPool.m_InstanceIndex % m_uiPoolSize;
  return {m_pCurrentFrame->m_pools[uiPoolIndex]->m_pool, uiQueryIndex};
}

WEnum<WGALAsyncResult> WQueryPoolVulkan::GetTimestampResult(WGALTimestampHandle hTimestamp, WTime& out_result, bool bForce)
{
  WUInt64 out_uiResult;
  WEnum<WGALAsyncResult> res = m_TimestampPool.GetResult(hTimestamp, out_uiResult, bForce);
  if (res == WGALAsyncResult::Ready)
  {
    out_uiResult &= m_uiValidBitsMask;
    out_result = WTime::Nanoseconds(m_fNanoSecondsPerTick * out_uiResult) + m_GpuToCpuDelta;
  }
  else
  {
    out_result = WTime();
  }
  return res;
}

WGALPoolHandle WQueryPoolVulkan::BeginOcclusionQuery(vk::CommandBuffer commandBuffer, WEnum<WGALQueryType> type)
{
  WGALPoolHandle hPool = m_OcclusionPool.CreateQuery(commandBuffer, m_bInsideRenderPass);
  Query query = m_OcclusionPool.GetQuery(hPool);
  commandBuffer.beginQuery(query.m_pool, query.uiQueryIndex, type == WGALQueryType::NumSamplesPassed ? vk::QueryControlFlagBits::ePrecise : (vk::QueryControlFlagBits)0);

  return hPool;
}

void WQueryPoolVulkan::EndOcclusionQuery(vk::CommandBuffer commandBuffer, WGALPoolHandle hPool)
{
  Query query = m_OcclusionPool.GetQuery(hPool);
  commandBuffer.endQuery(query.m_pool, query.uiQueryIndex);
}

WEnum<WGALAsyncResult> WQueryPoolVulkan::GetOcclusionQueryResult(WGALPoolHandle hPool, WUInt64& out_uiQueryResult, bool bForce)
{
  return m_OcclusionPool.GetResult(hPool, out_uiQueryResult, bForce);
}

WEnum<WGALAsyncResult> WQueryPoolVulkan::Pool::GetResult(WGALPoolHandle hPool, WUInt64& out_uiResult, bool bForce)
{
  out_uiResult = 0;
  if (hPool.m_Generation >= m_uiFirstFrameIndex)
  {
    const WUInt32 uiFrameIndex = static_cast<WUInt32>(hPool.m_Generation - m_uiFirstFrameIndex);
    const WUInt32 uiPoolIndex = (WUInt32)hPool.m_InstanceIndex / m_uiPoolSize;
    const WUInt32 uiQueryIndex = (WUInt32)hPool.m_InstanceIndex % m_uiPoolSize;
    FramePool& framePools = m_pendingFrames[uiFrameIndex];
    QueryPool* pPool = framePools.m_pools[uiPoolIndex];
    if (pPool->m_bReady)
    {
      out_uiResult = pPool->m_queryResults[uiQueryIndex];
      return WGALAsyncResult::Ready;
    }
    else if (bForce)
    {
      vk::Result res = m_device.getQueryPoolResults(pPool->m_pool, uiQueryIndex, 1, sizeof(WUInt64), &pPool->m_queryResults[uiQueryIndex], sizeof(WUInt64), vk::QueryResultFlagBits::e64);
      if (res == vk::Result::eSuccess)
      {
        out_uiResult = pPool->m_queryResults[uiQueryIndex];
        return WGALAsyncResult::Ready;
      }
    }
    return WGALAsyncResult::Pending;
  }
  else
  {
    return WGALAsyncResult::Expired;
  }
}

WQueryPoolVulkan::QueryPool* WQueryPoolVulkan::Pool::GetFreePool()
{
  if (!m_freePools.IsEmpty())
  {
    QueryPool* pPool = m_freePools.PeekBack();
    m_freePools.PopBack();
    return pPool;
  }

  return CreatePool();
}

WQueryPoolVulkan::QueryPool* WQueryPoolVulkan::Pool::CreatePool()
{
  vk::QueryPoolCreateInfo info;
  info.queryType = m_QueryType;
  info.queryCount = m_uiPoolSize;

  QueryPool* pPool = W_DEFAULT_NEW(QueryPool);
  pPool->m_queryResults.SetCount(m_uiPoolSize, 0);
  VK_ASSERT_DEV(m_device.createQueryPool(&info, nullptr, &pPool->m_pool));

  m_resetPools.PushBack(pPool->m_pool);
  return pPool;
}
