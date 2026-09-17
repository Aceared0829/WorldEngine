#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders transparent objects with proper blending.
///
/// Provides access to the scene color for refraction and distortion effects.
class W_RENDERERCORE_DLL WTransparentForwardRenderPass : public WForwardRenderPass
{
  W_ADD_DYNAMIC_REFLECTION(WTransparentForwardRenderPass, WForwardRenderPass);

public:
  WTransparentForwardRenderPass(const char* szName = "TransparentForwardRenderPass");
  ~WTransparentForwardRenderPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

protected:
  virtual void DeclareRenderObjectDependencies(WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_pass) override;
  virtual void RenderObjects(const WRenderViewContext& renderViewContext) override;

  void CreateSamplerState();

  WRenderPipelineNodeInputPin m_PinResolvedDepth;   ///< Optional resolved depth for soft particles.
  WRenderPipelineNodeInputPin m_PinSSAO;            ///< Optional SSAO input for ambient occlusion.
  WRenderPipelineNodeInputPin m_PinShadowMasks;     ///< Optional shadow mask input for deferred shadows.

  WGALSamplerStateHandle m_hSceneColorSamplerState; ///< Sampler for scene color texture.
};
