#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

class WRenderGraphPassBuilder;


/// Base class for forward rendering passes.
///
/// Forward rendering evaluates lighting for each rendered object during the geometry pass.
/// Derived classes implement specific object filtering (opaque, transparent, etc.).
class W_RENDERERCORE_DLL WForwardRenderPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WForwardRenderPass, WRenderPipelinePass);

public:
  WForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~WForwardRenderPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  /// Configures shader permutation variables based on render settings.
  virtual void SetupPermutationVars(const WRenderViewContext& renderViewContext);

  /// Declares the renderer dependencies for the render data categories that RenderObjects will render.
  ///
  /// Must declare the same set of categories that RenderObjects passes to RenderDataWithCategory.
  virtual void DeclareRenderObjectDependencies(WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_pass) = 0;

  /// Renders the objects for this pass. Must be implemented by derived classes.
  virtual void RenderObjects(const WRenderViewContext& renderViewContext) = 0;

  WRenderPipelineNodePassThroughPin m_PinColor;          ///< Color target for rendering.
  WRenderPipelineNodePassThroughPin m_PinDepthStencil;   ///< Depth-stencil target.

  WEnum<WForwardRenderShadingQuality> m_ShadingQuality; ///< Quality level for shading calculations.

  WTexture2DResourceHandle m_hWhiteTexture;              ///< Fallback white texture for unbound inputs.
};
