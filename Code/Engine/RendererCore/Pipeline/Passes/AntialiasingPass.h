#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that applies post-process anti-aliasing.
///
/// Currently it only does an advanced resolve of MSAA render targets using a two pixel wide bspline filter.
class W_RENDERERCORE_DLL WAntialiasingPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WAntialiasingPass, WRenderPipelinePass);

public:
  WAntialiasingPass();
  ~WAntialiasingPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodeInputPin m_PinInput;
  WRenderPipelineNodeOutputPin m_PinOutput;

  WHashedString m_sMsaaSampleCount;
  WShaderResourceHandle m_hShader;
};
