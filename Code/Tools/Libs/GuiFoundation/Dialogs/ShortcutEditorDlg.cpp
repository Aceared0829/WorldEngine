#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>
#include <QKeySequenceEdit>
#include <QTableWidget>
#include <QTreeWidget>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

WQtShortcutEditorDlg::WQtShortcutEditorDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  W_VERIFY(connect(Shortcuts, SIGNAL(itemSelectionChanged()), this, SLOT(SlotSelectionChanged())) != nullptr, "signal/slot connection failed");

  m_iSelectedAction = -1;
  KeyEditor->setEnabled(false);

  WMap<WString, WMap<WString, WInt32>> SortedItems;

  {
    auto itActions = WActionManager::GetActionIterator();

    while (itActions.IsValid())
    {
      if (itActions.Value()->m_Type == WActionType::Action)
      {
        SortedItems[itActions.Value()->m_sCategoryPath][itActions.Value()->m_sActionName] = m_ActionDescs.GetCount();
        m_ActionDescs.PushBack(itActions.Value());
      }

      itActions.Next();
    }
  }

  {
    WQtScopedBlockSignals bs(Shortcuts);
    WQtScopedUpdatesDisabled ud(Shortcuts);

    Shortcuts->setAlternatingRowColors(true);
    Shortcuts->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
    Shortcuts->setExpandsOnDoubleClick(true);

    WStringBuilder sTemp;

    for (auto it = SortedItems.GetIterator(); it.IsValid(); ++it)
    {
      auto pParent = new QTreeWidgetItem();
      pParent->setData(0, Qt::DisplayRole, it.Key().GetData());
      Shortcuts->addTopLevelItem(pParent);

      pParent->setExpanded(true);
      pParent->setFirstColumnSpanned(true);
      pParent->setFlags(Qt::ItemFlag::ItemIsEnabled);

      QFont font = pParent->font(0);
      font.setBold(true);

      pParent->setFont(0, font);

      for (auto it2 : it.Value())
      {
        const auto& item = m_ActionDescs[it2.Value()];
        auto pItem = new QTreeWidgetItem(pParent);

        /// \todo Instead of removing &, replace it by underlined text (requires formatted text output)
        sTemp = WTranslate(item->m_sActionName);
        sTemp.ReplaceAll("&", "");

        pItem->setData(0, Qt::UserRole, it2.Value());
        pItem->setData(0, Qt::DisplayRole, item->m_sActionName.GetData());
        pItem->setData(1, Qt::DisplayRole, sTemp.GetData());
        pItem->setData(2, Qt::DisplayRole, item->m_sShortcut.GetData());
        pItem->setData(3, Qt::DisplayRole, WMakeQString(WTranslateTooltip(item->m_sActionName)));

        if (item->m_sShortcut == item->m_sDefaultShortcut)
          pItem->setBackground(2, QBrush());
        else
          pItem->setBackground(2, Qt::darkYellow);

        sTemp.Set("Default: ", item->m_sDefaultShortcut.IsEmpty() ? "<none>" : item->m_sDefaultShortcut.GetData());

        pItem->setToolTip(2, QString::fromUtf8(sTemp.GetData()));
      }
    }

    Shortcuts->resizeColumnToContents(0);
    Shortcuts->resizeColumnToContents(2);
  }

  ButtonAssign->setEnabled(false);
  ButtonRemove->setEnabled(false);
  ButtonReset->setEnabled(false);
}

WQtShortcutEditorDlg::~WQtShortcutEditorDlg()
{
  WActionManager::SaveShortcutAssignment();
}

void WQtShortcutEditorDlg::UpdateTable()
{
  for (WInt32 iTop = 0; iTop < Shortcuts->topLevelItemCount(); ++iTop)
  {
    auto pTopItem = Shortcuts->topLevelItem(iTop);

    for (WInt32 iChild = 0; iChild < pTopItem->childCount(); ++iChild)
    {
      auto pChild = pTopItem->child(iChild);

      WInt32 idx = pChild->data(0, Qt::UserRole).toInt();

      const auto& item = m_ActionDescs[idx];

      pChild->setData(2, Qt::DisplayRole, QVariant(item->m_sShortcut.GetData()));

      if (item->m_sShortcut == item->m_sDefaultShortcut)
        pChild->setBackground(2, QBrush());
      else
        pChild->setBackground(2, Qt::darkYellow);
    }
  }

  if (m_iSelectedAction >= 0)
  {
    ButtonRemove->setEnabled(!m_ActionDescs[m_iSelectedAction]->m_sShortcut.IsEmpty());
    ButtonReset->setEnabled(m_ActionDescs[m_iSelectedAction]->m_sShortcut != m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut);
  }

  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  ButtonAssign->setEnabled(!sText.isEmpty());
}

void WQtShortcutEditorDlg::SlotSelectionChanged()
{
  auto selection = Shortcuts->selectedItems();
  if (selection.size() == 1)
  {
    m_iSelectedAction = selection[0]->data(0, Qt::UserRole).toInt();
    KeyEditor->clear();
    KeyEditor->setEnabled(true);
    ButtonAssign->setEnabled(false);
    ButtonRemove->setEnabled(!m_ActionDescs[m_iSelectedAction]->m_sShortcut.IsEmpty());
    ButtonReset->setEnabled(m_ActionDescs[m_iSelectedAction]->m_sShortcut != m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut);
  }
  else
  {
    m_iSelectedAction = -1;
    KeyEditor->clear();
    KeyEditor->setEnabled(false);
    ButtonAssign->setEnabled(false);
    ButtonRemove->setEnabled(false);
    ButtonReset->setEnabled(false);
  }
}

void WQtShortcutEditorDlg::on_KeyEditor_editingFinished()
{
  UpdateKeyEdit();
}

void WQtShortcutEditorDlg::UpdateKeyEdit()
{
  if (m_iSelectedAction < 0)
    return;

  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  ButtonAssign->setEnabled(!sText.isEmpty());
}

void WQtShortcutEditorDlg::on_KeyEditor_keySequenceChanged(const QKeySequence& keySequence)
{
  UpdateKeyEdit();
}

void WQtShortcutEditorDlg::on_ButtonAssign_clicked()
{
  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  KeyEditor->clear();

  m_ActionDescs[m_iSelectedAction]->m_sShortcut = sText.toUtf8().data();
  m_ActionDescs[m_iSelectedAction]->UpdateExistingActions();

  UpdateTable();
}

void WQtShortcutEditorDlg::on_ButtonRemove_clicked()
{
  KeyEditor->clear();

  m_ActionDescs[m_iSelectedAction]->m_sShortcut.Clear();
  m_ActionDescs[m_iSelectedAction]->UpdateExistingActions();

  UpdateTable();
}

void WQtShortcutEditorDlg::on_ButtonReset_clicked()
{
  KeyEditor->clear();

  m_ActionDescs[m_iSelectedAction]->m_sShortcut = m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut;
  m_ActionDescs[m_iSelectedAction]->UpdateExistingActions();

  UpdateTable();
}

void WQtShortcutEditorDlg::on_Search_textChanged(const QString& sText)
{
  WSearchPatternFilter filter;
  filter.SetSearchText(sText.toUtf8().data());

  WQtScopedUpdatesDisabled ud(Shortcuts);

  for (WInt32 iTop = 0; iTop < Shortcuts->topLevelItemCount(); ++iTop)
  {
    auto pTopItem = Shortcuts->topLevelItem(iTop);
    bool bAnyCategoryChildVisible = false;

    for (WInt32 iChild = 0; iChild < pTopItem->childCount(); ++iChild)
    {
      auto pChild = pTopItem->child(iChild);
      const WString sActionName = pChild->data(0, Qt::DisplayRole).toString().toUtf8().data();
      const WString sShortcut = pChild->data(2, Qt::DisplayRole).toString().toUtf8().data();

      const bool bVisible = filter.PassesFilters(sActionName) || filter.PassesFilters(sShortcut);
      pChild->setHidden(!bVisible);

      if (bVisible)
      {
        bAnyCategoryChildVisible = true;
      }
    }

    pTopItem->setHidden(!bAnyCategoryChildVisible);
  }
}
