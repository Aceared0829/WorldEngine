#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class WQtCVarWidget;

class W_EDITORFRAMEWORK_DLL WQtCVarPanel : public WQtApplicationPanel
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtCVarPanel);

public:
  WQtCVarPanel(ads::CDockManager* pDockManager);
  ~WQtCVarPanel();

protected:
  virtual void ToolsProjectEventHandler(const WToolsProjectEvent& e) override;

private Q_SLOTS:
  void UpdateUI();
  void BoolChanged(const char* szCVar, bool newValue);
  void FloatChanged(const char* szCVar, float newValue);
  void IntChanged(const char* szCVar, int newValue);
  void StringChanged(const char* szCVar, const char* newValue);

private:
  void EngineProcessMsgHandler(const WEditorEngineProcessConnection::Event& e);

  WQtCVarWidget* m_pCVarWidget = nullptr;

  WMap<WString, WCVarWidgetData> m_EngineCVarState;

  bool m_bUpdateUI = false;
  bool m_bRebuildUI = false;
  bool m_bUpdateConsole = false;
  WStringBuilder m_sCommandResult;
};
