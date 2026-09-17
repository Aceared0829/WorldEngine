#pragma once

#include <EditorFramework/ui_SettingsTab.h>
#include <Foundation/Configuration/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class WQtSettingsTab : public WQtDocumentWindow, Ui_SettingsTab
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtSettingsTab);

public:
  WQtSettingsTab();
  ~WQtSettingsTab();

  virtual WString GetWindowIcon() const override;
  virtual WString GetDisplayNameShort() const override;

private:
  virtual bool InternalCanCloseWindow() override;
  virtual void InternalCloseDocumentWindow() override;
};
