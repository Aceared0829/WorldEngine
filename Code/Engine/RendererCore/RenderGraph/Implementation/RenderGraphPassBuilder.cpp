#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>

namespace
{
  bool HasOneBitSet(WUInt32 x)
  {
    return x && !(x & (x - 1));
  }

  void ClampRenderTarget(WGALRenderTargetRange& ref_range, const WGALTextureCreationDescription& desc)
  {
    if (ref_range.m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES)
    {
      ref_range.m_uiArraySlices = desc.m_uiArraySize;
    }
  }

  // We encode a full resource rage as the default constructed WGALTextureRange to allow us to quickly categorize full resources barriers vs sub-resource barriers without having to lookup the resource description every time.
  void MakeFullRange(WGALTextureRange& ref_range, const WGALTextureCreationDescription& desc)
  {
    bool bAllSlices = (ref_range.m_uiBaseArraySlice == 0 && (ref_range.m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES || ref_range.m_uiArraySlices == desc.m_uiArraySize));
    bool bAllMips = (ref_range.m_uiBaseMipLevel == 0 && (ref_range.m_uiArraySlices == W_GAL_ALL_MIP_LEVELS || ref_range.m_uiArraySlices == desc.m_uiMipLevelCount));
    if (bAllSlices && bAllMips)
    {
      ref_range = WGALTextureRange{};
    }
  }

  bool MergeRanges(WGALTextureRange& ref_range, WBitflags<WGALResourceState>& ref_access, WBitflags<WGALShaderStageFlags>& ref_stage, const WGALTextureRange& range, WBitflags<WGALResourceState> access, WBitflags<WGALShaderStageFlags> stage)
  {
    // Write states are exclusive and can't be merged
    if ((ref_access.IsAnySet(WGALResourceState::AllWriteStates) || access.IsAnySet(WGALResourceState::AllWriteStates)) && ref_access != access)
      return false;

    if (ref_range.m_uiBaseArraySlice != range.m_uiBaseArraySlice || ref_range.m_uiArraySlices != range.m_uiArraySlices || ref_range.m_uiBaseMipLevel != range.m_uiBaseMipLevel || ref_range.m_uiMipLevels != range.m_uiMipLevels)
      return false;

    ref_access |= access;
    ref_stage |= stage;
    return true;
  }
} // namespace

WRenderGraphPassBuilder::WRenderGraphPassBuilder(WRenderGraph* pParent)
  : m_pParent(pParent)
{
}

WRenderGraphPassBuilder::WRenderGraphPassBuilder(WRenderGraphPassBuilder&& rhs) noexcept
{
  *this = std::move(rhs);
}

WRenderGraphPassBuilder::~WRenderGraphPassBuilder()
{
  if (m_pParent != nullptr)
  {
    // Flush render target / depth target writes / reads
    // This is deferred to here so that the load / store ops can be modified by SetClearColor etc. Note this will be needed once we can represent a barrier that discards the old state (triggered by WGALRenderTargetLoadOp::DontCare). For now, all barriers assume `Load` for simplicity.
    const WUInt16 uiColorTargetIndex = m_pParent->m_pCurrentPass->m_uiColorTargetIndex;
    const WUInt8 uiColorTargetCount = m_pParent->m_pCurrentPass->m_uiColorTargetCount;
    for (WUInt8 i = 0; i < uiColorTargetCount; ++i)
    {
      const WRenderGraph::ColorTargetInfo& info = m_pParent->m_ColorTargets[uiColorTargetIndex + i];
      WBitflags<WGALResourceState> state = WGALResourceState::RenderTarget;
      if (info.m_loadOp != WGALRenderTargetLoadOp::Load)
        state |= WGALResourceState::Discard;
      WriteTexture(info.m_hTexture, WGALTextureRange::MakeFromRenderTargetRange(info.m_range), state);
    }

    const WUInt16 uiDepthTargetIndex = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetIndex;
    const WUInt8 uiDepthTargetCount = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount;
    for (WUInt8 i = 0; i < uiDepthTargetCount; ++i)
    {
      const WRenderGraph::DepthStencilTargetInfo& info = m_pParent->m_DepthStencilTargets[uiDepthTargetIndex + i];
      WBitflags<WGALResourceState> state = info.m_bReadOnly ? WGALResourceState::DepthStencilRead : WGALResourceState::DepthStencilWrite;
      const bool bStencil = WGALResourceFormat::IsStencilFormat(m_pParent->GetTextureDesc(info.m_hTexture).m_Format);
      if (info.m_depthLoadOp != WGALRenderTargetLoadOp::Load && !bStencil || info.m_stencilLoadOp != WGALRenderTargetLoadOp::Load)
        state |= WGALResourceState::Discard;
      if (info.m_bReadOnly)
      {
        ReadTexture(info.m_hTexture, WGALTextureRange::MakeFromRenderTargetRange(info.m_range), state);
      }
      else
      {
        WriteTexture(info.m_hTexture, WGALTextureRange::MakeFromRenderTargetRange(info.m_range), state);
      }
    }

    m_pParent->EndPassBuilder();
  }
}

void WRenderGraphPassBuilder::operator=(WRenderGraphPassBuilder&& rhs) noexcept
{
  if (m_pParent)
  {
    m_pParent->EndPassBuilder();
  }
  m_pParent = rhs.m_pParent;
  rhs.m_pParent = nullptr;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::ReadTexture(WRenderGraphTextureHandle hTexture,
  WGALTextureRange range, WBitflags<WGALResourceState> access, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG(HasOneBitSet(access.GetValue()), "ReadTexture: Exactly one state must be set");
  if (m_pParent->m_pCurrentPass->m_QueueType == WGALQueueType::Compute && stage == WGALShaderStageFlags::Auto)
    stage = WGALShaderStageFlags::ComputeShader;

  W_ASSERT_DEBUG((access & WGALResourceState::AllTextureStates) == access, "ReadTexture: Only texture states are allowed");
  W_ASSERT_DEBUG(hTexture.GetInternalID().m_InstanceIndex < m_pParent->m_TextureCreationDescriptions.GetCount(), "ReadTexture: hTexture is invalid");
  W_ASSERT_DEBUG((access & WGALResourceState::AllReadStates) == access, "ReadTexture: Only read states are allowed");
  WGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  MakeFullRange(range, desc);

  // Convenience functionality. We still need the differentiation in the low level renderer but here we can just patch it.
  if (access.IsSet(WGALResourceState::ShaderResource) && WGALResourceFormat::IsDepthFormat(desc.m_Format))
  {
    access.Remove(WGALResourceState::ShaderResource);
    access.Add(WGALResourceState::DepthStencilRead);
  }

  const bool bIsImported = m_pParent->m_HandleToImportTexture.Contains(hTexture);
  if (bIsImported)
  {
    // Verify imported texture supports operation
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::ShaderResource:
        case WGALResourceState::DepthStencilRead:
          W_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::ShaderResource), "ShaderResource flag required on imported texture, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp texture
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::ShaderResource:
        case WGALResourceState::DepthStencilRead:
          if (!desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::ShaderResource))
          {
            desc.m_TextureFlags |= WGALTextureUsageFlags::ShaderResource;
            W_ASSERT_DEBUG(desc.Validate(m_pParent->m_pDevice).Succeeded(), "Invalid texture desc");
          }
        default:
          break;
      }
    }
  }

  WArrayPtr<const WRenderGraph::TextureInfo> reads = m_pParent->m_pCurrentPass->GetReadTextures(m_pParent);
  for (WUInt32 uiIndex = 0; uiIndex < reads.GetCount(); ++uiIndex)
  {
    // Cast away const so we can merge into the existing entry in-place.
    auto& readInfo = const_cast<WRenderGraph::TextureInfo&>(reads[uiIndex]);
    if (readInfo.m_hTexture != hTexture)
      continue;

    if (!range.Overlaps(readInfo.m_range))
      continue;

    if (MergeRanges(readInfo.m_range, readInfo.m_access, readInfo.m_stage, range, access, stage))
      return *this;

    W_REPORT_FAILURE("ReadTexture: Overlapping read-write conflict on the same texture within a single pass");
  }

  m_pParent->m_ReadTextures.PushBack({hTexture, range, stage, access});
  m_pParent->m_pCurrentPass->m_uiReadTextureCount++;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::WriteTexture(WRenderGraphTextureHandle hTexture,
  WGALTextureRange range, WBitflags<WGALResourceState> access, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG(HasOneBitSet(access.GetValue() & ~WGALResourceState::Discard), "WriteTexture: Exactly one state must be set");
  W_ASSERT_DEBUG((access & WGALResourceState::AllTextureStates) == access, "WriteTexture: Only texture states are allowed");
  W_ASSERT_DEBUG(hTexture.GetInternalID().m_InstanceIndex < m_pParent->m_TextureCreationDescriptions.GetCount(), "WriteTexture: hTexture is invalid");
  W_ASSERT_DEBUG((access & WGALResourceState::AllWriteStates) == access, "WriteTexture: Only write states are allowed");
  WGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  MakeFullRange(range, desc);

  const bool bIsImported = m_pParent->m_HandleToImportTexture.Contains(hTexture);
  if (bIsImported)
  {
    W_ASSERT_DEBUG(!desc.m_ResourceAccess.m_bImmutable, "Can't write to immutable textures");
    // Verify imported texture supports operation
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::UnorderedAccess:
          W_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess), "UnorderedAccess flag required on imported texture, operation invalid");
          break;
        case WGALResourceState::RenderTarget:
        case WGALResourceState::DepthStencilWrite:
          W_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget), "RenderTarget flag required on imported texture, operation invalid");
          break;
        case WGALResourceState::Present:
          W_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::Presentable), "Presentable flag required on imported texture, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp texture
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::UnorderedAccess:
          if (!desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::UnorderedAccess))
          {
            desc.m_TextureFlags |= WGALTextureUsageFlags::UnorderedAccess;
            W_ASSERT_DEBUG(desc.Validate(m_pParent->m_pDevice).Succeeded(), "Invalid texture desc");
          }
          break;
        case WGALResourceState::RenderTarget:
        case WGALResourceState::DepthStencilWrite:
          if (!desc.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
          {
            desc.m_TextureFlags |= WGALTextureUsageFlags::RenderTarget;
            W_ASSERT_DEBUG(desc.Validate(m_pParent->m_pDevice).Succeeded(), "Invalid texture desc");
          }
          break;
        case WGALResourceState::Present:
          desc.m_TextureFlags |= WGALTextureUsageFlags::Presentable;
          break;
        default:
          break;
      }
    }
  }

  WArrayPtr<const WRenderGraph::TextureInfo> writes = m_pParent->m_pCurrentPass->GetWriteTextures(m_pParent);
  for (WUInt32 uiIndex = 0; uiIndex < writes.GetCount(); ++uiIndex)
  {
    // Cast away const so we can merge into the existing entry in-place.
    auto& writeInfo = const_cast<WRenderGraph::TextureInfo&>(writes[uiIndex]);
    if (writeInfo.m_hTexture != hTexture)
      continue;

    if (!range.Overlaps(writeInfo.m_range))
      continue;

    if (MergeRanges(writeInfo.m_range, writeInfo.m_access, writeInfo.m_stage, range, access, stage))
      return *this;

    W_REPORT_FAILURE("WriteTexture: Overlapping write conflict on the same texture within a single pass");
  }

  m_pParent->m_WriteTextures.PushBack({hTexture, range, stage, access});
  m_pParent->m_pCurrentPass->m_uiWriteTextureCount++;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::ReadBuffer(WRenderGraphBufferHandle hBuffer,
  WBitflags<WGALResourceState> access, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG((access & WGALResourceState::AllBufferStates) == access, "ReadBuffer: Only buffer states are allowed");
  W_ASSERT_DEBUG(hBuffer.GetInternalID().m_InstanceIndex < m_pParent->m_BufferCreationDescriptions.GetCount(), "ReadBuffer: hBuffer is invalid");
  W_ASSERT_DEBUG(access.IsAnySet(WGALResourceState::AllReadStates), "ReadBuffer: Only read states are allowed");
  WGALBufferCreationDescription& desc = m_pParent->m_BufferCreationDescriptions[hBuffer.GetInternalID().m_InstanceIndex];

  const bool bIsImported = m_pParent->m_HandleToImportBuffer.Contains(hBuffer);
  if (bIsImported)
  {
    // Verify imported buffer supports operation
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::ShaderResource:
          W_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::ShaderResource), "ShaderResource flag required on imported buffer, operation invalid");
          break;
        case WGALResourceState::ConstantBuffer:
          W_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::ConstantBuffer), "ConstantBuffer flag required on imported buffer, operation invalid");
          break;
        case WGALResourceState::VertexBuffer:
          W_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::VertexBuffer), "VertexBuffer flag required on imported buffer, operation invalid");
          break;
        case WGALResourceState::IndexBuffer:
          W_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::IndexBuffer), "IndexBuffer flag required on imported buffer, operation invalid");
          break;
        case WGALResourceState::DrawIndirect:
          W_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::DrawIndirect), "DrawIndirect flag required on imported buffer, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp buffer
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::ShaderResource:
          desc.m_BufferFlags |= WGALBufferUsageFlags::ShaderResource;
          break;
        case WGALResourceState::ConstantBuffer:
          desc.m_BufferFlags |= WGALBufferUsageFlags::ConstantBuffer;
          break;
        case WGALResourceState::VertexBuffer:
          desc.m_BufferFlags |= WGALBufferUsageFlags::VertexBuffer;
          break;
        case WGALResourceState::IndexBuffer:
          desc.m_BufferFlags |= WGALBufferUsageFlags::IndexBuffer;
          break;
        case WGALResourceState::DrawIndirect:
          desc.m_BufferFlags |= WGALBufferUsageFlags::DrawIndirect;
          break;
        default:
          break;
      }
    }
  }

  m_pParent->m_ReadBuffers.PushBack({hBuffer, stage, access});
  m_pParent->m_pCurrentPass->m_uiReadBufferCount++;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::WriteBuffer(WRenderGraphBufferHandle hBuffer,
  WBitflags<WGALResourceState> access, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG((access & WGALResourceState::AllBufferStates) == access, "WriteBuffer: Only buffer states are allowed");
  W_ASSERT_DEBUG(hBuffer.GetInternalID().m_InstanceIndex < m_pParent->m_BufferCreationDescriptions.GetCount(), "WriteBuffer: hBuffer is invalid");
  W_ASSERT_DEBUG(access.IsAnySet(WGALResourceState::AllWriteStates), "WriteBuffer: Only write states are allowed");
  WGALBufferCreationDescription& desc = m_pParent->m_BufferCreationDescriptions[hBuffer.GetInternalID().m_InstanceIndex];

  const bool bIsImported = m_pParent->m_HandleToImportBuffer.Contains(hBuffer);
  if (bIsImported)
  {
    W_ASSERT_DEBUG(!desc.m_ResourceAccess.m_bImmutable, "Can't write to immutable buffers");
    // Verify imported buffer supports operation
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::UnorderedAccess:
          W_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(WGALBufferUsageFlags::UnorderedAccess), "UnorderedAccess flag required on imported buffer, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp buffer
    for (WGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case WGALResourceState::UnorderedAccess:
          desc.m_BufferFlags |= WGALBufferUsageFlags::UnorderedAccess;
          break;
        default:
          break;
      }
    }
  }

  m_pParent->m_WriteBuffers.PushBack({hBuffer, stage, access});
  m_pParent->m_pCurrentPass->m_uiWriteBufferCount++;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::AddColorTarget(WRenderGraphTextureHandle hTexture,
  WGALRenderTargetRange range, WEnum<WGALRenderTargetLoadOp> loadOp, WEnum<WGALRenderTargetStoreOp> storeOp, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  const WGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  ClampRenderTarget(range, desc);

  WUInt8& uiColorTargetCount = m_pParent->m_pCurrentPass->m_uiColorTargetCount;
  m_pParent->m_ColorTargets.PushBack({hTexture, range, loadOp, storeOp, overrideViewFormat, overrideViewType});
  ++uiColorTargetCount;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::SetClearColor(WUInt8 uiIndex, const WColor& color)
{
  WUInt8& uiColorTargetCount = m_pParent->m_pCurrentPass->m_uiColorTargetCount;
  WUInt8& uiClearColorCount = m_pParent->m_pCurrentPass->m_uiClearColorCount;
  W_ASSERT_DEBUG(uiIndex < uiColorTargetCount, "Render target not set, call AddColorTarget first");

  while (uiClearColorCount <= uiIndex)
  {
    m_pParent->m_ClearColors.PushBack({});
    uiClearColorCount++;
  }
  m_pParent->m_ClearColors[m_pParent->m_pCurrentPass->m_uiClearColorIndex + uiIndex] = color;
  m_pParent->m_ColorTargets[m_pParent->m_pCurrentPass->m_uiColorTargetIndex + uiIndex].m_loadOp = WGALRenderTargetLoadOp::Clear;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::AddDepthStencilTarget(WRenderGraphTextureHandle hTexture,
  WGALRenderTargetRange range,
  WEnum<WGALRenderTargetLoadOp> depthLoadOp, WEnum<WGALRenderTargetStoreOp> depthStoreOp,
  WEnum<WGALRenderTargetLoadOp> stencilLoadOp, WEnum<WGALRenderTargetStoreOp> stencilStoreOp, bool bReadOnly)
{
  const WGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  ClampRenderTarget(range, desc);

  W_ASSERT_DEBUG(m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount == 0, "Multiple depth/stencil targets are not supported");

  m_pParent->m_DepthStencilTargets.PushBack({hTexture, range, depthLoadOp, depthStoreOp, stencilLoadOp, stencilStoreOp, bReadOnly});
  m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount++;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::SetClearDepth(float fDepthClear)
{
  WUInt8 uiDepthTargetCount = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount;
  W_ASSERT_DEBUG(uiDepthTargetCount == 1, "Render target not set, call AddDepthStencilTarget first");

  if (m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount == 0)
  {
    m_pParent->m_ClearDepthStencils.ExpandAndGetRef().fDepthClear = fDepthClear;
    m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount = 1;
  }
  else
  {
    m_pParent->m_ClearDepthStencils.PeekBack().fDepthClear = fDepthClear;
  }
  m_pParent->m_DepthStencilTargets[m_pParent->m_pCurrentPass->m_uiDepthStencilTargetIndex].m_depthLoadOp = WGALRenderTargetLoadOp::Clear;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::SetClearStencil(WUInt8 uiStencilClear)
{
  WUInt8 uiDepthTargetCount = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount;
  W_ASSERT_DEBUG(uiDepthTargetCount == 1, "Render target not set, call AddDepthStencilTarget first");
  if (m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount == 0)
  {
    m_pParent->m_ClearDepthStencils.ExpandAndGetRef().uiStencilClear = uiStencilClear;
    m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount = 1;
  }
  else
  {
    m_pParent->m_ClearDepthStencils.PeekBack().uiStencilClear = uiStencilClear;
  }
  m_pParent->m_DepthStencilTargets[m_pParent->m_pCurrentPass->m_uiDepthStencilTargetIndex].m_stencilLoadOp = WGALRenderTargetLoadOp::Clear;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::HasSideEffects()
{
  m_pParent->m_pCurrentPass->m_bHasSideEffects = true;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::SetStereoscopic(bool bStereoscopic)
{
  m_pParent->m_pCurrentPass->m_bStereoscopic = bStereoscopic;
  return *this;
}

WRenderGraphPassBuilder& WRenderGraphPassBuilder::SetExecuteCallback(WRenderGraphExecuteFunction callback)
{
  m_pParent->m_pCurrentPass->m_ExecuteFunction = callback;
  return *this;
}
