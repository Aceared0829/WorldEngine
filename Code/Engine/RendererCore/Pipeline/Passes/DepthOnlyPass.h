#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Render pass that renders geometry to a depth buffer without color output.
///
/// Used for depth pre-pass, shadow map generation, or depth-only rendering.
/// Can selectively render static, dynamic, and transparent objects.
class W_RENDERERCORE_DLL WDepthOnlyPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WDepthOnlyPass, WRenderPipelinePass);

public:
  WDepthOnlyPass(const char* szName = "DepthOnlyPass");
  ~WDepthOnlyPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Depth-stencil target for depth writes.

  bool m_bRenderStaticObjects = true;                   ///< Whether to render static objects.
  bool m_bRenderDynamicObjects = true;                  ///< Whether to render dynamic objects.
  bool m_bRenderTransparentObjects = false;             ///< Whether to render transparent objects.
};
