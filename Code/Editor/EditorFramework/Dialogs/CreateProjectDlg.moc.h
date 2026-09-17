

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_CreateProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class W_EDITORFRAMEWORK_DLL WQtCreateProjectDlg : public WQtDialog, public Ui_WQtCreateProjectDlg
{
public:
  Q_OBJECT

public:
  WQtCreateProjectDlg(QWidget* pParent);

  WString GetFullTargetPath() const;

  WString m_sTargetFolder;
  WString m_sTargetName;

private Q_SLOTS:
  void on_BrowseFolder_clicked();
  void on_ProjectName_textChanged(QString text);
  void on_Prev_clicked();
  void on_Next_clicked();

private:
  void UpdateUI();
  void FillProjectTemplatesList();
  WStatus CreateProject();

  enum class State
  {
    Basics,
    Templates,
    Plugins,
    Summary,
    Create,
  };

  State m_State = State::Basics;
  WString m_sProjectTemplate;
  WPluginBundleSet m_LocalPluginSet;
};
