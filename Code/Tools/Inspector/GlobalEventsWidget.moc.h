#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_GlobalEventsWidget.h>
#include <ads/DockWidget.h>

class WQtGlobalEventsWidget : public ads::CDockWidget, public Ui_GlobalEventsWidget
{
public:
  Q_OBJECT

public:
  WQtGlobalEventsWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtGlobalEventsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void UpdateTable(bool bRecreate);

  struct GlobalEventsData
  {
    WInt32 m_iTableRow;
    WUInt32 m_uiTimesFired;
    WUInt16 m_uiNumHandlers;
    WUInt16 m_uiNumHandlersOnce;

    GlobalEventsData()
    {
      m_iTableRow = -1;

      m_uiTimesFired = 0;
      m_uiNumHandlers = 0;
      m_uiNumHandlersOnce = 0;
    }
  };

  WMap<WString, GlobalEventsData> m_Events;
};
