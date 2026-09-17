#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/BlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlurConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBlurPass, 1, WRTTIDefaultAllocator<WBlurPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_PinInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(15)),
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

WBlurPass::WBlurPass()
  : WRenderPipelinePass("BlurPass")

{
  {
    // Load shader.
    m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/Blur.WShader");
    W_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBlurCB = WRenderContext::CreateConstantBufferStorage<WBlurConstants>();
  }
}

WBlurPass::~WBlurPass()
{
  WRenderContext::DeleteConstantBufferStorage(m_hBlurCB);
}

WStatus WBlurPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  const WGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(inputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("Blur");
  pass.AddColorTarget(hOutput, {}, WGALRenderTargetLoadOp::Clear);
  pass.SetClearColor(0, WColor(1.0f, 0.0f, 0.0f));
  pass.ReadTexture(hInput, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    {
    const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
    renderViewContext.UpdateViewport();

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

    WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindTexture("Input", ctx.ResolveTexture(hInput));
    bindGroup.BindBuffer("WBlurConstants", m_hBlurCB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return W_SUCCESS;
}

WResult WBlurPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_iRadius;
  return W_SUCCESS;
}

WResult WBlurPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_iRadius;
  return W_SUCCESS;
}

void WBlurPass::SetRadius(WInt32 iRadius)
{
  m_iRadius = iRadius;

  WBlurConstants* cb = WRenderContext::GetConstantBufferData<WBlurConstants>(m_hBlurCB);
  cb->BlurRadius = m_iRadius;
}

WInt32 WBlurPass::GetRadius() const
{
  return m_iRadius;
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BlurPass);
