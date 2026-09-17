#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/BlendPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlendConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBlendPass, 1, WRTTIDefaultAllocator<WBlendPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InputA", m_PinInputA),
    W_MEMBER_PROPERTY("InputB", m_PinInputB),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_MEMBER_PROPERTY("BlendFactor", m_fBlendFactor)->AddAttributes(new WDefaultValueAttribute(0.5f)),
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

WBlendPass::WBlendPass()
  : WRenderPipelinePass("BlendPass")
{
  m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Blend.WShader");
  W_ASSERT_DEV(m_hShader.IsValid(), "Could not load blend shader!");
}

WBlendPass::~WBlendPass() = default;

WStatus WBlendPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hInputA = inputs[m_PinInputA.m_uiInputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hInputB = inputs[m_PinInputB.m_uiInputIndex].m_TextureHandle;
  if (hInputA.IsInvalidated() || hInputB.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  const WGALTextureCreationDescription inputDescA = ref_graph.GetTextureDesc(hInputA);
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(inputDescA);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("Blend");
  pass.AddColorTarget(hOutput, {}, WGALRenderTargetLoadOp::Clear);
  pass.SetClearColor(0, WColor(1.0f, 0.0f, 0.0f));
  pass.ReadTexture(hInputA, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  pass.ReadTexture(hInputB, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();

    WBlendConstants cb = {};
    cb.BlendFactor = m_fBlendFactor;
    renderViewContext.m_pRenderContext->SetPushConstants("WBlendConstants", cb);

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

    WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindTexture("InputA", ctx.ResolveTexture(hInputA));
    bindGroup.BindTexture("InputB", ctx.ResolveTexture(hInputB));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return W_SUCCESS;
}

WResult WBlendPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fBlendFactor;
  return W_SUCCESS;
}

WResult WBlendPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fBlendFactor;
  return W_SUCCESS;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BlendPass);
