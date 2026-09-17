#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_LaunchFileserveDlg.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class W_EDITORFRAMEWORK_DLL WQtLaunchFileserveDlg : public WQtDialog, public Ui_WQtLaunchFileserveDlg
{
public:
  Q_OBJECT

public:
  WQtLaunchFileserveDlg(QWidget* pParent);
  ~WQtLaunchFileserveDlg();

  WString m_sFileserveCmdLine;

private Q_SLOTS:
  void on_ButtonLaunch_clicked();

private:
  virtual void showEvent(QShowEvent* event) override;
};
