#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Utilities/ConversionUtils.h>

WRemoteInterface::~WRemoteInterface()
{
  // unfortunately we cannot do that ourselves here, because ShutdownConnection() calls virtual functions
  // and this object is already partially destructed here (derived class is already shut down)
  W_ASSERT_DEV(m_RemoteMode == WRemoteMode::None, "WRemoteInterface::ShutdownConnection() has to be called before destroying the interface");
}

WResult WRemoteInterface::CreateConnection(WUInt32 uiConnectionToken, WRemoteMode mode, WStringView sServerAddress, bool bStartUpdateThread)
{
  WUInt32 uiPrevID = m_uiApplicationID;
  ShutdownConnection();
  m_uiApplicationID = uiPrevID;

  W_LOCK(GetMutex());

  m_uiConnectionToken = uiConnectionToken;
  m_sServerAddress = sServerAddress;

  if (m_uiApplicationID == 0)
  {
    // create a 'unique' ID to identify this application
    m_uiApplicationID = (WUInt32)WTime::Now().GetSeconds();
  }

  if (InternalCreateConnection(mode, sServerAddress).Failed())
  {
    ShutdownConnection();
    return W_FAILURE;
  }

  m_RemoteMode = mode;

  UpdateRemoteInterface();

  if (bStartUpdateThread)
  {
    StartUpdateThread();
  }

  return W_SUCCESS;
}

WResult WRemoteInterface::StartServer(WUInt32 uiConnectionToken, WStringView sAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, WRemoteMode::Server, sAddress, bStartUpdateThread);
}

WResult WRemoteInterface::ConnectToServer(WUInt32 uiConnectionToken, WStringView sAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, WRemoteMode::Client, sAddress, bStartUpdateThread);
}

WResult WRemoteInterface::WaitForConnectionToServer(WTime timeout /*= WTime::MakeFromSeconds(10)*/)
{
  if (m_RemoteMode != WRemoteMode::Client)
    return W_FAILURE;

  const WTime tStart = WTime::Now();

  while (true)
  {
    UpdateRemoteInterface();

    if (IsConnectedToServer())
      return W_SUCCESS;

    if (timeout.GetSeconds() != 0)
    {
      if (WTime::Now() - tStart > timeout)
        return W_FAILURE;
    }

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
  }
}

void WRemoteInterface::ShutdownConnection()
{
  StopUpdateThread();

  W_LOCK(GetMutex());

  if (m_RemoteMode != WRemoteMode::None)
  {
    InternalShutdownConnection();

    m_RemoteMode = WRemoteMode::None;
    m_uiApplicationID = 0;
    m_uiConnectionToken = 0;
    m_uiConnectedToServerWithID = 0;
    m_iConnectionsToClients = 0;
  }
}

void WRemoteInterface::UpdatePingToServer()
{
  if (m_RemoteMode == WRemoteMode::Server)
  {
    W_LOCK(GetMutex());
    m_PingToServer = InternalGetPingToServer();
  }
}

void WRemoteInterface::UpdateRemoteInterface()
{
  W_LOCK(GetMutex());

  InternalUpdateRemoteInterface();
}

WResult WRemoteInterface::Transmit(WRemoteTransmitMode tm, const WArrayPtr<const WUInt8>& data)
{
  if (m_RemoteMode == WRemoteMode::None)
    return W_FAILURE;

  W_LOCK(GetMutex());

  if (InternalTransmit(tm, data).Failed())
    return W_FAILURE;

  // make sure the message is processed immediately
  UpdateRemoteInterface();

  return W_SUCCESS;
}


void WRemoteInterface::Send(WUInt32 uiSystemID, WUInt32 uiMsgID)
{
  Send(WRemoteTransmitMode::Reliable, uiSystemID, uiMsgID, WArrayPtr<const WUInt8>());
}

void WRemoteInterface::Send(WRemoteTransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const WArrayPtr<const WUInt8>& data)
{
  if (m_RemoteMode == WRemoteMode::None)
    return;

  // if (!IsConnectedToOther())
  //  return;

  m_TempSendBuffer.SetCountUninitialized(12 + data.GetCount());
  *((WUInt32*)&m_TempSendBuffer[0]) = m_uiApplicationID;
  *((WUInt32*)&m_TempSendBuffer[4]) = uiSystemID;
  *((WUInt32*)&m_TempSendBuffer[8]) = uiMsgID;

  if (!data.IsEmpty())
  {
    WUInt8* pCopyDst = &m_TempSendBuffer[12];
    WMemoryUtils::Copy(pCopyDst, data.GetPtr(), data.GetCount());
  }

  Transmit(tm, m_TempSendBuffer).IgnoreResult();
}

void WRemoteInterface::Send(WRemoteTransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const void* pData /*= nullptr*/, WUInt32 uiDataBytes /*= 0*/)
{
  Send(tm, uiSystemID, uiMsgID, WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(pData), uiDataBytes));
}

void WRemoteInterface::Send(WRemoteTransmitMode tm, WRemoteMessage& ref_msg)
{
  Send(tm, ref_msg.GetSystemID(), ref_msg.GetMessageID(), ref_msg.m_Storage);
}

void WRemoteInterface::Send(WRemoteTransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const WContiguousMemoryStreamStorage& data)
{
  if (m_RemoteMode == WRemoteMode::None)
    return;

  // if (!IsConnectedToOther())
  //  return;

  WArrayPtr<const WUInt8> range = {data.GetData(), data.GetStorageSize32()};

  m_TempSendBuffer.SetCountUninitialized(12 + range.GetCount());
  *((WUInt32*)&m_TempSendBuffer[0]) = m_uiApplicationID;
  *((WUInt32*)&m_TempSendBuffer[4]) = uiSystemID;
  *((WUInt32*)&m_TempSendBuffer[8]) = uiMsgID;

  if (!range.IsEmpty())
  {
    WUInt8* pCopyDst = &m_TempSendBuffer[12];
    WMemoryUtils::Copy(pCopyDst, range.GetPtr(), range.GetCount());
  }

  Transmit(tm, m_TempSendBuffer).IgnoreResult();
}

void WRemoteInterface::SetMessageHandler(WUInt32 uiSystemID, WRemoteMessageHandler messageHandler)
{
  m_MessageQueues[uiSystemID].m_MessageHandler = messageHandler;
}

void WRemoteInterface::SetUnhandledMessageHandler(WRemoteMessageHandler messageHandler)
{
  m_UnhandledMessageHandler = messageHandler;
}

WUInt32 WRemoteInterface::ExecuteMessageHandlers(WUInt32 uiSystem)
{
  W_LOCK(m_Mutex);

  return ExecuteMessageHandlersForQueue(m_MessageQueues[uiSystem]);
}

WUInt32 WRemoteInterface::ExecuteAllMessageHandlers()
{
  W_LOCK(m_Mutex);

  WUInt32 ret = 0;
  for (auto it = m_MessageQueues.GetIterator(); it.IsValid(); ++it)
  {
    ret += ExecuteMessageHandlersForQueue(it.Value());
  }

  return ret;
}

WUInt32 WRemoteInterface::ExecuteMessageHandlersForQueue(WRemoteMessageQueue& queue)
{
  queue.m_MessageQueueIn.Swap(queue.m_MessageQueueOut);
  const WUInt32 ret = queue.m_MessageQueueOut.GetCount();

  if (queue.m_MessageHandler.IsValid())
  {
    for (auto& msg : queue.m_MessageQueueOut)
    {
      queue.m_MessageHandler(msg);
    }
  }
  else if (m_UnhandledMessageHandler.IsValid())
  {
    for (auto& msg : queue.m_MessageQueueOut)
    {
      m_UnhandledMessageHandler(msg);
    }
  }

  queue.m_MessageQueueOut.Clear();

  return ret;
}

void WRemoteInterface::StartUpdateThread()
{
  StopUpdateThread();

  if (m_pUpdateThread == nullptr)
  {
    W_LOCK(m_Mutex);

    m_pUpdateThread = W_DEFAULT_NEW(WRemoteThread);
    m_pUpdateThread->m_pRemoteInterface = this;
    m_pUpdateThread->Start();
  }
}

void WRemoteInterface::StopUpdateThread()
{
  if (m_pUpdateThread != nullptr)
  {
    m_pUpdateThread->m_bKeepRunning = false;
    m_pUpdateThread->Join();

    W_LOCK(m_Mutex);
    W_DEFAULT_DELETE(m_pUpdateThread);
  }
}


void WRemoteInterface::ReportConnectionToServer(WUInt32 uiServerID)
{
  if (m_uiConnectedToServerWithID == uiServerID)
    return;

  m_uiConnectedToServerWithID = uiServerID;

  WRemoteEvent e;
  e.m_Type = WRemoteEvent::ConnectedToServer;
  e.m_uiOtherAppID = uiServerID;
  m_RemoteEvents.Broadcast(e);
}


void WRemoteInterface::ReportConnectionToClient(WUInt32 uiApplicationID)
{
  m_iConnectionsToClients++;

  WRemoteEvent e;
  e.m_Type = WRemoteEvent::ConnectedToClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}

void WRemoteInterface::ReportDisconnectedFromServer()
{
  m_uiConnectedToServerWithID = 0;

  WRemoteEvent e;
  e.m_Type = WRemoteEvent::DisconnectedFromServer;
  e.m_uiOtherAppID = m_uiConnectedToServerWithID;
  m_RemoteEvents.Broadcast(e);
}

void WRemoteInterface::ReportDisconnectedFromClient(WUInt32 uiApplicationID)
{
  m_iConnectionsToClients--;

  WRemoteEvent e;
  e.m_Type = WRemoteEvent::DisconnectedFromClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}


void WRemoteInterface::ReportMessage(WUInt32 uiApplicationID, WUInt32 uiSystemID, WUInt32 uiMsgID, const WArrayPtr<const WUInt8>& data)
{
  W_LOCK(m_Mutex);

  auto& queue = m_MessageQueues[uiSystemID];

  // store the data for later
  auto& msg = queue.m_MessageQueueIn.ExpandAndGetRef();
  msg.m_uiApplicationID = uiApplicationID;
  msg.SetMessageID(uiSystemID, uiMsgID);
  msg.GetWriter().WriteBytes(data.GetPtr(), data.GetCount()).IgnoreResult();
}

WResult WRemoteInterface::DetermineTargetAddress(WStringView sConnectTo0, WUInt32& out_IP, WUInt16& out_Port)
{
  out_IP = 0;
  out_Port = 0;

  WStringBuilder sConnectTo = sConnectTo0;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, WStringUtils::GetStringElementCount(szColon));

    WStringBuilder sPort = szColon + 1;

    WInt32 tmp;
    if (WConversionUtils::StringToInt(sPort, tmp).Succeeded())
      out_Port = static_cast<WUInt16>(tmp);
  }

  WInt32 ip1 = 0;
  WInt32 ip2 = 0;
  WInt32 ip3 = 0;
  WInt32 ip4 = 0;

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
  {
    ip1 = 127;
    ip2 = 0;
    ip3 = 0;
    ip4 = 1;
  }
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    WTempHybridArray<WString, 8> IP;
    sConnectTo.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return W_FAILURE;

    if (WConversionUtils::StringToInt(IP[0], ip1).Failed())
      return W_FAILURE;
    if (WConversionUtils::StringToInt(IP[1], ip2).Failed())
      return W_FAILURE;
    if (WConversionUtils::StringToInt(IP[2], ip3).Failed())
      return W_FAILURE;
    if (WConversionUtils::StringToInt(IP[3], ip4).Failed())
      return W_FAILURE;
  }
  else
  {
    return W_FAILURE;
  }

  out_IP = ((ip1 & 0xFF) | (ip2 & 0xFF) << 8 | (ip3 & 0xFF) << 16 | (ip4 & 0xFF) << 24);
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WRemoteThread::WRemoteThread()
  : WThread("WRemoteThread")
{
}

WUInt32 WRemoteThread::Run()
{
  WTime lastPing;

  while (m_bKeepRunning && m_pRemoteInterface)
  {
    m_pRemoteInterface->UpdateRemoteInterface();

    // Send a Ping every once in a while
    if (m_pRemoteInterface->GetRemoteMode() == WRemoteMode::Client)
    {
      WTime tNow = WTime::Now();

      if (tNow - lastPing > WTime::MakeFromMilliseconds(500))
      {
        lastPing = tNow;

        m_pRemoteInterface->UpdatePingToServer();
      }
    }

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
  }

  return 0;
}
