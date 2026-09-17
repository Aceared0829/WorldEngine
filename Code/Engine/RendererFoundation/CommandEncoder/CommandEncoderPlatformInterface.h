
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>
struct WShaderResourceBinding;
struct WGALRenderingSetup;
struct WGALBindGroupCreationDescription;

class W_RENDERERFOUNDATION_DLL WGALCommandEncoderCommonPlatformInterface
{
public:
  // Resource binding functions
  virtual void SetBindGroupPlatform(WUInt32 uiBindGroup, const WGALBindGroupCreationDescription& bindGroup) = 0;
  virtual void SetBindGroupPlatform(WUInt32 uiBindGroup, const WGALBindGroup* pBindGroup) = 0;
  virtual void SetPushConstantsPlatform(WArrayPtr<const WUInt8> data) = 0;

  // GPU -> CPU query functions

  virtual WGALTimestampHandle InsertTimestampPlatform() = 0;
  virtual WGALOcclusionHandle BeginOcclusionQueryPlatform(WEnum<WGALQueryType> type) = 0;
  virtual void EndOcclusionQueryPlatform(WGALOcclusionHandle hOcclusion) = 0;
  virtual WGALFenceHandle InsertFencePlatform() = 0;

  // Resource update functions

  virtual void CopyBufferPlatform(const WGALBuffer* pDestination, const WGALBuffer* pSource) = 0;
  virtual void CopyBufferRegionPlatform(const WGALBuffer* pDestination, WUInt32 uiDestOffset, const WGALBuffer* pSource, WUInt32 uiSourceOffset, WUInt32 uiByteCount) = 0;

  virtual void UpdateBufferPlatform(const WGALBuffer* pDestination, WUInt32 uiDestOffset, WArrayPtr<const WUInt8> sourceData, WGALUpdateMode::Enum updateMode) = 0;

  virtual void CopyTexturePlatform(const WGALTexture* pDestination, const WGALTexture* pSource) = 0;
  virtual void CopyTextureRegionPlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource, const WVec3U32& vDestinationPoint, const WGALTexture* pSource, const WGALTextureSubresource& sourceSubResource, const WBoundingBoxu32& box) = 0;

  virtual void UpdateTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource, const WBoundingBoxu32& destinationBox, const WGALSystemMemoryDescription& sourceData) = 0;

  virtual void ResolveTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource, const WGALTexture* pSource, const WGALTextureSubresource& sourceSubResource) = 0;

  virtual void ReadbackTexturePlatform(const WGALReadbackTexture* pDestination, const WGALTexture* pSource) = 0;
  virtual void ReadbackBufferPlatform(const WGALReadbackBuffer* pDestination, const WGALBuffer* pSource) = 0;

  // Barriers

  virtual void TextureBarrierPlatform(WArrayPtr<const WGALTextureBarrier> barriers) = 0;
  virtual void BufferBarrierPlatform(WArrayPtr<const WGALBufferBarrier> barriers) = 0;

  // Misc

  virtual void FlushPlatform() = 0;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) = 0;
  virtual void PopMarkerPlatform() = 0;
  virtual void InsertEventMarkerPlatform(const char* szMarker) = 0;

  // Compute Dispatch

  virtual void BeginComputePlatform() = 0;
  virtual void EndComputePlatform() = 0;


  virtual WResult DispatchPlatform(WUInt32 uiThreadGroupCountX, WUInt32 uiThreadGroupCountY, WUInt32 uiThreadGroupCountZ) = 0;
  virtual WResult DispatchIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes) = 0;

  // Draw functions

  virtual void BeginRenderingPlatform(const WGALRenderingSetup& renderingSetup) = 0;
  virtual void EndRenderingPlatform() = 0;

  virtual void ClearPlatform(const WColor& clearColor, WUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, WUInt8 uiStencilClear) = 0;

  virtual WResult DrawPlatform(WUInt32 uiVertexCount, WUInt32 uiStartVertex) = 0;
  virtual WResult DrawIndexedPlatform(WUInt32 uiIndexCount, WUInt32 uiStartIndex) = 0;
  virtual WResult DrawIndexedInstancedPlatform(WUInt32 uiIndexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartIndex) = 0;
  virtual WResult DrawIndexedInstancedIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes) = 0;
  virtual WResult DrawInstancedPlatform(WUInt32 uiVertexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartVertex) = 0;
  virtual WResult DrawInstancedIndirectPlatform(const WGALBuffer* pIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes) = 0;

  // State functions

  virtual void SetIndexBufferPlatform(const WGALBuffer* pIndexBuffer) = 0;
  virtual void SetVertexBufferPlatform(WUInt32 uiSlot, const WGALBuffer* pVertexBuffer, WUInt32 uiOffset) = 0;

  virtual void SetGraphicsPipelinePlatform(const WGALGraphicsPipeline* pGraphicsPipeline) = 0;
  virtual void SetComputePipelinePlatform(const WGALComputePipeline* pComputePipeline) = 0;

  // Dynamic State Functions

  virtual void SetViewportPlatform(const WRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;
  virtual void SetScissorRectPlatform(const WRectU32& rect) = 0;
  virtual void SetStencilReferencePlatform(WUInt8 uiStencilRefValue) = 0;
};
