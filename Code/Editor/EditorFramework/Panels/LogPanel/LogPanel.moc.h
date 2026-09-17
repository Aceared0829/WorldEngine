#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/ui_LogPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class WQtLogModel;
struct WLoggingEventData;
class WPreferences;

/// The application wide panel that shows the engine log output and the editor log output
class W_EDITORFRAMEWORK_DLL WQtLogPanel : public WQtApplicationPanel, public Ui_LogPanel
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtLogPanel);

public:
  WQtLogPanel(ads::CDockManager* pDockManager);
  ~WQtLogPanel();

protected:
  virtual void ToolsProjectEventHandler(const WToolsProjectEvent& e) override;

private Q_SLOTS:
  void OnNewWarningsOrErrors(const char* szText, bool bError);

private:
  void LogWriter(const WLoggingEventData& e);
  void EngineProcessMsgHandler(const WEditorEngineProcessConnection::Event& e);
  void UiServiceEventHandler(const WQtUiServices::Event& e);
  void OnPreferenceChange(WPreferences* pref);

  WUInt32 m_uiIgnoredNumErrors = 0;
  WUInt32 m_uiIgnoreNumWarnings = 0;
  WUInt32 m_uiKnownNumErrors = 0;
  WUInt32 m_uiKnownNumWarnings = 0;

  bool m_bCombineLogs = true;
};
