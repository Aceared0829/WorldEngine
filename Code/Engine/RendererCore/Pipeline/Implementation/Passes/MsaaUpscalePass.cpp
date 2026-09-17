#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/MsaaUpscalePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsaaUpscalePass, 2, WRTTIDefaultAllocator<WMsaaUpscalePass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_PinInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_ENUM_MEMBER_PROPERTY("MSAA_Mode", WGALMSAASampleCount, m_MsaaMode)
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

WMsaaUpscalePass::WMsaaUpscalePass()
  : WRenderPipelinePass("MsaaUpscalePass")

{
  {
    // Load shader.
    m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/MsaaUpscale.WShader");
    W_ASSERT_DEV(m_hShader.IsValid(), "Could not load msaa upscale shader!");
  }
}

WMsaaUpscalePass::~WMsaaUpscalePass() = default;

WStatus WMsaaUpscalePass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  const WGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  if (inputDesc.m_SampleCount != WGALMSAASampleCount::None)
    return WStatus(WFmt("Input must not be a msaa target"));

  WGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = m_MsaaMode;
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("MsaaUpscale");
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

WResult WMsaaUpscalePass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_MsaaMode;
  return W_SUCCESS;
}

WResult WMsaaUpscalePass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_MsaaMode;
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WMsaaUpscalePassPatch_1_2 : public WGraphPatch
{
public:
  WMsaaUpscalePassPatch_1_2()
    : WGraphPatch("WMsaaUpscalePass", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override { pNode->RenameProperty("MSAA Mode", "MSAA_Mode"); }
};

WMsaaUpscalePassPatch_1_2 g_WMsaaUpscalePassPatch_1_2;



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
