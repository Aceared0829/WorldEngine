#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

void WQtEditorApp::GuiOpenDashboard()
{
  if (WQtUiServices::SuppressModalWindow("WQtDashboardDlg"))
    return;

  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDashboard", Qt::ConnectionType::QueuedConnection);
}

void WQtEditorApp::GuiOpenDocsAndCommunity()
{
  if (WQtUiServices::SuppressModalWindow("WQtDashboardDlg (documentation)"))
    return;

  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDocsAndCommunity", Qt::ConnectionType::QueuedConnection);
}

bool WQtEditorApp::GuiCreateProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(true);
  }
  else
  {
    if (WQtUiServices::SuppressModalWindow("WQtCreateProjectDlg"))
      return false;

    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, true));
    return true;
  }
}

bool WQtEditorApp::GuiOpenProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(false);
  }
  else
  {
    if (WQtUiServices::SuppressModalWindow("Open Project (file picker)"))
      return false;

    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, false));
    return true;
  }
}

void WQtEditorApp::SlotQueuedGuiOpenDashboard()
{
  WQtDashboardDlg dlg(nullptr, WQtDashboardDlg::DashboardTab::Projects);
  dlg.exec();
}

void WQtEditorApp::SlotQueuedGuiOpenDocsAndCommunity()
{
  WQtDashboardDlg dlg(nullptr, WQtDashboardDlg::DashboardTab::Documentation);
  dlg.exec();
}

void WQtEditorApp::SlotQueuedGuiCreateOrOpenProject(bool bCreate)
{
  GuiCreateOrOpenProject(bCreate);
}

bool WQtEditorApp::GuiCreateOrOpenProject(bool bCreate)
{
  const QString sDir = QString::fromUtf8(m_sLastProjectFolder.GetData());
  WStringBuilder sFile;

  const char* szFilter = "WProject (WProject)";

  if (bCreate)
  {
    WQtCreateProjectDlg dlg(nullptr);
    if (dlg.exec() == QDialog::Rejected)
      return false;

    sFile = dlg.GetFullTargetPath();
  }
  else
  {
    // native window, so not covered by WQtDialog - see CreateOrOpenProject() for opening a known path
    if (WQtUiServices::SuppressModalWindow("Open Project (file picker)"))
      return false;

    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Project"), sDir, QLatin1String(szFilter), nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();
  }

  if (sFile.IsEmpty())
    return false;

  if (bCreate)
  {
    sFile.AppendPath("WProject");
  }

  m_sLastProjectFolder = WPathUtils::GetFileDirectory(sFile);

  return CreateOrOpenProject(bCreate, sFile).Succeeded();
}
