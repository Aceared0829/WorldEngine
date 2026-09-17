#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_TagsDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

class W_EDITORFRAMEWORK_DLL WQtTagsDlg : public WQtDialog, public Ui_WQtTagsDlg
{
public:
  Q_OBJECT

public:
  WQtTagsDlg(const WVariant& startup, QWidget* pParent);

private Q_SLOTS:
  void on_ButtonNewCategory_clicked();
  void on_ButtonNewTag_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeTags_itemSelectionChanged();

private:
  void LoadTags();
  void SaveTags();
  void FillList();
  void GetTagsFromList();

  QTreeWidgetItem* CreateTagItem(QTreeWidgetItem* pParentItem, const QString& tag, bool bBuiltIn);

  WString m_sStartupCategory;
  WHybridArray<WToolsTag, 32> m_Tags;
  WMap<WString, QTreeWidgetItem*> m_CategoryToItem;
};
