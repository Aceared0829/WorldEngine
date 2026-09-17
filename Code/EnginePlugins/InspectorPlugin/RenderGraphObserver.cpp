#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>

namespace RenderGraphObserverDetail
{
  static constexpr WUInt32 s_uiSystemId = 'RGPH';
  static constexpr WUInt16 s_uiProtocolVersion = 1;

  static WSharedPtr<WRenderGraphPassObserver> s_pObserver;
  static WRenderGraphInspectionSummary s_LastSummary;
  static WRenderGraphInspectionInfo s_LastInfo;
  static WUInt64 s_uiRequestedInfoGraph = 0;
  static WUInt64 s_uiLastInfoGraph = 0;
  static bool s_bLastInfoValid = false;
  static bool s_bSummaryDirty = false;
  static bool s_bInfoDirty = false;
  static WTime s_LastSummaryBroadcast;
  static WTime s_LastResponseBroadcast;
  static bool s_bObserverResponseEnabled = false;

  static void EnsureObserver()
  {
    if (s_pObserver == nullptr)
    {
      s_pObserver = WRenderGraphManager::CreateObserver();
    }
  }

  static void SendSummary()
  {
    WTelemetryMessage msg;
    msg.SetMessageID(s_uiSystemId, 'SUMM');
    msg.GetWriter() << s_uiProtocolVersion;
    msg.GetWriter() << s_LastSummary;
    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
    s_bSummaryDirty = false;
  }

  static void SendInspectionInfo(WUInt64 uiGraphIdentity)
  {
    WTelemetryMessage msg;
    msg.SetMessageID(s_uiSystemId, 'INFO');
    msg.GetWriter() << s_uiProtocolVersion;
    msg.GetWriter() << uiGraphIdentity;

    const bool bSuccess = s_bLastInfoValid && s_uiLastInfoGraph == uiGraphIdentity;
    msg.GetWriter() << bSuccess;
    if (bSuccess)
    {
      msg.GetWriter() << s_LastInfo;
    }

    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
    if (uiGraphIdentity == s_uiLastInfoGraph)
    {
      s_bInfoDirty = false;
    }
  }

  static void RenderGraphRenderEventHandler(const WRenderGraphRenderEvent& e)
  {
    if (!WTelemetry::IsConnectedToOther())
      return;

    if (e.m_Type != WRenderGraphRenderEvent::Type::AfterGraphExecution)
      return;

    WRenderGraphManager::GetExecutionSummary(s_LastSummary);
    s_bSummaryDirty = true;

    if (s_uiRequestedInfoGraph != 0)
    {
      s_bLastInfoValid = WRenderGraphManager::GetRenderGraphInspectionInfo(s_uiRequestedInfoGraph, s_LastInfo).Succeeded();
      if (!s_bLastInfoValid)
      {
        s_LastInfo.Clear();
      }

      s_uiLastInfoGraph = s_uiRequestedInfoGraph;
      s_bInfoDirty = true;
    }
  }

  static void SendObserverResponse()
  {
    if (s_pObserver == nullptr)
      return;

    WTelemetryMessage msg;
    msg.SetMessageID(s_uiSystemId, 'RESP');
    msg.GetWriter() << s_uiProtocolVersion;
    msg.GetWriter() << s_pObserver->GetResponse();
    WTelemetry::Broadcast(WTelemetry::Reliable, msg);
  }

  static void ClearObserverRequest()
  {
    if (s_pObserver == nullptr)
      return;

    WRenderGraphObserverRequest request;
    s_pObserver->SetRequest(request);
    s_bObserverResponseEnabled = false;
  }

  static void HandleMessages(void*)
  {
    WTelemetryMessage msg;
    while (WTelemetry::RetrieveMessage(s_uiSystemId, msg) == W_SUCCESS)
    {
      WUInt16 uiVersion = 0;

      switch (msg.GetMessageID())
      {
        case 'RSUM':
          SendSummary();
          break;

        case 'RINF':
        {
          WUInt64 uiGraphIdentity = 0;
          msg.GetReader() >> uiVersion;
          msg.GetReader() >> uiGraphIdentity;
          s_uiRequestedInfoGraph = uiGraphIdentity;
          SendInspectionInfo(uiGraphIdentity);
        }
        break;

        case 'RREQ':
        {
          WRenderGraphObserverRequest request;
          msg.GetReader() >> uiVersion;
          msg.GetReader() >> request;

          EnsureObserver();
          s_pObserver->SetRequest(request);
          s_bObserverResponseEnabled = request.m_uiRenderGraphId != 0 && !request.m_sPassName.IsEmpty();
        }
        break;

        case 'RCLR':
          ClearObserverRequest();
          break;

        default:
          break;
      }
    }
  }

  static void TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case WTelemetry::TelemetryEventData::ConnectedToClient:
        s_bSummaryDirty = true;
        break;

      case WTelemetry::TelemetryEventData::DisconnectedFromClient:
        s_pObserver = nullptr;
        s_bObserverResponseEnabled = false;
        break;

      case WTelemetry::TelemetryEventData::PerFrameUpdate:
      {
        if (!WTelemetry::IsConnectedToClient())
          return;

        const WTime now = WTime::Now();
        if (s_bSummaryDirty && (now - s_LastSummaryBroadcast > WTime::MakeFromSeconds(0.5)))
        {
          s_LastSummaryBroadcast = now;
          SendSummary();
        }

        if (s_bInfoDirty)
        {
          SendInspectionInfo(s_uiLastInfoGraph);
        }

        if (s_bObserverResponseEnabled && (now - s_LastResponseBroadcast > WTime::MakeFromSeconds(0.1)))
        {
          s_LastResponseBroadcast = now;
          SendObserverResponse();
        }
      }
      break;

      default:
        break;
    }
  }
} // namespace RenderGraphObserverDetail

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(InspectorPlugin, RenderGraphObserver)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "RenderGraphManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    RenderGraphObserverDetail::s_pObserver = nullptr;
    RenderGraphObserverDetail::s_bObserverResponseEnabled = false;
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

void AddRenderGraphEventHandler()
{
  WTelemetry::AcceptMessagesForSystem(RenderGraphObserverDetail::s_uiSystemId, true, RenderGraphObserverDetail::HandleMessages, nullptr);
  WTelemetry::AddEventHandler(RenderGraphObserverDetail::TelemetryEventsHandler);
  WRenderGraphManager::s_RenderEvent.AddEventHandler(RenderGraphObserverDetail::RenderGraphRenderEventHandler);
}

void RemoveRenderGraphEventHandler()
{
  WRenderGraphManager::s_RenderEvent.RemoveEventHandler(RenderGraphObserverDetail::RenderGraphRenderEventHandler);
  WTelemetry::RemoveEventHandler(RenderGraphObserverDetail::TelemetryEventsHandler);
  WTelemetry::AcceptMessagesForSystem(RenderGraphObserverDetail::s_uiSystemId, false);
}


W_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_RenderGraphObserver);
