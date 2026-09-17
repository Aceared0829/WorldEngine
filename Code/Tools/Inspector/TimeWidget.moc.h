#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_TimeWidget.h>
#include <QGraphicsView>
#include <QListWidgetItem>
#include <ads/DockWidget.h>

class WQtTimeWidget : public ads::CDockWidget, public Ui_TimeWidget
{
public:
  Q_OBJECT

public:
  static const WUInt8 s_uiMaxColors = 9;

  WQtTimeWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtTimeWidget* s_pWidget;

private Q_SLOTS:

  void on_ListClocks_itemChanged(QListWidgetItem* item);
  void on_ComboTimeframe_currentIndexChanged(int index);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene m_Scene;

  WUInt32 m_uiMaxSamples;

  WUInt8 m_uiColorsUsed;
  bool m_bClocksChanged;

  WTime m_MaxGlobalTime;
  WTime m_DisplayInterval;
  WTime m_LastUpdatedClockList;

  struct TimeSample
  {
    WTime m_AtGlobalTime;
    WTime m_Timestep;
  };

  struct ClockData
  {
    WDeque<TimeSample> m_TimeSamples;

    bool m_bDisplay = true;
    WUInt8 m_uiColor = 0xFF;
    WTime m_MinTimestep = WTime::MakeFromSeconds(60.0);
    WTime m_MaxTimestep;
    QListWidgetItem* m_pListItem = nullptr;
  };

  WMap<WString, ClockData> m_ClockData;
};
