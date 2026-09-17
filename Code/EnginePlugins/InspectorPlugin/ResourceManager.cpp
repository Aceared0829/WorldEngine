#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace ResourceManagerDetail
{

  static void SendFullResourceInfo(const WResource* pRes)
  {
    WTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' SET');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << pRes->GetResourceID();
    Msg.GetWriter() << pRes->GetDynamicRTTI()->GetTypeName();
    Msg.GetWriter() << static_cast<WUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<WUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<WUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;
    Msg.GetWriter() << pRes->GetResourceDescription();

    WTelemetry::Broadcast(WTelemetry::Reliable, Msg);
  }

  static void SendSmallResourceInfo(const WResource* pRes)
  {
    WTelemetryMessage Msg;

    Msg.SetMessageID('RESM', 'UPDT');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << static_cast<WUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<WUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<WUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;

    WTelemetry::Broadcast(WTelemetry::Reliable, Msg);
  }

  static void SendDeleteResourceInfo(const WResource* pRes)
  {
    WTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' DEL');

    Msg.GetWriter() << pRes->GetResourceIDHash();

    WTelemetry::Broadcast(WTelemetry::Reliable, Msg);
  }

  static void SendAllResourceTelemetry()
  {
    WResourceManager::BroadcastExistsEvent();
  }

  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllResourceTelemetry();
        break;

      default:
        break;
    }
  }

  static void ResourceManagerEventHandler(const WResourceEvent& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case WResourceEvent::Type::ResourceCreated:
      case WResourceEvent::Type::ResourceExists:
        SendFullResourceInfo(e.m_pResource);
        return;

      case WResourceEvent::Type::ResourceDeleted:
        SendDeleteResourceInfo(e.m_pResource);
        return;

      case WResourceEvent::Type::ResourceContentUpdated:
      case WResourceEvent::Type::ResourceContentUnloading:
      case WResourceEvent::Type::ResourcePriorityChanged:
        SendSmallResourceInfo(e.m_pResource);
        return;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
    }
  }
} // namespace ResourceManagerDetail

void AddResourceManagerEventHandler()
{
  WTelemetry::AddEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
  WResourceManager::GetResourceEvents().AddEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
}

void RemoveResourceManagerEventHandler()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
  WTelemetry::RemoveEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
}
