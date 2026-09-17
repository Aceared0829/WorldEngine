#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that draws visual highlights around selected objects.
///
/// Reads from the depth-stencil buffer to identify selected objects and renders
/// a colored outline and optional overlay. Used in editors and tools to show selection state.
class W_RENDERERCORE_DLL WSelectionHighlightPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSelectionHighlightPass, WRenderPipelinePass);

public:
  WSelectionHighlightPass(const char* szName = "SelectionHighlightPass");
  ~WSelectionHighlightPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodePassThroughPin m_PinColor;                            ///< Pass-through for color buffer with highlight rendered on top.
  WRenderPipelineNodeInputPin m_PinDepthStencil;                           ///< Depth-stencil input used to identify selected objects.

  WShaderResourceHandle m_hShader;                                         ///< Shader for rendering the highlight effect.
  WConstantBufferStorageHandle m_hConstantBuffer;                          ///< Constant buffer for highlight parameters.

  WColor m_HighlightColor = WColorScheme::LightUI(WColorScheme::Yellow); ///< Color of the highlight outline.
  float m_fOverlayOpacity = 0.1f;                                           ///< Opacity of the overlay drawn over selected objects.
};
