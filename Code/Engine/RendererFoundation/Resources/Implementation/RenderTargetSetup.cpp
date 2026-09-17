#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

namespace
{
  struct RenderTargetViewInfo
  {
    WEnum<WGALMSAASampleCount> m_MSAA;
    WSizeU32 m_Size = {0, 0};
    WUInt32 m_uiSliceCount = 0;
  };

  RenderTargetViewInfo getRenderTargetViewInfo(WGALRenderTargetViewHandle hView, WEnum<WGALResourceFormat>& out_format)
  {
    RenderTargetViewInfo info;
    const WGALRenderTargetView* pRTV = WGALDevice::GetDefaultDevice()->GetRenderTargetView(hView);
    W_ASSERT_DEV(pRTV, "Render target view must be valid");
    const WGALRenderTargetViewCreationDescription& viewDesc = pRTV->GetDescription();
    const WGALTextureCreationDescription& texDesc = pRTV->GetTexture()->GetDescription();
    out_format = viewDesc.m_OverrideViewFormat;
    if (out_format == WGALResourceFormat::Invalid)
      out_format = texDesc.m_Format;

    info.m_MSAA = texDesc.m_SampleCount;
    WVec3U32 size = pRTV->GetTexture()->GetMipMapSize(viewDesc.m_uiMipLevel);
    info.m_Size = {size.x, size.y};
    info.m_uiSliceCount = viewDesc.m_uiSliceCount;
    return info;
  }
} // namespace

bool WGALRenderTargets::operator==(const WGALRenderTargets& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  for (WUInt8 uiRTIndex = 0; uiRTIndex < W_GAL_MAX_RENDERTARGET_COUNT; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }
  return true;
}

bool WGALRenderTargets::operator!=(const WGALRenderTargets& other) const
{
  return !(*this == other);
}

WGALRenderingSetup& WGALRenderingSetup::SetColorTarget(WUInt8 uiIndex, WGALRenderTargetViewHandle hRenderTarget, WEnum<WGALRenderTargetLoadOp> loadOp, WEnum<WGALRenderTargetStoreOp> storeOp)
{
  W_ASSERT_DEBUG(uiIndex <= m_RenderPass.m_uiRTCount, "Render targets must be defined in order, starting at 0 and must not have gaps. The index {} should be less or equal to {}", uiIndex, m_RenderPass.m_uiRTCount);

  RenderTargetViewInfo info = getRenderTargetViewInfo(hRenderTarget, m_RenderPass.m_ColorFormat[uiIndex]);
  const bool bFirstRenderTarget = GetColorTargetCount() == 0 && m_FrameBuffer.m_hDepthTarget.IsInvalidated();
  if (!bFirstRenderTarget)
  {
    W_ASSERT_DEBUG(m_RenderPass.m_Msaa == info.m_MSAA, "Missmatch between this render target's MSAA mode ({}) and the previously set MSAA mode ({}).", info.m_MSAA, m_RenderPass.m_Msaa);
    W_ASSERT_DEBUG(m_FrameBuffer.m_Size == info.m_Size, "Missmatch between this render target's size ({}) and the previously set size ({}).", info.m_Size, m_FrameBuffer.m_Size);
    W_ASSERT_DEBUG(m_FrameBuffer.m_uiSliceCount == info.m_uiSliceCount, "Missmatch between this render target's slice count ({}) and the previously set slice count ({}).", info.m_uiSliceCount, m_FrameBuffer.m_uiSliceCount);
  }
  else
  {
    m_RenderPass.m_Msaa = info.m_MSAA;
    m_FrameBuffer.m_Size = info.m_Size;
    m_FrameBuffer.m_uiSliceCount = info.m_uiSliceCount;
  }

  W_ASSERT_DEBUG(!WGALResourceFormat::IsDepthFormat(m_RenderPass.m_ColorFormat[uiIndex]), "The format {} must be a color format", m_RenderPass.m_DepthFormat);

  m_FrameBuffer.m_hColorTarget[uiIndex] = hRenderTarget;
  m_RenderPass.m_uiRTCount = WMath::Max(m_RenderPass.m_uiRTCount, static_cast<WUInt8>(uiIndex + 1u));
  m_RenderPass.m_ColorLoadOp[uiIndex] = loadOp;
  m_RenderPass.m_ColorStoreOp[uiIndex] = storeOp;

  return *this;
}

WGALRenderingSetup& WGALRenderingSetup::SetDepthStencilTarget(WGALRenderTargetViewHandle hDSTarget, WEnum<WGALRenderTargetLoadOp> depthLoadOp, WEnum<WGALRenderTargetStoreOp> depthStoreOp, WEnum<WGALRenderTargetLoadOp> stencilLoadOp, WEnum<WGALRenderTargetStoreOp> stencilStoreOp)
{
  RenderTargetViewInfo info = getRenderTargetViewInfo(hDSTarget, m_RenderPass.m_DepthFormat);
  const bool bFirstRenderTarget = GetColorTargetCount() == 0 && m_FrameBuffer.m_hDepthTarget.IsInvalidated();
  if (!bFirstRenderTarget)
  {
    W_ASSERT_DEBUG(m_RenderPass.m_Msaa == info.m_MSAA, "Missmatch between this render target's MSAA mode ({}) and the previously set MSAA mode ({}).", info.m_MSAA, m_RenderPass.m_Msaa);
    m_FrameBuffer.m_Size.height = WMath::Min(m_FrameBuffer.m_Size.height, info.m_Size.height);
    m_FrameBuffer.m_Size.width = WMath::Min(m_FrameBuffer.m_Size.width, info.m_Size.width);
    W_ASSERT_DEBUG(m_FrameBuffer.m_uiSliceCount == info.m_uiSliceCount, "Missmatch between this render target's slice count ({}) and the previously set slice count ({}).", info.m_uiSliceCount, m_FrameBuffer.m_uiSliceCount);
  }
  else
  {
    m_RenderPass.m_Msaa = info.m_MSAA;
    m_FrameBuffer.m_Size = info.m_Size;
    m_FrameBuffer.m_uiSliceCount = info.m_uiSliceCount;
  }

  m_FrameBuffer.m_hDepthTarget = hDSTarget;
  m_RenderPass.m_DepthLoadOp = depthLoadOp;
  m_RenderPass.m_DepthStoreOp = depthStoreOp;

  W_ASSERT_DEBUG(WGALResourceFormat::IsDepthFormat(m_RenderPass.m_DepthFormat), "The format {} is not a depth format", m_RenderPass.m_DepthFormat);
  if (WGALResourceFormat::IsStencilFormat(m_RenderPass.m_DepthFormat))
  {
    m_RenderPass.m_StencilLoadOp = stencilLoadOp;
    m_RenderPass.m_StencilStoreOp = stencilStoreOp;
  }
  else
  {
    m_RenderPass.m_StencilLoadOp = WGALRenderTargetLoadOp::DontCare;
    m_RenderPass.m_StencilStoreOp = WGALRenderTargetStoreOp::Discard;
  }
  return *this;
}

WGALRenderingSetup& WGALRenderingSetup::SetClearColor(WUInt8 uiIndex, const WColor& color)
{
  W_ASSERT_DEBUG(uiIndex < m_RenderPass.m_uiRTCount, "Render target not set, call SetRenderTarget first");
  m_ClearColor[uiIndex] = color;
  m_RenderPass.m_ColorLoadOp[uiIndex] = WGALRenderTargetLoadOp::Clear;
  return *this;
}

WGALRenderingSetup& WGALRenderingSetup::SetClearDepth(float fDepthClear)
{
  W_ASSERT_DEBUG(HasDepthStencilTarget(), "Depth target not set, call SetDepthStencilTarget first");
  m_fClearDepth = fDepthClear;
  m_RenderPass.m_DepthLoadOp = WGALRenderTargetLoadOp::Clear;
  return *this;
}

WGALRenderingSetup& WGALRenderingSetup::SetClearStencil(WUInt8 uiStencilClear)
{
  W_ASSERT_DEBUG(HasDepthStencilTarget(), "Depth target not set, call SetDepthStencilTarget first");
  m_uiClearStencil = uiStencilClear;
  m_RenderPass.m_StencilLoadOp = WGALRenderTargetLoadOp::Clear;
  return *this;
}

void WGALRenderingSetup::Reset()
{
  *this = WGALRenderingSetup();
}
