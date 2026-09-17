#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SelectionHighlightConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSelectionHighlightPass, 1, WRTTIDefaultAllocator<WSelectionHighlightPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_PinColor),
    W_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),

    W_MEMBER_PROPERTY("HighlightColor", m_HighlightColor)->AddAttributes(new WDefaultValueAttribute(WColorScheme::LightUI(WColorScheme::Yellow))),
    W_MEMBER_PROPERTY("OverlayOpacity", m_fOverlayOpacity)->AddAttributes(new WDefaultValueAttribute(0.1f))
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSelectionHighlightPass::WSelectionHighlightPass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
  // Load shader.
  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/SelectionHighlight.WShader");
  W_ASSERT_DEV(m_hShader.IsValid(), "Could not load selection highlight shader!");

  m_hConstantBuffer = WRenderContext::CreateConstantBufferStorage<WSelectionHighlightConstants>();
}

WSelectionHighlightPass::~WSelectionHighlightPass()
{
  WRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

WStatus WSelectionHighlightPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return WStatus(WFmt("Color: Not connected"));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;

  WRenderGraphTextureHandle hDepth = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  if (hDepth.IsInvalidated())
    return WStatus(WFmt("DepthStencil: Not connected"));

  // Create temp depth texture for selection rendering
  const WGALTextureCreationDescription colorDesc = ref_graph.GetTextureDesc(hColor);
  WGALTextureCreationDescription depthDesc;
  depthDesc.SetAsRenderTarget(colorDesc.m_uiWidth, colorDesc.m_uiHeight, colorDesc.m_uiArraySize, WGALResourceFormat::D24S8, colorDesc.m_SampleCount);
  WRenderGraphTextureHandle hSelectionDepth = ref_graph.CreateTexture(depthDesc);

  // Render selection objects to depth only
  {
    auto pass = ref_graph.AddGraphicsPass("SelectionDepth");
    pass.AddDepthStencilTarget(hSelectionDepth, {}, WGALRenderTargetLoadOp::Clear, {}, WGALRenderTargetLoadOp::Clear);
    pass.SetClearDepth();
    pass.SetClearStencil();
    pass.SetStereoscopic(camera.IsStereoscopic());
    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::Selection, ref_graph, pass);
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      renderViewContext.UpdateViewport();

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
      RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::Selection); });
  }

  // Reconstruct selection overlay from depth
  {
    auto pass = ref_graph.AddGraphicsPass("SelectionHighlight");
    pass.AddColorTarget(hColor);
    pass.ReadTexture(hSelectionDepth, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepth, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      renderViewContext.UpdateViewport();

      auto constants = WRenderContext::GetConstantBufferData<WSelectionHighlightConstants>(m_hConstantBuffer);
      constants->HighlightColor = m_HighlightColor;
      constants->OverlayOpacity = m_fOverlayOpacity;

      renderViewContext.m_pRenderContext->BindShader(m_hShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

      WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
      bindGroupRenderPass.BindBuffer("WSelectionHighlightConstants", m_hConstantBuffer);
      bindGroupRenderPass.BindTexture("SelectionDepthTexture", ctx.ResolveTexture(hSelectionDepth));
      bindGroupRenderPass.BindTexture("SceneDepthTexture", ctx.ResolveTexture(hDepth));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return W_SUCCESS;
}

WResult WSelectionHighlightPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_HighlightColor;
  inout_stream << m_fOverlayOpacity;
  return W_SUCCESS;
}

WResult WSelectionHighlightPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_HighlightColor;
  inout_stream >> m_fOverlayOpacity;
  return W_SUCCESS;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
