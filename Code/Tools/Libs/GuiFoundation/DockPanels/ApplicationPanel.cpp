#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <ads/AutoHideDockContainer.h>
#include <ads/DockAreaWidget.h>
#include <ads/DockContainerWidget.h>
#include <ads/DockWidgetTab.h>

W_BEGIN_STATIC_REFLECTED_TYPE(WQtApplicationPanel, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

WDynamicArray<WQtApplicationPanel*> WQtApplicationPanel::s_AllApplicationPanels;

WQtApplicationPanel::WQtApplicationPanel(ads::CDockManager* pDockManager, const char* szPanelName)
  : ads::CDockWidget(pDockManager, szPanelName, WQtContainerWindow::GetContainerWindow())
{
  WStringBuilder sPanel("AppPanel_", szPanelName);

  setObjectName(WMakeQString(sPanel));
  setWindowTitle(WMakeQString(WTranslate(szPanelName)));

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  WQtContainerWindow::GetContainerWindow()->AddApplicationPanel(this);

  WToolsProject::s_Events.AddEventHandler(WMakeDelegate(&WQtApplicationPanel::ToolsProjectEventHandler, this));
}

WQtApplicationPanel::~WQtApplicationPanel()
{
  WToolsProject::s_Events.RemoveEventHandler(WMakeDelegate(&WQtApplicationPanel::ToolsProjectEventHandler, this));

  s_AllApplicationPanels.RemoveAndSwap(this);
}

void WQtApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this).IgnoreResult();

  QWidget* pThis = this;

  if (isAutoHide())
  {
    // Expand the auto-hide container to make the widget visible
    autoHideDockContainer()->collapseView(false);
  }

  if (dockAreaWidget())
  {
    dockAreaWidget()->setCurrentDockWidget(this);
  }

  while (pThis)
  {
    pThis->raise();
    pThis = qobject_cast<QWidget*>(pThis->parent());
  }
}


void WQtApplicationPanel::ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectClosing:
      setEnabled(false);
      break;
    case WToolsProjectEvent::Type::ProjectOpened:
      setEnabled(true);
      break;

    default:
      break;
  }
}

bool WQtApplicationPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (WQtProxy::TriggerDocumentAction(nullptr, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }
  return ads::CDockWidget::event(pEvent);
}
