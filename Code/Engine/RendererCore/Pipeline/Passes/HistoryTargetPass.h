#pragma once

#include <RendererCore/Pipeline/Passes/HistorySourcePass.h>

/// Render pass that stores data to be accessible in the next frame.
///
/// Works in pairs with WHistorySourcePass. Receives the current frame's data as input
/// and stores it for the next frame. Set SourcePassName to match the corresponding
/// WHistorySourcePass. See WHistorySourcePass for usage details.
class W_RENDERERCORE_DLL WHistoryTargetPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WHistoryTargetPass, WRenderPipelinePass);

public:
  WHistoryTargetPass(const char* szName = "HistoryTargetPass");
  ~WHistoryTargetPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  /// Provides the history texture handle for the input pin.
  virtual WGALTextureHandle QueryTextureProvider(const WRenderPipelineNodePin* pPin, const WGALTextureCreationDescription& desc) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodeInputProviderPin m_PinInput;  ///< Input texture to store for next frame.
  WString m_sSourcePassName = "HistorySourcePass"; ///< Name of the paired WHistorySourcePass.
};
