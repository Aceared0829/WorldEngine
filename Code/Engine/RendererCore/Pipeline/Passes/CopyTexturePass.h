#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that copies a texture from input to output.
///
/// Simple utility pass for duplicating textures within the render pipeline.
/// Can be used to preserve intermediate results or create copies for multi-pass effects.
// BEGIN-DOCS-CODE-SNIPPET: renderpass-header
class W_RENDERERCORE_DLL WCopyTexturePass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WCopyTexturePass, WRenderPipelinePass);

public:
  WCopyTexturePass();
  ~WCopyTexturePass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

protected:
  WRenderPipelineNodeInputPin m_PinInput;
  WRenderPipelineNodeOutputPin m_PinOutput;
};
// END-DOCS-CODE-SNIPPET
