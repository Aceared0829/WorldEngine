#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Basic render pass that renders into a color target.
///
/// Can work as passthrough or directly render into the view's render target if no input is present.
/// It is responsible for rendering unlit objects, all debug rendering and also GUI elements.
class W_RENDERERCORE_DLL WSimpleRenderPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSimpleRenderPass, WRenderPipelinePass);

public:
  WSimpleRenderPass(const char* szName = "SimpleRenderPass");
  ~WSimpleRenderPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  /// Sets a debug message that can be displayed during rendering.
  void SetMessage(const char* szMessage);

protected:
  WRenderPipelineNodePassThroughPin m_PinColor;        ///< Color target pass-through.
  WRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Depth-stencil target pass-through.

  WString m_sMessage;                                  ///< Debug message string.
};
