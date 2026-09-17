#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/StereoTestPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Core/Graphics/Camera.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStereoTestPass, 1, WRTTIDefaultAllocator<WStereoTestPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_PinInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Unit Tests")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStereoTestPass::WStereoTestPass()
  : WRenderPipelinePass("StereoTestPass", true)
{
  {
    // Load shader.
    m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/StereoTest.WShader");
    W_ASSERT_DEV(m_hShader.IsValid(), "Could not load stereo test shader!");
  }
}

WStereoTestPass::~WStereoTestPass() = default;

WStatus WStereoTestPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  const WGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  WGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = WGALMSAASampleCount::None;
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("StereoTest");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

    WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return W_SUCCESS;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_StereoTestPass);
