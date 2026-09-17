

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/ui_CppProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class W_EDITORFRAMEWORK_DLL WQtCppProjectDlg : public WQtDialog, public Ui_WQtCppProjectDlg
{
public:
  Q_OBJECT

public:
  WQtCppProjectDlg(QWidget* pParent);

private Q_SLOTS:
  void on_OpenPluginLocation_clicked();
  void on_OpenBuildFolder_clicked();
  void on_OpenSolution_clicked();
  void on_GenerateSolution_clicked();
  void on_PluginName_textEdited(const QString& text);

private:
  void UpdateUI();

  WCppSettings m_OldCppSettings;
  WCppSettings m_CppSettings;
};
