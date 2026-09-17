#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Tracing/TraceProvider.h>

#if W_ENABLED(W_SUPPORTS_IPC)
#  include <PipeChannel_Platform.h>
#endif

static_assert((WInt32)WIpcChannel::ConnectionState::Disconnected == (WInt32)WIpcChannelEvent::Disconnected);
static_assert((WInt32)WIpcChannel::ConnectionState::Connecting == (WInt32)WIpcChannelEvent::Connecting);
static_assert((WInt32)WIpcChannel::ConnectionState::Connected == (WInt32)WIpcChannelEvent::Connected);

WIpcChannel::WIpcChannel(WStringView sAddress, Mode::Enum mode)
  : m_sAddress(sAddress)
  , m_Mode(mode)
  , m_pOwner(WMessageLoop::GetSingleton())
{
  W_TRACE_EVENT("IpcChannel_Created", WTraceLevel::Info,
    W_TRACE_VALUE("Address", m_sAddress.GetData()),
    W_TRACE_VALUE("Mode", (int)m_Mode.GetValue()));
}

WIpcChannel::~WIpcChannel()
{
  m_pOwner->RemoveChannel(this);
}

WInternal::NewInstance<WIpcChannel> WIpcChannel::CreatePipeChannel(WStringView sAddress, Mode::Enum mode)
{
  W_IGNORE_UNUSED(sAddress);
  W_IGNORE_UNUSED(mode);

  if (sAddress.IsEmpty() || sAddress.GetElementCount() > 200)
  {
    WLog::Error("Failed co create pipe '{0}', name is not valid", sAddress);
    return nullptr;
  }

#if W_ENABLED(W_SUPPORTS_IPC)
  return W_DEFAULT_NEW(WPipeChannel_Platform, sAddress, mode);
#else
  W_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}


WInternal::NewInstance<WIpcChannel> WIpcChannel::CreateNetworkChannel(WStringView sAddress, Mode::Enum mode)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return W_DEFAULT_NEW(WIpcChannelEnet, sAddress, mode);
#else
  W_IGNORE_UNUSED(sAddress);
  W_IGNORE_UNUSED(mode);
  W_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}

WResult WIpcChannel::Connect()
{
  W_TRACE_EVENT("IpcChannel_Connect", WTraceLevel::Info,
    W_TRACE_VALUE("Address", m_sAddress.GetData()),
    W_TRACE_VALUE("Mode", (int)m_Mode.GetValue()));

  WEnum<ConnectionState> newState = ConnectionState::Connecting;
  WEnum<ConnectionState> previousState = m_ConnectionState.CompareAndSwap(ConnectionState::Disconnected, newState);

  if (previousState != ConnectionState::Disconnected)
  {
    return W_FAILURE;
  }

  LogAndBroadcastConnectionState(previousState, newState);

  W_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_ConnectQueue.PushBack(this);
  m_pOwner->WakeUp();
  return W_SUCCESS;
}


void WIpcChannel::Disconnect()
{
  W_TRACE_EVENT("IpcChannel_Disconnect", WTraceLevel::Info,
    W_TRACE_VALUE("Address", m_sAddress.GetData()),
    W_TRACE_VALUE("Mode", (int)m_Mode.GetValue()));

  W_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_DisconnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}


bool WIpcChannel::Send(WArrayPtr<const WUInt8> data)
{
  {
    W_LOCK(m_OutputQueueMutex);
    WMemoryStreamStorageInterface& storage = m_OutputQueue.ExpandAndGetRef();
    WMemoryStreamWriter writer(&storage);
    WUInt32 uiSize = data.GetCount() + HEADER_SIZE;
    WUInt32 uiMagic = MAGIC_VALUE;
    writer << uiMagic;
    writer << uiSize;
    W_ASSERT_DEBUG(storage.GetStorageSize32() == HEADER_SIZE, "Magic value and size should have written HEADER_SIZE bytes.");
    writer.WriteBytes(data.GetPtr(), data.GetCount()).AssertSuccess("Failed to write to in-memory buffer, out of memory?");
  }
  if (IsConnected())
  {
    W_LOCK(m_pOwner->m_TasksMutex);
    if (!m_pOwner->m_SendQueue.Contains(this))
      m_pOwner->m_SendQueue.PushBack(this);
    if (NeedWakeup())
    {
      m_pOwner->WakeUp();
    }
    return true;
  }
  return false;
}

void WIpcChannel::SetReceiveCallback(ReceiveCallback callback)
{
  W_LOCK(m_ReceiveCallbackMutex);
  m_ReceiveCallback = callback;
}

WResult WIpcChannel::WaitForMessages(WTime timeout)
{
  if (IsConnected())
  {
    if (timeout == WTime::MakeZero())
    {
      m_IncomingMessages.WaitForSignal();
    }
    else if (m_IncomingMessages.WaitForSignal(timeout) == WThreadSignal::WaitResult::Timeout)
    {
      return W_FAILURE;
    }
  }
  return W_SUCCESS;
}

void WIpcChannel::SetConnectionState(WEnum<WIpcChannel::ConnectionState> state)
{
  WEnum<ConnectionState> previousState = m_ConnectionState.Set(state);
  if (state != previousState)
  {
    LogAndBroadcastConnectionState(previousState, state);
  }
}

void WIpcChannel::ReceiveData(WArrayPtr<const WUInt8> data)
{
  W_LOCK(m_ReceiveCallbackMutex);

  if (!m_ReceiveCallback.IsValid())
  {
    m_MessageAccumulator.PushBackRange(data);
    return;
  }

  WArrayPtr<const WUInt8> remainingData = data;
  while (true)
  {
    if (m_MessageAccumulator.GetCount() < HEADER_SIZE)
    {
      if (remainingData.GetCount() + m_MessageAccumulator.GetCount() < HEADER_SIZE)
      {
        m_MessageAccumulator.PushBackRange(remainingData);
        return;
      }
      else
      {
        WUInt32 uiRemainingHeaderData = HEADER_SIZE - m_MessageAccumulator.GetCount();
        WArrayPtr<const WUInt8> headerData = remainingData.GetSubArray(0, uiRemainingHeaderData);
        m_MessageAccumulator.PushBackRange(headerData);
        W_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == HEADER_SIZE, "We should have a full header now.");
        remainingData = remainingData.GetSubArray(uiRemainingHeaderData);
      }
    }

    W_ASSERT_DEBUG(m_MessageAccumulator.GetCount() >= HEADER_SIZE, "Header must be complete at this point.");
    if (remainingData.IsEmpty())
      return;

    // Read and verify header
    WUInt32 uiMagic = *reinterpret_cast<const WUInt32*>(m_MessageAccumulator.GetData());
    W_IGNORE_UNUSED(uiMagic);
    W_ASSERT_DEBUG(uiMagic == MAGIC_VALUE, "Message received with wrong magic value.");
    WUInt32 uiMessageSize = *reinterpret_cast<const WUInt32*>(m_MessageAccumulator.GetData() + 4);
    W_ASSERT_DEBUG(uiMessageSize < MAX_MESSAGE_SIZE, "Message too big: {0}! Either the stream got corrupted or you need to increase MAX_MESSAGE_SIZE.", uiMessageSize);
    if (uiMessageSize > remainingData.GetCount() + m_MessageAccumulator.GetCount())
    {
      m_MessageAccumulator.PushBackRange(remainingData);
      return;
    }

    // Write missing data into message accumulator
    WUInt32 remainingMessageData = uiMessageSize - m_MessageAccumulator.GetCount();
    WArrayPtr<const WUInt8> messageData = remainingData.GetSubArray(0, remainingMessageData);
    m_MessageAccumulator.PushBackRange(messageData);
    W_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == uiMessageSize, "");
    remainingData = remainingData.GetSubArray(remainingMessageData);

    {
      m_ReceiveCallback(WArrayPtr<const WUInt8>(m_MessageAccumulator.GetData() + HEADER_SIZE, uiMessageSize - HEADER_SIZE));
      m_IncomingMessages.RaiseSignal();
      m_Events.Broadcast(WIpcChannelEvent(WIpcChannelEvent::NewMessages, this));
      m_MessageAccumulator.Clear();
    }
  }
}

void WIpcChannel::FlushPendingOperations()
{
  m_pOwner->WaitForMessages(-1, this);
}

void WIpcChannel::LogAndBroadcastConnectionState(WEnum<ConnectionState> previousState, WEnum<ConnectionState> currentState)
{
  W_IGNORE_UNUSED(previousState);

  W_TRACE_EVENT("IpcChannel_StateChanged", WTraceLevel::Info,
    W_TRACE_VALUE("Address", m_sAddress.GetData()),
    W_TRACE_VALUE("Mode", (int)m_Mode.GetValue()),
    W_TRACE_VALUE("OldState", previousState.GetValue()),
    W_TRACE_VALUE("NewState", currentState.GetValue()));
  m_Events.Broadcast(WIpcChannelEvent((WIpcChannelEvent::Type)currentState.GetValue(), this));
}
