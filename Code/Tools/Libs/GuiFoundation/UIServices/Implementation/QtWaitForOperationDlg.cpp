#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <QTimer>

WQtWaitForOperationDlg::WQtWaitForOperationDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  QTimer::singleShot(10, this, &WQtWaitForOperationDlg::onIdle);
}

WQtWaitForOperationDlg::~WQtWaitForOperationDlg() = default;

void WQtWaitForOperationDlg::on_ButtonCancel_clicked()
{
  reject();
}

void WQtWaitForOperationDlg::onIdle()
{
  if (m_OnIdle())
  {
    QTimer::singleShot(10, this, &WQtWaitForOperationDlg::onIdle);
  }
  else
  {
    accept();
  }
}
