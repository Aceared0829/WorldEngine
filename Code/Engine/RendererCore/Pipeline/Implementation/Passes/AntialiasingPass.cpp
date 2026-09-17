#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/AntialiasingPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAntialiasingPass, 1, WRTTIDefaultAllocator<WAntialiasingPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_PinInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Post Processing")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAntialiasingPass::WAntialiasingPass()
  : WRenderPipelinePass("AntialiasingPass", true)
{
  {
    // Load shader.
    m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Antialiasing.WShader");
    W_ASSERT_DEV(m_hShader.IsValid(), "Could not load antialiasing shader!");
  }
}

WAntialiasingPass::~WAntialiasingPass() = default;

WStatus WAntialiasingPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<WRenderPipelinePinConnection const> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  // Validate input
  WRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected "));
  const WGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);

  if (inputDesc.m_SampleCount == WGALMSAASampleCount::TwoSamples)
  {
    m_sMsaaSampleCount.Assign("MSAA_SAMPLES_TWO");
  }
  else if (inputDesc.m_SampleCount == WGALMSAASampleCount::FourSamples)
  {
    m_sMsaaSampleCount.Assign("MSAA_SAMPLES_FOUR");
  }
  else if (inputDesc.m_SampleCount == WGALMSAASampleCount::EightSamples)
  {
    m_sMsaaSampleCount.Assign("MSAA_SAMPLES_EIGHT");
  }
  else
  {
    return WStatus(WFmt("Input: Invalid MSAA sample count"));
  }

  // Create output
  WGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = WGALMSAASampleCount::None;
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  // Add passes
  WGALDevice* pDevice = ref_graph.GetDevice();

  auto pass = ref_graph.AddGraphicsPass("AntialiasingPass");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hInput, {}, WGALResourceState::ShaderResource);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MSAA_SAMPLES", m_sMsaaSampleCount);
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

    WBindGroupBuilder& bindGroup = WRenderContext::GetDefaultInstance()->GetBindGroup();
    if (!pDevice->GetCapabilities().m_bSupportsMultiSampledArrays)
    {
      W_ASSERT_DEV(inputDesc.m_uiArraySize == 1, "Stereo rendering is not supported.");
    }
    bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return W_SUCCESS;
}

WResult WAntialiasingPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}

WResult WAntialiasingPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AntialiasingPass);
