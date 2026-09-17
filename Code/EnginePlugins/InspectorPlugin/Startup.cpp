#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>

namespace StartupDetail
{
  static void SendSubsystemTelemetry();
  static WInt32 s_iSendSubSystemTelemetry = 0;
} // namespace StartupDetail

W_ON_GLOBAL_EVENT(WStartup_StartupCoreSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

W_ON_GLOBAL_EVENT(WStartup_StartupHighLevelSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

W_ON_GLOBAL_EVENT(WStartup_ShutdownCoreSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

W_ON_GLOBAL_EVENT(WStartup_ShutdownHighLevelSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

namespace StartupDetail
{
  static void SendSubsystemTelemetry()
  {
    if (s_iSendSubSystemTelemetry <= 0)
      return;

    WTelemetry::Broadcast(WTelemetry::Reliable, 'STRT', ' CLR', nullptr, 0);

    WSubSystem* pSub = WSubSystem::GetFirstInstance();

    while (pSub)
    {
      WTelemetryMessage msg;
      msg.SetMessageID('STRT', 'SYST');
      msg.GetWriter() << pSub->GetGroupName();
      msg.GetWriter() << pSub->GetSubSystemName();
      msg.GetWriter() << pSub->GetPluginName();

      for (WUInt32 i = 0; i < WStartupStage::ENUM_COUNT; ++i)
        msg.GetWriter() << pSub->IsStartupPhaseDone((WStartupStage::Enum)i);

      WUInt8 uiDependencies = 0;
      while (pSub->GetDependency(uiDependencies) != nullptr)
        ++uiDependencies;

      msg.GetWriter() << uiDependencies;

      for (WUInt8 i = 0; i < uiDependencies; ++i)
        msg.GetWriter() << pSub->GetDependency(i);

      WTelemetry::Broadcast(WTelemetry::Reliable, msg);

      pSub = pSub->GetNextInstance();
    }
  }

  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        SendSubsystemTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace StartupDetail

void AddStartupEventHandler()
{
  ++StartupDetail::s_iSendSubSystemTelemetry;
  WTelemetry::AddEventHandler(StartupDetail::TelemetryEventsHandler);
}

void RemoveStartupEventHandler()
{
  --StartupDetail::s_iSendSubSystemTelemetry;
  WTelemetry::RemoveEventHandler(StartupDetail::TelemetryEventsHandler);
}



W_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Startup);
