#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Time/Clock.h>

static void TimeEventHandler(const WClock::EventData& e)
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  WTelemetryMessage Msg;
  Msg.SetMessageID('TIME', 'UPDT');
  Msg.GetWriter() << e.m_sClockName;
  Msg.GetWriter() << WTime::Now();
  Msg.GetWriter() << e.m_RawTimeStep;
  Msg.GetWriter() << e.m_SmoothedTimeStep;

  WTelemetry::Broadcast(WTelemetry::Unreliable, Msg);
}

void AddTimeEventHandler()
{
  WClock::AddEventHandler(TimeEventHandler);
}

void RemoveTimeEventHandler()
{
  WClock::RemoveEventHandler(TimeEventHandler);
}


