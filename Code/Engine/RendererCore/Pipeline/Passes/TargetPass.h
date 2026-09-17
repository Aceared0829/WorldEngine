#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct WGALRenderTargets;

/// Render pass that outputs to final render targets or swap chain.
///
/// Terminal pass that writes the final pipeline output to the specified render targets.
/// Can output to multiple color targets and depth-stencil, typically used as the final
/// stage in a render pipeline to present results to the screen or external targets.
class W_RENDERERCORE_DLL WTargetPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WTargetPass, WRenderPipelinePass);

public:
  WTargetPass(const char* szName = "TargetPass");
  ~WTargetPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  /// Provides the actual target texture handles for connected pins.
  virtual WGALTextureHandle QueryTextureProvider(const WRenderPipelineNodePin* pPin, const WGALTextureCreationDescription& desc) override;

private:
  /// Validates that an input pin's texture matches expected dimensions and format.
  WStatus VerifyInput(WRenderGraph& graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WTempHashedString sPinName);

protected:
  WRenderPipelineNodeInputProviderPin m_PinColor0;       ///< Color target 0 input.
  WRenderPipelineNodeInputProviderPin m_PinColor1;       ///< Color target 1 input.
  WRenderPipelineNodeInputProviderPin m_PinColor2;       ///< Color target 2 input.
  WRenderPipelineNodeInputProviderPin m_PinColor3;       ///< Color target 3 input.
  WRenderPipelineNodeInputProviderPin m_PinColor4;       ///< Color target 4 input.
  WRenderPipelineNodeInputProviderPin m_PinColor5;       ///< Color target 5 input.
  WRenderPipelineNodeInputProviderPin m_PinColor6;       ///< Color target 6 input.
  WRenderPipelineNodeInputProviderPin m_PinColor7;       ///< Color target 7 input.
  WRenderPipelineNodeInputProviderPin m_PinDepthStencil; ///< Depth-stencil target input.

  WGALRenderTargets m_RenderTargets;                     ///< Configured render target setup.
  WGALSwapChainHandle m_hSwapChain;                      ///< Swap chain handle if rendering to screen.
};
