#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass for testing stereo rendering configurations.
///
/// Debug pass used to verify stereo rendering pipeline setup and eye separation.
class W_RENDERERCORE_DLL WStereoTestPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WStereoTestPass, WRenderPipelinePass);

public:
  WStereoTestPass();
  ~WStereoTestPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

protected:
  WRenderPipelineNodeInputPin m_PinInput;   ///< Input texture.
  WRenderPipelineNodeOutputPin m_PinOutput; ///< Output texture with stereo test pattern.

  WShaderResourceHandle m_hShader;          ///< Shader for generating the stereo test pattern.
};
