#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>

namespace PluginsDetail
{
  static void SendPluginTelemetry()
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    WTelemetry::Broadcast(WTelemetry::Reliable, 'PLUG', ' CLR', nullptr, 0);

    WTempHybridArray<WPlugin::PluginInfo, 16> infos;
    WPlugin::GetAllPluginInfos(infos);

    for (const auto& pi : infos)
    {
      WTelemetryMessage msg;
      msg.SetMessageID('PLUG', 'DATA');
      msg.GetWriter() << pi.m_sName;
      msg.GetWriter() << false; // deprecated 'IsReloadable' flag

      WStringBuilder s;

      for (const auto& dep : pi.m_sDependencies)
      {
        s.AppendWithSeparator(" | ", dep);
      }

      msg.GetWriter() << s;

      WTelemetry::Broadcast(WTelemetry::Reliable, msg);
    }
  }

  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }

  static void PluginEventHandler(const WPluginEvent& e)
  {
    switch (e.m_EventType)
    {
      case WPluginEvent::AfterPluginChanges:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace PluginsDetail

void AddPluginEventHandler()
{
  WTelemetry::AddEventHandler(PluginsDetail::TelemetryEventsHandler);
  WPlugin::Events().AddEventHandler(PluginsDetail::PluginEventHandler);
}

void RemovePluginEventHandler()
{
  WPlugin::Events().RemoveEventHandler(PluginsDetail::PluginEventHandler);
  WTelemetry::RemoveEventHandler(PluginsDetail::TelemetryEventsHandler);
}


