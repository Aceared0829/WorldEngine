#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiDocumentContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WRmlUiViewContext::WRmlUiViewContext(WRmlUiDocumentContext* pRmlUiContext)
  : WEngineProcessViewContext(pRmlUiContext)
{
  m_pRmlUiContext = pRmlUiContext;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.01f, 1000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WRmlUiViewContext::~WRmlUiViewContext() = default;

bool WRmlUiViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(5, -2, 3));
}

WViewHandle WRmlUiViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Rml Ui Editor - View");
  return pView->GetHandle();
}

void WRmlUiViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  WEngineProcessViewContext::SetCamera(pMsg);

  /*const WUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hResource = m_pRmlUiContext->GetResource();
  if (hResource.IsValid())
  {
    WResourceLock<WRmlUiResource> pResource(hResource, WResourceAcquireMode::AllowLoadingFallback);

    if (pResource->GetDetails().m_Bounds.IsValid())
    {
      const WBoundingBox& bbox = pResource->GetDetails().m_Bounds.GetBox();

      WStringBuilder sText;
      sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", WArgF(bbox.GetHalfExtents().x * 2, 2),
                          WArgF(bbox.GetHalfExtents().y * 2, 2), WArgF(bbox.GetHalfExtents().z * 2, 2));

      WDebugRenderer::DrawInfoText(m_hView, sText, WVec2I32(10, viewHeight - 26), WColor::White);
    }
  }*/
}
