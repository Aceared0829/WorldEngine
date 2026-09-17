#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcProcessMessageProtocol.h>
// #include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
// #include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Tracing/TraceProvider.h>

WIpcProcessMessageProtocol::WIpcProcessMessageProtocol(WIpcChannel* pChannel)
{
  WStringView sAddress = pChannel->GetAddress();

  m_uiReceiveChannelId = WHashingUtils::xxHash64String(sAddress);
  m_uiSendChannelId = WHashingUtils::xxHash64String(sAddress, 1337);

  if (pChannel->GetMode() == WIpcChannel::Mode::Client)
  {
    // To make sure telemetry events can be correlated, the client swaps the stable channel IDs.
    std::swap(m_uiReceiveChannelId, m_uiSendChannelId);
  }

  m_pChannel = pChannel;
  m_pChannel->SetReceiveCallback(WMakeDelegate(&WIpcProcessMessageProtocol::ReceiveMessageData, this));
}

WIpcProcessMessageProtocol::~WIpcProcessMessageProtocol()
{
  m_pChannel->SetReceiveCallback({});

  while (WUniquePtr<WProcessMessage> msg = PopMessage())
  {
  }
}

bool WIpcProcessMessageProtocol::Send(WProcessMessage* pMsg)
{
  WStringBuilder sTypeName = pMsg->GetDynamicRTTI()->GetTypeName();
  WUInt64 uiMessageId = (WUInt64)m_iSendMessageId.Increment();
  [[maybe_unused]] WUInt64 uiRequestId = m_uiSendChannelId + uiMessageId;
  pMsg->m_uiMessageId = uiMessageId;

  WContiguousMemoryStreamStorage storage;
  WMemoryStreamWriter writer(&storage);
  WReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);

  W_TRACE_ASYNC_BEGIN("IpcProtocol_Send", uiRequestId, WTraceLevel::Info,
    W_TRACE_VALUE("Address", m_pChannel->GetAddress().GetStartPointer()), // Safe as underlying storage is WString
    W_TRACE_VALUE("MessageId", uiMessageId),
    W_TRACE_VALUE("Type", sTypeName.GetData()));

  return m_pChannel->Send(WArrayPtr<const WUInt8>(storage.GetData(), storage.GetStorageSize32()));
}

bool WIpcProcessMessageProtocol::ProcessMessages()
{
  bool messagesPresent = false;

  while (WUniquePtr<WProcessMessage> msg = PopMessage())
  {
    [[maybe_unused]] WUInt64 uiRequestId = m_uiReceiveChannelId + msg->m_uiMessageId;
    WStringBuilder sTypeName = msg->GetDynamicRTTI()->GetTypeName();
    W_TRACE_ASYNC_END("IpcProtocol_Send", uiRequestId);

    W_TRACE_EVENT("IpcProtocol_Receive", WTraceLevel::Info,
      W_TRACE_VALUE("Address", m_pChannel->GetAddress().GetStartPointer()), // Safe as underlying storage is WString
      W_TRACE_VALUE("MessageId", msg->m_uiMessageId),
      W_TRACE_VALUE("Type", sTypeName.GetData()));
    messagesPresent = true;
    Event e;
    e.m_pMessage = msg.Borrow();
    e.m_bInterruptMessageProcessing = false;
    m_MessageEvent.Broadcast(e);
    if (e.m_bInterruptMessageProcessing)
      break;
  }

  return messagesPresent;
}

WResult WIpcProcessMessageProtocol::WaitForMessages(WTime timeout)
{
  // Message processing can be interrupted via the m_bInterruptMessageProcessing flag. Thus, there is no guarantee that the queue is empty at this point. Only wait if the queue is empty.
  if (ProcessMessages())
  {
    return W_SUCCESS;
  }

  WResult res = m_pChannel->WaitForMessages(timeout);
  if (res.Succeeded())
  {
    ProcessMessages();
  }
  return res;
}

void WIpcProcessMessageProtocol::ReceiveMessageData(WArrayPtr<const WUInt8> data)
{
  // Message complete, de-serialize
  WRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  const WRTTI* pRtti = nullptr;

  WProcessMessage* pMsg = (WProcessMessage*)WReflectionSerializer::ReadObjectFromBinary(reader, pRtti);
  WUniquePtr<WProcessMessage> msg(pMsg, WFoundation::GetDefaultAllocator());
  if (msg != nullptr)
  {
    EnqueueMessage(std::move(msg));
  }
  else
  {
    WLog::Error("Channel received invalid Message!");
  }
}

void WIpcProcessMessageProtocol::EnqueueMessage(WUniquePtr<WProcessMessage>&& msg)
{
  W_LOCK(m_IncomingQueueMutex);
  m_IncomingQueue.PushBack(std::move(msg));
}

WUniquePtr<WProcessMessage> WIpcProcessMessageProtocol::PopMessage()
{
  W_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return {};

  WUniquePtr<WProcessMessage> front = std::move(m_IncomingQueue.PeekFront());
  m_IncomingQueue.PopFront();
  return front;
}
