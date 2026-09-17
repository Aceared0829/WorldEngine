#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

WQtMenuBarActionMapView::WQtMenuBarActionMapView(QWidget* pParent)
  : QMenuBar(pParent)
{
}

WQtMenuBarActionMapView::~WQtMenuBarActionMapView()
{
  ClearView();
}

void WQtMenuBarActionMapView::SetActionContext(const WActionContext& context)
{
  auto pMap = WActionMapManager::GetActionMap(context.m_sMapping);

  W_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void WQtMenuBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void WQtMenuBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->BuildActionTree();

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);

    QSharedPointer<WQtProxy> pProxy = WQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case WActionType::Action:
      {
        W_REPORT_FAILURE("Cannot map actions in a menubar view!");
      }
      break;

      case WActionType::Category:
      {
        W_REPORT_FAILURE("Cannot map category in a menubar view!");
      }
      break;

      case WActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<WQtMenuProxy*>(pProxy.data())->GetQMenu();
        addMenu(pQtMenu);
        WQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

      case WActionType::ActionAndMenu:
      {
        W_REPORT_FAILURE("Cannot map ActionAndMenu in a menubar view!");
      }
      break;
    }
  }
}
