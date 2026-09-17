#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MsaaResolvePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsaaResolvePass, 1, WRTTIDefaultAllocator<WMsaaResolvePass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_PinInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Utilities")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMsaaResolvePass::WMsaaResolvePass()
  : WRenderPipelinePass("MsaaResolvePass", true)

{
  {
    // Load shader.
    m_hDepthResolveShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/MsaaDepthResolve.WShader");
    W_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Could not load depth resolve shader!");
  }
}

WMsaaResolvePass::~WMsaaResolvePass() = default;

WStatus WMsaaResolvePass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  const WGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  if (inputDesc.m_SampleCount == WGALMSAASampleCount::None)
    return WStatus(WFmt("Input is not a valid msaa target"));

  m_bIsDepth = WGALResourceFormat::IsDepthFormat(inputDesc.m_Format);
  m_MsaaSampleCount = inputDesc.m_SampleCount;

  WGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = WGALMSAASampleCount::None;
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  if (!ref_graph.GetDevice()->GetCapabilities().m_bSupportsMultiSampledArrays)
  {
    W_ASSERT_DEV(inputDesc.m_uiArraySize == 1, "Stereo rendering is not supported.");
  }

  if (m_bIsDepth)
  {
    auto pass = ref_graph.AddGraphicsPass("MsaaDepthResolve");
    pass.AddDepthStencilTarget(hOutput);
    pass.ReadTexture(hInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      renderViewContext.UpdateViewport();

      auto& globals = renderViewContext.m_pRenderContext->WriteGlobalConstants();
      globals.NumMsaaSamples = m_MsaaSampleCount;

      renderViewContext.m_pRenderContext->BindShader(m_hDepthResolveShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindTexture("DepthTexture", ctx.ResolveTexture(hInput));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }
  else
  {
    bool bStereo = camera.IsStereoscopic();
    auto pass = ref_graph.AddTransferPass("MsaaColorResolve");
    pass.ReadTexture(hInput, {}, WGALResourceState::ResolveSource);
    pass.WriteTexture(hOutput, {}, WGALResourceState::ResolveDestination);
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      WGALTextureSubresource subresource;
      subresource.m_uiMipLevel = 0;
      subresource.m_uiArraySlice = 0;
      ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hOutput), subresource, ctx.ResolveTexture(hInput), subresource);

      if (bStereo)
      {
        subresource.m_uiArraySlice = 1;
        ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hOutput), subresource, ctx.ResolveTexture(hInput), subresource);
      } });
  }

  return W_SUCCESS;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
