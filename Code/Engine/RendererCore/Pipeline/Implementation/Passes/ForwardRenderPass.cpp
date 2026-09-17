#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WForwardRenderPass, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_PinColor),
    W_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    W_ENUM_MEMBER_PROPERTY("ShadingQuality", WForwardRenderShadingQuality, m_ShadingQuality)->AddAttributes(new WDefaultValueAttribute((int)WForwardRenderShadingQuality::Normal)),
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

WForwardRenderPass::WForwardRenderPass(const char* szName)
  : WRenderPipelinePass(szName, true)
  , m_ShadingQuality(WForwardRenderShadingQuality::Normal)
{
  m_hWhiteTexture = WResourceManager::LoadResource<WTexture2DResource>("White.color");
}

WForwardRenderPass::~WForwardRenderPass() = default;

WStatus WForwardRenderPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  W_IGNORE_UNUSED(viewData);

  WRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return WStatus(WFmt("Color: Not connected"));

  WRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  if (hDepthStencil.IsInvalidated())
    return WStatus(WFmt("DepthStencil: Not connected"));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;
  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());
  DeclareRenderObjectDependencies(ref_graph, pass);
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();
    SetupPermutationVars(renderViewContext);
    RenderObjects(renderViewContext); });

  return W_SUCCESS;
}

WResult WForwardRenderPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_ShadingQuality;
  return W_SUCCESS;
}

WResult WForwardRenderPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_ShadingQuality;
  return W_SUCCESS;
}

void WForwardRenderPass::SetupPermutationVars(const WRenderViewContext& renderViewContext)
{
  WTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != WViewRenderMode::None)
  {
    sRenderPass = WViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  WStringBuilder sDebugText;
  WViewRenderMode::GetDebugText(renderViewContext.m_pViewData->m_ViewRenderMode, sDebugText);
  if (!sDebugText.IsEmpty())
  {
    WDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sDebugText, WVec2I32(10, 10), WColor::White);
  }

  // Set permutation for shading quality
  if (m_ShadingQuality == WForwardRenderShadingQuality::Normal)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");
  }
  else if (m_ShadingQuality == WForwardRenderShadingQuality::Simplified)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_SIMPLIFIED");
  }
  else
  {
    W_REPORT_FAILURE("Unknown shading quality setting.");
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);
