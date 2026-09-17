#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMenu>
#include <QToolButton>

WQtToolBarActionMapView::WQtToolBarActionMapView(QString sTitle, QWidget* pParent)
  : QToolBar(sTitle, pParent)
{
  setIconSize(QSize(16, 16));
  setFloatable(false);

  toggleViewAction()->setEnabled(false);
}

WQtToolBarActionMapView::~WQtToolBarActionMapView()
{
  ClearView();
}

void WQtToolBarActionMapView::SetActionContext(const WActionContext& context)
{
  auto pMap = WActionMapManager::GetActionMap(context.m_sMapping);

  W_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void WQtToolBarActionMapView::setVisible(bool bVisible)
{
  QToolBar::setVisible(true);
}

void WQtToolBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void WQtToolBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->BuildActionTree();

  CreateView(pObject);

  if (!actions().isEmpty() && actions().back()->isSeparator())
  {
    QAction* pAction = actions().back();
    removeAction(pAction);
    pAction->deleteLater();
  }
}

void WQtToolBarActionMapView::CreateView(const WActionMap::TreeNode* pObject)
{
  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);
    QSharedPointer<WQtProxy> pProxy = WQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case WActionType::Action:
      {
        QAction* pQtAction = static_cast<WQtActionProxy*>(pProxy.data())->GetQAction();
        addAction(pQtAction);
      }
      break;

      case WActionType::Category:
      {
        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());

        CreateView(pChild);

        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());
      }
      break;

      case WActionType::Menu:
      {
        WNamedAction* pNamed = static_cast<WNamedAction*>(pProxy->GetAction());

        QMenu* pQtMenu = static_cast<WQtMenuProxy*>(pProxy.data())->GetQMenu();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        pButton->setText(pQtMenu->title());
        pButton->setIcon(WQtUiServices::GetCachedIconResource(pNamed->GetIconPath()));

        WStringBuilder sTooltip = WTranslateTooltip(pNamed->GetName());
        if (sTooltip.IsEmpty())
        {
          sTooltip = WTranslate(pNamed->GetName());
          sTooltip.ReplaceAll("&", "");
        }
        pButton->setToolTip(WMakeQString(sTooltip));

        pNamed->m_StatusUpdateEvent.AddEventHandler([=](WAction* pAction)
          { pButton->setIcon(WQtUiServices::GetCachedIconResource(pNamed->GetIconPath())); });

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        WQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

      case WActionType::ActionAndMenu:
      {
        WNamedAction* pNamed = static_cast<WNamedAction*>(pProxy->GetAction());

        QMenu* pQtMenu = static_cast<WQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        QAction* pQtAction = static_cast<WQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setDefaultAction(pQtAction);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);

        WStringBuilder sTooltip = WTranslateTooltip(pNamed->GetName());
        if (sTooltip.IsEmpty())
        {
          sTooltip = WTranslate(pNamed->GetName());
          sTooltip.ReplaceAll("&", "");
        }
        pButton->setToolTip(WMakeQString(sTooltip));

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        WQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}
