#pragma once

#include <Foundation/Strings/String.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_PickDocumentObjectDlg.h>

class WDocumentObject;

class W_GUIFOUNDATION_DLL WQtPickDocumentObjectDlg : public WQtDialog, public Ui_PickDocumentObjectDlg
{
  Q_OBJECT

public:
  struct Element
  {
    const WDocumentObject* m_pObject;
    WString m_sDisplayName;
  };

  WQtPickDocumentObjectDlg(QWidget* pParent, const WArrayPtr<Element>& objects, const WUuid& currentObject);

  /// Stores the result that the user picked
  const WDocumentObject* m_pPickedObject = nullptr;

private Q_SLOTS:
  void on_ObjectTree_itemDoubleClicked(QTreeWidgetItem* pItem, int column);

private:
  void UpdateTable();

  WArrayPtr<Element> m_Objects;
  WUuid m_CurrentObject;
};
