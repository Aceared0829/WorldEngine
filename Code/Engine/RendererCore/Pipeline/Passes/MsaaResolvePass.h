#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that resolves a multi-sampled texture to a non-multi-sampled texture.
///
/// Converts MSAA render targets to regular textures by averaging the samples. Supports both
/// color and depth textures. Required when using MSAA rendering with post-processing effects
/// that cannot operate on multi-sampled textures.
class W_RENDERERCORE_DLL WMsaaResolvePass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WMsaaResolvePass, WRenderPipelinePass);

public:
  WMsaaResolvePass();
  ~WMsaaResolvePass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

protected:
  WRenderPipelineNodeInputPin m_PinInput;
  WRenderPipelineNodeOutputPin m_PinOutput;

  bool m_bIsDepth = false;
  WGALMSAASampleCount::Enum m_MsaaSampleCount = WGALMSAASampleCount::None;
  WShaderResourceHandle m_hDepthResolveShader;
};
