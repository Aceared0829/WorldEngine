#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>

#include <EditorFramework/Assets/AssetDocument.h>

WQtEngineDocumentWindow::WQtEngineDocumentWindow(WAssetDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  pDocument->m_ProcessMessageEvent.AddEventHandler(WMakeDelegate(&WQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  pDocument->m_CommonAssetUiChangeEvent.AddEventHandler(WMakeDelegate(&WQtEngineDocumentWindow::CommonAssetUiEventHandler, this));

  m_pCuratorControl = new WQtCuratorControl(this);
  statusBar()->addPermanentWidget(m_pCuratorControl, 0);
}

WQtEngineDocumentWindow::~WQtEngineDocumentWindow()
{
  // make sure the selection gets cleared before the views are destroyed, so that dependent code can clean up first
  GetDocument()->GetSelectionManager()->Clear();

  GetDocument()->m_ProcessMessageEvent.RemoveEventHandler(WMakeDelegate(&WQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  GetDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(WMakeDelegate(&WQtEngineDocumentWindow::CommonAssetUiEventHandler, this));

  // delete all view widgets, so that they can send their messages before we clean up the engine connection
  DestroyAllViews();
}


WEditorEngineConnection* WQtEngineDocumentWindow::GetEditorEngineConnection() const
{
  return GetDocument()->GetEditorEngineConnection();
}

static WObjectPickingResult s_DummyResult;

const WObjectPickingResult& WQtEngineDocumentWindow::PickObject(WUInt16 uiScreenPosX, WUInt16 uiScreenPosY, WQtEngineViewWidget* pView) const
{
  if (pView == nullptr)
    pView = GetHoveredViewWidget();

  if (pView != nullptr)
    return pView->PickObject(uiScreenPosX, uiScreenPosY);

  return s_DummyResult;
}


WAssetDocument* WQtEngineDocumentWindow::GetDocument() const
{
  return static_cast<WAssetDocument*>(WQtDocumentWindow::GetDocument());
}

void WQtEngineDocumentWindow::InternalRedraw()
{
  // TODO: Move this to a better place (some kind of regular update function, not redraw)
  GetDocument()->SyncObjectsToEngine();
}

WQtEngineViewWidget* WQtEngineDocumentWindow::GetHoveredViewWidget() const
{
  QWidget* pWidget = QApplication::widgetAt(QCursor::pos());

  while (pWidget != nullptr)
  {
    WQtEngineViewWidget* pCandidate = qobject_cast<WQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
        return pCandidate;

      return nullptr;
    }

    pWidget = pWidget->parentWidget();
  }

  return nullptr;
}

WQtEngineViewWidget* WQtEngineDocumentWindow::GetFocusedViewWidget() const
{
  QWidget* pWidget = QApplication::focusWidget();

  while (pWidget != nullptr)
  {
    WQtEngineViewWidget* pCandidate = qobject_cast<WQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
        return pCandidate;

      return nullptr;
    }

    pWidget = pWidget->parentWidget();
  }

  return nullptr;
}

WQtEngineViewWidget* WQtEngineDocumentWindow::GetViewWidgetByID(WUInt32 uiViewID) const
{
  for (auto pView : m_ViewWidgets)
  {
    if (pView && pView->GetViewID() == uiViewID)
      return pView;
  }

  return nullptr;
}

WArrayPtr<WQtEngineViewWidget* const> WQtEngineDocumentWindow::GetViewWidgets() const
{
  return m_ViewWidgets;
}

void WQtEngineDocumentWindow::AddViewWidget(WQtEngineViewWidget* pView)
{
  m_ViewWidgets.PushBack(pView);
  WEngineWindowEvent e;
  e.m_Type = WEngineWindowEvent::Type::ViewCreated;
  e.m_pView = pView;
  m_EngineWindowEvent.Broadcast(e);
}

void WQtEngineDocumentWindow::RemoveViewWidget(WQtEngineViewWidget* pView)
{
  m_ViewWidgets.RemoveAndSwap(pView);
  WEngineWindowEvent e;
  e.m_Type = WEngineWindowEvent::Type::ViewDestroyed;
  e.m_pView = pView;
  m_EngineWindowEvent.Broadcast(e);
}

void WQtEngineDocumentWindow::CommonAssetUiEventHandler(const WCommonAssetUiState& e)
{
  WSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "CommonAssetUiState";
  msg.m_PayloadValue = e.m_fValue;

  switch (e.m_State)
  {
    case WCommonAssetUiState::Restart:
      msg.m_sPayload = "Restart";
      break;

    case WCommonAssetUiState::Loop:
      msg.m_sPayload = "Loop";
      break;

    case WCommonAssetUiState::Pause:
      msg.m_sPayload = "Pause";
      break;

    case WCommonAssetUiState::Grid:
      msg.m_sPayload = "Grid";
      break;

    case WCommonAssetUiState::SimulationSpeed:
      msg.m_sPayload = "SimulationSpeed";
      break;

    case WCommonAssetUiState::Visualizers:
      msg.m_sPayload = "Visualizers";
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (!msg.m_sPayload.IsEmpty())
  {
    GetEditorEngineConnection()->SendMessage(&msg);
  }
}

void WQtEngineDocumentWindow::ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WEditorEngineViewMsg>())
  {
    const WEditorEngineViewMsg* pViewMsg = static_cast<const WEditorEngineViewMsg*>(pMsg);

    WQtEngineViewWidget* pView = GetViewWidgetByID(pViewMsg->m_uiViewID);

    if (pView != nullptr)
      pView->HandleViewMessage(pViewMsg);
  }
}

void WQtEngineDocumentWindow::DestroyAllViews()
{
  while (!m_ViewWidgets.IsEmpty())
  {
    delete m_ViewWidgets[0];
  }
}

void WQtEngineDocumentWindow::CreateImageCapture(const char* szOutputPath)
{
  if (!m_ViewWidgets.IsEmpty())
    m_ViewWidgets[0]->TakeScreenshot(szOutputPath);
}
