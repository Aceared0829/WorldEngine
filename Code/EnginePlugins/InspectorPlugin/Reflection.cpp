#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Reflection/Reflection.h>

namespace ReflectionDetail
{

  static void SendBasicTypesGroup()
  {
    WTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << "Basic Types";
    msg.GetWriter() << "";
    msg.GetWriter() << 0;
    msg.GetWriter() << "";
    msg.GetWriter() << (WUInt32)0U;
    msg.GetWriter() << (WUInt32)0U;

    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
  }

  static WStringView GetParentType(const WRTTI* pRTTI)
  {
    if (pRTTI->GetParentType())
    {
      return pRTTI->GetParentType()->GetTypeName();
    }

    if ((pRTTI->GetTypeName() == "bool") || (pRTTI->GetTypeName() == "float") ||
        (pRTTI->GetTypeName() == "double") || (pRTTI->GetTypeName() == "WInt8") ||
        (pRTTI->GetTypeName() == "WUInt8") || (pRTTI->GetTypeName() == "WInt16") ||
        (pRTTI->GetTypeName() == "WUInt16") || (pRTTI->GetTypeName() == "WInt32") ||
        (pRTTI->GetTypeName() == "WUInt32") || (pRTTI->GetTypeName() == "WInt64") ||
        (pRTTI->GetTypeName() == "WUInt64") || (pRTTI->GetTypeName() == "WConstCharPtr") ||
        (pRTTI->GetTypeName() == "WVec2") || (pRTTI->GetTypeName() == "WVec3") ||
        (pRTTI->GetTypeName() == "WVec4") || (pRTTI->GetTypeName() == "WMat3") ||
        (pRTTI->GetTypeName() == "WMat4") || (pRTTI->GetTypeName() == "WTime") ||
        (pRTTI->GetTypeName() == "WUuid") || (pRTTI->GetTypeName() == "WColor") ||
        (pRTTI->GetTypeName() == "WVariant") || (pRTTI->GetTypeName() == "WQuat"))
    {
      return "Basic Types";
    }

    return {};
  }

  static void SendReflectionTelemetry(const WRTTI* pRTTI)
  {
    WTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << pRTTI->GetTypeName();
    msg.GetWriter() << GetParentType(pRTTI);
    msg.GetWriter() << pRTTI->GetTypeSize();
    msg.GetWriter() << pRTTI->GetPluginName();

    {
      auto properties = pRTTI->GetProperties();

      msg.GetWriter() << properties.GetCount();

      for (auto& prop : properties)
      {
        msg.GetWriter() << prop->GetPropertyName();
        msg.GetWriter() << (WInt8)prop->GetCategory();

        const WRTTI* pType = prop->GetSpecificType();
        msg.GetWriter() << (pType ? pType->GetTypeName() : "<Unknown Type>");
      }
    }

    {
      const WArrayPtr<WAbstractMessageHandler*>& Messages = pRTTI->GetMessageHandlers();

      msg.GetWriter() << Messages.GetCount();

      for (WUInt32 i = 0; i < Messages.GetCount(); ++i)
      {
        msg.GetWriter() << Messages[i]->GetMessageId();
      }
    }

    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
  }

  static void SendAllReflectionTelemetry()
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    // clear
    {
      WTelemetryMessage msg;
      WTelemetry::Broadcast(WTelemetry::Reliable, 'RFLC', ' CLR', nullptr, 0);
    }

    SendBasicTypesGroup();

    WRTTI::ForEachType([](const WRTTI* pRtti)
      { SendReflectionTelemetry(pRtti); });
  }


  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllReflectionTelemetry();
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
        SendAllReflectionTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace ReflectionDetail

void AddReflectionEventHandler()
{
  WTelemetry::AddEventHandler(ReflectionDetail::TelemetryEventsHandler);

  WPlugin::Events().AddEventHandler(ReflectionDetail::PluginEventHandler);
}

void RemoveReflectionEventHandler()
{
  WPlugin::Events().RemoveEventHandler(ReflectionDetail::PluginEventHandler);

  WTelemetry::RemoveEventHandler(ReflectionDetail::TelemetryEventsHandler);
}


