#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>

void WQtEditorApp::AddRestartRequiredReason(const char* szReason)
{
  if (!m_RestartRequiredReasons.Find(szReason).IsValid())
  {
    m_RestartRequiredReasons.Insert(szReason);
    UpdateGlobalStatusBarMessage();
  }

  WStringBuilder s;
  s.SetFormat("The editor process must be restarted.\nReason: '{0}'\n\nDo you want to restart now?", szReason);

  if (WQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
  {
    if (WToolsProject::CanCloseProject())
    {
      LaunchEditor(WToolsProject::GetSingleton()->GetProjectFile(), false);

      QApplication::closeAllWindows();
      return;
    }
  }
}

void WQtEditorApp::AddReloadProjectRequiredReason(const char* szReason)
{
  if (!m_ReloadProjectRequiredReasons.Find(szReason).IsValid())
  {
    m_ReloadProjectRequiredReasons.Insert(szReason);
    UpdateGlobalStatusBarMessage();
  }

  WStringBuilder s;
  s.SetFormat("The project must be reloaded.\nReason: '{0}'\n\nDo you want to reload it now?", szReason);

  if (WQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
  {
    if (WToolsProject::CanCloseProject())
    {
      WStringBuilder sProjectFile = WToolsProject::GetSingleton()->GetProjectFile();

      SlotQueuedCloseProject();
      OpenProject(sProjectFile, true).IgnoreResult();
    }
  }
}

void WQtEditorApp::UpdateGlobalStatusBarMessage()
{
  WStringBuilder sText;

  if (!m_RestartRequiredReasons.IsEmpty())
    sText.Append("Restart the editor to apply changes.   ");

  if (!m_ReloadProjectRequiredReasons.IsEmpty())
    sText.Append("Reload the project to apply changes.   ");

  WQtUiServices::ShowGlobalStatusBarMessage(sText);
}
