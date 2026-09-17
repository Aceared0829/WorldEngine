#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/Log.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WGlobalEvent);

WGlobalEvent::EventMap WGlobalEvent::s_KnownEvents;

WGlobalEvent::EventData::EventData()
{
  m_uiNumTimesFired = 0;
  m_uiNumEventHandlersOnce = 0;
  m_uiNumEventHandlersRegular = 0;
}

WGlobalEvent::WGlobalEvent(WStringView sEventName, W_GLOBAL_EVENT_HANDLER handler, bool bOnlyOnce)
{
  m_sEventName = sEventName;
  m_bOnlyOnce = bOnlyOnce;
  m_bHasBeenFired = false;
  m_EventHandler = handler;
}

void WGlobalEvent::Broadcast(WStringView sEventName, WVariant p1, WVariant p2, WVariant p3, WVariant p4)
{
  WGlobalEvent* pHandler = WGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    if (pHandler->m_sEventName == sEventName)
    {
      if (!pHandler->m_bOnlyOnce || !pHandler->m_bHasBeenFired)
      {
        pHandler->m_bHasBeenFired = true;

        pHandler->m_EventHandler(p1, p2, p3, p4);
      }
    }

    pHandler = pHandler->GetNextInstance();
  }


  EventData& ed = s_KnownEvents[sEventName]; // this will make sure to record all fired events, even if there are no handlers for them
  ed.m_uiNumTimesFired++;
}

void WGlobalEvent::UpdateGlobalEventStatistics()
{
  for (EventMap::Iterator it = s_KnownEvents.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_uiNumEventHandlersRegular = 0;
    it.Value().m_uiNumEventHandlersOnce = 0;
  }

  WGlobalEvent* pHandler = WGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    EventData& ed = s_KnownEvents[pHandler->m_sEventName];

    if (pHandler->m_bOnlyOnce)
      ++ed.m_uiNumEventHandlersOnce;
    else
      ++ed.m_uiNumEventHandlersRegular;

    pHandler = pHandler->GetNextInstance();
  }
}

void WGlobalEvent::PrintGlobalEventStatistics()
{
  UpdateGlobalEventStatistics();

  W_LOG_BLOCK("Global Event Statistics");

  EventMap::Iterator it = s_KnownEvents.GetIterator();

  while (it.IsValid())
  {
    WLog::Info("Event: '{0}', Num Handlers Regular / Once: {1} / {2}, Num Times Fired: {3}", it.Key(), it.Value().m_uiNumEventHandlersRegular,
      it.Value().m_uiNumEventHandlersOnce, it.Value().m_uiNumTimesFired);

    ++it;
  }
}
