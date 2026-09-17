#pragma once

#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class WGALDeviceVulkan;

/// Pool for GPU queries.
class W_RENDERERVULKAN_DLL WQueryPoolVulkan
{
public:
  WQueryPoolVulkan(WGALDeviceVulkan* pDevice);

  /// Initializes the pool.
  /// \param uiValidBits The number of valid bits in the query result. Each queue has different query characteristics and a separate pool is needed for each queue.
  void Initialize(WUInt32 uiValidBits);
  void DeInitialize();

  /// Needs to be called every frame so the pool can figure out which queries have finished and reuse old data.
  void AfterBeginFrame(vk::CommandBuffer commandBuffer);

  /// We have to call this before each begin rendering call as if we run out of queries inside a render pass, we can't recover given that resetQueryPool can only be called outside a render pass which is necessary to be called on every new pool.
  void BeginRenderPass(vk::CommandBuffer commandBuffer);

  /// Must track the render pass state because vkCmdResetQueryPool is only legal outside a render pass instance.
  void EndRenderPass() { m_bInsideRenderPass = false; }

  /// Inserts a timestamp into the given command buffer.
  /// \param commandBuffer Target command buffer to insert the timestamp into.
  /// \param hTimestamp Timestamp to insert. After insertion the only valid option is to call GetTimestampResult.
  /// \param pipelineStage The value of the timestamp will be the point in time in which all previously committed commands have finished this stage.
  WGALTimestampHandle InsertTimestamp(vk::CommandBuffer commandBuffer, vk::PipelineStageFlagBits pipelineStage = vk::PipelineStageFlagBits::eBottomOfPipe);

  /// Retrieves the timestamp value if it is available.
  /// \param hTimestamp The target timestamp to resolve.
  /// \param result The time of the timestamp. If this is empty on success the timestamp has expired.
  /// \param bForce Wait for the timestamp to become available.
  /// \return Returns false if the result is not available yet.
  WEnum<WGALAsyncResult> GetTimestampResult(WGALTimestampHandle hTimestamp, WTime& out_result, bool bForce = false);

  WGALPoolHandle BeginOcclusionQuery(vk::CommandBuffer commandBuffer, WEnum<WGALQueryType> type);
  void EndOcclusionQuery(vk::CommandBuffer commandBuffer, WGALPoolHandle hPool);
  WEnum<WGALAsyncResult> GetOcclusionQueryResult(WGALPoolHandle hPool, WUInt64& out_uiQueryResult, bool bForce = false);

private:
  /// GPU and CPU timestamps have no relation in Vulkan. To establish it we need to measure the same timestamp in both and compute the difference.
  void Calibrate();

  static constexpr WUInt32 s_uiRetainFrames = 4;

  /// A singular query
  struct Query
  {
    vk::QueryPool m_pool;
    WUInt32 uiQueryIndex;
  };

  /// A fixed number of queries are stored in this pool (see m_uiPoolSize below)
  /// These can be reset queried and reset as a whole. A good compromise must be found between API overhead and resource waste.
  struct QueryPool
  {
    vk::QueryPool m_pool;
    bool m_bReady = false;
    WDynamicArray<uint64_t> m_queryResults;
  };

  /// Represents a frame of queries. Depending on the number of queries, multiple QueryPools will need to be used per frame.
  struct FramePool
  {
    WUInt64 m_uiNextIndex = 0;    // Next query index in this frame. Use % and / to find the pool / element index.
    WUInt64 m_uiFrameCounter = 0; // WGALDevice::GetCurrentFrame
    WUInt8 m_uiReadyFrames = 0;   ///< How many frames ago m_bReady was set on the last QueryPool in m_pools. Used to make sure frames are retained for s_uiRetainFrames after results become available.
    WHybridArray<QueryPool*, 2> m_pools;
  };

  /// High level pool of Vulkan queries. Will create more sub-pools to manage demand, reusing them after s_uiRetainFrames of available results.
  /// If the user does not retrieve the values within s_uiRetainFrames time, the result expires.
  struct Pool
  {
    Pool(WAllocator* pAllocator);

    void Initialize(vk::Device device, WUInt32 uiPoolSize, vk::QueryType queryType);
    void DeInitialize();

    QueryPool* GetFreePool();
    QueryPool* CreatePool();
    void BeginFrame(vk::CommandBuffer commandBuffer, WUInt64 uiCurrentFrame, WUInt64 uiSafeFrame);
    void EnsureFreeQueryPoolSize(vk::CommandBuffer commandBuffer, WUInt32 uiFreePools);
    WGALPoolHandle CreateQuery(vk::CommandBuffer commandBuffer, bool bInsideRenderPass);
    Query GetQuery(WGALPoolHandle hPool);
    WEnum<WGALAsyncResult> GetResult(WGALPoolHandle hPool, WUInt64& out_uiResult, bool bForce);

    // Current active data.
    FramePool* m_pCurrentFrame = nullptr;
    WUInt64 m_uiFirstFrameIndex = 0;
    WDeque<FramePool> m_pendingFrames;

    // Pools.
    WHybridArray<vk::QueryPool, 8> m_resetPools;
    WHybridArray<QueryPool*, 8> m_freePools;

    vk::Device m_device;
    WUInt32 m_uiPoolSize = 0;
    vk::QueryType m_QueryType;
  };

  WGALDeviceVulkan* m_pDevice = nullptr;
  bool m_bInsideRenderPass = false;

  // Timestamp conversion and calibration data.
  double m_fNanoSecondsPerTick = 0;
  WUInt64 m_uiValidBitsMask = 0;
  WTime m_GpuToCpuDelta;

  // Pools
  Pool m_TimestampPool;
  Pool m_OcclusionPool;
};
