#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

/// Render pass that renders render data with a custom render data category.
class W_RENDERERCORE_DLL WCustomRenderDataPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WCustomRenderDataPass, WRenderPipelinePass);

public:
  WCustomRenderDataPass(const char* szName = "CustomRenderDataPass");
  ~WCustomRenderDataPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodePassThroughPin m_PinColor;        ///< Color target for rendering.
  WRenderPipelineNodePassThroughPin m_PinDepthStencil; ///< Depth-stencil target.

  WString m_sRenderDataCategoryName;
  WRenderData::Category m_RenderDataCategory;
  WEnum<WRenderSortingFunctions> m_SortingFunction;
};
