#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimpleRenderPass, 1, WRTTIDefaultAllocator<WSimpleRenderPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_PinColor),
    W_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    W_MEMBER_PROPERTY("Message", m_sMessage),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSimpleRenderPass::WSimpleRenderPass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WSimpleRenderPass::~WSimpleRenderPass() = default;

WStatus WSimpleRenderPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  const WGALRenderTargets& renderTargets = viewData.GetActiveRenderTargets();

  WRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;

  // If no color input, create from view's render target
  if (hColor.IsInvalidated())
  {
    const WGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]);
    if (pTexture)
    {
      WGALTextureCreationDescription desc = pTexture->GetDescription();
      desc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::ShaderResource);
      desc.m_ResourceAccess.m_bImmutable = true;
      desc.m_pExisitingNativeObject = nullptr;
      hColor = ref_graph.CreateTexture(desc);
    }
  }
  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;

  // If no depth input, create from view's depth target
  if (hDepthStencil.IsInvalidated())
  {
    const WGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hDSTarget);
    if (pTexture)
    {
      hDepthStencil = ref_graph.CreateTexture(pTexture->GetDescription());
    }
  }
  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  if (!hColor.IsInvalidated())
    pass.AddColorTarget(hColor);
  if (!hDepthStencil.IsInvalidated())
    pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());

  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::SimpleOpaque, ref_graph, pass);
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::SimpleTransparent, ref_graph, pass);
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::SimpleForeground, ref_graph, pass);
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::GUI, ref_graph, pass);

  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();

    WTempHashedString sRenderPass("RENDER_PASS_FORWARD");
    if (renderViewContext.m_pViewData->m_ViewRenderMode != WViewRenderMode::None)
    {
      sRenderPass = WViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
    }
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

    RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::SimpleOpaque);
    RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::SimpleTransparent);

    if (!m_sMessage.IsEmpty())
    {
      WDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage.GetData(), WVec2I32(20, 20), WColor::OrangeRed);
    }

    WDebugRenderer::RenderWorldSpace(renderViewContext);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
    RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::SimpleForeground);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
    RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::SimpleForeground);

    RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::GUI);

    WDebugRenderer::RenderScreenSpace(renderViewContext); });

  return W_SUCCESS;
}

WResult WSimpleRenderPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sMessage;
  return W_SUCCESS;
}

WResult WSimpleRenderPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sMessage;
  return W_SUCCESS;
}

void WSimpleRenderPass::SetMessage(const char* szMessage)
{
  m_sMessage = szMessage;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
