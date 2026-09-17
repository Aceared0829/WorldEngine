#pragma once

#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ModifiedDocumentsDlg.h>
#include <ToolsFoundation/Document/Document.h>

class W_GUIFOUNDATION_DLL WQtModifiedDocumentsDlg : public WQtDialog, public Ui_DocumentList
{
public:
  Q_OBJECT

public:
  WQtModifiedDocumentsDlg(QWidget* pParent, const WHybridArray<WDocument*, 32>& modifiedDocs);


private Q_SLOTS:
  void on_ButtonSaveSelected_clicked();
  void on_ButtonDontSave_clicked();
  void SlotSaveDocument();
  void SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

private:
  WResult SaveDocument(WDocument* pDoc);

  WHybridArray<WDocument*, 32> m_ModifiedDocs;
};
