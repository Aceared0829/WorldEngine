#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_SubsystemsWidget.h>
#include <ads/DockWidget.h>

class WQtSubsystemsWidget : public ads::CDockWidget, public Ui_SubsystemsWidget
{
public:
  Q_OBJECT

public:
  WQtSubsystemsWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtSubsystemsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void UpdateSubSystems();

  struct SubsystemData
  {
    WString m_sPlugin;
    bool m_bStartupDone[WStartupStage::ENUM_COUNT];
    WString m_sDependencies;
  };

  bool m_bUpdateSubsystems;
  WMap<WString, SubsystemData> m_Subsystems;
};
