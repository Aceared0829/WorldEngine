#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SeparatedBilateralBlur.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BilateralBlurConstants.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSeparatedBilateralBlurPass, 2, WRTTIDefaultAllocator<WSeparatedBilateralBlurPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("BlurSource", m_PinBlurSourceInput),
    W_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_ACCESSOR_PROPERTY("BlurRadius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(7)),
      // Should we really expose that? This gives the user control over the error compared to a perfect gaussian.
      // In theory we could also compute this for a given error from the blur radius. See http://dev.theomader.com/gaussian-kernel-calculator/ for visualization.
    W_ACCESSOR_PROPERTY("GaussianSigma", GetGaussianSigma, SetGaussianSigma)->AddAttributes(new WDefaultValueAttribute(4.0f)),
    W_ACCESSOR_PROPERTY("Sharpness", GetSharpness, SetSharpness)->AddAttributes(new WDefaultValueAttribute(120.0f)),
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

WSeparatedBilateralBlurPass::WSeparatedBilateralBlurPass()
  : WRenderPipelinePass("SeparatedBilateral")

{
  {
    // Load shader.
    m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Pipeline/SeparatedBilateralBlur.WShader");
    W_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBilateralBlurCB = WRenderContext::CreateConstantBufferStorage<WBilateralBlurConstants>();
  }
}

WSeparatedBilateralBlurPass::~WSeparatedBilateralBlurPass()
{
  WRenderContext::DeleteConstantBufferStorage(m_hBilateralBlurCB);
}

WStatus WSeparatedBilateralBlurPass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hBlurSource = inputs[m_PinBlurSourceInput.m_uiInputIndex].m_TextureHandle;
  WRenderGraphTextureHandle hDepth = inputs[m_PinDepthInput.m_uiInputIndex].m_TextureHandle;
  if (hBlurSource.IsInvalidated())
    return WStatus(WFmt("BlurSource: Not connected"));
  if (hDepth.IsInvalidated())
    return WStatus(WFmt("Depth: Not connected"));

  const WGALTextureCreationDescription blurDesc = ref_graph.GetTextureDesc(hBlurSource);
  const WGALTextureCreationDescription depthDesc = ref_graph.GetTextureDesc(hDepth);
  if (blurDesc.m_uiWidth != depthDesc.m_uiWidth || blurDesc.m_uiHeight != depthDesc.m_uiHeight)
    return WStatus(WFmt("Blur target and depth buffer need same dimensions"));

  // Output
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(blurDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  // Temp texture for horizontal pass result
  WGALTextureCreationDescription tempDesc = blurDesc;
  tempDesc.m_TextureFlags.Add(WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::RenderTarget);
  WRenderGraphTextureHandle hTemp = ref_graph.CreateTexture(tempDesc);

  // Horizontal pass
  {
    auto pass = ref_graph.AddGraphicsPass("BilateralBlurH");
    pass.AddColorTarget(hTemp);
    pass.ReadTexture(hBlurSource, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepth, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      renderViewContext.UpdateViewport();

      renderViewContext.m_pRenderContext->BindShader(m_hShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");

      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindTexture("DepthBuffer", ctx.ResolveTexture(hDepth));
      bindGroup.BindBuffer("WBilateralBlurConstants", m_hBilateralBlurCB);
      bindGroup.BindTexture("BlurSource", ctx.ResolveTexture(hBlurSource));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  // Vertical pass
  {
    auto pass = ref_graph.AddGraphicsPass("BilateralBlurV");
    pass.AddColorTarget(hOutput);
    pass.ReadTexture(hTemp, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepth, {}, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
      {
      const WRenderViewContext& renderViewContext = *ctx.GetUserData<WRenderViewContext>();
      renderViewContext.UpdateViewport();

      renderViewContext.m_pRenderContext->BindShader(m_hShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");

      WBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindTexture("DepthBuffer", ctx.ResolveTexture(hDepth));
      bindGroup.BindBuffer("WBilateralBlurConstants", m_hBilateralBlurCB);
      bindGroup.BindTexture("BlurSource", ctx.ResolveTexture(hTemp));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return W_SUCCESS;
}

WResult WSeparatedBilateralBlurPass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_uiRadius;
  inout_stream << m_fGaussianSigma;
  inout_stream << m_fSharpness;
  return W_SUCCESS;
}

WResult WSeparatedBilateralBlurPass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_uiRadius;
  inout_stream >> m_fGaussianSigma;
  inout_stream >> m_fSharpness;
  return W_SUCCESS;
}

void WSeparatedBilateralBlurPass::SetRadius(WUInt32 uiRadius)
{
  m_uiRadius = uiRadius;

  WBilateralBlurConstants* cb = WRenderContext::GetConstantBufferData<WBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->BlurRadius = m_uiRadius;
}

WUInt32 WSeparatedBilateralBlurPass::GetRadius() const
{
  return m_uiRadius;
}

void WSeparatedBilateralBlurPass::SetGaussianSigma(const float fSigma)
{
  m_fGaussianSigma = fSigma;

  WBilateralBlurConstants* cb = WRenderContext::GetConstantBufferData<WBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->GaussianFalloff = 1.0f / (2.0f * m_fGaussianSigma * m_fGaussianSigma);
}

float WSeparatedBilateralBlurPass::GetGaussianSigma() const
{
  return m_fGaussianSigma;
}

void WSeparatedBilateralBlurPass::SetSharpness(const float fSharpness)
{
  m_fSharpness = fSharpness;

  WBilateralBlurConstants* cb = WRenderContext::GetConstantBufferData<WBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->Sharpness = m_fSharpness;
}

float WSeparatedBilateralBlurPass::GetSharpness() const
{
  return m_fSharpness;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSeparatedBilateralBlurPassPatch_1_2 : public WGraphPatch
{
public:
  WSeparatedBilateralBlurPassPatch_1_2()
    : WGraphPatch("WSeparatedBilateralBlurPass", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Blur Radius", "BlurRadius");
    pNode->RenameProperty("Gaussian Standard Deviation", "GaussianSigma");
    pNode->RenameProperty("Bilateral Sharpness", "Sharpness");
  }
};

WSeparatedBilateralBlurPassPatch_1_2 g_WSeparatedBilateralBlurPassPatch_1_2;



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
