#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

WQtDialog::WQtDialog(QWidget* pParent)
  : QDialog(pParent)
{
}

int WQtDialog::exec()
{
  if (WQtUiServices::IsUnattended())
  {
    // The Qt class name rather than a hand written string, so that no dialog can report a stale name, and
    // adding a dialog needs no further work. It is also what a caller would search the code base for.
    WQtUiServices::ReportSuppressedDialog(metaObject()->className());
    return QDialog::Rejected;
  }

  return QDialog::exec();
}
