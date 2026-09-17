#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Declarations.h>

#include <RendererCore/RenderContext/RenderContext.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderViewContext, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

void WRenderViewContext::UpdateViewport() const
{
  WRectFloat viewport = m_pViewData->m_ViewPortRect;
  auto& gc = m_pRenderContext->WriteGlobalConstants();
  gc.ViewportSize = WVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  m_pRenderContext->GetCommandEncoder()->SetViewport(viewport);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Declarations);
