#pragma once

#include <EditorPluginFmod/EditorPluginFmodDLL.h>
#include <EditorPluginFmod/ui_FmodProjectSettingsDlg.h>
#include <FmodPlugin/FmodSingleton.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WQtFmodProjectSettingsDlg : public WQtDialog, public Ui_FmodProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  WQtFmodProjectSettingsDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonBox_clicked(QAbstractButton* pButton);
  void on_ListPlatforms_itemSelectionChanged();
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonMB_clicked();

private:
  WResult Save();
  void Load();
  void SetCurrentPlatform(const char* szPlatform);
  void StoreCurrentPlatform();

  WString m_sCurrentPlatform;
  WFmodAssetProfiles m_ConfigsOld;
  WFmodAssetProfiles m_Configs;
};
