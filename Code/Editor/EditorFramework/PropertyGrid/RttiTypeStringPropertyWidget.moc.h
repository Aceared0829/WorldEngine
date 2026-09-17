#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

class QHBoxLayout;

class W_EDITORFRAMEWORK_DLL WQtRttiTypeStringPropertyWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtRttiTypeStringPropertyWidget();

protected Q_SLOTS:
  void onMenuAboutToShow();
  void OnTypeSelected(QString sTypeName);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  QPushButton* m_pButton = nullptr;
  QHBoxLayout* m_pLayout = nullptr;
  QMenu* m_pMenu = nullptr;

  WQtTypeMenu m_TypeMenu;
};
