
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Shader/BindGroup.h>

struct ID3D11DeviceChild;
struct ID3D11DeviceContext;
struct ID3DUserDefinedAnnotation;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11Query;

class WGALDeviceDX11;
struct WGALBindGroupCreationDescription;

class W_RENDERERDX11_DLL WGALCommandEncoderImplDX11 final : public WGALCommandEncoderCommonPlatformInterface
{
public:
  WGALCommandEncoderImplDX11(WGALDeviceDX11& ref_deviceDX11);
  ~WGALCommandEncoderImplDX11();

  void EndFrame();

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

  virtual void UpdateTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource,
    const WBoundingBoxu32& destinationBox, const WGALSystemMemoryDescription& sourceData) override;

  virtual void ResolveTexturePlatform(const WGALTexture* pDestination, const WGALTextureSubresource& destinationSubResource,
    const WGALTexture* pSource, const WGALTextureSubresource& sourceSubResource) override;

  virtual void ReadbackTexturePlatform(const WGALReadbackTexture* pDestination, const WGALTexture* pSource) override;
  virtual void ReadbackBufferPlatform(const WGALReadbackBuffer* pDestination, const WGALBuffer* pSource) override;

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

private:
  friend class WGALDeviceDX11;
  void SetShader(const WGALShader* pShader);
  void SetVertexDeclaration(const WGALVertexDeclaration* pVertexDeclaration);
  void SetPrimitiveTopology(WGALPrimitiveTopology::Enum topology);
  void SetBlendState(const WGALBlendState* pBlendState, const WColor& blendFactor = WColor::White, WUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(const WGALDepthStencilState* pDepthStencilState);
  void SetRasterizerState(const WGALRasterizerState* pRasterizerState);

  bool UnsetResourceViews(const WGALResourceBase* pResource);
  bool UnsetUnorderedAccessViews(const WGALResourceBase* pResource);

  void SetResourceView(const WShaderResourceBinding& binding, const WGALResourceBase* pResource, ID3D11ShaderResourceView* pResourceViewDX11);
  void SetUnorderedAccessView(const WShaderResourceBinding& binding, ID3D11UnorderedAccessView* pUnorderedAccessViewDX11, const WGALResourceBase* pResource);
  void SetConstantBuffer(const WShaderResourceBinding& binding, const WGALBuffer* pBuffer);
  void SetSamplerState(const WShaderResourceBinding& binding, const WGALSamplerState* pSamplerState);

  WResult FlushDeferredStateChanges();

  WGALDeviceDX11& m_GALDeviceDX11;

  ID3D11DeviceContext* m_pDXContext = nullptr;
  ID3DUserDefinedAnnotation* m_pDXAnnotation = nullptr;

  // Bound objects for deferred state flushes
  WEnum<WGALPrimitiveTopology> m_Topology;
  WUInt8 m_uiTessellationPatchControlPoints = 0;

  ID3D11Buffer* m_pBoundConstantBuffers[W_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  WGAL::ModifiedRange m_BoundConstantBuffersRange[WGALShaderStage::ENUM_COUNT];

  WHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[WGALShaderStage::ENUM_COUNT] = {};
  WHybridArray<const WGALResourceBase*, 16> m_ResourcesForResourceViews[WGALShaderStage::ENUM_COUNT];
  WGAL::ModifiedRange m_BoundShaderResourceViewsRange[WGALShaderStage::ENUM_COUNT];

  WHybridArray<ID3D11UnorderedAccessView*, 16> m_BoundUnorderedAccessViews;
  WHybridArray<const WGALResourceBase*, 16> m_ResourcesForUnorderedAccessViews;
  WGAL::ModifiedRange m_BoundUnorderedAccessViewsRange;

  ID3D11SamplerState* m_pBoundSamplerStates[WGALShaderStage::ENUM_COUNT][W_GAL_MAX_SAMPLER_COUNT] = {};
  WGAL::ModifiedRange m_BoundSamplerStatesRange[WGALShaderStage::ENUM_COUNT];

  ID3D11DeviceChild* m_pBoundShaders[WGALShaderStage::ENUM_COUNT] = {};
  WUInt8 m_uiStencilRefValue = 0;

  WGALRenderingSetup m_RenderTargetSetup;
  ID3D11RenderTargetView* m_pBoundRenderTargets[W_GAL_MAX_RENDERTARGET_COUNT] = {};
  WUInt32 m_uiBoundRenderTargetCount = 0;
  ID3D11DepthStencilView* m_pBoundDepthStencilTarget = nullptr;

  ID3D11Buffer* m_pBoundVertexBuffers[W_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  WGAL::ModifiedRange m_BoundVertexBuffersRange;

  WUInt32 m_VertexBufferStrides[W_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  WUInt32 m_VertexBufferOffsets[W_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  WHashSet<const WGALBuffer*> m_AlreadyUpdatedTransientBuffers;
};
