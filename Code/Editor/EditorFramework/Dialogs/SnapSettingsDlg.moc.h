#pragma once

#include <EditorFramework/ui_SnapSettingsDlg.h>
#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class QAbstractButton;

class WQtSnapSettingsDlg : public WQtDialog, public Ui_SnapSettingsDlg
{
  Q_OBJECT

public:
  WQtSnapSettingsDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonBox_clicked(QAbstractButton* button);

private:
  struct KeyValue
  {
    W_DECLARE_POD_TYPE();

    const char* m_szKey;
    float m_fValue;
  };

  WHybridArray<KeyValue, 16> m_Translation;
  WHybridArray<KeyValue, 16> m_Rotation;
  WHybridArray<KeyValue, 16> m_Scale;

  void QueryUI();
};
