#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class QHBoxLayout;
class WQtDynamicStringEnumMenuButton;

class W_EDITORFRAMEWORK_DLL WQtDynamicStringEnumPropertyWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtDynamicStringEnumPropertyWidget();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

  void SetNewValue(WStringView sNewValue);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  WQtDynamicStringEnumMenuButton* m_pButton = nullptr;
};
