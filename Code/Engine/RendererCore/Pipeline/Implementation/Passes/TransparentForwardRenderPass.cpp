#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTransparentForwardRenderPass, 1, WRTTIDefaultAllocator<WTransparentForwardRenderPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
    W_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    W_MEMBER_PROPERTY("ShadowMasks", m_PinShadowMasks),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTransparentForwardRenderPass::WTransparentForwardRenderPass(const char* szName)
  : WForwardRenderPass(szName)
{
}

WTransparentForwardRenderPass::~WTransparentForwardRenderPass()
{
  WGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSceneColorSamplerState);
}

WStatus WTransparentForwardRenderPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
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

  WRenderGraphTextureHandle hResolvedDepth = inputs[m_PinResolvedDepth.m_uiInputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hSSAO = inputs[m_PinSSAO.m_uiInputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hShadowMask = inputs[m_PinShadowMasks.m_uiInputIndex].m_TextureHandle;

  // Create temp scene color texture
  const WGALTextureCreationDescription colorDesc = ref_graph.GetTextureDesc(hColor);
  WGALTextureCreationDescription sceneColorDesc;
  sceneColorDesc.SetAsRenderTarget(colorDesc.m_uiWidth, colorDesc.m_uiHeight, colorDesc.m_Format);
  sceneColorDesc.m_Type = WGALTextureType::Texture2DArray;
  sceneColorDesc.m_uiArraySize = colorDesc.m_uiArraySize;
  sceneColorDesc.m_uiMipLevelCount = 1;
  WRenderGraphTextureHandle hSceneColor = ref_graph.CreateTexture(sceneColorDesc);

  // Transparent Pass1
  {
    CreateSamplerState();

    auto pass = ref_graph.AddGraphicsPass("TransparentForward1");
    pass.AddColorTarget(hColor);
    pass.AddDepthStencilTarget(hDepthStencil);
    pass.ReadTexture(hSceneColor, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    if (!hResolvedDepth.IsInvalidated())
      pass.ReadTexture(hResolvedDepth, {}, WGALResourceState::ShaderResource);
    if (!hSSAO.IsInvalidated())
      pass.ReadTexture(hSSAO, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    if (!hShadowMask.IsInvalidated())
      pass.ReadTexture(hShadowMask, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());

    DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitMeshDecal, ref_graph, pass);
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
        const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
        renderViewContext.UpdateViewport();
        WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
        if (!hResolvedDepth.IsInvalidated())
        {
          bindGroupRenderPass.BindTexture("SceneDepth", ctx.ResolveTexture(hResolvedDepth));
        }

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

        RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitMeshDecal); //
      });
  }

  // Copy current color to scene color texture
  {
    auto transferPass = ref_graph.AddTransferPass("CopySceneColor");
    transferPass.ReadTexture(hColor, {}, WGALResourceState::ResolveSource);
    transferPass.WriteTexture(hSceneColor, {}, WGALResourceState::ResolveDestination);
    transferPass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      WGALTextureSubresource subresource;
      subresource.m_uiMipLevel = 0;
      subresource.m_uiArraySlice = 0;
      ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hSceneColor), subresource, ctx.ResolveTexture(hColor), subresource); });
  }

  // Transparent pass 2
  {
    auto pass = ref_graph.AddGraphicsPass("TransparentForward2");
    pass.AddColorTarget(hColor);
    pass.AddDepthStencilTarget(hDepthStencil);
    pass.ReadTexture(hSceneColor, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    if (!hResolvedDepth.IsInvalidated())
      pass.ReadTexture(hResolvedDepth, {}, WGALResourceState::ShaderResource);
    pass.SetStereoscopic(camera.IsStereoscopic());
    DeclareRenderObjectDependencies(ref_graph, pass);
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      renderViewContext.UpdateViewport();
      SetupPermutationVars(renderViewContext);

      WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
      bindGroupRenderPass.BindTexture("SceneColor", ctx.ResolveTexture(hSceneColor));
      bindGroupRenderPass.BindSampler("SceneColorSampler", m_hSceneColorSamplerState);
      bindGroupRenderPass.BindTexture("SSAOTexture", m_hWhiteTexture, WResourceAcquireMode::BlockTillLoaded);
      bindGroupRenderPass.BindTexture("ShadowMasksTexture", m_hWhiteTexture, WResourceAcquireMode::BlockTillLoaded);

      if (!hResolvedDepth.IsInvalidated())
      {
        bindGroupRenderPass.BindTexture("SceneDepth", ctx.ResolveTexture(hResolvedDepth));
      }

      RenderObjects(renderViewContext); });
  }

  return W_SUCCESS;
}

void WTransparentForwardRenderPass::DeclareRenderObjectDependencies(WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_pass)
{
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitTransparent, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::LitForeground, ref_graph, ref_pass);
}

void WTransparentForwardRenderPass::RenderObjects(const WRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::LitForeground);
}

void WTransparentForwardRenderPass::CreateSamplerState()
{
  if (m_hSceneColorSamplerState.IsInvalidated())
  {
    WGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = WGALTextureFilterMode::Linear;
    desc.m_MagFilter = WGALTextureFilterMode::Linear;
    desc.m_MipFilter = WGALTextureFilterMode::Linear;
    desc.m_AddressU = WImageAddressMode::Clamp;
    desc.m_AddressV = WImageAddressMode::Mirror;
    desc.m_AddressW = WImageAddressMode::Mirror;

    m_hSceneColorSamplerState = WGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
