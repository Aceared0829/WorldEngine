#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_StatVisWidget.h>
#include <QAction>
#include <QGraphicsView>
#include <QListWidgetItem>
#include <ads/DockWidget.h>

class WQtStatVisWidget : public ads::CDockWidget, public Ui_StatVisWidget
{
public:
  Q_OBJECT

public:
  static const WUInt8 s_uiMaxColors = 9;

  WQtStatVisWidget(ads::CDockManager* pDockManager, QWidget* pParent, WInt32 iWindowNumber);
  ~WQtStatVisWidget();

  void UpdateStats();

  static WQtStatVisWidget* s_pWidget;

  void AddStat(const WString& sStatPath, bool bEnabled = true, bool bRaiseWindow = true);

  void Save();
  void Load();

private Q_SLOTS:

  void on_ComboTimeframe_currentIndexChanged(int index);
  void on_LineName_textChanged(const QString& text);
  void on_SpinMin_valueChanged(double val);
  void on_SpinMax_valueChanged(double val);
  void on_ToggleVisible();
  void on_ButtonRemove_clicked();
  void on_ListStats_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

public:
  QAction m_ShowWindowAction;

private:
  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene m_Scene;

  static WInt32 s_iCurColor;

  WTime m_DisplayInterval;

  WInt32 m_iWindowNumber;

  struct StatsData
  {
    QListWidgetItem* m_pListItem = nullptr;
    WUInt8 m_uiColor = 0;
  };

  WMap<WString, StatsData> m_Stats;
};
