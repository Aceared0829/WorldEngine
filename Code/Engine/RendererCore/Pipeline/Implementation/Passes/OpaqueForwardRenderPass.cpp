#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/OpaqueForwardRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WOpaqueForwardRenderPass, 1, WRTTIDefaultAllocator<WOpaqueForwardRenderPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    W_MEMBER_PROPERTY("ShadowMasks", m_PinShadowMasks),
    W_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WOpaqueForwardRenderPass::WOpaqueForwardRenderPass(const char* szName)
  : WForwardRenderPass(szName)
{
}

WOpaqueForwardRenderPass::~WOpaqueForwardRenderPass() = default;

WStatus WOpaqueForwardRenderPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
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

  WRenderGraphTextureHandle hSSAO = inputs[m_PinSSAO.m_uiInputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hShadowMask = inputs[m_PinShadowMasks.m_uiInputIndex].m_TextureHandle;

  // Validate SSAO dimensions if connected
  if (!hSSAO.IsInvalidated())
  {
    const auto& ssaoDesc = ref_graph.GetTextureDesc(hSSAO);
    const auto& colorDesc = ref_graph.GetTextureDesc(hColor);
    if (ssaoDesc.m_uiWidth != colorDesc.m_uiWidth || ssaoDesc.m_uiHeight != colorDesc.m_uiHeight)
    {
      WLog::Warning("Expected same resolution for SSAO and color input to pass '{0}'!", GetName());
    }
    if (m_ShadingQuality == WForwardRenderShadingQuality::Simplified)
    {
      WLog::Warning("SSAO input will be ignored for pass '{0}' since simplified shading is activated.", GetName());
    }
  }

  if (!hShadowMask.IsInvalidated())
  {
    const auto& shadowMaskDesc = ref_graph.GetTextureDesc(hShadowMask);
    const auto& colorDesc = ref_graph.GetTextureDesc(hColor);
    if (shadowMaskDesc.m_uiWidth != colorDesc.m_uiWidth ||
        shadowMaskDesc.m_uiHeight != colorDesc.m_uiHeight)
    {
      WLog::Warning("Expected same resolution for shadow mask and color input to pass '{0}'!", GetName());
    }
  }

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  pass.AddDepthStencilTarget(hDepthStencil);
  if (!hSSAO.IsInvalidated())
    pass.ReadTexture(hSSAO, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  if (!hShadowMask.IsInvalidated())
    pass.ReadTexture(hShadowMask, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  DeclareRenderObjectDependencies(ref_graph, pass);
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      renderViewContext.UpdateViewport();
      SetupPermutationVars(renderViewContext);

      // Bind SSAO texture
      WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
      if (m_ShadingQuality == WForwardRenderShadingQuality::Normal)
      {
        if (!hSSAO.IsInvalidated())
        {
          bindGroupRenderPass.BindTexture("SSAOTexture", ctx.ResolveTexture(hSSAO));
        }
        else
        {
          bindGroupRenderPass.BindTexture("SSAOTexture", m_hWhiteTexture, WResourceAcquireMode::BlockTillLoaded);
        }

        if (!hShadowMask.IsInvalidated())
        {
          bindGroupRenderPass.BindTexture("ShadowMasksTexture", ctx.ResolveTexture(hShadowMask));
        }
        else
        {
          bindGroupRenderPass.BindTexture("ShadowMasksTexture", m_hWhiteTexture, WResourceAcquireMode::BlockTillLoaded);
        }
      }
      RenderObjects(renderViewContext); //
    });

  return W_SUCCESS;
}

void WOpaqueForwardRenderPass::SetupPermutationVars(const WRenderViewContext& renderViewContext)
{
  SUPER::SetupPermutationVars(renderViewContext);

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "FALSE");
  }
}

void WOpaqueForwardRenderPass::DeclareRenderObjectDependencies(WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_pass)
{
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitOpaqueStatic, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitOpaqueDynamic, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitMaskedStatic, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitMaskedDynamic, ref_graph, ref_pass);
}

void WOpaqueForwardRenderPass::RenderObjects(const WRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitOpaqueStatic);
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitOpaqueDynamic);
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitMaskedStatic);
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitMaskedDynamic);
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
