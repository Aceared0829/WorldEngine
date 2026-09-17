#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphHistogramConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphMinMaxConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphPreviewConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphReadbackPixelConstants.h>

static WAtomicInteger32 s_uiObserverTextureCounter;

namespace
{
  constexpr WUInt32 s_uiHistogramBinCount = 256;
  constexpr WUInt32 s_uiHistogramChannelCount = 4;
  constexpr WUInt32 s_uiMinMaxValueCount = 2;

  float GetAdjustedHistogramMax(float fMin, float fMax)
  {
    const float fRange = WMath::Max(fMax - fMin, 0.0000001f);
    return fMax + fRange * 0.000001f;
  }

  float OrderedUintToFloat(WUInt32 uiOrderedValue)
  {
    // Inverse of FloatToOrderedUint in RenderGraphBuildMinMax.WShader.
    // The shader stores min/max floats in an integer domain that preserves float ordering for atomic operations.
    const WUInt32 uiRawValue = (uiOrderedValue & 0x80000000u) != 0u ? (uiOrderedValue ^ 0x80000000u) : ~uiOrderedValue;
    return *reinterpret_cast<const float*>(&uiRawValue);
  }
} // namespace

WRenderGraphPassObserver::WRenderGraphPassObserver(WGALDevice* pDevice)
  : m_pDevice(pDevice)
{
  m_Response.m_Histogram.SetCount(s_uiHistogramBinCount * s_uiHistogramChannelCount);
  WMemoryUtils::ZeroFill(m_Response.m_Histogram.GetData(), m_Response.m_Histogram.GetCount());
}

WRenderGraphPassObserver::~WRenderGraphPassObserver()
{
  DestroyInspectionResources();
  m_hCopyTextureResource.Invalidate();
  m_hCopyTexture.Invalidate();
}

WRenderGraphObserverResponse WRenderGraphPassObserver::GetResponse() const
{
  W_LOCK(m_Mutex);
  return m_Response;
}

void WRenderGraphPassObserver::SetRequest(const WRenderGraphObserverRequest& request)
{
  W_LOCK(m_Mutex);
  m_PendingRequest = request;
  m_bPendingRequestDirty = true;
}

void WRenderGraphPassObserver::ApplyPendingRequest()
{
  W_LOCK(m_Mutex);
  if (m_bPendingRequestDirty)
  {
    m_Request = m_PendingRequest;
    m_pGraph = WRenderGraphManager::GetRenderGraphById(m_Request.m_uiRenderGraphId);
    m_hSwapChain = WRenderGraphManager::GetSwapChainById(m_Request.m_uiSwapChainId);
    m_bPendingRequestDirty = false;
  }
}

void WRenderGraphPassObserver::EnsureInspectionResources()
{
  if (m_hPixelReadbackBuffer.IsInvalidated())
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WVec4);
    desc.m_uiTotalSize = desc.m_uiStructSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hPixelReadbackBuffer = m_pDevice->CreateBuffer(desc);
  }

  if (m_hHistogramBuffer.IsInvalidated())
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WUInt32);
    desc.m_uiTotalSize = s_uiHistogramBinCount * s_uiHistogramChannelCount * desc.m_uiStructSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hHistogramBuffer = m_pDevice->CreateBuffer(desc);
  }

  if (m_hMinMaxBuffer.IsInvalidated())
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WUInt32);
    desc.m_uiTotalSize = s_uiMinMaxValueCount * desc.m_uiStructSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hMinMaxBuffer = m_pDevice->CreateBuffer(desc);
  }

  if (!m_hReadbackPixelShader.IsValid())
  {
    m_hReadbackPixelShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RenderGraph/RenderGraphReadbackPixel.WShader");
  }

  if (!m_hClearHistogramShader.IsValid())
  {
    m_hClearHistogramShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RenderGraph/RenderGraphClearHistogram.WShader");
  }

  if (!m_hBuildHistogramShader.IsValid())
  {
    m_hBuildHistogramShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RenderGraph/RenderGraphBuildHistogram.WShader");
  }

  if (!m_hClearMinMaxShader.IsValid())
  {
    m_hClearMinMaxShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RenderGraph/RenderGraphClearMinMax.WShader");
  }

  if (!m_hBuildMinMaxShader.IsValid())
  {
    m_hBuildMinMaxShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RenderGraph/RenderGraphBuildMinMax.WShader");
  }

  if (!m_hPreviewShader.IsValid())
  {
    m_hPreviewShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RenderGraph/RenderGraphPreview.WShader");
  }
}

void WRenderGraphPassObserver::DestroyInspectionResources()
{
  m_PixelReadback.Reset();
  m_HistogramReadback.Reset();
  m_MinMaxReadback.Reset();
  m_pDevice->DestroyBuffer(m_hPixelReadbackBuffer);
  m_pDevice->DestroyBuffer(m_hHistogramBuffer);
  m_pDevice->DestroyBuffer(m_hMinMaxBuffer);
  m_hReadbackPixelShader.Invalidate();
  m_hClearHistogramShader.Invalidate();
  m_hBuildHistogramShader.Invalidate();
  m_hClearMinMaxShader.Invalidate();
  m_hBuildMinMaxShader.Invalidate();
  m_hPreviewShader.Invalidate();
}

void WRenderGraphPassObserver::PollReadbacks()
{
  if (m_bPixelReadbackInFlight)
  {
    const WEnum<WGALAsyncResult> result = m_PixelReadback.GetReadbackResult(WTime::MakeZero());
    if (result == WGALAsyncResult::Ready)
    {
      WArrayPtr<const WUInt8> memory;
      if (WReadbackBufferLock lock = m_PixelReadback.LockBuffer(memory))
      {
        ProcessPixelReadback(memory);
      }
      m_bPixelReadbackInFlight = false;
    }
    else if (result == WGALAsyncResult::Expired)
    {
      m_bPixelReadbackInFlight = false;
    }
  }

  if (m_bHistogramReadbackInFlight)
  {
    const WEnum<WGALAsyncResult> result = m_HistogramReadback.GetReadbackResult(WTime::MakeZero());
    if (result == WGALAsyncResult::Ready)
    {
      WArrayPtr<const WUInt8> memory;
      if (WReadbackBufferLock lock = m_HistogramReadback.LockBuffer(memory))
      {
        ProcessHistogramReadback(memory);
      }
      m_bHistogramReadbackInFlight = false;
    }
    else if (result == WGALAsyncResult::Expired)
    {
      m_bHistogramReadbackInFlight = false;
    }
  }

  if (m_bMinMaxReadbackInFlight)
  {
    const WEnum<WGALAsyncResult> result = m_MinMaxReadback.GetReadbackResult(WTime::MakeZero());
    if (result == WGALAsyncResult::Ready)
    {
      WArrayPtr<const WUInt8> memory;
      if (WReadbackBufferLock lock = m_MinMaxReadback.LockBuffer(memory))
      {
        ProcessMinMaxReadback(memory);
      }
      m_bMinMaxReadbackInFlight = false;
    }
    else if (result == WGALAsyncResult::Expired)
    {
      m_bMinMaxReadbackInFlight = false;
    }
  }
}

void WRenderGraphPassObserver::ProcessPixelReadback(WArrayPtr<const WUInt8> memory)
{
  W_ASSERT_DEBUG(memory.GetCount() >= sizeof(WVec4), "Readback size missmatch");

  WVec4 pixelValue;
  WMemoryUtils::RawByteCopy(&pixelValue, memory.GetPtr(), sizeof(WVec4));

  W_LOCK(m_Mutex);
  m_Response.m_PixelValue = pixelValue;
}

void WRenderGraphPassObserver::ProcessHistogramReadback(WArrayPtr<const WUInt8> memory)
{
  constexpr WUInt32 uiRequiredSize = s_uiHistogramBinCount * s_uiHistogramChannelCount * sizeof(WUInt32);
  W_ASSERT_DEBUG(memory.GetCount() >= uiRequiredSize, "Readback size missmatch");

  const WUInt32* pHistogram = reinterpret_cast<const WUInt32*>(memory.GetPtr());
  bool bIgnoreChannel[s_uiHistogramChannelCount] = {};
  WUInt32 uiMaxCount = 0;

  for (WUInt32 uiChannel = 0; uiChannel < s_uiHistogramChannelCount; ++uiChannel)
  {
    if ((m_Request.m_uiChannelMask & W_BIT(uiChannel)) == 0)
    {
      bIgnoreChannel[uiChannel] = true;
      continue;
    }

    WUInt32 uiNonZeroBuckets = 0;
    WUInt32 uiChannelMaxCount = 0;

    for (WUInt32 uiBucket = 0; uiBucket < s_uiHistogramBinCount; ++uiBucket)
    {
      const WUInt32 uiCount = pHistogram[uiChannel * s_uiHistogramBinCount + uiBucket];
      if (uiCount > 0)
      {
        ++uiNonZeroBuckets;
        uiChannelMaxCount = WMath::Max(uiChannelMaxCount, uiCount);
      }
    }

    bIgnoreChannel[uiChannel] = uiNonZeroBuckets <= 1;
    if (!bIgnoreChannel[uiChannel])
    {
      uiMaxCount = WMath::Max(uiMaxCount, uiChannelMaxCount);
    }
  }

  W_LOCK(m_Mutex);
  m_Response.m_bHistogramValid = true;

  if (uiMaxCount == 0)
  {
    WMemoryUtils::ZeroFill(m_Response.m_Histogram.GetData(), m_Response.m_Histogram.GetCount());
    return;
  }

  for (WUInt32 uiChannel = 0; uiChannel < s_uiHistogramChannelCount; ++uiChannel)
  {
    const WUInt32 uiChannelOffset = uiChannel * s_uiHistogramBinCount;
    if (bIgnoreChannel[uiChannel])
    {
      WMemoryUtils::ZeroFill(m_Response.m_Histogram.GetData() + uiChannelOffset, s_uiHistogramBinCount);
      continue;
    }

    for (WUInt32 uiBucket = 0; uiBucket < s_uiHistogramBinCount; ++uiBucket)
    {
      const WUInt64 uiNormalizedValue = (static_cast<WUInt64>(pHistogram[uiChannelOffset + uiBucket]) * 255ull) / uiMaxCount;
      m_Response.m_Histogram[uiChannelOffset + uiBucket] = static_cast<WUInt8>(WMath::Min<WUInt64>(255, uiNormalizedValue));
    }
  }
}

void WRenderGraphPassObserver::ProcessMinMaxReadback(WArrayPtr<const WUInt8> memory)
{
  constexpr WUInt32 uiRequiredSize = s_uiMinMaxValueCount * sizeof(WUInt32);
  W_ASSERT_DEBUG(memory.GetCount() >= uiRequiredSize, "Readback size missmatch");

  const WUInt32* pMinMax = reinterpret_cast<const WUInt32*>(memory.GetPtr());
  float fMin = 0.0f;
  float fMax = 1.0f;

  if (pMinMax[0] != WMath::MaxValue<WUInt32>() || pMinMax[1] != 0)
  {
    fMin = OrderedUintToFloat(pMinMax[0]);
    fMax = OrderedUintToFloat(pMinMax[1]);
  }

  W_LOCK(m_Mutex);
  m_Response.m_fImageMin = fMin;
  m_Response.m_fImageMax = fMax;
}

void WRenderGraphPassObserver::Reset()
{
  m_bValid = false;
  m_uiSortedPassIndex = 0xFFFFFFFF;
  m_hResolvedSourceTexture.Invalidate();
  m_PreCopyBarriers.Clear();
  m_PostCopyBarriers.Clear();
}

void WRenderGraphPassObserver::EnsureCopyTexture(const WGALTextureCreationDescription& srcDesc)
{
  WGALTextureCreationDescription newDesc = srcDesc;
  newDesc.m_TextureFlags = WGALTextureUsageFlags::ShaderResource;
  newDesc.m_ResourceAccess.m_bImmutable = false;
  newDesc.m_pExisitingNativeObject = nullptr;

  if (!m_hCopyTexture.IsInvalidated())
  {
    const WGALTexture* pExisting = m_pDevice->GetTexture(m_hCopyTexture);
    if (pExisting != nullptr && newDesc.CalculateHash() == pExisting->GetDescription().CalculateHash())
    {
      return; // Already matches.
    }

    // Release old resource. The resource manager will clean up the GAL texture.
    m_hCopyTextureResource.Invalidate();
    m_hCopyTexture.Invalidate();
  }

  WStringBuilder resourceName;
  resourceName.SetFormat("RGObserverCopy_{}", s_uiObserverTextureCounter.Increment());

  WTexture2DResourceDescriptor texDesc;
  texDesc.m_DescGAL = newDesc;
  texDesc.m_SamplerDesc.m_MinFilter = WGALTextureFilterMode::Point;
  texDesc.m_SamplerDesc.m_MagFilter = WGALTextureFilterMode::Point;
  texDesc.m_SamplerDesc.m_MipFilter = WGALTextureFilterMode::Point;

  m_hCopyTextureResource = WResourceManager::CreateResource<WTexture2DResource>(resourceName, std::move(texDesc), "Render Graph Observer Copy");

  WResourceLock<WTexture2DResource> pResource(m_hCopyTextureResource, WResourceAcquireMode::BlockTillLoaded);
  m_hCopyTexture = pResource->GetGALTexture();
}

void WRenderGraphPassObserver::RecordPreview(WRenderGraph& ref_graph)
{
  if (m_pGraph == nullptr || m_Request.m_sPassName.IsEmpty())
    return;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALTextureHandle hCopyTexture = GetCopyTexture();
  if (hCopyTexture.IsInvalidated())
    return;
  const WGALTexture* pCopyTexture = pDevice->GetTexture(hCopyTexture);
  if (pCopyTexture == nullptr)
    return;

  PollReadbacks();
  EnsureInspectionResources();

  const WGALTextureCreationDescription& copyTextureDesc = pCopyTexture->GetDescription();
  const WUInt32 uiMipLevel = WMath::Min<WUInt32>(m_Request.m_uiMipLevel, copyTextureDesc.m_uiMipLevelCount - 1);
  const WUInt32 uiArraySlice = WMath::Min<WUInt32>(m_Request.m_uiArraySlice, copyTextureDesc.GetNumberOfSlices() - 1);
  const WVec3U32 mipSize = copyTextureDesc.GetMipMapSize(uiMipLevel);
  const bool bIsMSAA = copyTextureDesc.m_SampleCount != WGALMSAASampleCount::None;
  const WUInt32 uiSampleCount = (WUInt32)copyTextureDesc.m_SampleCount;
  const WInt32 iSampleIndex = m_Request.m_iSampleIndex < 0 ? -1 : WMath::Min<WInt32>(m_Request.m_iSampleIndex, uiSampleCount - 1u);

  WGALTextureRange sourceRange;
  sourceRange.m_uiBaseMipLevel = uiMipLevel;
  sourceRange.m_uiMipLevels = 1;
  sourceRange.m_uiBaseArraySlice = uiArraySlice;
  sourceRange.m_uiArraySlices = 1;

  WRenderGraphTextureHandle hSource = ref_graph.ImportTexture(hCopyTexture);
  WRenderGraphBufferHandle hPixelBuffer = ref_graph.ImportBuffer(m_hPixelReadbackBuffer);
  WRenderGraphBufferHandle hHistogramBuffer = ref_graph.ImportBuffer(m_hHistogramBuffer);
  WRenderGraphBufferHandle hMinMaxBuffer = ref_graph.ImportBuffer(m_hMinMaxBuffer);

  if (!m_bPixelReadbackInFlight && m_Request.m_vPixelPosition.x >= 0 && m_Request.m_vPixelPosition.y >= 0)
  {
    WRenderGraphReadbackPixelConstants constants;
    constants.PixelPosition = m_Request.m_vPixelPosition;
    constants.SampleIndex = iSampleIndex;
    constants.SampleCount = uiSampleCount;
    constants.TextureSize = WVec2U32(mipSize.x, mipSize.y);

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Read Pixel");
      pass.ReadTexture(hSource, sourceRange);
      pass.WriteBuffer(hPixelBuffer);
      pass.SetExecuteCallback([this, hSource, hPixelBuffer, sourceRange, constants, bIsMSAA](const WRenderGraphContext& ctx)
        {
        WRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? WTempHashedString("TRUE") : WTempHashedString("FALSE"));
        pContext->BindShader(m_hReadbackPixelShader);
        pContext->SetPushConstants("WRenderGraphReadbackPixelConstants", constants);
        WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, {}, WGALTextureType::Texture2D);
        bindGroup.BindBuffer("PixelOutput", ctx.ResolveBuffer(hPixelBuffer));
        pContext->Dispatch(1, 1, 1).AssertSuccess(); });
    }
    {
      auto readbackPass = ref_graph.AddTransferPass("RenderGraphObserver Pixel Readback");
      readbackPass.ReadBuffer(hPixelBuffer, WGALResourceState::CopySource);
      readbackPass.HasSideEffects();
      readbackPass.SetExecuteCallback([this, hPixelBuffer](const WRenderGraphContext& ctx)
        { m_PixelReadback.ReadbackBuffer(*ctx.GetCommandEncoder(), ctx.ResolveBuffer(hPixelBuffer)); });
    }

    m_bPixelReadbackInFlight = true;
  }

  if (!m_bMinMaxReadbackInFlight)
  {
    WRenderGraphMinMaxConstants constants;
    constants.TextureSize = WVec2U32(mipSize.x, mipSize.y);
    constants.SampleIndex = iSampleIndex;
    constants.SampleCount = uiSampleCount;
    constants.ChannelMask = m_Request.m_uiChannelMask;

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Clear MinMax");
      pass.WriteBuffer(hMinMaxBuffer);
      pass.HasSideEffects();
      pass.SetExecuteCallback([this, hMinMaxBuffer](const WRenderGraphContext& ctx)
        {
        WRenderContext* pContext = ctx.GetRenderContext();
        pContext->BindShader(m_hClearMinMaxShader);
        WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindBuffer("MinMaxOutput", ctx.ResolveBuffer(hMinMaxBuffer));
        pContext->Dispatch(1, 1, 1).AssertSuccess(); });
    }

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Build MinMax");
      pass.ReadTexture(hSource, sourceRange);
      pass.WriteBuffer(hMinMaxBuffer);
      pass.SetExecuteCallback([this, hSource, hMinMaxBuffer, sourceRange, constants, bIsMSAA](const WRenderGraphContext& ctx)
        {
        WRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? WTempHashedString("TRUE") : WTempHashedString("FALSE"));
        pContext->BindShader(m_hBuildMinMaxShader);
        pContext->SetPushConstants("WRenderGraphMinMaxConstants", constants);
        WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, WGALResourceFormat::Invalid, WGALTextureType::Texture2D);
        bindGroup.BindBuffer("MinMaxOutput", ctx.ResolveBuffer(hMinMaxBuffer));
        pContext->Dispatch((constants.TextureSize.x + 7u) / 8u, (constants.TextureSize.y + 7u) / 8u, 1).AssertSuccess(); });
    }

    {
      auto readbackPass = ref_graph.AddTransferPass("RenderGraphObserver MinMax Readback");
      readbackPass.ReadBuffer(hMinMaxBuffer, WGALResourceState::CopySource);
      readbackPass.HasSideEffects();
      readbackPass.SetExecuteCallback([this, hMinMaxBuffer](const WRenderGraphContext& ctx)
        { m_MinMaxReadback.ReadbackBuffer(*ctx.GetCommandEncoder(), ctx.ResolveBuffer(hMinMaxBuffer)); });
    }
    m_bMinMaxReadbackInFlight = true;
  }

  if (!m_bHistogramReadbackInFlight)
  {
    WRenderGraphHistogramConstants constants;
    constants.TextureSize = WVec2U32(mipSize.x, mipSize.y);
    constants.SampleIndex = iSampleIndex;
    constants.SampleCount = uiSampleCount;
    constants.ChannelMask = m_Request.m_uiChannelMask;
    constants.RangeMin = m_Request.m_fRangeMin;
    constants.RangeMax = GetAdjustedHistogramMax(m_Request.m_fRangeMin, m_Request.m_fRangeMax);

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Clear Histogram");
      pass.WriteBuffer(hHistogramBuffer);
      pass.HasSideEffects();
      pass.SetExecuteCallback([this, hHistogramBuffer](const WRenderGraphContext& ctx)
        {
        WRenderContext* pContext = ctx.GetRenderContext();
        pContext->BindShader(m_hClearHistogramShader);
        WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindBuffer("HistogramOutput", ctx.ResolveBuffer(hHistogramBuffer));
        pContext->Dispatch(1, 1, 1).AssertSuccess(); });
    }

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Build Histogram");
      pass.ReadTexture(hSource, sourceRange);
      pass.WriteBuffer(hHistogramBuffer);
      pass.SetExecuteCallback([this, hSource, hHistogramBuffer, sourceRange, constants, bIsMSAA](const WRenderGraphContext& ctx)
        {
        WRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? WTempHashedString("TRUE") : WTempHashedString("FALSE"));
        pContext->BindShader(m_hBuildHistogramShader);
        pContext->SetPushConstants("WRenderGraphHistogramConstants", constants);
        WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, WGALResourceFormat::Invalid, WGALTextureType::Texture2D);
        bindGroup.BindBuffer("HistogramOutput", ctx.ResolveBuffer(hHistogramBuffer));
        pContext->Dispatch((constants.TextureSize.x + 7u) / 8u, (constants.TextureSize.y + 7u) / 8u, 1).AssertSuccess(); });
    }

    {
      auto readbackPass = ref_graph.AddTransferPass("RenderGraphObserver Histogram Readback");
      readbackPass.ReadBuffer(hHistogramBuffer, WGALResourceState::CopySource);
      readbackPass.HasSideEffects();
      readbackPass.SetExecuteCallback([this, hHistogramBuffer](const WRenderGraphContext& ctx)
        { m_HistogramReadback.ReadbackBuffer(*ctx.GetCommandEncoder(), ctx.ResolveBuffer(hHistogramBuffer)); });
    }
    m_bHistogramReadbackInFlight = true;
  }

  const WGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
  if (!pSwapChain)
    return;

  WGALTextureHandle hBackbuffer = pSwapChain->GetBackBufferTexture();
  const WGALTexture* pBackbuffer = pDevice->GetTexture(hBackbuffer);
  if (!pBackbuffer)
    return;

  const WGALTextureCreationDescription& desc = pBackbuffer->GetDescription();
  WRenderGraphTextureHandle hTarget = ref_graph.ImportTexture(hBackbuffer);

  const float fZoom = WMath::Clamp(m_Request.m_fZoom, 0.1f, 64.0f);
  const float fTexAspect = (float)mipSize.x / (float)WMath::Max(mipSize.y, 1u);
  const float fViewAspect = (float)desc.m_uiWidth / (float)WMath::Max(desc.m_uiHeight, 1u);

  float fHalfExtentU = 0.5f;
  float fHalfExtentV = 0.5f;
  if (fTexAspect > fViewAspect)
  {
    fHalfExtentU = 0.5f / fZoom;
    fHalfExtentV = 0.5f / fZoom * (fTexAspect / fViewAspect);
  }
  else
  {
    fHalfExtentU = 0.5f / fZoom * (fViewAspect / fTexAspect);
    fHalfExtentV = 0.5f / fZoom;
  }

  const WVec2 uv0(m_Request.m_vPanCenter.x - fHalfExtentU, m_Request.m_vPanCenter.y - fHalfExtentV);
  const WVec2 uv1(m_Request.m_vPanCenter.x + fHalfExtentU, m_Request.m_vPanCenter.y + fHalfExtentV);

  WRenderGraphPreviewConstants previewConstants;
  previewConstants.UVTransform = WVec4(uv1.x - uv0.x, uv1.y - uv0.y, uv0.x, uv0.y);
  previewConstants.ValueRange = WVec2(m_Request.m_fRangeMin, m_Request.m_fRangeMax);
  previewConstants.ChannelMask = m_Request.m_uiChannelMask;
  previewConstants.SampleIndex = iSampleIndex;
  previewConstants.PixelPosition = m_Request.m_vPixelPosition;
  previewConstants.HighlightPixel = m_Request.m_bHighlightPixel ? 1 : 0;

  {
    auto pass = ref_graph.AddGraphicsPass("RenderGraphObserver Preview");
    pass.AddColorTarget(hTarget);
    pass.ReadTexture(hSource, sourceRange, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetExecuteCallback([this, hSource, sourceRange, previewConstants, bIsMSAA](const WRenderGraphContext& ctx)
      {
        WRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? WTempHashedString("TRUE") : WTempHashedString("FALSE"));

        pContext->BindShader(m_hPreviewShader);
        pContext->SetPushConstants("WRenderGraphPreviewConstants", previewConstants);
        pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

        WBindGroupBuilder& bindGroup = pContext->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, WGALResourceFormat::Invalid, WGALTextureType::Texture2D);

        pContext->DrawMeshBuffer().AssertSuccess(); //
      });
  }
}
