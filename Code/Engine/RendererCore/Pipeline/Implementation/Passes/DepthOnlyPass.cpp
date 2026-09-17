#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/DepthOnlyPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Foundation/IO/TypeVersionContext.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDepthOnlyPass, 3, WRTTIDefaultAllocator<WDepthOnlyPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    W_MEMBER_PROPERTY("RenderStaticObjects", m_bRenderStaticObjects)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("RenderDynamicObjects", m_bRenderDynamicObjects)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("RenderTransparentObjects", m_bRenderTransparentObjects),
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

WDepthOnlyPass::WDepthOnlyPass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WDepthOnlyPass::~WDepthOnlyPass() = default;

WStatus WDepthOnlyPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  if (hDepthStencil.IsInvalidated())
    return WStatus(WFmt("DepthStencil: Not connected"));

  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());
  if (m_bRenderStaticObjects)
  {
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitOpaqueStatic, ref_graph, pass);
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitMaskedStatic, ref_graph, pass);
  }
  if (m_bRenderDynamicObjects)
  {
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitOpaqueDynamic, ref_graph, pass);
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitMaskedDynamic, ref_graph, pass);
  }
  if (m_bRenderTransparentObjects)
  {
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitTransparent, ref_graph, pass);
  }
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");

    if (m_bRenderStaticObjects)
      RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitOpaqueStatic);
    if (m_bRenderDynamicObjects)
      RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitOpaqueDynamic);
    if (m_bRenderStaticObjects)
      RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitMaskedStatic);
    if (m_bRenderDynamicObjects)
      RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitMaskedDynamic);
    if (m_bRenderTransparentObjects)
      RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitTransparent); });

  return W_SUCCESS;
}

// BEGIN-DOCS-CODE-SNIPPET: renderpass-serialization
WResult WDepthOnlyPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_bRenderStaticObjects;
  inout_stream << m_bRenderDynamicObjects;
  inout_stream << m_bRenderTransparentObjects;
  return W_SUCCESS;
}

WResult WDepthOnlyPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  if (uiVersion >= 3)
  {
    inout_stream >> m_bRenderStaticObjects;
    inout_stream >> m_bRenderDynamicObjects;
  }

  if (uiVersion >= 2)
  {
    inout_stream >> m_bRenderTransparentObjects;
  }

  return W_SUCCESS;
}
// END-DOCS-CODE-SNIPPET



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
