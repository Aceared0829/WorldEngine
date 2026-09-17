#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Profiling/Profiling.h>

void WTelemetry::QueueOutgoingMessage(TransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const void* pData, WUInt32 uiDataBytes)
{
  // unreliable packages can just be dropped
  if (tm == WTelemetry::Unreliable)
    return;

  W_LOCK(GetTelemetryMutex());

  // add a new message to the queue
  MessageQueue& Queue = s_SystemMessages[uiSystemID];
  Queue.m_OutgoingQueue.PushBack();

  // and fill it out properly
  WTelemetryMessage& msg = Queue.m_OutgoingQueue.PeekBack();
  msg.SetMessageID(uiSystemID, uiMsgID);

  if (uiDataBytes > 0)
  {
    msg.GetWriter().WriteBytes(pData, uiDataBytes).IgnoreResult();
  }

  // if our outgoing queue has grown too large, dismiss older messages
  if (Queue.m_OutgoingQueue.GetCount() > Queue.m_uiMaxQueuedOutgoing)
    Queue.m_OutgoingQueue.PopFront(Queue.m_OutgoingQueue.GetCount() - Queue.m_uiMaxQueuedOutgoing);
}

void WTelemetry::FlushOutgoingQueues()
{
  static bool bRecursion = false;

  if (bRecursion)
    return;

  // if there is no connection to anyone (yet), don't do anything
  if (!IsConnectedToOther())
    return;

  bRecursion = true;

  W_LOCK(GetTelemetryMutex());

  // go through all system types
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_OutgoingQueue.IsEmpty())
      continue;

    const WUInt32 uiCurCount = it.Value().m_OutgoingQueue.GetCount();

    // send all messages that are queued for this system
    for (WUInt32 i = 0; i < uiCurCount; ++i)
      Send(WTelemetry::Reliable, it.Value().m_OutgoingQueue[i]); // Send() will already update the network

    // check that they have not been queue again
    W_ASSERT_DEV(it.Value().m_OutgoingQueue.GetCount() == uiCurCount, "Implementation Error: When queued messages are flushed, they should not get queued again.");

    it.Value().m_OutgoingQueue.Clear();
  }

  bRecursion = false;
}


WResult WTelemetry::ConnectToServer(WStringView sConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return OpenConnection(Client, sConnectTo);
#else
  W_IGNORE_UNUSED(sConnectTo);
  WLog::SeriousWarning("Enet is not compiled into this build, WTelemetry::ConnectToServer() will be ignored.");
  return W_FAILURE;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::CreateServer()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (OpenConnection(Server).Failed())
  {
    WLog::Error("WTelemetry: Failed to open a connection as a server.");
    s_ConnectionMode = ConnectionMode::None;
  }
#else
  WLog::SeriousWarning("Enet is not compiled into this build, WTelemetry::CreateServer() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::AcceptMessagesForSystem(WUInt32 uiSystemID, bool bAccept, ProcessMessagesCallback callback, void* pPassThrough)
{
  W_LOCK(GetTelemetryMutex());

  s_SystemMessages[uiSystemID].m_bAcceptMessages = bAccept;
  s_SystemMessages[uiSystemID].m_Callback = callback;
  s_SystemMessages[uiSystemID].m_pPassThrough = pPassThrough;
}

void WTelemetry::PerFrameUpdate()
{
  W_PROFILE_SCOPE("Telemetry.PerFrameUpdate");
  W_LOCK(GetTelemetryMutex());

  // Call each callback to process the incoming messages
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_IncomingQueue.IsEmpty() && it.Value().m_Callback)
      it.Value().m_Callback(it.Value().m_pPassThrough);
  }

  TelemetryEventData e;
  e.m_EventType = TelemetryEventData::PerFrameUpdate;

  const bool bAllowUpdate = s_bAllowNetworkUpdate;
  s_bAllowNetworkUpdate = false;
  s_TelemetryEvents.Broadcast(e);
  s_bAllowNetworkUpdate = bAllowUpdate;
}

void WTelemetry::SetOutgoingQueueSize(WUInt32 uiSystemID, WUInt16 uiMaxQueued)
{
  W_LOCK(GetTelemetryMutex());

  s_SystemMessages[uiSystemID].m_uiMaxQueuedOutgoing = uiMaxQueued;
}


bool WTelemetry::IsConnectedToOther()
{
  return ((s_ConnectionMode == Client && IsConnectedToServer()) || (s_ConnectionMode == Server && IsConnectedToClient()));
}

void WTelemetry::Broadcast(TransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const void* pData, WUInt32 uiDataBytes)
{
  if (s_ConnectionMode != WTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void WTelemetry::Broadcast(TransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, WStreamReader& inout_stream, WInt32 iDataBytes)
{
  if (s_ConnectionMode != WTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, inout_stream, iDataBytes);
}

void WTelemetry::Broadcast(TransmitMode tm, WTelemetryMessage& ref_msg)
{
  if (s_ConnectionMode != WTelemetry::Server)
    return;

  Send(tm, ref_msg);
}

void WTelemetry::SendToServer(WUInt32 uiSystemID, WUInt32 uiMsgID, const void* pData, WUInt32 uiDataBytes)
{
  if (s_ConnectionMode != WTelemetry::Client)
    return;

  Send(WTelemetry::Reliable, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void WTelemetry::SendToServer(WUInt32 uiSystemID, WUInt32 uiMsgID, WStreamReader& inout_stream, WInt32 iDataBytes)
{
  if (s_ConnectionMode != WTelemetry::Client)
    return;

  Send(WTelemetry::Reliable, uiSystemID, uiMsgID, inout_stream, iDataBytes);
}

void WTelemetry::SendToServer(WTelemetryMessage& ref_msg)
{
  if (s_ConnectionMode != WTelemetry::Client)
    return;

  Send(WTelemetry::Reliable, ref_msg);
}

void WTelemetry::Send(TransmitMode tm, WTelemetryMessage& msg)
{
  Send(tm, msg.GetSystemID(), msg.GetMessageID(), msg.GetReader(), (WInt32)msg.m_Storage.GetStorageSize32());
}
