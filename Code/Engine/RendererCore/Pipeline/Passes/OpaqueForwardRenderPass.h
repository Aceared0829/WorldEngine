#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders opaque objects.
///
/// Renders all opaque geometry with full lighting and shading.
/// Optionally accepts an ambient occlusion input for enhanced shading.
class W_RENDERERCORE_DLL WOpaqueForwardRenderPass : public WForwardRenderPass
{
  W_ADD_DYNAMIC_REFLECTION(WOpaqueForwardRenderPass, WForwardRenderPass);

public:
  WOpaqueForwardRenderPass(const char* szName = "OpaqueForwardRenderPass");
  ~WOpaqueForwardRenderPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

protected:
  virtual void SetupPermutationVars(const WRenderViewContext& renderViewContext) override;

  virtual void DeclareRenderObjectDependencies(WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_pass) override;

  virtual void RenderObjects(const WRenderViewContext& renderViewContext) override;

  WRenderPipelineNodeInputPin m_PinSSAO;        ///< Optional SSAO input for ambient occlusion.
  WRenderPipelineNodeInputPin m_PinShadowMasks; ///< Optional shadow mask input for deferred shadows.

  bool m_bWriteDepth = true;                     ///< Whether to write to the depth buffer.
};
