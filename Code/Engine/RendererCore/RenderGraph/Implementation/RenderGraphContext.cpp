#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraphContext.h>

WGALTextureHandle WRenderGraphContext::ResolveTexture(WRenderGraphTextureHandle hTexture) const
{
  WUInt32 uiInstanceId = hTexture.GetInternalID().m_InstanceIndex;
  W_ASSERT_DEBUG(uiInstanceId < m_pTextureToResolvedTexture->GetCount(), "Invalid texture ID");
  const WUInt16 uiResolvedTextureIndex = (*m_pTextureToResolvedTexture)[uiInstanceId];
  return (*m_pResolvedTextures)[uiResolvedTextureIndex];
}

WGALBufferHandle WRenderGraphContext::ResolveBuffer(WRenderGraphBufferHandle hBuffer) const
{
  WUInt32 uiInstanceId = hBuffer.GetInternalID().m_InstanceIndex;
  W_ASSERT_DEBUG(uiInstanceId < m_pBufferToResolvedBuffer->GetCount(), "Invalid Buffer ID");
  const WUInt16 uiResolvedBufferIndex = (*m_pBufferToResolvedBuffer)[uiInstanceId];
  return (*m_pResolvedBuffers)[uiResolvedBufferIndex];
}

WGALCommandEncoder* WRenderGraphContext::GetCommandEncoder() const
{
  return m_pCommandEncoder;
}

WGALDevice* WRenderGraphContext::GetDevice() const
{
  return m_pDevice;
}

WRenderContext* WRenderGraphContext::GetRenderContext() const
{
  return m_pRenderContext;
}
