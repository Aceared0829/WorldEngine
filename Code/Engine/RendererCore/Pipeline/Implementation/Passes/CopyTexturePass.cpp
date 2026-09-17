#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/CopyTexturePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: renderpass-reflection
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCopyTexturePass, 1, WRTTIDefaultAllocator<WCopyTexturePass>)
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
// END-DOCS-CODE-SNIPPET
// clang-format on

WCopyTexturePass::WCopyTexturePass()
  : WRenderPipelinePass("CopyTexturePass")
{
}

WCopyTexturePass::~WCopyTexturePass() = default;

// BEGIN-DOCS-CODE-SNIPPET: renderpass-add-render-passes
WStatus WCopyTexturePass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return WStatus(WFmt("Input: Not connected"));

  const WGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(inputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddTransferPass("CopyTexture");
  pass.ReadTexture(hInput, {}, WGALResourceState::CopySource);
  pass.WriteTexture(hOutput, {}, WGALResourceState::CopyDestination);
  pass.SetExecuteCallback([=](const WRenderGraphContext& ctx)
    { ctx.GetCommandEncoder()->CopyTexture(ctx.ResolveTexture(hOutput), ctx.ResolveTexture(hInput)); });

  return W_SUCCESS;
}
// END-DOCS-CODE-SNIPPET



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_CopyTexturePass);
