#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Plugins/RmlUiPlugin/Shaders/RmlUiBlitConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiRenderer, 1, WRTTIDefaultAllocator<WRmlUiRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRmlUiRenderer::WRmlUiRenderer()
{
  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RmlUiBlit.WShader");
  m_hConstantBuffer = WRenderContext::CreateConstantBufferStorage<WRmlUiBlitConstants>();
}

WRmlUiRenderer::~WRmlUiRenderer()
{
  WRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void WRmlUiRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WRmlUiRenderData>());
}

void WRmlUiRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  pRenderContext->BindShader(m_hShader);
  WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
  bindGroup.BindBuffer("WRmlUiBlitConstants", m_hConstantBuffer);
  pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::TriangleStrip, 2);

  const WVec2 targetSize = WVec2(renderViewContext.m_pViewData->m_ViewPortRect.width, renderViewContext.m_pViewData->m_ViewPortRect.height);
  const WVec2 scale = WVec2(2.0f, -2.0f).CompDiv(targetSize);
  const WVec2 offset = WVec2(-1.0f, 1.0f);

  for (auto it = batch.GetIterator<WRmlUiRenderData>(); it.IsValid(); ++it)
  {
    const WRmlUiRenderData* pRenderData = it;
    const WGALTexture* pTexture = pDevice->GetTexture(pRenderData->m_hTexture);

    const WVec2 targetSize = WVec2(renderViewContext.m_pViewData->m_ViewPortRect.width, renderViewContext.m_pViewData->m_ViewPortRect.height);
    const WVec2 textureSize = WVec2(static_cast<float>(pTexture->GetDescription().m_uiWidth), static_cast<float>(pTexture->GetDescription().m_uiHeight));

    WRmlUiBlitConstants* pConstants = pRenderContext->GetConstantBufferData<WRmlUiBlitConstants>(m_hConstantBuffer);
    pConstants->Scale = textureSize.CompMul(scale);
    pConstants->Offset = pRenderData->m_vOffset.CompMul(scale) + offset;

    bindGroup.BindTexture("BaseTexture", pRenderData->m_hTexture);

    pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Implementation_RmlUiRenderer);
