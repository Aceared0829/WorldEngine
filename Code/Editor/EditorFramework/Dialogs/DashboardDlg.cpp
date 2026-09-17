#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

WQtDashboardDlg::WQtDashboardDlg(QWidget* pParent, DashboardTab activeTab)
  : WQtDialog(pParent)
{
  setupUi(this);

  TabArea->tabBar()->hide();

  ProjectsTab->setFlat(true);
  SamplesTab->setFlat(true);
  DocumentationTab->setFlat(true);

  if (WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>())
  {
    LoadLastProject->setChecked(pPreferences->m_bLoadLastProjectAtStartup);
  }

  {
    SamplesList->setResizeMode(QListView::ResizeMode::Adjust);
    SamplesList->setIconSize(QSize(220, 220));
    SamplesList->setItemAlignment(Qt::AlignHCenter | Qt::AlignBottom);
  }

  FillRecentProjectsList();
  FillSampleProjectsList();

  ProjectsList->installEventFilter(this);

  if (ProjectsList->rowCount() > 0)
  {
    ProjectsList->setFocus();
    ProjectsList->clearSelection();
    ProjectsList->selectRow(0);
  }
  else
  {
    if (activeTab == DashboardTab::Projects)
    {
      activeTab = DashboardTab::Samples;
    }
  }

  SetActiveTab(activeTab);
}

void WQtDashboardDlg::SetActiveTab(DashboardTab activeTab)
{
  TabArea->setCurrentIndex((int)activeTab);

  ProjectsTab->setChecked(activeTab == DashboardTab::Projects);
  SamplesTab->setChecked(activeTab == DashboardTab::Samples);
  DocumentationTab->setChecked(activeTab == DashboardTab::Documentation);
}

void WQtDashboardDlg::FillRecentProjectsList()
{
  const auto& list = WQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList();

  ProjectsList->clear();
  ProjectsList->setColumnCount(2);
  ProjectsList->setRowCount(list.GetCount());

  WStringBuilder tmp;

  for (WUInt32 r = 0; r < list.GetCount(); ++r)
  {
    const auto& path = list[r];

    QTableWidgetItem* pItemProjectName = new QTableWidgetItem();
    QTableWidgetItem* pItemProjectPath = new QTableWidgetItem();

    pItemProjectName->setData(Qt::UserRole, path.m_File.GetData());

    tmp = path.m_File;
    tmp.MakeCleanPath();
    tmp.PathParentDirectory(1); // remove '/WProject'
    tmp.Trim("/");

    pItemProjectPath->setText(tmp.GetData());
    pItemProjectName->setText(tmp.GetFileName().GetStartPointer());

    ProjectsList->setItem(r, 0, pItemProjectName);
    ProjectsList->setItem(r, 1, pItemProjectPath);
  }

  ProjectsList->resizeColumnToContents(0);
}

void WQtDashboardDlg::FillSampleProjectsList()
{
  WTempHybridArray<WString, 32> samples;
  FindSampleProjects(samples);

  SamplesList->clear();

  WStringBuilder tmp, iconPath;

  WStringBuilder samplesIcon = WApplicationServices::GetSingleton()->GetSampleProjectsFolder();
  samplesIcon.AppendPath("Thumbnail.jpg");

  QIcon fallbackIcon;

  if (WOSFile::ExistsFile(samplesIcon))
  {
    fallbackIcon.addFile(samplesIcon.GetData());
  }

  for (const WString& path : samples)
  {
    tmp = path;
    const bool bIsLocal = tmp.TrimWordEnd("/WProject");
    const bool bIsRemote = tmp.TrimWordEnd("/WRemoteProject");

    QIcon projectIcon;

    iconPath = tmp;
    iconPath.AppendPath("Thumbnail.jpg");

    if (WOSFile::ExistsFile(iconPath))
    {
      projectIcon.addFile(iconPath.GetData());
    }
    else
    {
      projectIcon = fallbackIcon;
    }

    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setText(tmp.GetFileName().GetStartPointer());
    pItem->setData(Qt::UserRole, path.GetData());

    pItem->setIcon(projectIcon);

    SamplesList->addItem(pItem);
  }
}

void WQtDashboardDlg::FindSampleProjects(WDynamicArray<WString>& out_Projects)
{
  out_Projects.Clear();

  const WString& sSampleProjects = WApplicationServices::GetSingleton()->GetSampleProjectsFolder();

  WFileSystemIterator fsIt;
  fsIt.StartSearch(sSampleProjects, WFileSystemIteratorFlags::ReportFoldersRecursive);

  WStringBuilder path;

  while (fsIt.IsValid())
  {
    fsIt.GetStats().GetFullPath(path);
    path.AppendPath("WProject");

    if (WOSFile::ExistsFile(path))
    {
      out_Projects.PushBack(path);

      // no need to go deeper
      fsIt.SkipFolder();
    }
    else
    {
      fsIt.GetStats().GetFullPath(path);
      path.AppendPath("WRemoteProject");

      if (WOSFile::ExistsFile(path))
      {
        out_Projects.PushBack(path);

        // no need to go deeper
        fsIt.SkipFolder();
      }
      else
      {
        fsIt.Next();
      }
    }
  }
}

void WQtDashboardDlg::on_ProjectsTab_clicked()
{
  SetActiveTab(DashboardTab::Projects);
}

void WQtDashboardDlg::on_SamplesTab_clicked()
{
  SetActiveTab(DashboardTab::Samples);
}

void WQtDashboardDlg::on_DocumentationTab_clicked()
{
  SetActiveTab(DashboardTab::Documentation);
}

void WQtDashboardDlg::on_NewProject_clicked()
{
  if (WQtEditorApp::GetSingleton()->GuiCreateProject(true))
  {
    accept();
  }
}

void WQtDashboardDlg::on_BrowseProject_clicked()
{
  if (WQtEditorApp::GetSingleton()->GuiOpenProject(true))
  {
    accept();
  }
}

void WQtDashboardDlg::on_ProjectsList_cellDoubleClicked(int row, int column)
{
  if (row < 0 || row >= ProjectsList->rowCount())
    return;

  QTableWidgetItem* pItem = ProjectsList->item(row, 0);

  QString sPath = pItem->data(Qt::UserRole).toString();

  if (WQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void WQtDashboardDlg::on_OpenProject_clicked()
{
  on_ProjectsList_cellDoubleClicked(ProjectsList->currentRow(), 0);
}

void WQtDashboardDlg::on_OpenSample_clicked()
{
  on_SamplesList_itemDoubleClicked(SamplesList->currentItem());
}

void WQtDashboardDlg::on_LoadLastProject_stateChanged(int)
{
  if (WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>())
  {
    pPreferences->m_bLoadLastProjectAtStartup = LoadLastProject->isChecked();
  }
}

void WQtDashboardDlg::on_SamplesList_itemDoubleClicked(QListWidgetItem* pItem)
{
  if (pItem == nullptr)
    return;

  QString sPath = pItem->data(Qt::UserRole).toString().toUtf8().data();

  if (WQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void WQtDashboardDlg::on_OpenDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://ezengine.net"));
}

void WQtDashboardDlg::on_OpenApiDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://ezengine.github.io/api-docs/"));
}

void WQtDashboardDlg::on_GitHubDiscussions_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/ezEngine/ezEngine/discussions"));
}

void WQtDashboardDlg::on_ReportProblem_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/ezEngine/ezEngine/issues"));
}

void WQtDashboardDlg::on_OpenDiscord_clicked()
{
  QDesktopServices::openUrl(QUrl("https://discord.gg/rfJewc5khZ"));
}

void WQtDashboardDlg::on_OpenTwitter_clicked()
{
  QDesktopServices::openUrl(QUrl("https://twitter.com/ezEngineProject"));
}

void WQtDashboardDlg::on_OpenBsky_clicked()
{
  QDesktopServices::openUrl(QUrl("https://bsky.app/profile/ezengine.bsky.social"));
}

bool WQtDashboardDlg::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::KeyPress)
  {
    QKeyEvent* key = static_cast<QKeyEvent*>(e);

    if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return))
    {
      on_OpenProject_clicked();
      return true;
    }
  }

  return QObject::eventFilter(obj, e);
}
