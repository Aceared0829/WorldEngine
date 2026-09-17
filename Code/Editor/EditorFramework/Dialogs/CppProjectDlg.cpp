#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


WQtCppProjectDlg::WQtCppProjectDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  m_OldCppSettings.Load().IgnoreResult();
  m_CppSettings.Load().IgnoreResult();

  {
    WQtScopedBlockSignals _1(PluginName);
    PluginName->setPlaceholderText(WToolsProject::GetSingleton()->GetProjectName(true).GetData());
    PluginName->setText(m_CppSettings.m_sPluginName.GetData());
  }

  if (WStatus compilerTestResult = WCppProject::TestCompiler(); compilerTestResult.Failed())
  {
    // TODO: how do I color the ErrorText label in Red (or whatever error color is configured?)
    WStringBuilder fmt;
    fmt.SetFormat("<html><b>Error:</b> {}<br>Please go to preferences and configure the C & C++ compiler.", compilerTestResult.GetMessageString());

    ErrorText->setText(WMakeQString(fmt));
    GenerateSolution->setDisabled(true);
  }

  UpdateUI();
}

void WQtCppProjectDlg::on_OpenPluginLocation_clicked()
{
  WQtUiServices::OpenInExplorer(PluginLocation->text().toUtf8().data(), false);
}

void WQtCppProjectDlg::on_OpenBuildFolder_clicked()
{
  WQtUiServices::OpenInExplorer(BuildFolder->text().toUtf8().data(), false);
}

void WQtCppProjectDlg::on_OpenSolution_clicked()
{
  if (auto result = WCppProject::OpenSolution(m_CppSettings); result.Failed())
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning(result.GetMessageString().GetView());
  }
}

void WQtCppProjectDlg::on_PluginName_textEdited(const QString& text)
{
  WStringBuilder name = PluginName->text().toUtf8().data();

  if (name.EndsWith_NoCase("Plugin"))
  {
    name.Shrink(0, 6);
  }

  m_CppSettings.m_sPluginName = name;

  UpdateUI();
}

void WQtCppProjectDlg::UpdateUI()
{
  PluginLocation->setText(WCppProject::GetTargetSourceDir().GetData());
  BuildFolder->setText(WCppProject::GetBuildDir(m_CppSettings).GetData());

  OpenPluginLocation->setEnabled(WOSFile::ExistsDirectory(PluginLocation->text().toUtf8().data()));
  OpenBuildFolder->setEnabled(WOSFile::ExistsDirectory(BuildFolder->text().toUtf8().data()));
  OpenSolution->setEnabled(WCppProject::ExistsSolution(m_CppSettings));
}

class WForwardToQTextEdit : public WLogInterface
{
public:
  QTextEdit* m_pTextEdit = nullptr;

  void HandleLogMessage(const WLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case WLogMsgType::GlobalDefault:
      case WLogMsgType::Flush:
      case WLogMsgType::BeginGroup:
      case WLogMsgType::EndGroup:
      case WLogMsgType::None:
      case WLogMsgType::All:
      case WLogMsgType::ENUM_COUNT:
        return;

      case WLogMsgType::ErrorMsg:
      case WLogMsgType::SeriousWarningMsg:
      case WLogMsgType::WarningMsg:
      case WLogMsgType::SuccessMsg:
      case WLogMsgType::InfoMsg:
      case WLogMsgType::DevMsg:
      case WLogMsgType::DebugMsg:
      {
        WStringBuilder tmp(le.m_sText, "\n");

        QString s = m_pTextEdit->toPlainText();
        s.append(tmp);
        m_pTextEdit->setText(s);
        return;
      }

        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
};

void WQtCppProjectDlg::on_GenerateSolution_clicked()
{
  if (WCppProject::ExistsSolution(m_CppSettings))
  {
    if (WQtUiServices::MessageBoxQuestion("The solution already exists, do you want to recreate it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.m_sPluginName.IsEmpty())
  {
    m_CppSettings.m_sPluginName = PluginName->placeholderText().toUtf8().data();
  }

  if (!m_OldCppSettings.m_sPluginName.IsEmpty() && m_OldCppSettings.m_sPluginName != m_CppSettings.m_sPluginName)
  {
    if (WQtUiServices::MessageBoxQuestion("You are attempting to change the name of the existing C++ plugin.\n\nTHIS IS A BAD IDEA.\n\nThe C++ sources and CMake files were already created with the old name in it. To not accidentally delete your work, W won't touch any of those files. Therefore this change won't have any effect, unless you have already deleted those files yourself and W can just create new ones. Only select YES if you have done the necessary steps and/or know what you are doing.", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.Save().Failed())
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning("Saving new C++ project settings failed.");
    return;
  }

  m_OldCppSettings.Load().IgnoreResult();

#if W_ENABLED(W_PLATFORM_WINDOWS)
  if (WSystemInformation::IsDebuggerAttached())
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, CMake usually fails with the error that no C/C++ compiler can be found.\n\nDetach the debugger now, then press OK to continue.");
  }
#endif

  OutputLog->clear();

  {
    WForwardToQTextEdit log;
    log.m_pTextEdit = OutputLog;
    WLogSystemScope _logScope(&log);

    WProgressRange progress("Generating Solution", 3, false);
    progress.SetStepWeighting(0, 0.1f);
    progress.SetStepWeighting(1, 0.1f);
    progress.SetStepWeighting(2, 0.8f);

    W_SCOPE_EXIT(UpdateUI());

    {
      progress.BeginNextStep("Clean Build Directory");

      if (WCppProject::CleanBuildDir(m_CppSettings).Failed())
      {
        WLog::Warning("Couldn't delete build output directory:\n{}\n\nProject is probably already open in Visual Studio.\n", WCppProject::GetBuildDir(m_CppSettings));
      }
    }

    {
      progress.BeginNextStep("Populate with Default Sources");
      if (WCppProject::PopulateWithDefaultSources(m_CppSettings).Failed())
      {
        WQtUiServices::GetSingleton()->MessageBoxWarning("Failed to populate the CppSource directory with the default files.\n\nCheck the log for details.");
        return;
      }
    }

    // run CMake
    {
      progress.BeginNextStep("Running CMake");

      if (WCppProject::RunCMake(m_CppSettings).Failed())
      {

        WQtUiServices::GetSingleton()->MessageBoxWarning("Generating the solution failed.\n\nCheck the log for details.");
        return;
      }
    }

    if (WCppProject::BuildCodeIfNecessary(m_CppSettings).Failed())
    {
      WLog::Error("Failed to compile the newly generated C++ solution.");
    }
  }

  WCppProject::UpdatePluginConfig(m_CppSettings);

  WQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);

  if (WQtUiServices::GetSingleton()->MessageBoxQuestion("The solution was generated successfully.\n\nDo you want to open it now?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
  {
    on_OpenSolution_clicked();
  }
}
