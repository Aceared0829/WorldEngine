#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QToolButton>

class W_GUIFOUNDATION_DLL WQtElementGroupButton : public QToolButton
{
  Q_OBJECT
public:
  enum class ElementAction
  {
    MoveElementUp,
    MoveElementDown,
    DeleteElement,
    Help,
  };

  explicit WQtElementGroupButton(QWidget* pParent, ElementAction action, WQtPropertyWidget* pGroupWidget);
  ElementAction GetAction() const { return m_Action; }
  WQtPropertyWidget* GetGroupWidget() const { return m_pGroupWidget; }

private:
  ElementAction m_Action;
  WQtPropertyWidget* m_pGroupWidget;
};
