#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>

WQtElementGroupButton::WQtElementGroupButton(QWidget* pParent, WQtElementGroupButton::ElementAction action, WQtPropertyWidget* pGroupWidget)
  : QToolButton(pParent)
{
  m_Action = action;
  m_pGroupWidget = pGroupWidget;

  setAutoRaise(true);

  setIconSize(QSize(16, 16));

  switch (action)
  {
    case WQtElementGroupButton::ElementAction::MoveElementUp:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveUp.svg")));
      break;
    case WQtElementGroupButton::ElementAction::MoveElementDown:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveDown.svg")));
      break;
    case WQtElementGroupButton::ElementAction::DeleteElement:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Delete.svg")));
      setToolTip("Remove this element.");
      break;
    case WQtElementGroupButton::ElementAction::Help:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Help-BW.svg")));
      setToolTip("Open the online help for this.");
      break;
  }
}
