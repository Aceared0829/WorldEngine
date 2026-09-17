
/// Used to guard WGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed
#define W_GALDEVICE_LOCK_AND_CHECK() \
  W_LOCK(m_Mutex);                   \
  VerifyMultithreadedAccess()

W_ALWAYS_INLINE const WGALDeviceCreationDescription* WGALDevice::GetDescription() const
{
  return &m_Description;
}

W_ALWAYS_INLINE WUInt64 WGALDevice::GetCurrentFrame() const
{
  return GetCurrentFramePlatform();
}

W_ALWAYS_INLINE WUInt64 WGALDevice::GetSafeFrame() const
{
  return GetSafeFramePlatform();
}

W_ALWAYS_INLINE WEnum<WGALAsyncResult> WGALDevice::GetTimestampResult(WGALTimestampHandle hTimestamp, WTime& out_result)
{
  if (hTimestamp.IsInvalidated())
    return WGALAsyncResult::Expired;

  return GetTimestampResultPlatform(hTimestamp, out_result);
}

W_ALWAYS_INLINE WEnum<WGALAsyncResult> WGALDevice::GetOcclusionQueryResult(WGALOcclusionHandle hOcclusion, WUInt64& out_uiResult)
{
  if (hOcclusion.IsInvalidated())
    return WGALAsyncResult::Expired;

  return GetOcclusionResultPlatform(hOcclusion, out_uiResult);
}

template <typename IdTableType, typename ReturnType>
W_ALWAYS_INLINE ReturnType* WGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  W_GALDEVICE_LOCK_AND_CHECK();

  ReturnType* pObject = nullptr;
  bool _1 = IdTable.TryGetValue(hHandle, pObject);
  W_IGNORE_UNUSED(_1);
  return pObject;
}

inline const WGALSwapChain* WGALDevice::GetSwapChain(WGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, WGALSwapChain>(hSwapChain, m_SwapChains);
}

inline const WGALShader* WGALDevice::GetShader(WGALShaderHandle hShader) const
{
  return Get<ShaderTable, WGALShader>(hShader, m_Shaders);
}

inline const WGALTexture* WGALDevice::GetTexture(WGALTextureHandle hTexture) const
{
  return Get<TextureTable, WGALTexture>(hTexture, m_Textures);
}

inline const WGALBuffer* WGALDevice::GetBuffer(WGALBufferHandle hBuffer) const
{
  return Get<BufferTable, WGALBuffer>(hBuffer, m_Buffers);
}

inline const WGALDynamicBuffer* WGALDevice::GetDynamicBuffer(WGALDynamicBufferHandle hBuffer) const
{
  return Get<DynamicBufferTable, WGALDynamicBuffer>(hBuffer, m_DynamicBuffers);
}

inline WGALDynamicBuffer* WGALDevice::GetDynamicBuffer(WGALDynamicBufferHandle hBuffer)
{
  return Get<DynamicBufferTable, WGALDynamicBuffer>(hBuffer, m_DynamicBuffers);
}

inline const WGALReadbackBuffer* WGALDevice::GetReadbackBuffer(WGALReadbackBufferHandle hBuffer) const
{
  return Get<ReadbackBufferTable, WGALReadbackBuffer>(hBuffer, m_ReadbackBuffers);
}

inline const WGALReadbackTexture* WGALDevice::GetReadbackTexture(WGALReadbackTextureHandle hTexture) const
{
  return Get<ReadbackTextureTable, WGALReadbackTexture>(hTexture, m_ReadbackTextures);
}

inline const WGALDepthStencilState* WGALDevice::GetDepthStencilState(WGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, WGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

inline const WGALBlendState* WGALDevice::GetBlendState(WGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, WGALBlendState>(hBlendState, m_BlendStates);
}

inline const WGALRasterizerState* WGALDevice::GetRasterizerState(WGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, WGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

inline const WGALVertexDeclaration* WGALDevice::GetVertexDeclaration(WGALVertexDeclarationHandle hVertexDeclaration) const
{
  return Get<VertexDeclarationTable, WGALVertexDeclaration>(hVertexDeclaration, m_VertexDeclarations);
}

inline const WGALSamplerState* WGALDevice::GetSamplerState(WGALSamplerStateHandle hSamplerState) const
{
  return Get<SamplerStateTable, WGALSamplerState>(hSamplerState, m_SamplerStates);
}

inline const WGALBindGroupLayout* WGALDevice::GetBindGroupLayout(WGALBindGroupLayoutHandle hBindGroupLayout) const
{
  return Get<BindGroupLayoutTable, WGALBindGroupLayout>(hBindGroupLayout, m_BindGroupLayouts);
}

inline const WGALBindGroup* WGALDevice::GetBindGroup(WGALBindGroupHandle hBindGroup) const
{
  return Get<BindGroupTable, WGALBindGroup>(hBindGroup, m_BindGroups);
}

inline const WGALPipelineLayout* WGALDevice::GetPipelineLayout(WGALPipelineLayoutHandle hPipelineLayout) const
{
  return Get<PipelineLayoutTable, WGALPipelineLayout>(hPipelineLayout, m_PipelineLayouts);
}

inline const WGALGraphicsPipeline* WGALDevice::GetGraphicsPipeline(WGALGraphicsPipelineHandle hGraphicsPipeline) const
{
  return Get<GraphicsPipelineTable, WGALGraphicsPipeline>(hGraphicsPipeline, m_GraphicsPipelines);
}

inline const WGALComputePipeline* WGALDevice::GetComputePipeline(WGALComputePipelineHandle hComputePipeline) const
{
  return Get<ComputePipelineTable, WGALComputePipeline>(hComputePipeline, m_ComputePipelines);
}

inline const WGALRenderTargetView* WGALDevice::GetRenderTargetView(WGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, WGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}

// static
W_ALWAYS_INLINE void WGALDevice::SetDefaultDevice(WGALDevice* pDefaultDevice)
{
  s_pDefaultDevice = pDefaultDevice;
}

// static
W_ALWAYS_INLINE WGALDevice* WGALDevice::GetDefaultDevice()
{
  W_ASSERT_DEBUG(s_pDefaultDevice != nullptr, "Default device not set.");
  return s_pDefaultDevice;
}

// static
W_ALWAYS_INLINE bool WGALDevice::HasDefaultDevice()
{
  return s_pDefaultDevice != nullptr;
}

template <typename HandleType>
W_FORCE_INLINE void WGALDevice::AddDeadObject(WUInt32 uiType, HandleType handle)
{
  DeadObject deadObject = {uiType, handle.GetInternalID().m_Data};
  W_ASSERT_DEBUG(!m_DeadObjects.Contains(deadObject), "The same object is being destroyed multiple times.");
  m_DeadObjects.PushBack(deadObject);
}

template <typename HandleType>
void WGALDevice::ReviveDeadObject(WUInt32 uiType, HandleType handle)
{
  WUInt32 uiHandle = handle.GetInternalID().m_Data;

  for (WUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    if (deadObject.m_uiType == uiType && deadObject.m_uiHandle == uiHandle)
    {
      m_DeadObjects.RemoveAtAndCopy(i);
      return;
    }
  }
}

W_ALWAYS_INLINE void WGALDevice::VerifyMultithreadedAccess() const
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  W_ASSERT_DEV(m_Capabilities.m_bSupportsMultithreadedResourceCreation || WThreadUtils::IsMainThread(),
    "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}

inline WAllocator* WGALDevice::GetAllocator()
{
  return &m_Allocator;
}
