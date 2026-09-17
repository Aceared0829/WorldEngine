#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ShortcutEditorDlg.h>

struct WActionDescriptor;

class W_GUIFOUNDATION_DLL WQtShortcutEditorDlg : public WQtDialog, public Ui_ShortcutEditor
{
public:
  Q_OBJECT

public:
  WQtShortcutEditorDlg(QWidget* pParent);
  ~WQtShortcutEditorDlg();

  void UpdateTable();

private Q_SLOTS:
  void SlotSelectionChanged();
  void on_KeyEditor_editingFinished();

  void UpdateKeyEdit();

  void on_KeyEditor_keySequenceChanged(const QKeySequence& keySequence);
  void on_ButtonAssign_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonReset_clicked();
  void on_Search_textChanged(const QString& sText);

private:
  WInt32 m_iSelectedAction;
  WHybridArray<WActionDescriptor*, 32> m_ActionDescs;
};
