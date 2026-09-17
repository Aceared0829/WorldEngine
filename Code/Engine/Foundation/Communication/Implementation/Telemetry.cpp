#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
#  include <enet/enet.h>
#endif

class WTelemetryThread;

WTelemetry::WEventTelemetry WTelemetry::s_TelemetryEvents;
WUInt32 WTelemetry::s_uiApplicationID = 0;
WUInt32 WTelemetry::s_uiServerID = 0;
WUInt16 WTelemetry::s_uiPort = 1040;
bool WTelemetry::s_bConnectedToServer = false;
bool WTelemetry::s_bConnectedToClient = false;
bool WTelemetry::s_bAllowNetworkUpdate = true;
WTime WTelemetry::s_PingToServer;
WString WTelemetry::s_sServerName;
WString WTelemetry::s_sServerIP;
WTelemetry::ConnectionMode WTelemetry::s_ConnectionMode = WTelemetry::None;
WMap<WUInt64, WTelemetry::MessageQueue> WTelemetry::s_SystemMessages;

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
static bool g_bInitialized = false;
static ENetAddress g_pServerAddress;
static ENetHost* g_pHost = nullptr;
static ENetPeer* g_pConnectionToServer = nullptr;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

void WTelemetry::UpdateServerPing()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  enet_peer_ping(g_pConnectionToServer);
  WTelemetry::s_PingToServer = WTime::MakeFromMilliseconds(g_pConnectionToServer->lastRoundTripTime);
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::UpdateNetwork()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  if (!s_bAllowNetworkUpdate)
    return;

  s_bAllowNetworkUpdate = false;

  ENetEvent NetworkEvent;

  while (true)
  {
    W_LOCK(GetTelemetryMutex());

    const WInt32 iStatus = enet_host_service(g_pHost, &NetworkEvent, 0);

    if (iStatus <= 0)
    {
      s_bAllowNetworkUpdate = true;
      return;
    }

    switch (NetworkEvent.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
      {
        if ((WTelemetry::s_ConnectionMode == WTelemetry::Server) && (NetworkEvent.peer->eventData != 'EZBC'))
        {
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }

        if (s_ConnectionMode == Client)
        {
          char szHostIP[64] = "<unknown>";
          // char szHostName[64] = "<unknown>";

          enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);

          // Querying host IP and name can take a lot of time which can lead to timeouts
          // enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

          WTelemetry::s_sServerIP = szHostIP;
          // WTelemetry::s_ServerName = szHostName;

          // now we are waiting for the server to send its ID
        }
        else
        {
          // got a new client, send the server ID to it
          s_bConnectedToClient = true; // we need this fake state, otherwise Broadcast will queue the message instead of sending it
          Broadcast(WTelemetry::Reliable, 'EZBC', 'EZID', &s_uiApplicationID, sizeof(WUInt32));
          s_bConnectedToClient = false;

          // then wait for its acknowledgment message
        }
      }
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (s_ConnectionMode == Client)
        {
          s_bConnectedToServer = false;

          // First wait a bit to ensure that the Server could shut down, if this was a legitimate disconnect
          WThreadUtils::Sleep(WTime::MakeFromSeconds(1));

          // Now try to reconnect. If the Server still exists, fine, connect to that.
          // If it does not exist anymore, this will connect to the next best Server that can be found.
          g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'EZBC');

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromServer;

          s_TelemetryEvents.Broadcast(e);
        }
        else
        {
          /// \todo This assumes we only connect to a single client ...
          s_bConnectedToClient = false;

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromClient;

          s_TelemetryEvents.Broadcast(e);
        }
      }
      break;

      case ENET_EVENT_TYPE_RECEIVE:
      {
        const WUInt32 uiSystemID = *((WUInt32*)&NetworkEvent.packet->data[0]);
        const WUInt32 uiMsgID = *((WUInt32*)&NetworkEvent.packet->data[4]);
        const WUInt8* pData = &NetworkEvent.packet->data[8];

        if (uiSystemID == 'EZBC')
        {
          switch (uiMsgID)
          {
            case 'EZID':
            {
              s_uiServerID = *((WUInt32*)pData);

              // connection to server is finalized
              s_bConnectedToServer = true;

              // acknowledge that the ID has been received
              SendToServer('EZBC', 'AKID', nullptr, 0);

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToServer;

              s_TelemetryEvents.Broadcast(e);

              FlushOutgoingQueues();
            }
            break;
            case 'AKID':
            {
              // the client received the server ID -> the connection has been established properly

              /// \todo This assumes we only connect to a single client ...
              s_bConnectedToClient = true;

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToClient;

              s_TelemetryEvents.Broadcast(e);

              SendServerName();
              FlushOutgoingQueues();
            }
            break;

            case 'NAME':
            {
              s_sServerName = reinterpret_cast<const char*>(pData);
            }
            break;
          }
        }
        else
        {
          MessageQueue& Queue = s_SystemMessages[uiSystemID];

          if (Queue.m_bAcceptMessages)
          {
            Queue.m_IncomingQueue.PushBack();
            WTelemetryMessage& Msg = Queue.m_IncomingQueue.PeekBack();

            Msg.SetMessageID(uiSystemID, uiMsgID);

            W_ASSERT_DEV((WUInt32)NetworkEvent.packet->dataLength >= 8, "Message Length Invalid: {0}", (WUInt32)NetworkEvent.packet->dataLength);

            Msg.GetWriter().WriteBytes(pData, NetworkEvent.packet->dataLength - 8).IgnoreResult();
          }
        }

        enet_packet_destroy(NetworkEvent.packet);
      }
      break;

      default:
        break;
    }
  }

  s_bAllowNetworkUpdate = true;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::SetServerName(WStringView sName)
{
  if (s_ConnectionMode == ConnectionMode::Client)
    return;

  if (s_sServerName == sName)
    return;

  s_sServerName = sName;

  SendServerName();
}

void WTelemetry::SendServerName()
{
  if (!IsConnectedToOther())
    return;

  char data[48];
  WStringUtils::Copy(data, W_ARRAY_SIZE(data), s_sServerName.GetData());

  Broadcast(WTelemetry::Reliable, 'EZBC', 'NAME', data, W_ARRAY_SIZE(data));
}

WResult WTelemetry::RetrieveMessage(WUInt32 uiSystemID, WTelemetryMessage& out_message)
{
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return W_FAILURE;

  W_LOCK(GetTelemetryMutex());

  // check again while inside the lock
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return W_FAILURE;

  out_message = s_SystemMessages[uiSystemID].m_IncomingQueue.PeekFront();
  s_SystemMessages[uiSystemID].m_IncomingQueue.PopFront();

  return W_SUCCESS;
}

void WTelemetry::InitializeAsServer()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pServerAddress.host = ENET_HOST_ANY;
  g_pServerAddress.port = s_uiPort;

  g_pHost = enet_host_create(&g_pServerAddress, 32, 2, 0, 0);
#else
  WLog::SeriousWarning("Enet is not compiled into this build, WTelemetry::InitializeAsServer() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

WResult WTelemetry::InitializeAsClient(WStringView sConnectTo0)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pHost = enet_host_create(nullptr, 1, 2, 0, 0);

  WStringBuilder sConnectTo = sConnectTo0;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, WStringUtils::GetStringElementCount(szColon));

    WStringBuilder sPort = szColon + 1;
    s_uiPort = static_cast<WUInt16>(atoi(sPort.GetData()));
  }

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
    enet_address_set_host(&g_pServerAddress, "localhost");
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    WTempHybridArray<WString, 8> IP;
    sConnectTo.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return W_FAILURE;

    const WUInt32 ip1 = atoi(IP[0].GetData()) & 0xFF;
    const WUInt32 ip2 = atoi(IP[1].GetData()) & 0xFF;
    const WUInt32 ip3 = atoi(IP[2].GetData()) & 0xFF;
    const WUInt32 ip4 = atoi(IP[3].GetData()) & 0xFF;

    const WUInt32 uiIP = (ip1 | ip2 << 8 | ip3 << 16 | ip4 << 24);

    g_pServerAddress.host = uiIP;
  }
  else
    enet_address_set_host(&g_pServerAddress, sConnectTo.GetData());

  g_pServerAddress.port = s_uiPort;

  g_pConnectionToServer = nullptr;
  g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'EZBC');

  if (g_pConnectionToServer)
    return W_SUCCESS;
#else
  W_IGNORE_UNUSED(sConnectTo0);
  WLog::SeriousWarning("Enet is not compiled into this build, WTelemetry::InitializeAsClient() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

  return W_FAILURE;
}

WResult WTelemetry::OpenConnection(ConnectionMode Mode, WStringView sConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  CloseConnection();

  if (!g_bInitialized)
  {
    if (enet_initialize() != 0)
    {
      WLog::Error("Enet could not be initialized.");
      return W_FAILURE;
    }

    g_bInitialized = true;
  }

  s_uiApplicationID = (WUInt32)WTime::Now().GetSeconds();

  switch (Mode)
  {
    case WTelemetry::Server:
      InitializeAsServer();
      break;
    case WTelemetry::Client:
      if (InitializeAsClient(sConnectTo) == W_FAILURE)
      {
        CloseConnection();
        return W_FAILURE;
      }
      break;
    default:
      break;
  }

  s_ConnectionMode = Mode;

  WTelemetry::UpdateNetwork();

  StartTelemetryThread();

  return W_SUCCESS;
#else
  W_IGNORE_UNUSED(Mode);
  W_IGNORE_UNUSED(sConnectTo);
  WLog::SeriousWarning("Enet is not compiled into this build, WTelemetry::OpenConnection() will be ignored.");
  return W_FAILURE;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::Transmit(TransmitMode tm, const void* pData, WUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  W_LOCK(GetTelemetryMutex());

  ENetPacket* pPacket = enet_packet_create(pData, uiDataBytes, (tm == Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(g_pHost, 0, pPacket);

  // make sure the message is processed immediately
  WTelemetry::UpdateNetwork();
#else
  W_IGNORE_UNUSED(tm);
  W_IGNORE_UNUSED(pData);
  W_IGNORE_UNUSED(uiDataBytes);
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::Send(TransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const void* pData, WUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
    QueueOutgoingMessage(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
  else
  {
    // when we do have a connection, just send the message out

    WTempHybridArray<WUInt8, 64> TempData;
    TempData.SetCountUninitialized(8 + uiDataBytes);
    *((WUInt32*)&TempData[0]) = uiSystemID;
    *((WUInt32*)&TempData[4]) = uiMsgID;

    if (pData && uiDataBytes > 0)
      WMemoryUtils::Copy((WUInt8*)&TempData[8], (WUInt8*)pData, uiDataBytes);

    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#else
  W_IGNORE_UNUSED(tm);
  W_IGNORE_UNUSED(uiSystemID);
  W_IGNORE_UNUSED(uiMsgID);
  W_IGNORE_UNUSED(pData);
  W_IGNORE_UNUSED(uiDataBytes);
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::Send(TransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, WStreamReader& Stream, WInt32 iDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  const WUInt32 uiStackSize = 1024;

  WTempHybridArray<WUInt8, uiStackSize + 8> TempData;
  TempData.SetCountUninitialized(8);
  *((WUInt32*)&TempData[0]) = uiSystemID;
  *((WUInt32*)&TempData[4]) = uiMsgID;

  // if we don't know how much to take out of the stream, read the data piece by piece from the input stream
  if (iDataBytes < 0)
  {
    while (true)
    {
      const WUInt32 uiOffset = TempData.GetCount();
      TempData.SetCountUninitialized(uiOffset + uiStackSize); // no allocation the first time

      const WUInt32 uiRead = static_cast<WUInt32>(Stream.ReadBytes(&TempData[uiOffset], uiStackSize));

      if (uiRead < uiStackSize)
      {
        // resize the array down to its actual size
        TempData.SetCountUninitialized(uiOffset + uiRead);
        break;
      }
    }
  }
  else
  {
    TempData.SetCountUninitialized(8 + iDataBytes);

    if (iDataBytes > 0)
      Stream.ReadBytes(&TempData[8], iDataBytes);
  }

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
  {
    if (TempData.GetCount() > 8)
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, &TempData[8], TempData.GetCount() - 8);
    else
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, nullptr, 0);
  }
  else
  {
    // when we do have a connection, just send the message out
    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#else
  W_IGNORE_UNUSED(tm);
  W_IGNORE_UNUSED(uiSystemID);
  W_IGNORE_UNUSED(uiMsgID);
  W_IGNORE_UNUSED(Stream);
  W_IGNORE_UNUSED(iDataBytes);
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void WTelemetry::CloseConnection()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  s_ConnectionMode = None;
  s_uiServerID = 0;
  g_pConnectionToServer = nullptr;

  StopTelemetryThread();

  // prevent other threads from interfering
  W_LOCK(GetTelemetryMutex());

  UpdateNetwork();
  WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

  if (g_pHost)
  {
    // send all peers that we are disconnecting
    for (WUInt32 i = (WUInt32)g_pHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&g_pHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateNetwork();
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
  }

  {
    // Fire disconnect event.
    if (s_bConnectedToClient)
    {
      TelemetryEventData e;
      e.m_EventType = TelemetryEventData::DisconnectedFromClient;
      s_TelemetryEvents.Broadcast(e);
      s_bConnectedToClient = false;
    }

    if (s_bConnectedToServer)
    {
      TelemetryEventData e;
      e.m_EventType = TelemetryEventData::DisconnectedFromServer;
      s_TelemetryEvents.Broadcast(e);
      s_bConnectedToServer = false;
    }
  }
  // finally close the network connection
  if (g_pHost)
  {
    enet_host_destroy(g_pHost);
    g_pHost = nullptr;
  }

  if (g_bInitialized)
  {
    enet_deinitialize();
    g_bInitialized = false;
  }

  // if there are any queued messages, throw them away
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_IncomingQueue.Clear();
    it.Value().m_OutgoingQueue.Clear();
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}
