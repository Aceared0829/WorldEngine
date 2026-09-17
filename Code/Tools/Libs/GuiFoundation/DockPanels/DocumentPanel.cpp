#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

WQtDocumentPanel::WQtDocumentPanel(ads::CDockManager* pDockManager, QWidget* pParent, WDocument* pDocument)
  : ads::CDockWidget(pDockManager, "WQtDocumentPanel", pParent)
{
  m_pDocument = pDocument;

  setMinimumWidth(300);
  setMinimumHeight(200);

  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetClosable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFloatable, true);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetMovable, true);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFocusable, true);
}

WQtDocumentPanel::~WQtDocumentPanel() = default;

bool WQtDocumentPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (WQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }

  return CDockWidget::event(pEvent);
}
