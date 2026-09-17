#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that upscales a texture to a multi-sampled render target.
///
/// Converts a regular texture to an MSAA texture by replicating samples. Used when
/// transitioning from non-MSAA to MSAA rendering in the pipeline.
class W_RENDERERCORE_DLL WMsaaUpscalePass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WMsaaUpscalePass, WRenderPipelinePass);

public:
  WMsaaUpscalePass();
  ~WMsaaUpscalePass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodeInputPin m_PinInput;
  WRenderPipelineNodeOutputPin m_PinOutput;

  WEnum<WGALMSAASampleCount> m_MsaaMode = WGALMSAASampleCount::None;
  WShaderResourceHandle m_hShader;
};
