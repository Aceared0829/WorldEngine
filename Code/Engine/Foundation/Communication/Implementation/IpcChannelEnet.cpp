#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Communication/RemoteInterfaceEnet.h>
#  include <Foundation/Communication/RemoteMessage.h>
#  include <Foundation/Logging/Log.h>

WIpcChannelEnet::WIpcChannelEnet(WStringView sAddress, Mode::Enum mode)
  : WIpcChannel(sAddress, mode)
  , m_sAddress(sAddress)
{
  m_pNetwork = WRemoteInterfaceEnet::Make();
  m_pNetwork->SetMessageHandler(0, WMakeDelegate(&WIpcChannelEnet::NetworkMessageHandler, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(WMakeDelegate(&WIpcChannelEnet::EnetEventHandler, this));

  m_pOwner->AddChannel(this);
}

WIpcChannelEnet::~WIpcChannelEnet()
{
  m_pNetwork->m_RemoteEvents.RemoveEventHandler(WMakeDelegate(&WIpcChannelEnet::EnetEventHandler, this));
  m_pNetwork->ShutdownConnection();

  m_pOwner->RemoveChannel(this);
}

void WIpcChannelEnet::InternalConnect()
{
  if (GetConnectionState() != ConnectionState::Connecting)
    return;

  if (m_Mode == Mode::Server)
  {
    if (m_pNetwork->StartServer('RMOT', m_sAddress, false).Failed())
    {
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
  }
  else
  {
    if (m_pNetwork->ConnectToServer('RMOT', m_sAddress, false).Failed())
    {
      SetConnectionState(ConnectionState::Disconnected);
      return;
    }
    m_LastConnectAttempt = WTime::Now();
  }
}

void WIpcChannelEnet::InternalDisconnect()
{
  m_pNetwork->ShutdownConnection();
  SetConnectionState(ConnectionState::Disconnected);
}

void WIpcChannelEnet::InternalSend()
{
  {
    W_LOCK(m_OutputQueueMutex);

    while (!m_OutputQueue.IsEmpty())
    {
      WContiguousMemoryStreamStorage& storage = m_OutputQueue.PeekFront();

      m_pNetwork->Send(WRemoteTransmitMode::Reliable, 0, 0, storage);

      m_OutputQueue.PopFront();
    }
  }

  m_pNetwork->UpdateRemoteInterface();
}

bool WIpcChannelEnet::NeedWakeup() const
{
  return true;
}

void WIpcChannelEnet::Tick()
{
  m_pNetwork->UpdateRemoteInterface();

  if (GetConnectionState() == ConnectionState::Connecting)
  {
    if (m_pNetwork->IsConnectedToOther())
    {
      m_LastConnectAttempt = WTime::MakeZero();
      SetConnectionState(ConnectionState::Connected);
    }
    else if (m_Mode == Mode::Client && !m_LastConnectAttempt.IsZero() && WTime::Now() - m_LastConnectAttempt > WTime::MakeFromSeconds(2))
    {
      m_LastConnectAttempt = WTime::MakeZero();
      SetConnectionState(ConnectionState::Disconnected);
    }
  }

  if (!m_pNetwork->IsConnectedToOther())
  {
    if (GetConnectionState() == ConnectionState::Connected)
    {
      SetConnectionState(ConnectionState::Disconnected);
    }
  }
  m_pNetwork->ExecuteAllMessageHandlers();
}

void WIpcChannelEnet::NetworkMessageHandler(WRemoteMessage& msg)
{
  ReceiveData(msg.GetMessageData());
}

void WIpcChannelEnet::EnetEventHandler(const WRemoteEvent& e)
{
  if (e.m_Type == WRemoteEvent::ConnectedToClient)
  {
    SetConnectionState(ConnectionState::Connected);
  }

  if (e.m_Type == WRemoteEvent::DisconnectedFromServer)
  {
    Disconnect();
  }

  if (e.m_Type == WRemoteEvent::DisconnectedFromClient)
  {
    SetConnectionState(ConnectionState::Disconnected);
  }

  if (e.m_Type == WRemoteEvent::ConnectedToServer)
  {
    m_LastConnectAttempt = WTime::MakeZero();
    SetConnectionState(ConnectionState::Connected);
  }
}

#endif
