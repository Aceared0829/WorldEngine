#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>

#include <Core/GameApplication/GameApplicationBase.h>

static WGlobalEvent::EventMap s_LastState;

static void SendGlobalEventTelemetry(WStringView sEvent, const WGlobalEvent::EventData& ed)
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  WTelemetryMessage msg;
  msg.SetMessageID('EVNT', 'DATA');
  msg.GetWriter() << sEvent;
  msg.GetWriter() << ed.m_uiNumTimesFired;
  msg.GetWriter() << ed.m_uiNumEventHandlersRegular;
  msg.GetWriter() << ed.m_uiNumEventHandlersOnce;

  WTelemetry::Broadcast(WTelemetry::Reliable, msg);
}

static void SendAllGlobalEventTelemetry()
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    WTelemetryMessage msg;
    WTelemetry::Broadcast(WTelemetry::Reliable, 'EVNT', ' CLR', nullptr, 0);
  }

  WGlobalEvent::UpdateGlobalEventStatistics();

  s_LastState = WGlobalEvent::GetEventStatistics();

  for (WGlobalEvent::EventMap::ConstIterator it = s_LastState.GetIterator(); it.IsValid(); ++it)
  {
    SendGlobalEventTelemetry(it.Key(), it.Value());
  }
}

static void SendChangedGlobalEventTelemetry()
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  static WTime LastUpdate = WTime::Now();

  if ((WTime::Now() - LastUpdate).GetSeconds() < 0.5)
    return;

  LastUpdate = WTime::Now();

  WGlobalEvent::UpdateGlobalEventStatistics();

  const WGlobalEvent::EventMap& data = WGlobalEvent::GetEventStatistics();

  if (data.GetCount() != s_LastState.GetCount())
  {
    SendAllGlobalEventTelemetry();
    return;
  }

  for (WGlobalEvent::EventMap::ConstIterator it = data.GetIterator(); it.IsValid(); ++it)
  {
    const WGlobalEvent::EventData& currentEventData = it.Value();
    WGlobalEvent::EventData& lastEventData = s_LastState[it.Key()];

    if (WMemoryUtils::Compare(&currentEventData, &lastEventData) != 0)
    {
      SendGlobalEventTelemetry(it.Key().GetData(), it.Value());

      lastEventData = currentEventData;
    }
  }
}

namespace GlobalEventsDetail
{
  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllGlobalEventTelemetry();
        break;
      case WTelemetry::TelemetryEventData::DisconnectedFromClient:
      {
        WGlobalEvent::EventMap tmp;
        s_LastState.Swap(tmp);
        break;
      }
      default:
        break;
    }
  }

  static void PerframeUpdateHandler(const WGameApplicationExecutionEvent& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case WGameApplicationExecutionEvent::Type::AfterPresent:
        SendChangedGlobalEventTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace GlobalEventsDetail

void AddGlobalEventHandler()
{
  WTelemetry::AddEventHandler(GlobalEventsDetail::TelemetryEventsHandler);

  // We're handling the per frame update by a different event since
  // using WTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the WStats and WTelemetry system.
  if (WGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }
}

void RemoveGlobalEventHandler()
{
  if (WGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }

  WTelemetry::RemoveEventHandler(GlobalEventsDetail::TelemetryEventsHandler);
}


