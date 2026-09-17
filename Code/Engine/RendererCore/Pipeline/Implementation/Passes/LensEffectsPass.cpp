#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/LensEffectsPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLensEffectsPass, 1, WRTTIDefaultAllocator<WLensEffectsPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_PinColor),
    W_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
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

WLensEffectsPass::WLensEffectsPass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WLensEffectsPass::~WLensEffectsPass() = default;

WStatus WLensEffectsPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  W_IGNORE_UNUSED(viewData);

  WRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return WStatus(WFmt("Color: Not connected"));

  // Pass-through color
  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;

  WRenderGraphTextureHandle hResolvedDepth = inputs[m_PinResolvedDepth.m_uiInputIndex].m_TextureHandle;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  if (!hResolvedDepth.IsInvalidated())
    pass.ReadTexture(hResolvedDepth, {}, WGALResourceState::ShaderResource);
  pass.SetStereoscopic(camera.IsStereoscopic());
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LensEffects, ref_graph, pass);
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    //Needed? SetupPermutationVars(renderViewContext);
    if (!hResolvedDepth.IsInvalidated())
    {
      WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
      bindGroupRenderPass.BindTexture("SceneDepth", ctx.ResolveTexture(hResolvedDepth));
    }
    RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LensEffects); });

  return W_SUCCESS;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LensEffectsPass);
