#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_EditDynamicEnumsDlg.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WDynamicStringEnum;

class W_EDITORFRAMEWORK_DLL WQtEditDynamicEnumsDlg : public WQtDialog, public Ui_WQtEditDynamicEnumsDlg
{
public:
  Q_OBJECT

public:
  WQtEditDynamicEnumsDlg(WDynamicStringEnum* pEnum, QWidget* pParent);

  WInt32 GetSelectedItem() const { return m_iSelectedItem; }

private Q_SLOTS:
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_Buttons_clicked(QAbstractButton* button);
  void on_EnumValues_itemDoubleClicked(QListWidgetItem* item);

private:
  void FillList();
  bool EditItem(WString& item);

  bool m_bModified = false;
  WDynamicStringEnum* m_pEnum = nullptr;
  WDynamicArray<WString> m_Values;
  WInt32 m_iSelectedItem = -1;
};
