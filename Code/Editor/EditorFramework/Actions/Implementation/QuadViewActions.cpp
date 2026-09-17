#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

WActionDescriptorHandle WQuadViewActions::s_hToggleViews;
WActionDescriptorHandle WQuadViewActions::s_hSpawnView;

void WQuadViewActions::RegisterActions()
{
  s_hToggleViews = W_REGISTER_ACTION_1("Scene.View.Toggle", WActionScope::Window, "Scene", "", WQuadViewAction, WQuadViewAction::ButtonType::ToggleViews);
  s_hSpawnView = W_REGISTER_ACTION_1("Scene.View.Span", WActionScope::Window, "Scene", "", WQuadViewAction, WQuadViewAction::ButtonType::SpawnView);
}

void WQuadViewActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hToggleViews);
  WActionManager::UnregisterAction(s_hSpawnView);
}

void WQuadViewActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hToggleViews, "", 3.0f);
}

////////////////////////////////////////////////////////////////////////
// WSceneViewAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WQuadViewAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WQuadViewAction::WQuadViewAction(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(context.m_pWindow);
  W_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'WQtGameObjectViewWidget'!");
  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      SetIconPath(":/EditorFramework/Icons/ToggleViews.svg");
      break;
    case ButtonType::SpawnView:
      SetIconPath(":/EditorFramework/Icons/SpawnView.svg");
      break;
  }
}

WQuadViewAction::~WQuadViewAction() = default;

void WQuadViewAction::Execute(const WVariant& value)
{
  WQtEngineViewWidget* pView = qobject_cast<WQtEngineViewWidget*>(m_Context.m_pWindow);
  WQtEngineDocumentWindow* pWindow = static_cast<WQtEngineDocumentWindow*>(pView->GetDocumentWindow());

  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      // Duck-typing to the rescue!
      QMetaObject::invokeMethod(pWindow, "ToggleViews", Qt::ConnectionType::QueuedConnection, Q_ARG(QWidget*, pView));
      break;
    case ButtonType::SpawnView:
      break;
  }
}
