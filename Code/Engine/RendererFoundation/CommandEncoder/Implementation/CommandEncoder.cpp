#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/ReadbackBuffer.h>
#include <RendererFoundation/Resources/ReadbackTexture.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/State/ComputePipeline.h>
#include <RendererFoundation/State/GraphicsPipeline.h>

#if W_ENABLED(W_BARRIER_VALIDATION)

namespace
{
  void ResolveProxyTexture(WGALDevice* pDevice, WGALTextureHandle& ref_hTexture, WGALTextureRange& ref_range)
  {
    const WGALTexture* pTexture = pDevice->GetTexture(ref_hTexture);
    W_ASSERT_DEBUG(pTexture != nullptr, "Invalid texture handle.");
    // Resolve proxy texture as they only cause pain down the pipeline.
    if (pTexture->GetDescription().m_Type == WGALTextureType::Texture2DProxy)
    {
      const auto pProxy = static_cast<const WGALProxyTexture*>(pTexture);
      ref_hTexture = pProxy->GetParentTextureHandle();
      ref_range = {pProxy->GetSlice(), 1, ref_range.m_uiBaseMipLevel, ref_range.m_uiMipLevels};
    }
  }

  /// Maps a shader resource type to the expected WGALResourceState for validation.
  /// For texture SRVs, depth textures expect DepthStencilRead instead of ShaderResource.
  WBitflags<WGALResourceState> GetExpectedResourceState(WGALShaderResourceType::Enum resourceType, const WGALDevice& device, WGALTextureHandle hTexture = {})
  {
    switch (resourceType)
    {
      case WGALShaderResourceType::Texture:
      case WGALShaderResourceType::TextureAndSampler:
      {
        if (!hTexture.IsInvalidated())
        {
          const WGALTexture* pTexture = device.GetTexture(hTexture);
          if (pTexture != nullptr && WGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format))
            return WGALResourceState::DepthStencilRead;
        }
        return WGALResourceState::ShaderResource;
      }
      case WGALShaderResourceType::TextureRW:
        return WGALResourceState::UnorderedAccess;
      case WGALShaderResourceType::ConstantBuffer:
        return WGALResourceState::ConstantBuffer;
      case WGALShaderResourceType::TexelBuffer:
      case WGALShaderResourceType::StructuredBuffer:
      case WGALShaderResourceType::ByteAddressBuffer:
        return WGALResourceState::ShaderResource;
      case WGALShaderResourceType::TexelBufferRW:
      case WGALShaderResourceType::StructuredBufferRW:
      case WGALShaderResourceType::ByteAddressBufferRW:
        return WGALResourceState::UnorderedAccess;
      default:
        return {};
    }
  }

  WResult CheckTextureSubResourceState(
    const WGALResourceStateTracker::TextureState& textureState,
    const WGALTextureRange& range,
    WTextureValidationError& ref_error)
  {
    auto checkSubResource = [&](const WGALResourceStateTracker::SubResourceState& sr, const WGALTextureSubresource& subResource) -> WResult
    {
      if (WGALResourceStateTracker::IsTextureBarrierNeeded(sr, {ref_error.m_expectedState, ref_error.m_expectedStages}, false))
      {
        ref_error.m_failedSubResource = subResource;
        ref_error.m_actualState = sr.m_State;
        ref_error.m_actualStages = sr.m_Stages;
        return W_FAILURE;
      }
      return W_SUCCESS;
    };

    if (textureState.m_SubResourceStates.GetCount() == 1)
    {
      WGALTextureSubresource subResource = {0, 0};
      return checkSubResource(textureState.m_SubResourceStates[0], subResource);
    }

    const WUInt32 uiEndSlice = WMath::Min(static_cast<WUInt32>(range.m_uiBaseArraySlice) + range.m_uiArraySlices, static_cast<WUInt32>(textureState.m_FullRange.m_uiArraySlices));
    const WUInt32 uiEndMip = WMath::Min(static_cast<WUInt32>(range.m_uiBaseMipLevel) + range.m_uiMipLevels, static_cast<WUInt32>(textureState.m_FullRange.m_uiMipLevels));

    for (WUInt32 uiSlice = range.m_uiBaseArraySlice; uiSlice < uiEndSlice; ++uiSlice)
    {
      for (WUInt32 uiMip = range.m_uiBaseMipLevel; uiMip < uiEndMip; ++uiMip)
      {
        const WUInt32 uiSubresourceIndex = uiMip + uiSlice * textureState.m_FullRange.m_uiMipLevels;
        WGALTextureSubresource subResource = {uiMip, uiSlice};
        if (uiSubresourceIndex < textureState.m_SubResourceStates.GetCount())
        {
          W_SUCCEED_OR_RETURN(checkSubResource(textureState.m_SubResourceStates[uiSubresourceIndex], subResource));
        }
      }
    }
    return W_SUCCESS;
  }

  WGALTextureRange MakeSingleSubresourceRange(const WGALTextureSubresource& subResource)
  {
    return {static_cast<WUInt16>(subResource.m_uiArraySlice), 1, static_cast<WUInt8>(subResource.m_uiMipLevel), 1};
  }
} // namespace


WUInt32 WGALCommandEncoder::ValidationHash::Hash(const WTextureValidationError& a)
{
  return a.CalculateHash();
}

bool WGALCommandEncoder::ValidationHash::Equal(const WTextureValidationError& a, const WTextureValidationError& b)
{
  return a == b;
}

WUInt32 WGALCommandEncoder::ValidationHash::Hash(const WBufferValidationError& a)
{
  return a.CalculateHash();
}

bool WGALCommandEncoder::ValidationHash::Equal(const WBufferValidationError& a, const WBufferValidationError& b)
{
  return a == b;
}

#endif

WEvent<const WTextureValidationError&> WGALCommandEncoder::s_TextureBarrierValidationFailed;
WEvent<const WBufferValidationError&> WGALCommandEncoder::s_BufferBarrierValidationFailed;

void WGALCommandEncoder::SetBindGroup(WUInt32 uiBindGroup, const WGALBindGroupCreationDescription& bindGroup)
{
  AssertRenderingThread();

  // Validation
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  {
    W_LOG_BLOCK("SetBindGroup");
    bindGroup.AssertValidDescription(m_Device);
  }
#endif

  W_ASSERT_DEBUG(uiBindGroup < W_GAL_MAX_BIND_GROUPS, "Bind group index {} out of range", uiBindGroup);
#if W_ENABLED(W_BARRIER_VALIDATION)
  m_BindGroups[uiBindGroup] = bindGroup;
  m_uiBindGroupsMask |= static_cast<WUInt8>(W_BIT(uiBindGroup));
#endif

  m_CommonImpl.SetBindGroupPlatform(uiBindGroup, bindGroup);
}

void WGALCommandEncoder::SetBindGroup(WUInt32 uiBindGroup, WGALBindGroupHandle hBindGroup)
{
  AssertRenderingThread();

  const WGALBindGroup* pBindGroup = m_Device.GetBindGroup(hBindGroup);
  W_ASSERT_DEBUG(pBindGroup != nullptr, "Bind group does not exist");
  W_ASSERT_DEBUG(!pBindGroup->IsInvalidated(), "Bind group is invalidated. One of its dependencies was destroyed.");

  W_ASSERT_DEBUG(uiBindGroup < W_GAL_MAX_BIND_GROUPS, "Bind group index {} out of range", uiBindGroup);
#if W_ENABLED(W_BARRIER_VALIDATION)
  m_BindGroups[uiBindGroup] = pBindGroup->GetDescription();
  m_uiBindGroupsMask |= static_cast<WUInt8>(W_BIT(uiBindGroup));
#endif

  m_CommonImpl.SetBindGroupPlatform(uiBindGroup, pBindGroup);
}

void WGALCommandEncoder::SetPushConstants(WArrayPtr<const WUInt8> data)
{
  AssertRenderingThread();
  m_CommonImpl.SetPushConstantsPlatform(data);
}

WGALTimestampHandle WGALCommandEncoder::InsertTimestamp()
{
  AssertRenderingThread();
  m_Stats.m_uiInsertTimestamp++;
  return m_CommonImpl.InsertTimestampPlatform();
}

WGALOcclusionHandle WGALCommandEncoder::BeginOcclusionQuery(WEnum<WGALQueryType> type)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Occlusion queries can only be started within a render scope");
  AssertRenderingThread();
  m_Stats.m_uiBeginOcclusionQuery++;
  WGALOcclusionHandle hOcclusion = m_CommonImpl.BeginOcclusionQueryPlatform(type);

  W_ASSERT_DEBUG(m_hPendingOcclusionQuery.IsInvalidated(), "Only one occusion query can be active at any give time.");
  m_hPendingOcclusionQuery = hOcclusion;

  return hOcclusion;
}

void WGALCommandEncoder::EndOcclusionQuery(WGALOcclusionHandle hOcclusion)
{
  AssertRenderingThread();
  m_CommonImpl.EndOcclusionQueryPlatform(hOcclusion);

  W_ASSERT_DEBUG(m_hPendingOcclusionQuery == hOcclusion, "The EndOcclusionQuery parameter does not match the currently started query");
  m_hPendingOcclusionQuery = {};
}

WGALFenceHandle WGALCommandEncoder::InsertFence()
{
  m_Stats.m_uiInsertFence++;
  return m_CommonImpl.InsertFencePlatform();
}

void WGALCommandEncoder::CopyBuffer(WGALBufferHandle hDest, WGALBufferHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const WGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    W_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
#if W_ENABLED(W_BARRIER_VALIDATION)
    ValidateBufferState(hDest, WGALResourceState::CopyDestination, WGALShaderStageFlags::Auto).IgnoreResult();
    ValidateBufferState(hSource, WGALResourceState::CopySource, WGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyBuffer++;
    m_CommonImpl.CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    W_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", WArgP(pDest), WArgP(pSource));
  }
}

void WGALCommandEncoder::CopyBufferRegion(
  WGALBufferHandle hDest, WUInt32 uiDestOffset, WGALBufferHandle hSource, WUInt32 uiSourceOffset, WUInt32 uiByteCount)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const WGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    W_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable buffers");
    const WUInt32 uiDestSize = pDest->GetSize();
    const WUInt32 uiSourceSize = pSource->GetSize();

    W_IGNORE_UNUSED(uiDestSize);
    W_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    W_IGNORE_UNUSED(uiSourceSize);
    W_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

#if W_ENABLED(W_BARRIER_VALIDATION)
    ValidateBufferState(hDest, WGALResourceState::CopyDestination, WGALShaderStageFlags::Auto).IgnoreResult();
    ValidateBufferState(hSource, WGALResourceState::CopySource, WGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyBuffer++;
    m_CommonImpl.CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    W_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", WArgP(pDest), WArgP(pSource));
  }
}

void WGALCommandEncoder::GALStaticDeviceEventHandler(const WGALDeviceEvent& e)
{
  W_IGNORE_UNUSED(e);
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  if (e.m_Type == WGALDeviceEvent::BeforeBeginFrame)
    m_BufferUpdates.Clear();
#endif
}

void WGALCommandEncoder::UpdateBuffer(WGALBufferHandle hDest, WUInt32 uiDestOffset, WArrayPtr<const WUInt8> sourceData, WGALUpdateMode::Enum updateMode)
{
  AssertRenderingThread();
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render || updateMode == WGALUpdateMode::TransientConstantBuffer || updateMode == WGALUpdateMode::AheadOfTime, "Only discard updates on dynamic buffers are supported within a render scope");

  W_ASSERT_DEV(!sourceData.IsEmpty(), "Source data for buffer update is invalid!");
  W_CHECK_ALIGNMENT(sourceData.GetPtr(), 16);

  const WGALBuffer* pDest = m_Device.GetBuffer(hDest);

  if (pDest != nullptr)
  {
    W_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable buffers");
    W_ASSERT_DEV(pDest->GetSize() >= (uiDestOffset + sourceData.GetCount()), "Buffer {} is too small (or offset {} too big) for {} bytes", pDest->GetSize(), uiDestOffset, sourceData.GetCount());

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (updateMode == WGALUpdateMode::AheadOfTime)
    {
      auto it = m_BufferUpdates.Find(hDest);
      if (!it.IsValid())
      {
        it = m_BufferUpdates.Insert(hDest, WHybridArray<BufferRange, 1>());
      }
      WHybridArray<BufferRange, 1>& ranges = it.Value();
      for (const BufferRange& range : ranges)
      {
        W_ASSERT_DEBUG(!range.overlapRange(uiDestOffset, sourceData.GetCount()), "A buffer was updated twice in one frame on the same memory range.");
      }
      BufferRange* pLastElement = ranges.IsEmpty() ? nullptr : &ranges.PeekBack();
      if (pLastElement && pLastElement->m_uiOffset + pLastElement->m_uiLength == uiDestOffset)
      {
        // In most cases we update the buffer in order so we can compact the write operations to reduce the number of elements we have to loop through.
        pLastElement->m_uiLength += sourceData.GetCount();
      }
      else
      {
        ranges.PushBack({uiDestOffset, sourceData.GetCount()});
      }
    }
#endif
    m_Stats.m_uiUpdateBuffer++;
    m_CommonImpl.UpdateBufferPlatform(pDest, uiDestOffset, sourceData, updateMode);
  }
  else
  {
    WLog::Error("UpdateBuffer failed, buffer handle invalid");
    // W_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void WGALCommandEncoder::CopyTexture(WGALTextureHandle hDest, WGALTextureHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALTexture* pDest = m_Device.GetTexture(hDest);
  const WGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    W_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
    W_ASSERT_DEBUG(pDest->GetDescription().m_Format == pSource->GetDescription().m_Format, "CopyTexture formats must match");
    W_ASSERT_DEBUG(pDest->GetDescription().m_SampleCount == pSource->GetDescription().m_SampleCount, "CopyTexture sample count must match");
    W_ASSERT_DEBUG(pDest->GetDescription().m_Type == pSource->GetDescription().m_Type, "CopyTexture type must match");
    W_ASSERT_DEBUG(pDest->GetDescription().m_uiHeight == pSource->GetDescription().m_uiHeight, "CopyTexture height must match");
    W_ASSERT_DEBUG(pDest->GetDescription().m_uiWidth == pSource->GetDescription().m_uiWidth, "CopyTexture width must match");
    W_ASSERT_DEBUG(pDest->GetDescription().m_uiDepth == pSource->GetDescription().m_uiDepth, "CopyTexture depth must match");
    W_ASSERT_DEBUG(pDest->GetDescription().m_uiMipLevelCount == pSource->GetDescription().m_uiMipLevelCount, "CopyTexture mip levels must match");
    W_ASSERT_DEBUG(pDest->GetDescription().m_uiArraySize == pSource->GetDescription().m_uiArraySize, "CopyTexture array size must match");

#if W_ENABLED(W_BARRIER_VALIDATION)
    ValidateTextureState(hDest, {}, WGALResourceState::CopyDestination, WGALShaderStageFlags::Auto).IgnoreResult();
    ValidateTextureState(hSource, {}, WGALResourceState::CopySource, WGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyTexture++;
    m_CommonImpl.CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    W_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}", WArgP(pDest), WArgP(pSource));
  }
}

void WGALCommandEncoder::CopyTextureRegion(WGALTextureHandle hDest, const WGALTextureSubresource& destinationSubResource,
  const WVec3U32& vDestinationPoint, WGALTextureHandle hSource, const WGALTextureSubresource& sourceSubResource, const WBoundingBoxu32& box)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALTexture* pDest = m_Device.GetTexture(hDest);
  const WGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    W_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
#if W_ENABLED(W_BARRIER_VALIDATION)
    ValidateTextureState(hDest, MakeSingleSubresourceRange(destinationSubResource), WGALResourceState::CopyDestination, WGALShaderStageFlags::Auto).IgnoreResult();
    ValidateTextureState(hSource, MakeSingleSubresourceRange(sourceSubResource), WGALResourceState::CopySource, WGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyTexture++;
    m_CommonImpl.CopyTextureRegionPlatform(pDest, destinationSubResource, vDestinationPoint, pSource, sourceSubResource, box);
  }
  else
  {
    W_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}", WArgP(pDest), WArgP(pSource));
  }
}

void WGALCommandEncoder::UpdateTexture(WGALTextureHandle hDest, const WGALTextureSubresource& destinationSubResource,
  const WBoundingBoxu32& destinationBox, const WGALSystemMemoryDescription& sourceData)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALTexture* pDest = m_Device.GetTexture(hDest);

  if (pDest != nullptr)
  {
    W_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
    m_Stats.m_uiUpdateTexture++;
    m_CommonImpl.UpdateTexturePlatform(pDest, destinationSubResource, destinationBox, sourceData);
  }
  else
  {
    W_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}", WArgP(pDest));
  }
}

void WGALCommandEncoder::ResolveTexture(WGALTextureHandle hDest, const WGALTextureSubresource& destinationSubResource, WGALTextureHandle hSource,
  const WGALTextureSubresource& sourceSubResource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALTexture* pDest = m_Device.GetTexture(hDest);
  const WGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    W_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
#if W_ENABLED(W_BARRIER_VALIDATION)
    ValidateTextureState(hDest, MakeSingleSubresourceRange(destinationSubResource), WGALResourceState::ResolveDestination, WGALShaderStageFlags::Auto).IgnoreResult();
    ValidateTextureState(hSource, MakeSingleSubresourceRange(sourceSubResource), WGALResourceState::ResolveSource, WGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiResolveTexture++;
    m_CommonImpl.ResolveTexturePlatform(pDest, destinationSubResource, pSource, sourceSubResource);
  }
  else
  {
    W_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}", WArgP(pDest), WArgP(pSource));
  }
}

void WGALCommandEncoder::ReadbackTexture(WGALReadbackTextureHandle hDestination, WGALTextureHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALReadbackTexture* pDestination = m_Device.GetReadbackTexture(hDestination);
  const WGALTexture* pSource = m_Device.GetTexture(hSource);

  W_ASSERT_DEBUG(pDestination != nullptr && pSource != nullptr, "Invalid handle provided");

  const WGALTextureCreationDescription& sourceDesc = pSource->GetDescription();
  const WGALTextureCreationDescription& destinationDesc = pDestination->GetDescription();

  bool bMissmatch = sourceDesc.m_uiWidth != destinationDesc.m_uiWidth || sourceDesc.m_uiHeight != destinationDesc.m_uiHeight || sourceDesc.m_uiDepth != destinationDesc.m_uiDepth || sourceDesc.m_uiMipLevelCount != destinationDesc.m_uiMipLevelCount || sourceDesc.m_uiArraySize != destinationDesc.m_uiArraySize || sourceDesc.m_Format != destinationDesc.m_Format || sourceDesc.m_Type != destinationDesc.m_Type || sourceDesc.m_Format != destinationDesc.m_Format || sourceDesc.m_SampleCount != destinationDesc.m_SampleCount;
  W_IGNORE_UNUSED(bMissmatch);
  W_ASSERT_DEBUG(!bMissmatch, "Source and destination formats do not match");

  if (pDestination != nullptr && pSource != nullptr)
  {
#if W_ENABLED(W_BARRIER_VALIDATION)
    ValidateTextureState(hSource, {}, WGALResourceState::CopySource, WGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiReadbackTexture++;
    m_CommonImpl.ReadbackTexturePlatform(pDestination, pSource);
  }
}


void WGALCommandEncoder::ReadbackBuffer(WGALReadbackBufferHandle hDestination, WGALBufferHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const WGALReadbackBuffer* pDestination = m_Device.GetReadbackBuffer(hDestination);
  const WGALBuffer* pSource = m_Device.GetBuffer(hSource);

  W_ASSERT_DEBUG(pDestination != nullptr && pSource != nullptr, "Invalid handle provided");
  W_ASSERT_DEBUG(pSource->GetDescription().m_uiTotalSize == pDestination->GetDescription().m_uiTotalSize, "Source and destination size do not match");

  if (pDestination != nullptr && pSource != nullptr)
  {
#if W_ENABLED(W_BARRIER_VALIDATION)
    ValidateBufferState(hSource, WGALResourceState::CopySource, WGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiReadbackBuffer++;
    m_CommonImpl.ReadbackBufferPlatform(pDestination, pSource);
  }
}

void WGALCommandEncoder::Flush()
{
  AssertRenderingThread();
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render, "Flush can't be called inside a rendering scope");

  m_Stats.m_uiFlush++;
  m_CommonImpl.FlushPlatform();
}

void WGALCommandEncoder::TextureBarrier(WArrayPtr<const WGALTextureBarrier> barriers)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

#if W_ENABLED(W_BARRIER_VALIDATION)
  ValidateTextureBarriers(barriers);
#endif

  m_CommonImpl.TextureBarrierPlatform(barriers);
}

void WGALCommandEncoder::TextureBarrier(
  WGALTextureHandle hTexture,
  WGALTextureRange range,
  WBitflags<WGALResourceState> stateBefore,
  WBitflags<WGALResourceState> stateAfter,
  WBitflags<WGALShaderStageFlags> stagesBefore,
  WBitflags<WGALShaderStageFlags> stagesAfter)
{
  if (range == WGALTextureRange{})
  {
    const WGALTextureBarrier barrier{
      hTexture,
      stateBefore,
      stateAfter,
      stagesBefore,
      stagesAfter,
      {},
      true};

    TextureBarrier(WArrayPtr<const WGALTextureBarrier>(&barrier, 1));
    return;
  }

  const WGALTexture* pTexture = m_Device.GetTexture(hTexture);
  W_ASSERT_DEBUG(pTexture != nullptr, "Invalid handle provided");
  if (pTexture == nullptr)
    return;

  range = pTexture->ClampRange(range);

  WHybridArray<WGALTextureBarrier, 16> barriers;
  barriers.Reserve(static_cast<WUInt32>(range.m_uiMipLevels) * static_cast<WUInt32>(range.m_uiArraySlices));

  for (WUInt32 uiArraySlice = range.m_uiBaseArraySlice; uiArraySlice < static_cast<WUInt32>(range.m_uiBaseArraySlice + range.m_uiArraySlices); ++uiArraySlice)
  {
    for (WUInt32 uiMipLevel = range.m_uiBaseMipLevel; uiMipLevel < static_cast<WUInt32>(range.m_uiBaseMipLevel + range.m_uiMipLevels); ++uiMipLevel)
    {
      barriers.PushBack({hTexture,
        stateBefore,
        stateAfter,
        stagesBefore,
        stagesAfter,
        {uiMipLevel, uiArraySlice},
        false});
    }
  }

  TextureBarrier(barriers);
}

void WGALCommandEncoder::BufferBarrier(WArrayPtr<const WGALBufferBarrier> barriers)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

#if W_ENABLED(W_BARRIER_VALIDATION)
  ValidateBufferBarriers(barriers);
#endif

  m_CommonImpl.BufferBarrierPlatform(barriers);
}

void WGALCommandEncoder::BufferBarrier(
  WGALBufferHandle hBuffer,
  WBitflags<WGALResourceState> stateBefore,
  WBitflags<WGALResourceState> stateAfter,
  WBitflags<WGALShaderStageFlags> stagesBefore,
  WBitflags<WGALShaderStageFlags> stagesAfter)
{
  const WGALBufferBarrier barrier{
    hBuffer,
    stateBefore,
    stateAfter,
    stagesBefore,
    stagesAfter};

  BufferBarrier(WArrayPtr<const WGALBufferBarrier>(&barrier, 1));
}

// Debug helper functions

void WGALCommandEncoder::PushMarker(const char* szMarker)
{
  AssertRenderingThread();

  W_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.PushMarkerPlatform(szMarker);
}

void WGALCommandEncoder::PopMarker()
{
  AssertRenderingThread();

  m_CommonImpl.PopMarkerPlatform();
}

void WGALCommandEncoder::InsertEventMarker(const char* szMarker)
{
  AssertRenderingThread();

  W_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");
  m_CommonImpl.InsertEventMarkerPlatform(szMarker);
}

WGALCommandEncoder::WGALCommandEncoder(WGALDevice& ref_device, WGALCommandEncoderCommonPlatformInterface& ref_commonImpl)
  : m_Device(ref_device)
  , m_CommonImpl(ref_commonImpl)
#if W_ENABLED(W_BARRIER_VALIDATION)
  , m_ResourceStateTracker(&ref_device)
#endif
{
  WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WGALCommandEncoder::GALStaticDeviceEventHandler, this));
}

WGALCommandEncoder::~WGALCommandEncoder()
{
  WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&WGALCommandEncoder::GALStaticDeviceEventHandler, this));
}

void WGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
#if W_ENABLED(W_BARRIER_VALIDATION)
  m_ResourceStateTracker.Clear();
  m_uiBindGroupsMask = 0;
  m_bVertexBufferStatesDirty = true;
  m_bIndexBufferStateDirty = true;
#endif
}

WResult WGALCommandEncoder::Dispatch(WUInt32 uiThreadGroupCountX, WUInt32 uiThreadGroupCountY, WUInt32 uiThreadGroupCountZ)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  W_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateComputePipelineResources());
#endif

  m_Stats.m_uiDispatch++;
  return m_CommonImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

WResult WGALCommandEncoder::DispatchIndirect(WGALBufferHandle hIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  const WGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  W_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateComputePipelineResources());
  W_SUCCEED_OR_RETURN(ValidateBufferState(hIndirectArgumentBuffer, WGALResourceState::DrawIndirect, WGALShaderStageFlags::Auto));
#endif

  m_Stats.m_uiDispatch++;
  return m_CommonImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void WGALCommandEncoder::Clear(const WColor& clearColor, WUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, WUInt8 uiStencilClear /*= 0x0u*/)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  m_Stats.m_uiClear++;
  m_CommonImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

WResult WGALCommandEncoder::Draw(WUInt32 uiVertexCount, WUInt32 uiStartVertex)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  W_SUCCEED_OR_RETURN(ValidateVertexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawPlatform(uiVertexCount, uiStartVertex);
}

WResult WGALCommandEncoder::DrawIndexed(WUInt32 uiIndexCount, WUInt32 uiStartIndex)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  W_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  W_SUCCEED_OR_RETURN(ValidateIndexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

WResult WGALCommandEncoder::DrawIndexedInstanced(WUInt32 uiIndexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartIndex)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  W_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  W_SUCCEED_OR_RETURN(ValidateIndexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

WResult WGALCommandEncoder::DrawIndexedInstancedIndirect(WGALBufferHandle hIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const WGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  W_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  W_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  W_SUCCEED_OR_RETURN(ValidateIndexBufferState());
  W_SUCCEED_OR_RETURN(ValidateBufferState(hIndirectArgumentBuffer, WGALResourceState::DrawIndirect, WGALShaderStageFlags::Auto));
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

WResult WGALCommandEncoder::DrawInstanced(WUInt32 uiVertexCountPerInstance, WUInt32 uiInstanceCount, WUInt32 uiStartVertex)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  W_SUCCEED_OR_RETURN(ValidateVertexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

WResult WGALCommandEncoder::DrawInstancedIndirect(WGALBufferHandle hIndirectArgumentBuffer, WUInt32 uiArgumentOffsetInBytes)
{
  W_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const WGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  W_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

#if W_ENABLED(W_BARRIER_VALIDATION)
  W_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  W_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  W_SUCCEED_OR_RETURN(ValidateBufferState(hIndirectArgumentBuffer, WGALResourceState::DrawIndirect, WGALShaderStageFlags::Auto));
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void WGALCommandEncoder::SetIndexBuffer(WGALBufferHandle hIndexBuffer)
{
  if (m_State.m_hIndexBuffer == hIndexBuffer)
  {
    return;
  }

  const WGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);

#if W_ENABLED(W_BARRIER_VALIDATION)
  m_bIndexBufferStateDirty = true;
#endif

  m_Stats.m_uiSetIndexBuffer++;
  m_CommonImpl.SetIndexBufferPlatform(pBuffer);

  m_State.m_hIndexBuffer = hIndexBuffer;
}

void WGALCommandEncoder::SetVertexBuffer(WUInt32 uiSlot, WGALBufferHandle hVertexBuffer, WUInt32 uiOffset)
{
  if (m_State.m_hVertexBuffers[uiSlot] == hVertexBuffer && m_State.m_hVertexBufferOffsets[uiSlot] == uiOffset)
  {
    return;
  }

  const WGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);

#if W_ENABLED(W_BARRIER_VALIDATION)
  m_bVertexBufferStatesDirty = true;
#endif

  m_Stats.m_uiSetVertexBuffer++;
  m_CommonImpl.SetVertexBufferPlatform(uiSlot, pBuffer, uiOffset);

  m_State.m_hVertexBuffers[uiSlot] = hVertexBuffer;
  m_State.m_hVertexBufferOffsets[uiSlot] = uiOffset;
}

void WGALCommandEncoder::SetGraphicsPipeline(WGALGraphicsPipelineHandle hGraphicsPipeline)
{
  AssertRenderingThread();
  m_State.m_hComputePipeline.Invalidate();
  if (m_State.m_hGraphicsPipeline == hGraphicsPipeline)
    return;

  const WGALGraphicsPipeline* pGraphicsPipeline = GetDevice().GetGraphicsPipeline(hGraphicsPipeline);
  m_Stats.m_uiSetGraphicsPipeline++;
  m_CommonImpl.SetGraphicsPipelinePlatform(pGraphicsPipeline);
  m_State.m_hGraphicsPipeline = hGraphicsPipeline;
}

void WGALCommandEncoder::SetComputePipeline(WGALComputePipelineHandle hComputePipeline)
{
  AssertRenderingThread();
  m_State.m_hGraphicsPipeline.Invalidate();
  if (m_State.m_hComputePipeline == hComputePipeline)
    return;

  const WGALComputePipeline* pComputePipeline = GetDevice().GetComputePipeline(hComputePipeline);
  m_Stats.m_uiSetComputePipeline++;
  m_CommonImpl.SetComputePipelinePlatform(pComputePipeline);
  m_State.m_hComputePipeline = hComputePipeline;
}

void WGALCommandEncoder::SetViewport(const WRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_State.m_ViewPortRect == rect && m_State.m_fViewPortMinDepth == fMinDepth && m_State.m_fViewPortMaxDepth == fMaxDepth)
  {
    return;
  }

  m_Stats.m_uiSetViewport++;
  m_CommonImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_State.m_ViewPortRect = rect;
  m_State.m_fViewPortMinDepth = fMinDepth;
  m_State.m_fViewPortMaxDepth = fMaxDepth;
}

void WGALCommandEncoder::SetScissorRect(const WRectU32& rect)
{
  AssertRenderingThread();

  if (m_State.m_ScissorRect == rect)
  {
    return;
  }

  m_Stats.m_uiSetScissorRect++;
  m_CommonImpl.SetScissorRectPlatform(rect);

  m_State.m_ScissorRect = rect;
}

void WGALCommandEncoder::SetStencilReference(WUInt8 uiStencilRefValue)
{
  AssertRenderingThread();

  if (m_State.m_uiStencilRefValue == uiStencilRefValue)
    return;

  m_Stats.m_uiSetStencilReference++;
  m_CommonImpl.SetStencilReferencePlatform(uiStencilRefValue);

  m_State.m_uiStencilRefValue = uiStencilRefValue;
}

void WGALCommandEncoder::BeginCompute(const char* szName)
{
  W_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  m_Stats.m_uiBeginCompute++;
  m_CommonImpl.BeginComputePlatform();

  m_bMarker = !WStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    PushMarker(szName);
  }
}

void WGALCommandEncoder::EndCompute()
{
  W_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "BeginCompute has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    PopMarker();
    m_bMarker = false;
  }

  m_CommonImpl.EndComputePlatform();
}

void WGALCommandEncoder::BeginRendering(const WGALRenderingSetup& renderingSetup, const char* szName)
{
  W_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

#if W_ENABLED(W_BARRIER_VALIDATION)
  ValidateRenderTargetStates(renderingSetup);
  m_bVertexBufferStatesDirty = true;
  m_bIndexBufferStateDirty = true;
#endif

  m_Stats.m_uiBeginRendering++;
  m_CommonImpl.BeginRenderingPlatform(renderingSetup);

  m_bMarker = !WStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    PushMarker(szName);
  }
}

void WGALCommandEncoder::EndRendering()
{
  W_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Render, "BeginRendering has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    PopMarker();
    m_bMarker = false;
  }

  W_ASSERT_DEBUG(m_hPendingOcclusionQuery.IsInvalidated(), "An occlusion query was started and not stopped within this render scope.");

  m_CommonImpl.EndRenderingPlatform();
}

bool WGALCommandEncoder::IsInRenderingScope() const
{
  return m_CurrentCommandEncoderType == CommandEncoderType::Render;
}

void WGALCommandEncoder::ResetStats()
{
  m_Stats = WGALCommandEncoderStats();
}

#if W_ENABLED(W_BARRIER_VALIDATION)

WResult WGALCommandEncoder::ValidateGraphicsPipelineResources()
{
  if (const WGALGraphicsPipeline* pPipeline = m_Device.GetGraphicsPipeline(m_State.m_hGraphicsPipeline))
    return ValidateBindGroupResourceStates(m_Device.GetShader(pPipeline->GetDescription().m_hShader));

  return W_SUCCESS;
}

WResult WGALCommandEncoder::ValidateComputePipelineResources()
{
  if (const WGALComputePipeline* pPipeline = m_Device.GetComputePipeline(m_State.m_hComputePipeline))
    return ValidateBindGroupResourceStates(m_Device.GetShader(pPipeline->GetDescription().m_hShader));

  return W_SUCCESS;
}

WResult WGALCommandEncoder::ValidateVertexBufferState()
{
  if (m_bVertexBufferStatesDirty)
  {
    for (WGALBufferHandle hVertexBuffer : m_State.m_hVertexBuffers)
    {
      if (!hVertexBuffer.IsInvalidated())
      {
        W_SUCCEED_OR_RETURN(ValidateBufferState(hVertexBuffer, WGALResourceState::VertexBuffer, WGALShaderStageFlags::Auto));
      }
    }

    m_bVertexBufferStatesDirty = false;
  }

  return W_SUCCESS;
}

WResult WGALCommandEncoder::ValidateIndexBufferState()
{
  if (m_bIndexBufferStateDirty)
  {
    if (!m_State.m_hIndexBuffer.IsInvalidated())
    {
      W_SUCCEED_OR_RETURN(ValidateBufferState(m_State.m_hIndexBuffer, WGALResourceState::IndexBuffer, WGALShaderStageFlags::Auto));
    }

    m_bIndexBufferStateDirty = false;
  }

  return W_SUCCESS;
}

WResult WGALCommandEncoder::ValidateBindGroupResourceStates(const WGALShader* pShader)
{
  if (pShader == nullptr)
  {
    WLog::Error("Can't validate bind groups, shader is invalid.");
    return W_FAILURE;
  }
  const WUInt32 uiBindGroupCount = pShader->GetBindGroupCount();
  for (WUInt32 uiBindGroup = 0; uiBindGroup < uiBindGroupCount; ++uiBindGroup)
  {
    if ((m_uiBindGroupsMask & W_BIT(uiBindGroup)) == 0)
      continue;
    // Only check once per bind group
    m_uiBindGroupsMask &= ~static_cast<WUInt8>(W_BIT(uiBindGroup));

    const WGALBindGroupLayout* pLayout = m_Device.GetBindGroupLayout(pShader->GetBindGroupLayout(uiBindGroup));
    if (pLayout == nullptr)
    {
      WLog::Error("Can't validate bind group {}, bind group layout is invalid.", uiBindGroup);
      continue;
    }
    WArrayPtr<const WShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
    const WGALBindGroupCreationDescription& bindGroupDesc = m_BindGroups[uiBindGroup];

    if (bindGroupDesc.m_BindGroupItems.GetCount() != bindings.GetCount())
    {
      WLog::Error("Can't validate bind group {}, binding count {} does not match bind group item count {}", uiBindGroup, bindings.GetCount(), bindGroupDesc.m_BindGroupItems.GetCount());
      continue;
    }

    for (WUInt32 i = 0; i < bindings.GetCount(); ++i)
    {
      const WShaderResourceBinding& binding = bindings[i];
      const WGALBindGroupItem& item = bindGroupDesc.m_BindGroupItems[i];
      if (ValidateBindGroupItemResourceState(uiBindGroup, binding, item).Failed())
      {

        return W_FAILURE;
      }
    }
  }
  return W_SUCCESS;
}

WResult WGALCommandEncoder::ValidateBindGroupItemResourceState(WUInt32 uiBindGroup, const WShaderResourceBinding& binding, const WGALBindGroupItem& item)
{
  const WBitflags<WGALBindGroupItemFlags> typeFlags = item.m_Flags & WGALBindGroupItemFlags::TypeFlags;
  if (typeFlags.IsSet(WGALBindGroupItemFlags::Texture))
  {
    const WBitflags<WGALResourceState> expectedState = GetExpectedResourceState(binding.m_ResourceType, m_Device, item.m_Texture.m_hTexture);
    W_ASSERT_DEBUG(!expectedState.IsNoFlagSet(), "At least one flag should e set in the expected resource state.");
    return ValidateTextureState(item.m_Texture.m_hTexture, item.m_Texture.m_TextureRange, expectedState, binding.m_Stages, uiBindGroup, binding.m_sName);
  }
  else if (typeFlags.IsSet(WGALBindGroupItemFlags::Buffer))
  {
    const WBitflags<WGALResourceState> expectedState = GetExpectedResourceState(binding.m_ResourceType, m_Device);
    return ValidateBufferState(item.m_Buffer.m_hBuffer, expectedState, binding.m_Stages, uiBindGroup, binding.m_sName);
  }
  // Samplers and push constants have no resource state to validate.
  return W_SUCCESS;
}

WResult WGALCommandEncoder::ValidateTextureState(WGALTextureHandle hTexture, WGALTextureRange range, WBitflags<WGALResourceState> expectedState, WBitflags<WGALShaderStageFlags> expectedStages, WUInt32 uiBindGroup, const WHashedString& sBinding)
{
  if (expectedState.IsNoFlagSet())
    return W_SUCCESS;

  ResolveProxyTexture(&m_Device, hTexture, range);

  const WGALTexture* pTexture = m_Device.GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    if (sBinding.IsEmpty())
      WLog::Error("Can't validate texture state as the texture is invalid.");
    else
      WLog::Error("Can't validate bind group {} binding '{}' as the texture is invalid.", uiBindGroup, sBinding);
    return W_FAILURE;
  }

  range = pTexture->ClampRange(range);

  const WGALResourceStateTracker::TextureState* pState = m_ResourceStateTracker.GetTextureState(hTexture);
  WGALResourceStateTracker::TextureState dummy;
  if (pState == nullptr)
  {
    // Nothing is tracked, compare to default state.
    dummy.m_FullRange = pTexture->ClampRange({});
    dummy.m_SubResourceStates.PushBack({pTexture->GetDescription().GetDefaultState(), WGALShaderStageFlags::Auto});
    pState = &dummy;
  }

  WTextureValidationError error;
  error.m_uiBindGroup = uiBindGroup;
  error.m_sBinding = sBinding;
  error.m_hTexture = hTexture;
  error.m_expectedState = expectedState;
  error.m_expectedStages = expectedStages;

  if (CheckTextureSubResourceState(*pState, range, error).Failed())
  {
    if (!m_TextureErrors.Insert(error))
      s_TextureBarrierValidationFailed.Broadcast(error);
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WGALCommandEncoder::ValidateBufferState(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> expectedState, WBitflags<WGALShaderStageFlags> expectedStages, WUInt32 uiBindGroup, const WHashedString& sBinding)
{
  if (expectedState.IsNoFlagSet())
    return W_SUCCESS;

  const WGALResourceStateTracker::SubResourceState* pState = m_ResourceStateTracker.GetBufferState(hBuffer);
  WGALResourceStateTracker::SubResourceState dummy;
  if (pState == nullptr)
  {
    // Nothing is tracked, compare to default state.
    const WGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
    if (pBuffer == nullptr)
    {
      if (sBinding.IsEmpty())
        WLog::Error("Can't validate buffer state as the buffer is invalid.");
      else
        WLog::Error("Can't validate bind group {} binding '{}' as the buffer is invalid.", uiBindGroup, sBinding);
      return W_FAILURE;
    }
    dummy.m_State = pBuffer->GetDescription().GetDefaultState();
    dummy.m_Stages = WGALShaderStageFlags::Auto;
    pState = &dummy;
  }

  if (WGALResourceStateTracker::IsBufferBarrierNeeded(*pState, {expectedState, expectedStages}, false))
  {
    WBufferValidationError error;
    error.m_uiBindGroup = uiBindGroup;
    error.m_sBinding = sBinding;
    error.m_hBuffer = hBuffer;
    error.m_expectedState = expectedState;
    error.m_expectedStages = expectedStages;
    error.m_actualState = pState->m_State;
    error.m_actualStages = pState->m_Stages;

    if (!m_BufferErrors.Insert(error))
      s_BufferBarrierValidationFailed.Broadcast(error);
    return W_FAILURE;
  }

  return W_SUCCESS;
}

void WGALCommandEncoder::ValidateTextureBarriers(WArrayPtr<const WGALTextureBarrier> barriers)
{
  for (const WGALTextureBarrier& barrier : barriers)
  {
    const WGALResourceStateTracker::TextureState* pState = m_ResourceStateTracker.GetTextureState(barrier.m_hTexture);
    if (pState != nullptr)
    {
      if (pState->m_SubResourceStates.GetCount() == 1)
      {
        W_ASSERT_DEV(pState->m_SubResourceStates[0].m_State == barrier.m_StateBefore,
          "Texture barrier before-state mismatch. Tracked: {}, Barrier: {}",
          WArgEnum(pState->m_SubResourceStates[0].m_State), WArgEnum(barrier.m_StateBefore));
      }
      else if (!barrier.m_bAllSubresources)
      {
        const WUInt32 uiSubresourceIndex = barrier.m_Subresource.m_uiMipLevel + barrier.m_Subresource.m_uiArraySlice * pState->m_FullRange.m_uiMipLevels;
        if (uiSubresourceIndex < pState->m_SubResourceStates.GetCount())
        {
          W_ASSERT_DEV(pState->m_SubResourceStates[uiSubresourceIndex].m_State == barrier.m_StateBefore,
            "Texture barrier before-state mismatch at subresource (mip={}, slice={}). Tracked: {}, Barrier: {}",
            barrier.m_Subresource.m_uiMipLevel, barrier.m_Subresource.m_uiArraySlice,
            WArgEnum(pState->m_SubResourceStates[uiSubresourceIndex].m_State), WArgEnum(barrier.m_StateBefore));
        }
      }
    }

    WGALTextureRange range;
    if (barrier.m_bAllSubresources)
    {
      range = {};
    }
    else
    {
      range.m_uiBaseMipLevel = static_cast<WUInt8>(barrier.m_Subresource.m_uiMipLevel);
      range.m_uiMipLevels = 1;
      range.m_uiBaseArraySlice = static_cast<WUInt16>(barrier.m_Subresource.m_uiArraySlice);
      range.m_uiArraySlices = 1;
    }
    m_ResourceStateTracker.ChangeState(barrier.m_hTexture, range, barrier.m_StateAfter, barrier.m_StagesAfter, [](const WGALTextureBarrier&) {});
  }
}

void WGALCommandEncoder::ValidateBufferBarriers(WArrayPtr<const WGALBufferBarrier> barriers)
{
  for (const WGALBufferBarrier& barrier : barriers)
  {
    const WGALResourceStateTracker::SubResourceState* pState = m_ResourceStateTracker.GetBufferState(barrier.m_hBuffer);
    if (pState != nullptr)
    {
      W_ASSERT_DEV(pState->m_State == barrier.m_StateBefore,
        "Buffer barrier before-state mismatch. Tracked: {}, Barrier: {}",
        WArgEnum(pState->m_State), WArgEnum(barrier.m_StateBefore));
    }

    m_ResourceStateTracker.ChangeState(barrier.m_hBuffer, barrier.m_StateAfter, barrier.m_StagesAfter, [](const WGALBufferBarrier&) {});
  }
}

void WGALCommandEncoder::ValidateRenderTargetStates(const WGALRenderingSetup& renderingSetup)
{
  const WGALFrameBufferDescriptor& fb = renderingSetup.GetFrameBuffer();

  for (WUInt8 i = 0; i < renderingSetup.GetColorTargetCount(); ++i)
  {
    if (fb.m_hColorTarget[i].IsInvalidated())
      continue;

    const WGALRenderTargetView* pView = m_Device.GetRenderTargetView(fb.m_hColorTarget[i]);
    if (pView == nullptr)
      continue;

    const auto& viewDesc = pView->GetDescription();
    const WGALTextureRange colorRange = WGALTextureRange::MakeFromRenderTargetRange({static_cast<WUInt16>(viewDesc.m_uiFirstSlice), static_cast<WUInt16>(viewDesc.m_uiSliceCount), static_cast<WUInt8>(viewDesc.m_uiMipLevel)});
    ValidateTextureState(viewDesc.m_hTexture, colorRange, WGALResourceState::RenderTarget, WGALShaderStageFlags::Auto).IgnoreResult();
  }

  if (!fb.m_hDepthTarget.IsInvalidated())
  {
    const WGALRenderTargetView* pView = m_Device.GetRenderTargetView(fb.m_hDepthTarget);
    if (pView != nullptr)
    {
      const auto& viewDesc = pView->GetDescription();
      const WBitflags<WGALResourceState> expectedState = viewDesc.m_bReadOnly ? WGALResourceState::DepthStencilRead : WGALResourceState::DepthStencilWrite;
      const WGALTextureRange depthRange = WGALTextureRange::MakeFromRenderTargetRange({static_cast<WUInt16>(viewDesc.m_uiFirstSlice), static_cast<WUInt16>(viewDesc.m_uiSliceCount), static_cast<WUInt8>(viewDesc.m_uiMipLevel)});
      ValidateTextureState(viewDesc.m_hTexture, depthRange, expectedState, WGALShaderStageFlags::Auto).IgnoreResult();
    }
  }
}

#endif // W_BARRIER_VALIDATION
