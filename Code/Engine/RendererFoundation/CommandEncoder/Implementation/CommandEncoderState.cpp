#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Utilities/Stats.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

void WGALCommandEncoderRenderState::InvalidateState()
{
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
    m_hVertexBufferOffsets[i] = 0;
  }
  m_hIndexBuffer.Invalidate();

  m_hGraphicsPipeline.Invalidate();
  m_hComputePipeline.Invalidate();
  m_uiStencilRefValue = 0;

  m_ScissorRect = WRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect = WRectFloat(WMath::MaxValue<float>(), WMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = WMath::MaxValue<float>();
  m_fViewPortMaxDepth = -WMath::MaxValue<float>();
}

void WGALCommandEncoderStats::Reset()
{
  WMemoryUtils::ZeroFill<WGALCommandEncoderStats>(static_cast<WGALCommandEncoderStats*>(this), 1);
}

void WGALCommandEncoderStats::SetStatistics()
{
  WStats::SetStat("GalCommandEncoder/Operations/InsertTimestamp", m_uiInsertTimestamp);
  WStats::SetStat("GalCommandEncoder/Operations/BeginOcclusionQuery", m_uiBeginOcclusionQuery);
  WStats::SetStat("GalCommandEncoder/Operations/InsertFence", m_uiInsertFence);
  WStats::SetStat("GalCommandEncoder/Operations/CopyBuffer", m_uiCopyBuffer);
  WStats::SetStat("GalCommandEncoder/Operations/UpdateBuffer", m_uiUpdateBuffer);
  WStats::SetStat("GalCommandEncoder/Operations/CopyTexture", m_uiCopyTexture);
  WStats::SetStat("GalCommandEncoder/Operations/UpdateTexture", m_uiUpdateTexture);
  WStats::SetStat("GalCommandEncoder/Operations/ResolveTexture", m_uiResolveTexture);
  WStats::SetStat("GalCommandEncoder/Operations/ReadbackTexture", m_uiReadbackTexture);
  WStats::SetStat("GalCommandEncoder/Operations/ReadbackBuffer", m_uiReadbackBuffer);
  WStats::SetStat("GalCommandEncoder/Operations/Flush", m_uiFlush);
  WStats::SetStat("GalCommandEncoder/Compute/BeginCompute", m_uiBeginCompute);
  WStats::SetStat("GalCommandEncoder/Compute/Dispatch", m_uiDispatch);
  WStats::SetStat("GalCommandEncoder/Render/BeginRendering", m_uiBeginRendering);
  WStats::SetStat("GalCommandEncoder/Render/Clear", m_uiClear);
  WStats::SetStat("GalCommandEncoder/Render/Draw", m_uiDraw);
  WStats::SetStat("GalCommandEncoder/States/SetIndexBuffer", m_uiSetIndexBuffer);
  WStats::SetStat("GalCommandEncoder/States/SetVertexBuffer", m_uiSetVertexBuffer);
  WStats::SetStat("GalCommandEncoder/States/SetGraphicsPipeline", m_uiSetGraphicsPipeline);
  WStats::SetStat("GalCommandEncoder/States/SetComputePipeline", m_uiSetComputePipeline);
  WStats::SetStat("GalCommandEncoder/DynamicStates/SetViewport", m_uiSetViewport);
  WStats::SetStat("GalCommandEncoder/DynamicStates/SetScissorRect", m_uiSetScissorRect);
  WStats::SetStat("GalCommandEncoder/DynamicStates/SetStencilReference", m_uiSetStencilReference);
}

void WGALCommandEncoderStats::operator+=(const WGALCommandEncoderStats& rhs)
{
  const WUInt32 uiCount = sizeof(WGALCommandEncoderStats) / sizeof(WUInt32);
  WUInt32* pDest = reinterpret_cast<WUInt32*>(this);
  const WUInt32* pSource = reinterpret_cast<const WUInt32*>(&rhs);
  for (int i = 0; i < uiCount; ++i)
  {
    pDest[i] += pSource[i];
  }
}
