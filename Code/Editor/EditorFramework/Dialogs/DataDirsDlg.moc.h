#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_DataDirsDlg.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class W_EDITORFRAMEWORK_DLL WQtDataDirsDlg : public WQtDialog, public Ui_WQtDataDirsDlg
{
public:
  Q_OBJECT

public:
  WQtDataDirsDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonOK_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonUp_clicked();
  void on_ButtonDown_clicked();
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOpenFolder_clicked();
  void on_ListDataDirs_itemSelectionChanged();
  void on_ListDataDirs_itemDoubleClicked(QListWidgetItem* pItem);

private:
  void FillList();

  WInt32 m_iSelection;
  WApplicationFileSystemConfig m_Config;
};
