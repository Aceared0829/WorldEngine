#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

#include <Core/GameApplication/GameApplicationBase.h>

namespace MemoryDetail
{

  static void BroadcastMemoryStats()
  {
    WUInt64 uiTotalAllocations = 0;
    WUInt64 uiTotalPerFrameAllocationSize = 0;
    WTime TotalPerFrameAllocationTime;

    {
      WTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'BGN');
      WTelemetry::Broadcast(WTelemetry::Unreliable, msg);
    }

    for (auto it = WMemoryTracker::GetIterator(); it.IsValid(); ++it)
    {
      WTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'STAT');
      msg.GetWriter() << it.Id().m_Data;
      msg.GetWriter() << it.Name();
      msg.GetWriter() << (it.ParentId().IsInvalidated() ? WInvalidIndex : it.ParentId().m_Data);
      msg.GetWriter() << it.Stats();

      uiTotalAllocations += it.Stats().m_uiNumAllocations;
      uiTotalPerFrameAllocationSize += it.Stats().m_uiPerFrameAllocationSize;
      TotalPerFrameAllocationTime += it.Stats().m_PerFrameAllocationTime;

      WTelemetry::Broadcast(WTelemetry::Unreliable, msg);
    }

    {
      WTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'END');
      WTelemetry::Broadcast(WTelemetry::Unreliable, msg);
    }

    static WUInt64 uiLastTotalAllocations = 0;

    WStats::SetStat("App/Allocs Per Frame", uiTotalAllocations - uiLastTotalAllocations);
    WStats::SetStat("App/Per Frame Alloc Size (byte)", uiTotalPerFrameAllocationSize);
    WStats::SetStat("App/Per Frame Alloc Time", TotalPerFrameAllocationTime);

    uiLastTotalAllocations = uiTotalAllocations;

    WMemoryTracker::ResetPerFrameAllocatorStats();
  }

  static void PerframeUpdateHandler(const WGameApplicationExecutionEvent& e)
  {
    if (!WTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case WGameApplicationExecutionEvent::Type::AfterPresent:
        BroadcastMemoryStats();
        break;

      default:
        break;
    }
  }
} // namespace MemoryDetail


void AddMemoryEventHandler()
{
  // We're handling the per frame update by a different event since
  // using WTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the WStats and WTelemetry system.
  if (WGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}

void RemoveMemoryEventHandler()
{
  if (WGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}


