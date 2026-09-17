#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>

WProcessCommunicationChannel::WProcessCommunicationChannel() = default;

WProcessCommunicationChannel::~WProcessCommunicationChannel()
{
  m_pProtocol.Clear();
  m_pChannel.Clear();
}

bool WProcessCommunicationChannel::SendMessage(WProcessMessage* pMessage)
{
  if (m_pFirstAllowedMessageType != nullptr)
  {
    // ignore all messages that are not the first allowed message
    // this is necessary to make sure that during an engine restart we don't accidentally send stray messages while
    // the engine is not yet correctly set up
    if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
    {
      WLog::Warning("[IPC]Ignored send message of type {} because it is not a {}", pMessage->GetDynamicRTTI()->GetTypeName(), m_pFirstAllowedMessageType->GetTypeName());
      return false;
    }

    m_pFirstAllowedMessageType = nullptr;
  }

  {
    if (m_pProtocol == nullptr)
      return false;

    return m_pProtocol->Send(pMessage);
  }
}

bool WProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pProtocol)
    return false;

  return m_pProtocol->ProcessMessages();
}


void WProcessCommunicationChannel::WaitForMessages(WTime timeout /*= WTime::MakeZero()*/)
{
  if (!m_pProtocol)
    return;

  m_pProtocol->WaitForMessages(timeout).IgnoreResult();
}

void WProcessCommunicationChannel::OnIpcProtocolEvent(const WIpcProcessMessageProtocol::Event& msg)
{
  const WProcessMessage* pMsg = msg.m_pMessage;
  const WRTTI* pRtti = pMsg->GetDynamicRTTI();

  if (m_pWaitForMessageType != nullptr && pMsg->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
  {
    if (m_WaitForMessageCallback.IsValid())
    {
      if (m_WaitForMessageCallback(const_cast<WProcessMessage*>(pMsg)))
      {
        m_WaitForMessageCallback = WaitForMessageCallback();
        m_pWaitForMessageType = nullptr;
      }
    }
    else
    {
      m_pWaitForMessageType = nullptr;
    }
  }

  W_ASSERT_DEV(pRtti != nullptr, "Message Type unknown");
  W_ASSERT_DEV(pMsg != nullptr, "Object could not be allocated");
  W_ASSERT_DEV(pRtti->IsDerivedFrom<WProcessMessage>(), "Msg base type is invalid");

  Event e;
  e.m_pMessage = pMsg;
  e.m_bInterruptMessageProcessing = msg.m_bInterruptMessageProcessing;
  m_Events.Broadcast(e);
  msg.m_bInterruptMessageProcessing = e.m_bInterruptMessageProcessing;
}

void WProcessCommunicationChannel::OnIpcChannelEvent(const WIpcChannelEvent& msg)
{
  m_IpcChannelEvents.Broadcast(msg);
}

WResult WProcessCommunicationChannel::CreateAndConnectChannel(WInternal::NewInstance<WIpcChannel>&& channel)
{
  W_ASSERT_DEBUG(m_pChannel == nullptr, "Channel already created");
  m_pChannel = channel;
  m_pProtocol = W_DEFAULT_NEW(WIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(WMakeDelegate(&WProcessCommunicationChannel::OnIpcProtocolEvent, this));
  m_pChannel->m_Events.AddEventHandler(WMakeDelegate(&WProcessCommunicationChannel::OnIpcChannelEvent, this));
  return m_pChannel->Connect();
}

void WProcessCommunicationChannel::DestroyChannel()
{
  if (m_pProtocol)
  {
    m_pProtocol->m_MessageEvent.RemoveEventHandler(WMakeDelegate(&WProcessCommunicationChannel::OnIpcProtocolEvent, this));
    m_pProtocol.Clear();
  }
  if (m_pChannel)
  {
    m_pChannel->m_Events.RemoveEventHandler(WMakeDelegate(&WProcessCommunicationChannel::OnIpcChannelEvent, this));
    m_pChannel.Clear();
  }
}

WResult WProcessCommunicationChannel::WaitForMessage(const WRTTI* pMessageType, WTime timeout, WaitForMessageCallback* pMessageCallack)
{
  W_ASSERT_DEV(m_pProtocol != nullptr && m_pChannel != nullptr, "Need to connect first before waiting for a message.");
  // W_ASSERT_DEV(WThreadUtils::IsMainThread(), "This function is not thread safe");
  W_ASSERT_DEV(m_pWaitForMessageType == nullptr, "Already waiting for another message!");

  m_pWaitForMessageType = pMessageType;
  if (pMessageCallack)
  {
    m_WaitForMessageCallback = *pMessageCallack;
  }
  else
  {
    m_WaitForMessageCallback = WaitForMessageCallback();
  }

  W_SCOPE_EXIT(m_WaitForMessageCallback = WaitForMessageCallback(););

  const WTime tStart = WTime::Now();

  while (m_pWaitForMessageType != nullptr)
  {
    if (timeout == WTime())
    {
      m_pProtocol->WaitForMessages().IgnoreResult();
    }
    else
    {
      WTime tTimeLeft = timeout - (WTime::Now() - tStart);

      if (tTimeLeft < WTime::MakeZero())
      {
        // Don't time out if a debugger is attached to make stepping easier.
        if (WSystemInformation::IsDebuggerAttached())
        {
          tTimeLeft = WTime::MakeFromSeconds(1);
        }
        else
        {
          m_pWaitForMessageType = nullptr;
          WLog::Dev("Reached time-out of {0} seconds while waiting for {1}", WArgF(timeout.GetSeconds(), 1), pMessageType->GetTypeName());
          return W_FAILURE;
        }
      }

      m_pProtocol->WaitForMessages(tTimeLeft).IgnoreResult();
    }

    if (!m_pChannel->IsConnected())
    {
      m_pWaitForMessageType = nullptr;
      WLog::Dev("Lost connection while waiting for {}", pMessageType->GetTypeName());
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

WResult WProcessCommunicationChannel::WaitForConnection(WTime timeout)
{
  if (m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Connected)
  {
    return W_SUCCESS;
  }

  if (m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected)
  {
    return W_FAILURE;
  }

  WThreadSignal waitForConnectionSignal;

  WEventSubscriptionID eventSubscriptionId = m_pChannel->m_Events.AddEventHandler([&](const WIpcChannelEvent& event)
    {
    switch (event.m_Type)
    {
      case WIpcChannelEvent::Disconnected:
      case WIpcChannelEvent::Connected:
        waitForConnectionSignal.RaiseSignal();
        break;
      default:
        break;
    } });

  W_SCOPE_EXIT(m_pChannel->m_Events.RemoveEventHandler(eventSubscriptionId));

  if (m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Connected)
  {
    return W_SUCCESS;
  }

  if (m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Disconnected)
  {
    return W_FAILURE;
  }

  if (timeout == WTime())
  {
    waitForConnectionSignal.WaitForSignal();
  }
  else
  {
    if (waitForConnectionSignal.WaitForSignal(timeout) == WThreadSignal::WaitResult::Timeout)
    {
      return W_FAILURE;
    }
  }

  return m_pChannel->IsConnected() ? W_SUCCESS : W_FAILURE;
}

bool WProcessCommunicationChannel::IsConnected() const
{
  if (!m_pChannel)
    return false;

  return m_pChannel->IsConnected();
}
