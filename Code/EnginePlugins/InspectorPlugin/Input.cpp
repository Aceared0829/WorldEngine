#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace InputDetail
{

  static void SendInputSlotData(WStringView sInputSlot)
  {
    float fValue = 0.0f;

    WTelemetryMessage msg;
    msg.SetMessageID('INPT', 'SLOT');
    msg.GetWriter() << sInputSlot;
    msg.GetWriter() << WInputManager::GetInputSlotFlags(sInputSlot).GetValue();
    msg.GetWriter() << (WUInt8)WInputManager::GetInputSlotState(sInputSlot, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << WInputManager::GetInputSlotDeadZone(sInputSlot);

    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
  }

  static void SendInputActionData(WStringView sInputSet, WStringView sInputAction)
  {
    float fValue = 0.0f;

    const WInputActionConfig cfg = WInputManager::GetInputActionConfig(sInputSet, sInputAction);

    WTelemetryMessage msg;
    msg.SetMessageID('INPT', 'ACTN');
    msg.GetWriter() << sInputSet;
    msg.GetWriter() << sInputAction;
    msg.GetWriter() << (WUInt8)WInputManager::GetInputActionState(sInputSet, sInputAction, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << cfg.m_bApplyTimeScaling;

    for (WUInt32 i = 0; i < WInputActionConfig::MaxInputSlotAlternatives; ++i)
    {
      msg.GetWriter() << cfg.m_sInputSlotTrigger[i];
      msg.GetWriter() << cfg.m_fInputSlotScale[i];
    }

    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
  }

  static void SendAllInputSlots()
  {
    WDynamicArray<WStringView> InputSlots;
    WInputManager::RetrieveAllKnownInputSlots(InputSlots);

    for (WUInt32 i = 0; i < InputSlots.GetCount(); ++i)
    {
      SendInputSlotData(InputSlots[i]);
    }
  }

  static void SendAllInputActions()
  {
    WDynamicArray<WString> InputSetNames;
    WInputManager::GetAllInputSets(InputSetNames);

    for (WUInt32 s = 0; s < InputSetNames.GetCount(); ++s)
    {
      WTempHybridArray<WString, 24> InputActions;

      WInputManager::GetAllInputActions(InputSetNames[s].GetData(), InputActions);

      for (WUInt32 a = 0; a < InputActions.GetCount(); ++a)
        SendInputActionData(InputSetNames[s].GetData(), InputActions[a].GetData());
    }
  }

  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllInputSlots();
        SendAllInputActions();
        break;

      default:
        break;
    }
  }

  static void InputManagerEventHandler(const WInputManager::InputEventData& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case WInputManager::InputEventData::InputActionChanged:
        SendInputActionData(e.m_sInputSet, e.m_sInputAction);
        break;
      case WInputManager::InputEventData::InputSlotChanged:
        SendInputSlotData(e.m_sInputSlot);
        break;

      default:
        break;
    }
  }
} // namespace InputDetail

void AddInputEventHandler()
{
  WTelemetry::AddEventHandler(InputDetail::TelemetryEventsHandler);
  WInputManager::AddEventHandler(InputDetail::InputManagerEventHandler);
}

void RemoveInputEventHandler()
{
  WInputManager::RemoveEventHandler(InputDetail::InputManagerEventHandler);
  WTelemetry::RemoveEventHandler(InputDetail::TelemetryEventsHandler);
}


