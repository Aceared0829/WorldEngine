#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_QtWaitForOperationDlg.h>

class QWinTaskbarProgress;
class QWinTaskbarButton;

class W_GUIFOUNDATION_DLL WQtWaitForOperationDlg : public WQtDialog, public Ui_QtWaitForOperationDlg
{
  Q_OBJECT

public:
  WQtWaitForOperationDlg(QWidget* pParent);
  ~WQtWaitForOperationDlg();

  WDelegate<bool()> m_OnIdle;

private Q_SLOTS:
  void on_ButtonCancel_clicked();
  void onIdle();
};
