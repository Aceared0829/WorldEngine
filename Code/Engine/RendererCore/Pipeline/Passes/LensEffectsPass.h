#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Forward render pass that renders secondary lens flares.
class W_RENDERERCORE_DLL WLensEffectsPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WLensEffectsPass, WRenderPipelinePass);

public:
  WLensEffectsPass(const char* szName = "LensEffectsPass");
  ~WLensEffectsPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

protected:
  WRenderPipelineNodePassThroughPin m_PinColor;
  WRenderPipelineNodeInputPin m_PinResolvedDepth;
};
