#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WUInt32 WRemoteEngineProcessViewContext::s_uiActiveViewID = 0;
WRemoteEngineProcessViewContext* WRemoteEngineProcessViewContext::s_pActiveRemoteViewContext = nullptr;

WRemoteEngineProcessViewContext::WRemoteEngineProcessViewContext(WEngineProcessDocumentContext* pContext)
  : WEngineProcessViewContext(pContext)
{
}

WRemoteEngineProcessViewContext::~WRemoteEngineProcessViewContext()
{
  if (s_pActiveRemoteViewContext == this)
  {
    s_pActiveRemoteViewContext = nullptr;

    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetWorld(nullptr);
    }
  }

  // make sure the base class destructor doesn't destroy the view
  m_hView.Invalidate();
}

void WRemoteEngineProcessViewContext::HandleViewMessage(const WEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WActivateRemoteViewMsgToEngine>())
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = WEditorEngineProcessApp::GetSingleton()->CreateRemoteWindowAndView(&m_Camera);
    }

    s_pActiveRemoteViewContext = this;

    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView))
    {
      WEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
      pView->SetWorld(pDocumentContext->GetWorld());
      pView->SetCamera(&m_Camera);

      s_uiActiveViewID = pMsg->m_uiViewID;
    }
  }

  // ignore all messages for views that are currently not activated
  if (pMsg->m_uiViewID != s_uiActiveViewID)
    return;

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewRedrawMsgToEngine>())
  {
    const WViewRedrawMsgToEngine* pMsg2 = static_cast<const WViewRedrawMsgToEngine*>(pMsg);
    SetCamera(pMsg2);

    // skip the on-message redraw, in remote mode it will just render as fast as it can
    // Redraw(false);
  }
}

WViewHandle WRemoteEngineProcessViewContext::CreateView()
{
  W_ASSERT_NOT_IMPLEMENTED;
  return WViewHandle();
}
