#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_InputConfigDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class QTreeWidgetItem;

class W_EDITORFRAMEWORK_DLL WQtInputConfigDlg : public WQtDialog, public Ui_InputConfigDialog
{
public:
  Q_OBJECT

public:
  WQtInputConfigDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonNewInputSet_clicked();
  void on_ButtonNewAction_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeActions_itemSelectionChanged();

private:
  void LoadActions();
  void SaveActions();
  void FillList();
  void GetActionsFromList();

  QTreeWidgetItem* CreateActionItem(QTreeWidgetItem* pParentItem, const WGameAppInputConfig& action);

  WMap<WString, QTreeWidgetItem*> m_InputSetToItem;
  WHybridArray<WGameAppInputConfig, 32> m_Actions;
  WDynamicArray<WString> m_AllInputSlots;
};
