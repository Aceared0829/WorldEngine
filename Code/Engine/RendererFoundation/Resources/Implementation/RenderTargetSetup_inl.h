
#pragma once

WUInt8 WGALRenderingSetup::GetColorTargetCount() const
{
  return m_RenderPass.m_uiRTCount;
}

const WColor& WGALRenderingSetup::GetClearColor(WUInt8 uiIndex) const
{
  W_ASSERT_DEBUG(uiIndex < m_RenderPass.m_uiRTCount, "Render target at index {} does no exist, there are only {} render targets. Call GetRenderTargetCount first to determine max render targets.", uiIndex, m_RenderPass.m_uiRTCount);
  return m_ClearColor[uiIndex];
}

bool WGALRenderingSetup::HasDepthStencilTarget() const
{
  return !m_FrameBuffer.m_hDepthTarget.IsInvalidated();
}

float WGALRenderingSetup::GetClearDepth() const
{
  W_ASSERT_DEBUG(HasDepthStencilTarget(), "No depth target exists, check HasDepthStencilTarget() first.");
  return m_fClearDepth;
}

WUInt8 WGALRenderingSetup::GetClearStencil() const
{
  W_ASSERT_DEBUG(HasDepthStencilTarget(), "No depth target exists, check HasDepthStencilTarget() first.");
  return m_uiClearStencil;
}

const WGALRenderPassDescriptor& WGALRenderingSetup::GetRenderPass() const
{
  return m_RenderPass;
}
const WGALFrameBufferDescriptor& WGALRenderingSetup::GetFrameBuffer() const
{
  return m_FrameBuffer;
}
