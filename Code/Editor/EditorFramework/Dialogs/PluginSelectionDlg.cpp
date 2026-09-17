#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/PluginSelectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OpenDdlWriter.h>

WQtPluginSelectionDlg::WQtPluginSelectionDlg(WPluginBundleSet* pPluginSet, QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  m_pPluginSet = pPluginSet;
  m_LocalPluginSet = *pPluginSet;

  PluginSelectionWidget->SetPluginSet(&m_LocalPluginSet);
}

WQtPluginSelectionDlg::~WQtPluginSelectionDlg() = default;

void WQtPluginSelectionDlg::on_Buttons_clicked(QAbstractButton* pButton)
{
  if (Buttons->standardButton(pButton) == QDialogButtonBox::Ok)
  {
    PluginSelectionWidget->SyncStateToSet();

    if (!m_pPluginSet->IsStateEqual(m_LocalPluginSet))
    {
      *m_pPluginSet = m_LocalPluginSet;

      WQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
      WCppProject::UpdateEnginePluginDependencies().IgnoreResult();
      WQtEditorApp::GetSingleton()->AddRestartRequiredReason("The set of active plugins has changed.");
    }

    accept();
  }
  else
  {
    reject();
  }
}
