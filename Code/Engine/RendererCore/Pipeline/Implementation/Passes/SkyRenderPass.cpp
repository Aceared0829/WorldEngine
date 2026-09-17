#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SkyRenderPass.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkyRenderPass, 1, WRTTIDefaultAllocator<WSkyRenderPass>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSkyRenderPass::WSkyRenderPass(const char* szName)
  : WForwardRenderPass(szName)
{
}

WSkyRenderPass::~WSkyRenderPass() = default;

void WSkyRenderPass::DeclareRenderObjectDependencies(WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_pass)
{
  DeclareRendererDependenciesForCategory(WDefaultRenderDataCategories::Sky, ref_graph, ref_pass);
}

void WSkyRenderPass::RenderObjects(const WRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, WDefaultRenderDataCategories::Sky);
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SkyRenderPass);
