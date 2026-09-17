#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <Inspector/ui_CVarsWidget.h>
#include <ads/DockWidget.h>

class WQtCVarsWidget : public ads::CDockWidget, public Ui_CVarsWidget
{
public:
  Q_OBJECT

public:
  WQtCVarsWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtCVarsWidget* s_pWidget;

private Q_SLOTS:
  void BoolChanged(WStringView sCVar, bool newValue);
  void FloatChanged(WStringView sCVar, float newValue);
  void IntChanged(WStringView sCVar, int newValue);
  void StringChanged(WStringView sCVar, WStringView sNewValue);

public:
  static void ProcessTelemetry(void* pUnuseed);
  static void ProcessTelemetryConsole(void* pUnuseed);

  void ResetStats();

private:
  // void UpdateCVarsTable(bool bRecreate);


  void SendCVarUpdateToServer(WStringView sName, const WCVarWidgetData& cvd);
  void SyncAllCVarsToServer();

  WMap<WString, WCVarWidgetData> m_CVars;
  WMap<WString, WCVarWidgetData> m_CVarsBackup;
};
