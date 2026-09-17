#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

WQtMenuActionMapView::WQtMenuActionMapView(QWidget* pParent)
{
  setToolTipsVisible(true);
}

WQtMenuActionMapView::~WQtMenuActionMapView()
{
  ClearView();
}

void WQtMenuActionMapView::SetActionContext(const WActionContext& context)
{
  auto pMap = WActionMapManager::GetActionMap(context.m_sMapping);

  W_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void WQtMenuActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void WQtMenuActionMapView::AddDocumentObjectToMenu(WHashTable<WUuid, QSharedPointer<WQtProxy>>& ref_proxies, WActionContext& ref_context,
  WActionMap* pActionMap, QMenu* pCurrentRoot, const WActionMap::TreeNode* pObject)
{
  if (pObject == nullptr)
    return;

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = pActionMap->GetDescriptor(pChild);
    QSharedPointer<WQtProxy> pProxy = WQtProxy::GetProxy(ref_context, pDesc->m_hAction);
    ref_proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case WActionType::Action:
      {
        QAction* pQtAction = static_cast<WQtActionProxy*>(pProxy.data())->GetQAction();
        pCurrentRoot->addAction(pQtAction);
      }
      break;

      case WActionType::Category:
      {
        pCurrentRoot->addSeparator();

        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pCurrentRoot, pChild);

        pCurrentRoot->addSeparator();
      }
      break;

      case WActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<WQtMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;

      case WActionType::ActionAndMenu:
      {
        QAction* pQtAction = static_cast<WQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        QMenu* pQtMenu = static_cast<WQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addAction(pQtAction);
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}

void WQtMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->BuildActionTree();

  AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, this, pObject);
}
