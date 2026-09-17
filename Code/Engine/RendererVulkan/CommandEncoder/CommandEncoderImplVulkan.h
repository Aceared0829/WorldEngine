#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>

class WGALBlendStateVulkan;
class WGALBufferVulkan;
class WGALDepthStencilStateVulkan;
class WGALRasterizerStateVulkan;
class WGALBufferResourceViewVulkan;
class WGALSamplerStateVulkan;
class WGALShaderVulkan;
class WGALDeviceVulkan;
class WFenceQueueVulkan;
class WGALGraphicsPipelineVulkan;
class WGALComputePipelineVulkan;
struct WGALBindGroupCreationDescription;
class WDescriptorWritePoolVulkan;
class WGALBindGroupVulkan;

class W_RENDERERVULKAN_DLL WGALCommandEncoderImplVulkan : public WGALCommandEncoderCommonPlatformInterface
{
public:
  WGALCommandEncoderImplVulkan(WGALDeviceVulkan& ref_device);
  ~WGALCommandEncoderImplVulkan();

  void Reset();

  void EndFrame();
  void SetCurrentCommandBuffer(vk::CommandBuffer* pCommandBuffer);
  void BeforeCommandBufferSubmit();
  void AfterCommandBufferSubmit(vk::Fence submitFence);
  WDescriptorWritePoolVulkan& GetDescriptorWritePool() const;

  // WGALCommandEncoderCommonPlatformInterface
  // State setting functions
  virtual void SetBindGroupPlatform(WUInt32 uiBindGroup, const WGALBindGroupCreationDescription& bindGroup) override;
  virtual void SetBindGroupPlatform(WUInt32 uiBindGroup, const WGALBindGroup* pBindGroup) override;
  virtual void SetPushConstantsPlatform(WArrayPtr<const WUInt8> data) override;

  // GPU -> CPU query functions

  virtual WGALTimestampHandle InsertTimestampPlatform() override;
  virtual WGALOcclusionHandle BeginOcclusionQueryPlatform(WEnum<WGALQueryType> type) override;
  virtual void EndOcclusionQueryPlatform(WGALOcclusionHandle hOcclusion) override;
  virtual WGALFenceHandle InsertFencePlatform() override;


  // Resource update functions

  virtual void CopyBufferPlatform(const WGALBuffer* pDestination, const WGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const WGALBuffer* pDestination, WUInt32 uiDestOffset, const WGALBuffer* pSource, WUInt32 uiSourceOffset, WUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const WGALBuffer* pDestination, WUInt32 uiDestOffset, WArrayPtr<const WUInt8> sourceData, WGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const WGALTexture* pDestination, const WGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource, const WVec3U32& vDestinationPoint, const WGALTexture* pSource, const WGALTextureSubresource& sourceSubResource, const WBoundingBoxu32& box) override;

  virtual void UpdateTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource, const WBoundingBoxu32& destinationBox, const WGALSystemMemoryDescription& sourceData) override;

  virtual void ResolveTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource, const WGALTexture* pSource, const WGALTextureSubresource& sourceSubResource) override;

  virtual void ReadbackTexturePlatform(const WGALReadbackTexture* pDestination, const WGALTexture* pSource) override;
  virtual void ReadbackBufferPlatform(const WGALReadbackBuffer* pDestination, const WGALBuffer* pSource) override;

  void CopyImageToBuffer(const WGALTextureVulkan* pSource, const WGALBufferVulkan* pDestination);
  void CopyImageToBuffer(const WGALTextureVulkan* pSource, vk::Buffer destination);

  // Barriers

  virtual void TextureBarrierPlatform(WArrayPtr<const WGALTextureBarrier> barriers) override;
  virtual void BufferBarrierPlatform(WArrayPtr<const WGALBufferBarrier> barriers) override;

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;


  // WGALCommandEncoderComputePlatformInterface
  // Dispatch
  virtual void BeginComputePlatform() override;
  virtual void EndComputePlatform() override;

  virtual WResult DispatchPlatform(WUInt32 uiThreadGroupCountX, WUInt32 uiThreadGroupCountY, WUInt32 uiThreadGroupCountZ) override;
  virtual WResult DispatchIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes) override;


  // WGALCommandEncoderRenderPlatformInterface
  virtual void BeginRenderingPlatform(const WGALRenderingSetup& renderingSetup) override;
  virtual void EndRenderingPlatform() override;

  // Draw functions

  virtual void ClearPlatform(const WColor& clearColor, WUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, WUInt8 uiStencilClear) override;

  virtual WResult DrawPlatform(WUInt32 uiVertexCount, WUInt32 uiStartVertex) override;
  virtual WResult DrawIndexedPlatform(WUInt32 uiIndexCount, WUInt32 uiStartIndex) override;
  virtual WResult DrawIndexedInstancedPlatform(WUInt32 uiIndexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartIndex) override;
  virtual WResult DrawIndexedInstancedIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes) override;
  virtual WResult DrawInstancedPlatform(WUInt32 uiVertexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartVertex) override;
  virtual WResult DrawInstancedIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes) override;

  // State functions

  virtual void SetIndexBufferPlatform(const WGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(WUInt32 uiSlot, const WGALBuffer* pVertexBuffer, WUInt32 uiOffset) override;

  virtual void SetGraphicsPipelinePlatform(const WGALGraphicsPipeline* pGraphicsPipeline) override;
  virtual void SetComputePipelinePlatform(const WGALComputePipeline* pComputePipeline) override;

  virtual void SetViewportPlatform(const WRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const WRectU32& rect) override;
  virtual void SetStencilReferencePlatform(WUInt8 uiStencilRefValue) override;

  struct Statistics
  {
    WUInt32 m_uiDescriptorSetsCreated = 0;
    WUInt32 m_uiDescriptorSetsUpdated = 0;
    WUInt32 m_uiDescriptorSetsReused = 0;
    WUInt32 m_uiDescriptorWrites = 0;
    WUInt32 m_uiDynamicUniformBufferChanged = 0;
  };
  Statistics GetAndResetStatistics();

private:
  /// To be able to cache descriptor sets, we not only need the bind group description but also the currently used dynamic uniform buffers used by transient constant buffers.
  /// All constant buffers in W are marked as dynamic in the layout so we need to provide offsets for each slot, no mater if a normal or transient constant buffer is bound to a slot.
  struct DynamicOffsets
  {
    WHybridArray<const WGALBufferVulkan*, 6> m_DynamicUniformBuffers; ///< Constant buffers in order of appearance in the bind group. Normal constant buffers write a nullptr here as they have fixed offsets and will never have to be updated. Only updated once via FindDynamicUniformBuffers.
    WHybridArray<vk::Buffer, 6> m_DynamicUniformVkBuffers;             ///< Current vk::Buffer for each dynamic uniform buffer in m_DynamicUniformBuffers. Updated via UpdateDynamicUniformBufferOffsets. If any of these change, a new descriptor has to be created.
    WHybridArray<WUInt32, 6> m_DynamicUniformBufferOffsets;           ///< Offsets in this bind group. Normal constant buffers have fixed offsets determined in FindDynamicUniformBuffers which never change. Transient constant buffer offsets are updated with each UpdateDynamicUniformBufferOffsets call.
  };

  WResult FlushDeferredStateChanges();
  void MarkAllStateDirty();
  void FindDynamicUniformBuffers(const WGALBindGroupCreationDescription& desc, DynamicOffsets& out_offsets);
  static WUInt64 HashBindGroup(const WGALBindGroupCreationDescription& desc, const DynamicOffsets& offsets);
  vk::DescriptorSet CreateDescriptorSet(const WGALBindGroupCreationDescription& desc, const DynamicOffsets& offsets);
  void EnsureBindGroupTextureLayout(const WGALBindGroupCreationDescription& desc);

  enum class DynamicUniformBufferChanges
  {
    None,           ///< Neither offsets nor buffers have changed.
    OffsetsChanged, ///< Offsets have changed, call bindDescriptorSets with new offsets.
    BuffersChanged, ///< Buffers have changed, create new descriptor set for new buffers. This should only happen if we exhaust the current dynamic uniform buffer and request a new one from the pool.
  };
  DynamicUniformBufferChanges UpdateDynamicUniformBufferOffsets(DynamicOffsets& ref_offsets);

private:
  WGALDeviceVulkan& m_GALDeviceVulkan;
  vk::Device m_VkDevice;

  vk::CommandBuffer* m_pCommandBuffer = nullptr;

  WUniquePtr<WUniformBufferPoolVulkan> m_pUniformBufferPool;

  // Cache flags.
  bool m_bPipelineStateDirty = true;
  bool m_bViewportDirty = true;
  bool m_bScissorDirty = true;
  bool m_bStencilRefDirty = false;
  bool m_bIndexBufferDirty = false;
  bool m_BindGroupDirty[W_GAL_MAX_BIND_GROUPS] = {};
  bool m_bDynamicOffsetsDirty = false;
  WGAL::ModifiedRange m_BoundVertexBuffersRange;
  bool m_bInsideCompute = false; ///< Within BeginCompute / EndCompute block.
  bool m_bPushConstantsDirty = false;

  // Bound objects for deferred state flushes
  const WGALShaderVulkan* m_pShader = nullptr;
  const WGALGraphicsPipelineVulkan* m_pGraphicsPipeline = nullptr;
  const WGALComputePipelineVulkan* m_pComputePipeline = nullptr;

  vk::RenderPassBeginInfo m_RenderPass;
  WHybridArray<vk::ClearValue, W_GAL_MAX_RENDERTARGET_COUNT + 1> m_ClearValues;
  vk::ImageAspectFlags m_DepthMask = {};
  WUInt32 m_uiLayers = 0;

  vk::Viewport m_Viewport;
  vk::Rect2D m_Scissor;
  bool m_bScissorEnabled = false;
  WUInt8 m_uiStencilRefValue = 0;

  const WGALBufferVulkan* m_pIndexBuffer = nullptr;
  vk::Buffer m_pBoundVertexBuffers[W_GAL_MAX_VERTEX_BUFFER_COUNT];
  vk::DeviceSize m_VertexBufferOffsets[W_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  // Bind Groups
  WGALBindGroupCreationDescription m_BindGroups[W_GAL_MAX_BIND_GROUPS];
  const WGALBindGroupVulkan* m_pBindGroups[W_GAL_MAX_BIND_GROUPS] = {};
  DynamicOffsets m_DynamicOffsets[W_GAL_MAX_BIND_GROUPS];

  // Descriptor Writes
  mutable WUniquePtr<WDescriptorWritePoolVulkan> m_pWritePool;

  // Actual bound descriptor sets
  WHashTable<WUInt64, vk::DescriptorSet> m_DescriptorCache;
  vk::DescriptorSet m_DescriptorSets[W_GAL_MAX_BIND_GROUPS];

  WDynamicArray<WUInt8> m_PushConstants;

  Statistics m_Statistics;
};
