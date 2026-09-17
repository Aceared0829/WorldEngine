#pragma once

#include <Core/System/Window.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_WindowCfgDlg.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class W_EDITORFRAMEWORK_DLL WQtWindowCfgDlg : public WQtDialog, public Ui_WQtWindowCfgDlg
{
public:
  Q_OBJECT

public:
  WQtWindowCfgDlg(QWidget* pParent);

private Q_SLOTS:
  void on_m_ButtonBox_clicked(QAbstractButton* button);
  void on_m_ComboWnd_currentIndexChanged(int index);
  void on_m_CheckOverrideDefault_stateChanged(int state);

private:
  void FillUI(const WWindowCreationDesc& desc);
  void GrabUI(WWindowCreationDesc& desc);
  void UpdateUI();
  void LoadDescs();
  void SaveDescs();

  WUInt8 m_uiCurDesc = 0;
  WWindowCreationDesc m_Descs[2];
  bool m_bOverrideProjectDefault[2];
};
