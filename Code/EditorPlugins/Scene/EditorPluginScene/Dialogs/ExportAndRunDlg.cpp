#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>

bool WQtExportAndRunDlg::s_bTransformAll = true;
bool WQtExportAndRunDlg::s_bUpdateThumbnail = false;
bool WQtExportAndRunDlg::s_bCompileCpp = true;

static int s_iLastPlayerApp = 0;

WQtExportAndRunDlg::WQtExportAndRunDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

#if W_ENABLED(W_PLATFORM_WINDOWS)
  ToolCombo->addItem("WPlayer", "WPlayer.exe");
#else
  ToolCombo->addItem("WPlayer", "WPlayer");
#endif

  WProjectPreferencesUser* pPref = WPreferences::QueryPreferences<WProjectPreferencesUser>();

  for (const auto& app : pPref->m_PlayerApps)
  {
    WStringBuilder name = WPathUtils::GetFileName(app);

    ToolCombo->addItem(name.GetData(), QString::fromUtf8(app.GetData()));
  }

  ToolCombo->setCurrentIndex(s_iLastPlayerApp);

  m_CppSettings.Load().IgnoreResult();
}

void WQtExportAndRunDlg::PullFromUI()
{
  s_bTransformAll = TransformAll->isChecked();
  s_bUpdateThumbnail = UpdateThumbnail->isChecked();
  s_iLastPlayerApp = ToolCombo->currentIndex();
  s_bCompileCpp = CompileCpp->isChecked();

  WProjectPreferencesUser* pPref = WPreferences::QueryPreferences<WProjectPreferencesUser>();
  pPref->m_PlayerApps.Clear();

  for (int i = 1; i < ToolCombo->count(); ++i)
  {
    WStringBuilder path = ToolCombo->itemData(i).toString().toUtf8().data();
    path.MakeCleanPath();

    pPref->m_PlayerApps.PushBack(path);
  }
}

void WQtExportAndRunDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdateThumbnail->setVisible(m_bShowThumbnailCheckbox);
  TransformAll->setChecked(s_bTransformAll);
  UpdateThumbnail->setChecked(s_bUpdateThumbnail);
  PlayerCmdLine->setPlainText(m_sCmdLine.GetData());

  if (!WCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(s_bCompileCpp);
  }
}

void WQtExportAndRunDlg::on_ExportOnly_clicked()
{
  PullFromUI();
  m_bRunAfterExport = false;
  accept();
}

void WQtExportAndRunDlg::on_ExportAndRun_clicked()
{
  PullFromUI();
  m_bRunAfterExport = true;
  m_sApplication = ToolCombo->currentData().toString().toUtf8().data();
  accept();
}

void WQtExportAndRunDlg::on_AddToolButton_clicked()
{
  WStringBuilder appDir = WOSFile::GetApplicationDirectory();
  appDir.MakeCleanPath();
  static QString sLastPath = appDir.GetData();

#if W_ENABLED(W_PLATFORM_WINDOWS)
  const QString sFile = QFileDialog::getOpenFileName(this, "Select Program", sLastPath, "Application (*.exe)", nullptr, QFileDialog::Option::DontResolveSymlinks);
#else
  const QString sFile = QFileDialog::getOpenFileName(this, "Select Program", sLastPath, "Executable (*)", nullptr, QFileDialog::Option::DontResolveSymlinks);
#endif


  if (sFile.isEmpty())
    return;

  sLastPath = sFile;

  WStringBuilder path = sFile.toUtf8().data();
  path.MakeCleanPath();
  path.TrimWordStart(appDir);
  path.Trim("/", "");

  WStringBuilder tmp;
  ToolCombo->addItem(QString::fromUtf8(path.GetFileName().GetData(tmp)), QString::fromUtf8(path.GetData()));
  ToolCombo->setCurrentIndex(ToolCombo->count() - 1);
}

void WQtExportAndRunDlg::on_RemoveToolButton_clicked()
{
  ToolCombo->removeItem(ToolCombo->currentIndex());
}

void WQtExportAndRunDlg::on_ToolCombo_currentIndexChanged(int idx)
{
  RemoveToolButton->setEnabled(idx != 0);
}
