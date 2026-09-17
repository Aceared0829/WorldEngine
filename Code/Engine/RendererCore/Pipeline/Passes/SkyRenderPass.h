#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders skybox and sky objects.
///
/// Renders the skybox and other sky-related objects at infinite distance.
/// Typically rendered after opaque objects but before transparent objects.
class W_RENDERERCORE_DLL WSkyRenderPass : public WForwardRenderPass
{
  W_ADD_DYNAMIC_REFLECTION(WSkyRenderPass, WForwardRenderPass);

public:
  WSkyRenderPass(const char* szName = "SkyRenderPass");
  ~WSkyRenderPass();

protected:
  virtual void DeclareRenderObjectDependencies(WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_pass) override;
  virtual void RenderObjects(const WRenderViewContext& renderViewContext) override;
};
