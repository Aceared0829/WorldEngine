#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectCreation.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

WQtCreateProjectDlg::WQtCreateProjectDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  Prev->setVisible(false);

  m_sTargetFolder = WApplicationServices::GetSingleton()->GetSampleProjectsFolder().GetData();

  WQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(WOSFile::GetApplicationDirectory());
  m_LocalPluginSet = WQtEditorApp::GetSingleton()->GetPluginBundles();
  m_LocalPluginSet.SetFromTemplate("General3D");

  Plugins->SetPluginSet(&m_LocalPluginSet);
  Plugins->SelectTemplate("General3D");

  ProjectTemplates->setResizeMode(QListView::ResizeMode::Adjust);
  ProjectTemplates->setIconSize(QSize(220, 220));
  ProjectTemplates->setItemAlignment(Qt::AlignHCenter | Qt::AlignBottom);

  UpdateUI();

  FillProjectTemplatesList();
}

WString WQtCreateProjectDlg::GetFullTargetPath() const
{
  WStringBuilder name = m_sTargetName;

  name.Trim();

  if (name.IsEmpty())
    return {};

  WStringBuilder path;
  path.SetPath(m_sTargetFolder, m_sTargetName);

  return path;
}

void WQtCreateProjectDlg::UpdateUI()
{
  WQtScopedBlockSignals _1(ProjectFolder);
  WQtScopedBlockSignals _2(ProjectName);

  ProjectFolder->setText(m_sTargetFolder.GetData());

  if (m_sProjectTemplate.IsEmpty())
    ChosenTemplate->setText("<none>");
  else
  {
    WStringBuilder tmp = m_sProjectTemplate;
    tmp.PathParentDirectory();
    tmp.TrimRight("/\\");

    ChosenTemplate->setText(WMakeQString(tmp.GetFileName()));
  }

  WString sFullPath = GetFullTargetPath();

  if (sFullPath.IsEmpty() || !sFullPath.IsAbsolutePath())
  {
    ResultPath->setText("<Choose a name and parent folder>");
    Next->setEnabled(false);
  }
  else if (WOSFile::ExistsDirectory(sFullPath))
  {
    // ResultPath->setColor(qRgb(255, 0, 0));
    ResultPath->setText("Directory already exists");
    Next->setEnabled(false);
  }
  else
  {
    // ResultPath->setColor(qRgb(0, 255, 0));
    ResultPath->setText(sFullPath.GetData());
    ResultPath2->setText(sFullPath.GetData());
    Next->setEnabled(!sFullPath.IsEmpty());
  }

  switch (m_State)
  {
    case State::Basics:
      StackedPages->setCurrentIndex(0);
      Prev->setVisible(false);
      Next->setText("Next >");
      break;

    case State::Templates:
      StackedPages->setCurrentIndex(2);
      Prev->setVisible(true);
      Next->setText("Next >");
      break;

    case State::Plugins:
      StackedPages->setCurrentIndex(1);
      Prev->setVisible(true);
      Next->setText("Next >");
      break;

    case State::Summary:
      StackedPages->setCurrentIndex(3);
      Prev->setVisible(true);
      Next->setText("Create");
      break;

    case State::Create:
      break;
  }
}

void WQtCreateProjectDlg::FillProjectTemplatesList()
{
  WDynamicArray<WString> templateNames;
  WProjectCreation::FindProjectTemplates(templateNames);

  WTempHybridArray<WString, 32> templates;
  for (const WString& sName : templateNames)
  {
    WStringBuilder sProjectFile;
    if (WProjectCreation::FindProjectTemplate(sName, sProjectFile).Succeeded())
    {
      templates.PushBack(sProjectFile);
    }
  }

  ProjectTemplates->clear();

  WStringBuilder tmp, iconPath;

  WStringBuilder samplesIcon = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
  samplesIcon.AppendPath("ProjectTemplates/Thumbnail.jpg");

  QIcon fallbackIcon;

  if (WOSFile::ExistsFile(samplesIcon))
  {
    fallbackIcon.addFile(samplesIcon.GetData());
  }

  {
    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setText("Blank Project");
    pItem->setData(Qt::UserRole, QString());

    pItem->setIcon(fallbackIcon);

    ProjectTemplates->addItem(pItem);

    ProjectTemplates->setCurrentItem(pItem);

    pItem->setSelected(true);
  }

  for (const WString& path : templates)
  {
    tmp = path;
    const bool bIsLocal = tmp.TrimWordEnd("/WProject");
    // const bool bIsRemote = tmp.TrimWordEnd("/WRemoteProject");

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

    ProjectTemplates->addItem(pItem);
  }
}

void WQtCreateProjectDlg::on_BrowseFolder_clicked()
{
  QString sFile = QFileDialog::getExistingDirectory(QApplication::activeWindow(), "Choose Folder", m_sTargetFolder.GetData(), QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  m_sTargetFolder = sFile.toUtf8().data();

  UpdateUI();
}

void WQtCreateProjectDlg::on_ProjectName_textChanged(QString text)
{
  m_sTargetName = ProjectName->text().toUtf8().data();

  UpdateUI();
}

void WQtCreateProjectDlg::on_Prev_clicked()
{
  switch (m_State)
  {
    case State::Templates:
      m_State = State::Basics;
      break;

    case State::Plugins:
      m_State = State::Templates;
      break;

    case State::Summary:
      if (m_sProjectTemplate.IsEmpty())
        m_State = State::Plugins;
      else
        m_State = State::Templates;
      break;

    default:
      break;
  }

  UpdateUI();
}

void WQtCreateProjectDlg::on_Next_clicked()
{
  switch (m_State)
  {
    case State::Basics:
      m_State = State::Templates;
      break;

    case State::Templates:
    {
      // no current item -> nothign selected -> blank project
      if (ProjectTemplates->currentItem())
      {
        m_sProjectTemplate = ProjectTemplates->currentItem()->data(Qt::UserRole).toString().toUtf8().data();
      }

      if (m_sProjectTemplate.IsEmpty())
        m_State = State::Plugins;
      else
        m_State = State::Summary;

      break;
    }

    case State::Plugins:
      Plugins->SyncStateToSet();
      m_State = State::Summary;
      break;

    case State::Summary:
      m_State = State::Create;
      break;
    case State::Create:
      break;
  }

  UpdateUI();

  if (m_State == State::Create)
  {
    const WStatus res = CreateProject();

    if (res.Failed())
    {
      WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Creating the project failed.");

      // back to the summary page, so that the user can change the name or folder and try again
      m_State = State::Summary;
      UpdateUI();
      return;
    }

    QDialog::accept();
  }
}

WStatus WQtCreateProjectDlg::CreateProject()
{
  WProjectCreationOptions options;
  options.m_sTargetDirectory = GetFullTargetPath();

  if (m_sProjectTemplate.IsEmpty())
  {
    // the plugin page wrote its state into m_LocalPluginSet, so pass that on as it is rather than
    // letting the creation apply a plugin template again
    return WProjectCreation::CreateProject(options, m_LocalPluginSet);
  }

  // m_sProjectTemplate is the path of the template's 'WProject' file, the creation takes the name
  WStringBuilder sTemplateName = m_sProjectTemplate;
  sTemplateName.PathParentDirectory();
  sTemplateName.TrimRight("/\\");
  options.m_sProjectTemplate = sTemplateName.GetFileName();

  return WProjectCreation::CreateProject(options, m_LocalPluginSet);
}
