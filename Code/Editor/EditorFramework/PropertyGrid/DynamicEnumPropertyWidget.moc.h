#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class WDynamicEnum;
class WQtSearchableMenu;

class W_EDITORFRAMEWORK_DLL WQtDynamicEnumPropertyWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtDynamicEnumPropertyWidget();

protected slots:
  void onMenuAboutToShow();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  QHBoxLayout* m_pLayout = nullptr;
  WDynamicEnum* m_pEnum = nullptr;
  QPushButton* m_pButton = nullptr;
  QMenu* m_pMenu = nullptr;
  WQtSearchableMenu* m_pSearchableMenu = nullptr;
  WString m_sEnumAttribute;
  static WMap<WString, QString> s_LastSearch;
};
