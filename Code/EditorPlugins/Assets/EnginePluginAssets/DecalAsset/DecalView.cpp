#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WDecalViewContext::WDecalViewContext(WDecalContext* pDecalContext)
  : WEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

WDecalViewContext::~WDecalViewContext() = default;

WViewHandle WDecalViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Decal Editor - View");
  return pView->GetHandle();
}
