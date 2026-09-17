#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  WTelemetryMessage Msg;

  while (WTelemetry::RetrieveMessage('SVAR', Msg) == W_SUCCESS)
  {
    if (Msg.GetMessageID() == ' SET')
    {
      WString sCVar;
      WUInt8 uiType;

      float fValue;
      WInt32 iValue;
      bool bValue;
      WString sValue;

      Msg.GetReader() >> sCVar;
      Msg.GetReader() >> uiType;

      switch (uiType)
      {
        case WCVarType::Float:
          Msg.GetReader() >> fValue;
          break;
        case WCVarType::Int:
          Msg.GetReader() >> iValue;
          break;
        case WCVarType::Bool:
          Msg.GetReader() >> bValue;
          break;
        case WCVarType::String:
          Msg.GetReader() >> sValue;
          break;
      }

      WCVar* pCVar = WCVar::GetFirstInstance();

      while (pCVar)
      {
        if (((WUInt8)pCVar->GetType() == uiType) && (pCVar->GetName() == sCVar))
        {
          switch (uiType)
          {
            case WCVarType::Float:
              *((WCVarFloat*)pCVar) = fValue;
              break;
            case WCVarType::Int:
              *((WCVarInt*)pCVar) = iValue;
              break;
            case WCVarType::Bool:
              *((WCVarBool*)pCVar) = bValue;
              break;
            case WCVarType::String:
              *((WCVarString*)pCVar) = sValue;
              break;
          }
        }

        pCVar = pCVar->GetNextInstance();
      }
    }
  }
}

static void SendCVarTelemetry(WCVar* pCVar)
{
  WTelemetryMessage msg;
  msg.SetMessageID('CVAR', 'DATA');
  msg.GetWriter() << pCVar->GetName();
  msg.GetWriter() << pCVar->GetPluginName();
  // msg.GetWriter() << (WUInt8) pCVar->GetFlags().GetValue(); // currently not used
  msg.GetWriter() << (WUInt8)pCVar->GetType();
  msg.GetWriter() << pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case WCVarType::Float:
    {
      const float val = ((WCVarFloat*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case WCVarType::Int:
    {
      const int val = ((WCVarInt*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case WCVarType::Bool:
    {
      const bool val = ((WCVarBool*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case WCVarType::String:
    {
      WStringView val = ((WCVarString*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;

    case WCVarType::ENUM_COUNT:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  WTelemetry::Broadcast(WTelemetry::Reliable, msg);
}

static void SendAllCVarTelemetry()
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    WTelemetryMessage msg;
    WTelemetry::Broadcast(WTelemetry::Reliable, 'CVAR', ' CLR', nullptr, 0);
  }

  WCVar* pCVar = WCVar::GetFirstInstance();

  while (pCVar)
  {
    SendCVarTelemetry(pCVar);

    pCVar = pCVar->GetNextInstance();
  }

  {
    WTelemetryMessage msg;
    WTelemetry::Broadcast(WTelemetry::Reliable, 'CVAR', 'SYNC', nullptr, 0);
  }
}

namespace CVarsDetail
{

  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }

  static void CVarEventHandler(const WCVarEvent& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case WCVarEvent::ValueChanged:
        SendCVarTelemetry(e.m_pCVar);
        break;

      case WCVarEvent::ListOfVarsChanged:
        SendAllCVarTelemetry();
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
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace CVarsDetail

void AddCVarEventHandler()
{
  WTelemetry::AddEventHandler(CVarsDetail::TelemetryEventsHandler);
  WTelemetry::AcceptMessagesForSystem('SVAR', true, TelemetryMessage, nullptr);

  WCVar::s_AllCVarEvents.AddEventHandler(CVarsDetail::CVarEventHandler);
  WPlugin::Events().AddEventHandler(CVarsDetail::PluginEventHandler);
}

void RemoveCVarEventHandler()
{
  WPlugin::Events().RemoveEventHandler(CVarsDetail::PluginEventHandler);
  WCVar::s_AllCVarEvents.RemoveEventHandler(CVarsDetail::CVarEventHandler);

  WTelemetry::RemoveEventHandler(CVarsDetail::TelemetryEventsHandler);
  WTelemetry::AcceptMessagesForSystem('SVAR', false);
}


