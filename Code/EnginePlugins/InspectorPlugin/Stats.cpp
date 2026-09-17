#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/Stats.h>

static void StatsEventHandler(const WStats::StatsEventData& e)
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  WTelemetry::TransmitMode Mode = WTelemetry::Reliable;

  switch (e.m_EventType)
  {
    case WStats::StatsEventData::Set:
      Mode = WTelemetry::Unreliable;
      // fall-through
    case WStats::StatsEventData::Add:
    {
      WTelemetryMessage msg;
      msg.SetMessageID('STAT', ' SET');
      msg.GetWriter() << e.m_sStatName;
      msg.GetWriter() << e.m_NewStatValue;
      msg.GetWriter() << WTime::Now();

      WTelemetry::Broadcast(Mode, msg);
    }
    break;
    case WStats::StatsEventData::Remove:
    {
      WTelemetryMessage msg;
      msg.SetMessageID('STAT', ' DEL');
      msg.GetWriter() << e.m_sStatName;
      msg.GetWriter() << WTime::Now();

      WTelemetry::Broadcast(WTelemetry::Reliable, msg);
    }
    break;
  }
}


static void SendAllStatsTelemetry()
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  for (WStats::MapType::ConstIterator it = WStats::GetAllStats().GetIterator(); it.IsValid(); ++it)
  {
    WTelemetryMessage msg;
    msg.SetMessageID('STAT', ' SET');
    msg.GetWriter() << it.Key().GetData();
    msg.GetWriter() << it.Value();

    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
  }
}

static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
{
  switch (e.m_EventType)
  {
    case WTelemetry::TelemetryEventData::ConnectedToClient:
      SendAllStatsTelemetry();
      break;

    default:
      break;
  }
}

static void PerFrameUpdateHandler(const WGameApplicationExecutionEvent& e)
{
  switch (e.m_Type)
  {
    case WGameApplicationExecutionEvent::Type::AfterPresent:
    {
      WTime FrameTime;

      if (WGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
      {
        FrameTime = WGameApplicationBase::GetGameApplicationBaseInstance()->GetFrameTime();
      }

      WStringBuilder s;
      WStats::SetStat("App/FrameTime[ms]", FrameTime.GetMilliseconds());
      WStats::SetStat("App/FPS", 1.0 / FrameTime.GetSeconds());

      WStats::SetStat("App/Active Threads", WOSThread::GetThreadCount());

      // Tasksystem Thread Utilization
      {
        for (WUInt32 t = 0; t < WTaskSystem::GetWorkerThreadCount(WWorkerThreadType::ShortTasks); ++t)
        {
          WUInt32 uiNumTasks = 0;
          const double Utilization = WTaskSystem::GetThreadUtilization(WWorkerThreadType::ShortTasks, t, &uiNumTasks);

          s.SetFormat("Utilization/Short{0}_Load[%%]", WArgI(t, 2, true));
          WStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Short{0}_Tasks", WArgI(t, 2, true));
          WStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (WUInt32 t = 0; t < WTaskSystem::GetWorkerThreadCount(WWorkerThreadType::LongTasks); ++t)
        {
          WUInt32 uiNumTasks = 0;
          const double Utilization = WTaskSystem::GetThreadUtilization(WWorkerThreadType::LongTasks, t, &uiNumTasks);

          s.SetFormat("Utilization/Long{0}_Load[%%]", WArgI(t, 2, true));
          WStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Long{0}_Tasks", WArgI(t, 2, true));
          WStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (WUInt32 t = 0; t < WTaskSystem::GetWorkerThreadCount(WWorkerThreadType::FileAccess); ++t)
        {
          WUInt32 uiNumTasks = 0;
          const double Utilization = WTaskSystem::GetThreadUtilization(WWorkerThreadType::FileAccess, t, &uiNumTasks);

          s.SetFormat("Utilization/File{0}_Load[%%]", WArgI(t, 2, true));
          WStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/File{0}_Tasks", WArgI(t, 2, true));
          WStats::SetStat(s.GetData(), uiNumTasks);
        }
      }
    }
    break;

    default:
      break;
  }
}

void AddStatsEventHandler()
{
  WStats::AddEventHandler(StatsEventHandler);

  WTelemetry::AddEventHandler(TelemetryEventsHandler);

  // We're handling the per frame update by a different event since
  // using WTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the WStats and WTelemetry system.
  if (WGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(PerFrameUpdateHandler);
  }
}

void RemoveStatsEventHandler()
{
  if (WGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(PerFrameUpdateHandler);
  }

  WTelemetry::RemoveEventHandler(TelemetryEventsHandler);

  WStats::RemoveEventHandler(StatsEventHandler);
}


