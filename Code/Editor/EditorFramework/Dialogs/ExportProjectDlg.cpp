#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFileDialog>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>


bool WQtExportProjectDlg::s_bTransformAll = true;
bool WQtExportProjectDlg::s_bCreateLaunchScripts = true;
bool WQtExportProjectDlg::s_bOpenOutputFolder = true;

WQtExportProjectDlg::WQtExportProjectDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  WProjectPreferencesUser* pPref = WPreferences::QueryPreferences<WProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());
}

void WQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);
  CreateLaunchScripts->setChecked(s_bCreateLaunchScripts);
  OpenOutputFolder->setChecked(s_bOpenOutputFolder);

  if (!WCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(true);
  }
}

void WQtExportProjectDlg::on_BrowseDestination_clicked()
{
  QString sPath = QFileDialog::getExistingDirectory(this, QLatin1String("Select output directory"), Destination->text());

  if (!sPath.isEmpty())
  {
    Destination->setText(sPath);
    WProjectPreferencesUser* pPref = WPreferences::QueryPreferences<WProjectPreferencesUser>();
    pPref->m_sExportFolder = sPath.toUtf8().data();
  }
}

void WQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  // TODO:
  // filter out unused runtime/game plugins
  // select asset profile for export
  // copy inputs into resource: RML files

  s_bTransformAll = TransformAll->isChecked();
  s_bCreateLaunchScripts = CreateLaunchScripts->isChecked();
  s_bOpenOutputFolder = OpenOutputFolder->isChecked();

  WProjectExportOptions options;
  options.m_bCompileCppPlugin = CompileCpp->isChecked();
  options.m_bTransformAssets = s_bTransformAll;
  options.m_bCreateLaunchScripts = s_bCreateLaunchScripts;

  const WString sDstFolder = Destination->text().toUtf8().data();

  WStringBuilder sLog;
  const WStatus res = WProjectExport::ExportProjectComplete(sDstFolder, options, &sLog);

  ExportLog->setPlainText(sLog.GetData());

  if (res.Failed())
  {
    WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Project export failed. See log for details.");
  }
  else
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("Project export successful.", "project-export-success");

    if (s_bOpenOutputFolder)
    {
      WQtUiServices::GetSingleton()->OpenInExplorer(sDstFolder, false);
    }
  }
}
