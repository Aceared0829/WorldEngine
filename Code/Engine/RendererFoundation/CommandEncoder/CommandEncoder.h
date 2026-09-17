
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

#define W_BARRIER_VALIDATION W_COMPILE_FOR_DEBUG

#if W_ENABLED(W_BARRIER_VALIDATION)
#  include <RendererFoundation/Shader/BindGroup.h>
#  include <RendererFoundation/Utils/ResourceStateTracker.h>
#endif

struct WGALRenderingSetup;
struct WGALDeviceEvent;
class WGALShader;
struct WGALBindGroupCreationDescription;

struct WTextureValidationError : public WHashableStruct<WTextureValidationError>
{
  WUInt32 m_uiBindGroup = 0;
  WHashedString m_sBinding;
  WGALTextureHandle m_hTexture;
  WBitflags<WGALResourceState> m_expectedState;
  WBitflags<WGALShaderStageFlags> m_expectedStages;
  WGALTextureSubresource m_failedSubResource;
  WBitflags<WGALResourceState> m_actualState;
  WBitflags<WGALShaderStageFlags> m_actualStages;
};

struct WBufferValidationError : public WHashableStruct<WBufferValidationError>
{
  WUInt32 m_uiBindGroup = 0;
  WHashedString m_sBinding;
  WGALBufferHandle m_hBuffer;
  WBitflags<WGALResourceState> m_expectedState;
  WBitflags<WGALShaderStageFlags> m_expectedStages;
  WBitflags<WGALResourceState> m_actualState;
  WBitflags<WGALShaderStageFlags> m_actualStages;
};

class W_RENDERERFOUNDATION_DLL WGALCommandEncoder
{
  W_DISALLOW_COPY_AND_ASSIGN(WGALCommandEncoder);

public:
  WGALCommandEncoder(WGALDevice& ref_device, WGALCommandEncoderCommonPlatformInterface& ref_commonImpl);
  virtual ~WGALCommandEncoder();

  // State setting functions

  /// Sets a bind group to the given bind group index. Preferably, bindGroup should be created via WBindGroupBuilder::CreateBindGroup.
  ///
  /// This function binds a collection of resources (buffers, textures, samplers) to a specific bind group index. In debug builds, it performs extensive validation of each WGALBindGroupItem against the layout's WShaderResourceBinding to ensure:
  ///
  /// **General Validation:**
  /// - Bind group layout matches the number of provided items
  /// - All resource handles are valid (non-null)
  ///
  /// **Buffer Validation:**
  /// - Buffer handles are valid and match expected binding types
  /// - Constant buffers: No texel format override, zero offset, full buffer size
  /// - Structured/Texel/ByteAddress buffers: Proper usage flags, correct element alignment
  /// - Buffer ranges: Offset/size alignment with element boundaries, bounds checking
  /// - Access permissions: SRV/UAV flags match buffer usage flags
  ///
  /// **Texture Validation:**
  /// - Texture handles are valid with proper view format compatibility
  /// - Array slice counts match binding requirements (e.g. 1 for non-arrays, multiple of 6 for cubes)
  /// - MSAA sample counts match between texture and binding
  /// - Access permissions: SRV/UAV flags match texture capabilities
  /// - Texture ranges: Mip levels and array slices within bounds
  /// - Make sure no proxy texture is present
  ///
  /// **Resource State Validation:**
  /// - Constant buffer bindings must be in WGALResourceState::ConstantBuffer
  /// - SRV texture and buffer bindings must be in WGALResourceState::ShaderResource, except depth textures which must be in WGALResourceState::DepthStencilRead
  /// - UAV texture and buffer bindings must be in WGALResourceState::UnorderedAccess
  ///
  /// \param uiBindGroup The bind group set index to set
  /// \param bindGroup Description containing the layout and resource items to bind
  void SetBindGroup(WUInt32 uiBindGroup, const WGALBindGroupCreationDescription& bindGroup);

  /// Sets a bind group resource to the given bind group index.
  /// As there are two functions to set bind groups (this one and the overload for transient bind groups) the last call takes precedence if both functions are called for the same index.
  /// Resources in hBindGroup must be in the states required by their shader binding when the bind group is used by a draw or dispatch call.
  /// \param uiBindGroup The bind group set index to set
  /// \param hBindGroup Handle to the bind group that is to be used.
  void SetBindGroup(WUInt32 uiBindGroup, WGALBindGroupHandle hBindGroup);

  void SetPushConstants(WArrayPtr<const WUInt8> data);

  // GPU -> CPU query functions

  /// Inserts a timestamp.
  /// \return A handle to be passed into WGALDevice::GetTimestampResult.
  WGALTimestampHandle InsertTimestamp();

  /// Starts an occlusion query.
  /// This function must be called within a render scope and EndOcclusionQuery must be called within the same scope. Only one occlusion query can be active at any given time.
  /// \param type The type of the occlusion query.
  /// \return A handle to be passed into EndOcclusionQuery.
  /// \sa EndOcclusionQuery
  WGALOcclusionHandle BeginOcclusionQuery(WEnum<WGALQueryType> type);

  /// Ends an occlusion query.
  /// The given handle must afterwards be passed into the WGALDevice::GetOcclusionQueryResult function, which needs to be repeated every frame until results are ready.
  /// \param hOcclusion Value returned by the previous call to BeginOcclusionQuery.
  /// \sa WGALDevice::GetOcclusionQueryResult
  void EndOcclusionQuery(WGALOcclusionHandle hOcclusion);

  /// Inserts a fence.
  /// You need to flush commands to the GPU in order to be able to wait for a fence by either ending a frame or calling `WCommandEncoder::Flush` explicitly.
  /// \return A handle to be passed into WGALDevice::GetFenceResult.
  /// \sa WGALDevice::GetFenceResult
  WGALFenceHandle InsertFence();

  // Update functions

  /// hDest must be in WGALResourceState::CopyDestination. hSource must be in WGALResourceState::CopySource.
  void CopyBuffer(WGALBufferHandle hDest, WGALBufferHandle hSource);

  /// hDest must be in WGALResourceState::CopyDestination. hSource must be in WGALResourceState::CopySource.
  void CopyBufferRegion(WGALBufferHandle hDest, WUInt32 uiDestOffset, WGALBufferHandle hSource, WUInt32 uiSourceOffset, WUInt32 uiByteCount);

  /// Updates a buffer region. TransientConstantBuffer is only allowed on transient constant buffers. AheadOfTime means the update happens before the encoder commands are executed. No state prerequisites are required.
  void UpdateBuffer(WGALBufferHandle hDest, WUInt32 uiDestOffset, WArrayPtr<const WUInt8> sourceData, WGALUpdateMode::Enum updateMode = WGALUpdateMode::TransientConstantBuffer);

  /// hDest must be in WGALResourceState::CopyDestination. hSource must be in WGALResourceState::CopySource.
  void CopyTexture(WGALTextureHandle hDest, WGALTextureHandle hSource);

  /// destinationSubResource of hDest must be in WGALResourceState::CopyDestination. sourceSubResource of hSource must be in WGALResourceState::CopySource.
  void CopyTextureRegion(WGALTextureHandle hDest, const WGALTextureSubresource& destinationSubResource, const WVec3U32& vDestinationPoint, WGALTextureHandle hSource, const WGALTextureSubresource& sourceSubResource, const WBoundingBoxu32& box);

  /// Updates a texture region. Similar to UpdateBuffer with AheadOfTime. No state prerequisites are required.
  void UpdateTexture(WGALTextureHandle hDest, const WGALTextureSubresource& destinationSubResource, const WBoundingBoxu32& destinationBox, const WGALSystemMemoryDescription& sourceData);

  /// destinationSubResource of hDest must be in WGALResourceState::ResolveDestination. sourceSubResource of hSource must be in WGALResourceState::ResolveSource.
  void ResolveTexture(WGALTextureHandle hDest, const WGALTextureSubresource& destinationSubResource, WGALTextureHandle hSource, const WGALTextureSubresource& sourceSubResource);

  /// hSource must be in WGALResourceState::CopySource.
  void ReadbackTexture(WGALReadbackTextureHandle hDestination, WGALTextureHandle hSource);

  /// hSource must be in WGALResourceState::CopySource.
  void ReadbackBuffer(WGALReadbackBufferHandle hDestination, WGALBufferHandle hSource);

  // Barriers

  /// Inserts resource barriers for texture state transitions.
  ///
  /// All barriers in a single call are batched into one API-level barrier command.
  /// Must be called outside of rendering and compute scopes.
  /// Each texture must currently be in the barrier's m_StateBefore state.
  void TextureBarrier(WArrayPtr<const WGALTextureBarrier> barriers);

  /// Inserts a single texture barrier for a layout/state transition.
  /// Must be called outside of rendering and compute scopes.
  /// hTexture must currently be in stateBefore.
  void TextureBarrier(
    WGALTextureHandle hTexture,
    WGALTextureRange range = {},
    WBitflags<WGALResourceState> stateBefore = WGALResourceState::Default,
    WBitflags<WGALResourceState> stateAfter = WGALResourceState::Default,
    WBitflags<WGALShaderStageFlags> stagesBefore = WGALShaderStageFlags::Auto,
    WBitflags<WGALShaderStageFlags> stagesAfter = WGALShaderStageFlags::Auto);

  /// Inserts resource barriers for buffer state transitions.
  ///
  /// All barriers in a single call are batched into one API-level barrier command.
  /// Must be called outside of rendering and compute scopes.
  /// Each buffer must currently be in the barrier's m_StateBefore state.
  void BufferBarrier(WArrayPtr<const WGALBufferBarrier> barriers);

  /// Inserts a single buffer barrier for a state transition.
  /// Must be called outside of rendering and compute scopes.
  /// hBuffer must currently be in stateBefore.
  void BufferBarrier(
    WGALBufferHandle hBuffer,
    WBitflags<WGALResourceState> stateBefore = WGALResourceState::Default,
    WBitflags<WGALResourceState> stateAfter = WGALResourceState::Default,
    WBitflags<WGALShaderStageFlags> stagesBefore = WGALShaderStageFlags::Auto,
    WBitflags<WGALShaderStageFlags> stagesAfter = WGALShaderStageFlags::Auto);

  // Misc

  /// Submits all pending work to the GPU.
  /// Call this if you want to wait for a fence or some other kind of GPU synchronization to take place to ensure the work is actually submitted to the GPU.
  void Flush();

  // Debug helper functions

  void PushMarker(const char* szMarker);
  void PopMarker();
  void InsertEventMarker(const char* szMarker);

  // Dispatch

  void BeginCompute(const char* szName = "");
  void EndCompute();

  WResult Dispatch(WUInt32 uiThreadGroupCountX, WUInt32 uiThreadGroupCountY, WUInt32 uiThreadGroupCountZ);
  /// hIndirectArgumentBuffer must be in WGALResourceState::DrawIndirect.
  WResult DispatchIndirect(WGALBufferHandle hIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes);

  // Draw functions

  /// Color targets in renderingSetup must be in WGALResourceState::RenderTarget. The depth target must be in WGALResourceState::DepthStencilRead or WGALResourceState::DepthStencilWrite depending on the render target view.
  void BeginRendering(const WGALRenderingSetup& renderingSetup, const char* szName = "");
  void EndRendering();
  bool IsInRenderingScope() const;

  /// Clears active rendertargets.
  ///
  /// \param uiRenderTargetClearMask
  ///   Each bit represents a bound color target. If all bits are set, all bound color targets will be cleared.
  void Clear(const WColor& clearColor, WUInt32 uiRenderTargetClearMask = 0xFFFFFFFFu, bool bClearDepth = true, bool bClearStencil = true, float fDepthClear = 1.0f, WUInt8 uiStencilClear = 0x0u);

  /// Bound vertex buffers must be in WGALResourceState::VertexBuffer.
  WResult Draw(WUInt32 uiVertexCount, WUInt32 uiStartVertex);

  /// Bound vertex buffers must be in WGALResourceState::VertexBuffer. The bound index buffer must be in WGALResourceState::IndexBuffer.
  WResult DrawIndexed(WUInt32 uiIndexCount, WUInt32 uiStartIndex);

  /// Bound vertex buffers must be in WGALResourceState::VertexBuffer. The bound index buffer must be in WGALResourceState::IndexBuffer.
  WResult DrawIndexedInstanced(WUInt32 uiIndexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartIndex);

  /// Bound vertex buffers must be in WGALResourceState::VertexBuffer. The bound index buffer must be in WGALResourceState::IndexBuffer. hIndirectArgumentBuffer must be in WGALResourceState::DrawIndirect.
  WResult DrawIndexedInstancedIndirect(WGALBufferHandle hIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes);

  /// Bound vertex buffers must be in WGALResourceState::VertexBuffer.
  WResult DrawInstanced(WUInt32 uiVertexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartVertex);

  /// Bound vertex buffers must be in WGALResourceState::VertexBuffer. hIndirectArgumentBuffer must be in WGALResourceState::DrawIndirect.
  WResult DrawInstancedIndirect(WGALBufferHandle hIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes);

  // State Functions
  /// hIndexBuffer must be invalidated or in WGALResourceState::IndexBuffer.
  void SetIndexBuffer(WGALBufferHandle hIndexBuffer);

  /// hVertexBuffer must be invalidated or in WGALResourceState::VertexBuffer.
  void SetVertexBuffer(WUInt32 uiSlot, WGALBufferHandle hVertexBuffer, WUInt32 uiOffset = 0);

  void SetGraphicsPipeline(WGALGraphicsPipelineHandle hGraphicsPipeline);
  void SetComputePipeline(WGALComputePipelineHandle hComputePipeline);

  // Dynamic State functions
  void SetViewport(const WRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const WRectU32& rect);
  void SetStencilReference(WUInt8 uiStencilRefValue);

  // Internal
  W_ALWAYS_INLINE WGALDevice& GetDevice() { return m_Device; }
  // Don't use light hearted ;)
  void InvalidateState();

  const WGALCommandEncoderStats& GetStats() const { return m_Stats; }
  void ResetStats();

public:
  /// Fired when a texture barrier validation error is detected.
  static WEvent<const WTextureValidationError&> s_TextureBarrierValidationFailed;
  /// Fired when a buffer barrier validation error is detected.
  static WEvent<const WBufferValidationError&> s_BufferBarrierValidationFailed;

protected:
  friend class WGALDevice;

  void GALStaticDeviceEventHandler(const WGALDeviceEvent& e);

  void AssertRenderingThread()
  {
    W_ASSERT_DEV(WThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

  void AssertOutsideRenderingScope()
  {
    W_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render, "This function can only be executed outside a render scope.");
  }

private:
  friend class WMemoryUtils;

  enum class CommandEncoderType
  {
    Invalid,
    Render,
    Compute
  };

  CommandEncoderType m_CurrentCommandEncoderType = CommandEncoderType::Invalid;
  bool m_bMarker = false;

  // Parent Device
  WGALDevice& m_Device;
  WGALCommandEncoderRenderState m_State;
  WGALCommandEncoderCommonPlatformInterface& m_CommonImpl;
  WGALCommandEncoderStats m_Stats;

  WGALOcclusionHandle m_hPendingOcclusionQuery = {};

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  // This code ensures in debug build that a buffer is not updated twice per frame in the same location
  struct BufferRange
  {
    inline bool overlapRange(WUInt32 uiOffset, WUInt32 uiLength) const
    {
      return !(m_uiOffset > (uiOffset + uiLength - 1) || (m_uiOffset + m_uiLength - 1) < uiOffset);
    }
    WUInt32 m_uiOffset = 0;
    WUInt32 m_uiLength = 0;
    W_DECLARE_POD_TYPE();
  };
  WMap<WGALBufferHandle, WHybridArray<BufferRange, 1>> m_BufferUpdates;
#endif

  // Barrier validation
#if W_ENABLED(W_BARRIER_VALIDATION)
  WResult ValidateBindGroupResourceStates(const WGALShader* pShader);
  WResult ValidateBindGroupItemResourceState(WUInt32 uiBindGroup, const WShaderResourceBinding& binding, const WGALBindGroupItem& item);
  WResult ValidateTextureState(WGALTextureHandle hTexture, WGALTextureRange range, WBitflags<WGALResourceState> expectedState, WBitflags<WGALShaderStageFlags> expectedStages, WUInt32 uiBindGroup = 0, const WHashedString& sBinding = WHashedString());
  WResult ValidateBufferState(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> expectedState, WBitflags<WGALShaderStageFlags> expectedStages, WUInt32 uiBindGroup = 0, const WHashedString& sBinding = WHashedString());
  WResult ValidateVertexBufferState();
  WResult ValidateIndexBufferState();
  WResult ValidateGraphicsPipelineResources();
  WResult ValidateComputePipelineResources();
  void ValidateTextureBarriers(WArrayPtr<const WGALTextureBarrier> barriers);
  void ValidateBufferBarriers(WArrayPtr<const WGALBufferBarrier> barriers);
  void ValidateRenderTargetStates(const WGALRenderingSetup& renderingSetup);

  WGALResourceStateTracker m_ResourceStateTracker;
  WGALBindGroupCreationDescription m_BindGroups[W_GAL_MAX_BIND_GROUPS];
  WUInt8 m_uiBindGroupsMask = 0;
  bool m_bVertexBufferStatesDirty = true;
  bool m_bIndexBufferStateDirty = true;

  struct ValidationHash
  {
    static WUInt32 Hash(const WTextureValidationError& a);
    static bool Equal(const WTextureValidationError& a, const WTextureValidationError& b);

    static WUInt32 Hash(const WBufferValidationError& a);
    static bool Equal(const WBufferValidationError& a, const WBufferValidationError& b);
  };
  WHashSet<WTextureValidationError, ValidationHash> m_TextureErrors;
  WHashSet<WBufferValidationError, ValidationHash> m_BufferErrors;
#endif
};
