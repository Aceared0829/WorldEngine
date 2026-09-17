#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

WQtLaunchFileserveDlg::WQtLaunchFileserveDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);
}

WQtLaunchFileserveDlg::~WQtLaunchFileserveDlg() = default;

void WQtLaunchFileserveDlg::showEvent(QShowEvent* event)
{
  WStringBuilder sCmdLine = WQtEditorApp::GetSingleton()->BuildFileserveCommandLine();
  EditFileserve->setPlainText(sCmdLine.GetData());

  QDialog::showEvent(event);
}

void WQtLaunchFileserveDlg::on_ButtonLaunch_clicked()
{
  WQtEditorApp::GetSingleton()->RunFileserve();
  accept();
}
