#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/CustomRenderDataPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCustomRenderDataPass, 1, WRTTIDefaultAllocator<WCustomRenderDataPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_PinColor),
    W_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    W_MEMBER_PROPERTY("RenderDataCategory", m_sRenderDataCategoryName),
    W_ENUM_MEMBER_PROPERTY("SortingFunction", WRenderSortingFunctions, m_SortingFunction),
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

WCustomRenderDataPass::WCustomRenderDataPass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WCustomRenderDataPass::~WCustomRenderDataPass() = default;

WStatus WCustomRenderDataPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;

  if (!hColor.IsInvalidated())
    outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;
  if (!hDepthStencil.IsInvalidated())
    outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  if (!hColor.IsInvalidated())
    pass.AddColorTarget(hColor);
  if (!hDepthStencil.IsInvalidated())
    pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());

  // BEGIN-DOCS-CODE-SNIPPET: renderpass-render-objects
  if (m_RenderDataCategory.IsValid())
    DeclareRendererDependenciesForCategory(m_RenderDataCategory, ref_graph, pass);

  pass.SetExecuteCallback(
    [=](const WRenderGraphContext& ctx)
    {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      if (!m_RenderDataCategory.IsValid())
        return;

      auto batchList = GetPipeline()->GetRenderDataBatchesWithCategory(m_RenderDataCategory);
      if (batchList.GetBatchCount() == 0)
        return;

      RenderDataWithCategory(renderViewContext, m_RenderDataCategory);
    });
  // END-DOCS-CODE-SNIPPET

  return W_SUCCESS;
}

WResult WCustomRenderDataPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sRenderDataCategoryName;
  inout_stream << m_SortingFunction;
  return W_SUCCESS;
}

WResult WCustomRenderDataPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sRenderDataCategoryName;
  inout_stream >> m_SortingFunction;

  if (m_sRenderDataCategoryName.IsEmpty() == false)
  {
    m_RenderDataCategory = WRenderData::RegisterCategory(m_sRenderDataCategoryName, WRenderSortingFunctions::GetFunction(m_SortingFunction));
  }

  return W_SUCCESS;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_CustomRenderDataPass);
