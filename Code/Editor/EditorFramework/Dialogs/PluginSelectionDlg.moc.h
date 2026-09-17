#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_PluginSelectionDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class W_EDITORFRAMEWORK_DLL WQtPluginSelectionDlg : public WQtDialog, public Ui_PluginSelectionDlg
{
public:
  Q_OBJECT

public:
  WQtPluginSelectionDlg(WPluginBundleSet* pPluginSet, QWidget* pParent = nullptr);
  ~WQtPluginSelectionDlg();


private Q_SLOTS:
  void on_Buttons_clicked(QAbstractButton* pButton);

private:
  WPluginBundleSet m_LocalPluginSet;
  WPluginBundleSet* m_pPluginSet = nullptr;
};
