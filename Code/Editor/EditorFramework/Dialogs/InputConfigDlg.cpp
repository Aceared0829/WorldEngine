#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

void UpdateInputDynamicEnumValues()
{
  WTempHybridArray<WGameAppInputConfig, 32> Actions;

  WStringBuilder sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

  WFileReader file;
  if (file.Open(sPath).Failed())
    return;

  WGameAppInputConfig::ReadFromDDL(file, Actions);

  auto& dynEnum = WDynamicStringEnum::CreateDynamicEnum("InputSet");

  for (const auto& a : Actions)
  {
    dynEnum.AddValidValue(a.m_sInputSet, true);
  }
}

WQtInputConfigDlg::WQtInputConfigDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  LoadActions();

  WQtEditorApp::GetSingleton()->GetKnownInputSlots(m_AllInputSlots);

  // make sure existing slots are always in the list
  // to prevent losing data when some plugin is not loaded
  {
    for (const auto& action : m_Actions)
    {
      for (int i = 0; i < 3; ++i)
      {
        if (m_AllInputSlots.IndexOf(action.m_sInputSlotTrigger[i]) == WInvalidIndex)
          m_AllInputSlots.PushBack(action.m_sInputSlotTrigger[i]);
      }
    }
  }

  FillList();

  on_TreeActions_itemSelectionChanged();
}

void WQtInputConfigDlg::on_ButtonNewInputSet_clicked()
{
  QString sResult = QInputDialog::getText(this, "Input Set Name", "Name:");

  if (sResult.isEmpty())
    return;

  TreeActions->clearSelection();

  const WString sName = sResult.toUtf8().data();

  if (m_InputSetToItem.Find(sName).IsValid())
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("An Input Set with this name already exists.");
  }
  else
  {
    auto* pItem = new QTreeWidgetItem(TreeActions);
    pItem->setText(0, sResult);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Input.svg"));

    m_InputSetToItem[sName] = pItem;
  }

  m_InputSetToItem[sName]->setSelected(true);
}

void WQtInputConfigDlg::on_ButtonNewAction_clicked()
{
  if (TreeActions->selectedItems().isEmpty())
    return;

  auto pItem = TreeActions->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeActions->indexOfTopLevelItem(pItem) < 0)
    pItem = pItem->parent();

  WGameAppInputConfig action;
  auto pNewItem = CreateActionItem(pItem, action);
  pItem->setExpanded(true);

  TreeActions->clearSelection();
  pNewItem->setSelected(true);
  TreeActions->editItem(pNewItem);
}

void WQtInputConfigDlg::on_ButtonRemove_clicked()
{
  if (TreeActions->selectedItems().isEmpty())
    return;

  auto pItem = TreeActions->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeActions->indexOfTopLevelItem(pItem) >= 0)
  {
    if (WQtUiServices::GetSingleton()->MessageBoxQuestion("Do you really want to remove the entire Input Set?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
      return;

    m_InputSetToItem.Remove(pItem->text(0).toUtf8().data());
  }
  else
  {
    if (WQtUiServices::GetSingleton()->MessageBoxQuestion("Do you really want to remove this action?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
      return;
  }

  delete pItem;
}

void WQtInputConfigDlg::on_ButtonOk_clicked()
{
  GetActionsFromList();
  SaveActions();
  UpdateInputDynamicEnumValues();
  accept();
}

void WQtInputConfigDlg::on_ButtonCancel_clicked()
{
  reject();
}

void WQtInputConfigDlg::on_ButtonReset_clicked()
{
  LoadActions();
  FillList();
  on_TreeActions_itemSelectionChanged();
}

void WQtInputConfigDlg::on_TreeActions_itemSelectionChanged()
{
  const bool hasSelection = !TreeActions->selectedItems().isEmpty();

  ButtonRemove->setEnabled(hasSelection);
  ButtonNewAction->setEnabled(hasSelection);
}

void WQtInputConfigDlg::LoadActions()
{
  m_Actions.Clear();

  WStringBuilder sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

  WFileReader file;
  if (file.Open(sPath).Failed())
    return;

  WGameAppInputConfig::ReadFromDDL(file, m_Actions);
}

void WQtInputConfigDlg::SaveActions()
{
  WStringBuilder sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

  WDeferredFileWriter file;
  file.SetOutput(sPath);

  WGameAppInputConfig::WriteToDDL(file, m_Actions);

  if (file.Close().Failed())
    WLog::Error("Failed to save '{0}'.", sPath);
}

void WQtInputConfigDlg::FillList()
{
  WQtScopedBlockSignals bs(TreeActions);
  WQtScopedUpdatesDisabled bu(TreeActions);

  m_InputSetToItem.Clear();
  TreeActions->clear();

  WSet<WString> InputSets;

  for (const auto& action : m_Actions)
  {
    InputSets.Insert(action.m_sInputSet);
  }

  for (auto it = InputSets.GetIterator(); it.IsValid(); ++it)
  {
    auto* pItem = new QTreeWidgetItem(TreeActions);
    pItem->setText(0, it.Key().GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Input.svg"));

    m_InputSetToItem[it.Key()] = pItem;
  }

  for (const auto& action : m_Actions)
  {
    QTreeWidgetItem* pParentItem = m_InputSetToItem[action.m_sInputSet];

    CreateActionItem(pParentItem, action);


    pParentItem->setExpanded(true);
  }

  TreeActions->resizeColumnToContents(0);
  TreeActions->resizeColumnToContents(1);
  TreeActions->resizeColumnToContents(2);
  TreeActions->resizeColumnToContents(3);
  TreeActions->resizeColumnToContents(4);
  TreeActions->resizeColumnToContents(5);
  TreeActions->resizeColumnToContents(6);
  TreeActions->resizeColumnToContents(7);
}

void WQtInputConfigDlg::GetActionsFromList()
{
  m_Actions.Clear();

  for (int sets = 0; sets < TreeActions->topLevelItemCount(); ++sets)
  {
    const auto* pSetItem = TreeActions->topLevelItem(sets);
    const WString sSetName = pSetItem->text(0).toUtf8().data();

    for (int children = 0; children < pSetItem->childCount(); ++children)
    {
      WGameAppInputConfig& cfg = m_Actions.ExpandAndGetRef();
      cfg.m_sInputSet = sSetName;

      auto* pActionItem = pSetItem->child(children);

      cfg.m_sInputAction = pActionItem->text(0).toUtf8().data();
      cfg.m_bApplyTimeScaling = qobject_cast<QCheckBox*>(TreeActions->itemWidget(pActionItem, 1))->isChecked();

      for (int i = 0; i < 3; ++i)
      {
        cfg.m_sInputSlotTrigger[i] = qobject_cast<QComboBox*>(TreeActions->itemWidget(pActionItem, 2 + i * 2))->currentText().toUtf8().data();
        cfg.m_fInputSlotScale[i] = qobject_cast<QDoubleSpinBox*>(TreeActions->itemWidget(pActionItem, 3 + i * 2))->value();
      }
    }
  }
}

QTreeWidgetItem* WQtInputConfigDlg::CreateActionItem(QTreeWidgetItem* pParentItem, const WGameAppInputConfig& action)
{
  auto* pItem = new QTreeWidgetItem(pParentItem);
  pItem->setText(0, action.m_sInputAction.GetData());
  pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEditable);

  QCheckBox* pTimeScale = new QCheckBox(TreeActions);
  pTimeScale->setChecked(action.m_bApplyTimeScaling);
  TreeActions->setItemWidget(pItem, 1, pTimeScale);

  for (int i = 0; i < 3; ++i)
  {
    QDoubleSpinBox* spin = new QDoubleSpinBox(TreeActions);
    spin->setDecimals(3);
    spin->setMinimum(0.0);
    spin->setMaximum(100.0);
    spin->setSingleStep(0.01);
    spin->setValue(action.m_fInputSlotScale[i]);

    TreeActions->setItemWidget(pItem, 3 + 2 * i, spin);

    QComboBox* combo = new QComboBox(TreeActions);
    combo->setEditable(true);

    QCompleter* completer = new QCompleter(this);
    completer->setModel(combo->model());
    combo->setCompleter(completer);
    combo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    combo->setInsertPolicy(QComboBox::InsertAtBottom);
    combo->setMaxVisibleItems(15);

    for (WUInt32 it = 0; it < m_AllInputSlots.GetCount(); ++it)
    {
      combo->addItem(m_AllInputSlots[it].GetData());
    }

    int index = combo->findText(action.m_sInputSlotTrigger[i].GetData());

    if (index > 0)
      combo->setCurrentIndex(index);
    else
      combo->setCurrentIndex(0);

    TreeActions->setItemWidget(pItem, 2 + 2 * i, combo);
  }

  return pItem;
}
