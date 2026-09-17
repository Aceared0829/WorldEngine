#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WAnimationClipViewContext::WAnimationClipViewContext(WAnimationClipContext* pContext)
  : WEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WAnimationClipViewContext::~WAnimationClipViewContext() = default;

bool WAnimationClipViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(5, -2, 3));
}


WViewHandle WAnimationClipViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Animation Clip Editor - View");
  return pView->GetHandle();
}

void WAnimationClipViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    WEngineProcessViewContext::DrawSimpleGrid();
  }

  WEngineProcessViewContext::SetCamera(pMsg);
}
