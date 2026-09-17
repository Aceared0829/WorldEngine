#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_LongOpsPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <QTimer>

struct WLongOpControllerEvent;

/// This panel listens to events from WLongOpControllerManager and displays all currently known long operations
class W_EDITORFRAMEWORK_DLL WQtLongOpsPanel : public WQtApplicationPanel, public Ui_LongOpsPanel
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtLongOpsPanel);

public:
  WQtLongOpsPanel(ads::CDockManager* pDockManager);
  ~WQtLongOpsPanel();

private:
  void LongOpsEventHandler(const WLongOpControllerEvent& e);
  void RebuildTable();
  void UpdateTable();

  bool m_bUpdateTimerRunning = false;
  bool m_bRebuildTable = true;
  bool m_bUpdateTable = false;
  WHashTable<WUuid, WUInt32> m_LongOpGuidToRow;


private Q_SLOTS:
  void StartUpdateTimer();
  void UpdateUI();
  void OnClickButton(bool);
  void OnCellDoubleClicked(int row, int column);
};
