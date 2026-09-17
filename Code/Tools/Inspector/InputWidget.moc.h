#pragma once

#include <Core/Input/InputManager.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_InputWidget.h>
#include <ads/DockWidget.h>

class WQtInputWidget : public ads::CDockWidget, public Ui_InputWidget
{
public:
  Q_OBJECT

public:
  WQtInputWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtInputWidget* s_pWidget;

private Q_SLOTS:
  virtual void on_ButtonClearSlots_clicked();
  virtual void on_ButtonClearActions_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void ClearSlots();
  void ClearActions();

  void UpdateSlotTable(bool bRecreate);
  void UpdateActionTable(bool bRecreate);

  struct SlotData
  {
    WInt32 m_iTableRow;
    WUInt16 m_uiSlotFlags;
    WKeyState::Enum m_KeyState;
    float m_fValue;
    float m_fDeadZone;

    SlotData()
    {
      m_iTableRow = -1;
      m_uiSlotFlags = 0;
      m_KeyState = WKeyState::Up;
      m_fValue = 0;
      m_fDeadZone = 0;
    }
  };

  WMap<WString, SlotData> m_InputSlots;

  struct ActionData
  {
    WInt32 m_iTableRow;
    WKeyState::Enum m_KeyState;
    float m_fValue;
    bool m_bUseTimeScaling;

    WString m_sTrigger[WInputActionConfig::MaxInputSlotAlternatives];
    float m_fTriggerScaling[WInputActionConfig::MaxInputSlotAlternatives];

    ActionData()
    {
      m_iTableRow = -1;
      m_KeyState = WKeyState::Up;
      m_fValue = 0;
      m_bUseTimeScaling = false;
    }
  };

  WMap<WString, ActionData> m_InputActions;
};
