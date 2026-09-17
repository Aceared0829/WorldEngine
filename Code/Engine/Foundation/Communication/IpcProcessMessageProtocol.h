#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Types/UniquePtr.h>

class WIpcChannel;
class WMessageLoop;


/// A protocol around WIpcChannel to send reflected messages instead of byte array messages between client and server.
///
/// This wrapper class hooks into an existing WIpcChannel. The WIpcChannel is still responsible for all connection logic. This class merely provides a high-level messaging protocol via reflected messages derived from WProcessMessage.
/// Note that if this class is used, WIpcChannel::Send must not be called manually anymore, only use WIpcProcessMessageProtocol::Send.
/// Received messages are stored in a queue and must be flushed via calling ProcessMessages or WaitForMessages.
class W_FOUNDATION_DLL WIpcProcessMessageProtocol
{
public:
  WIpcProcessMessageProtocol(WIpcChannel* pChannel);
  ~WIpcProcessMessageProtocol();

  /// Sends a message. pMsg can be destroyed after the call.
  bool Send(WProcessMessage* pMsg);


  /// Processes all pending messages by broadcasting m_MessageEvent. Not re-entrant.
  bool ProcessMessages();
  /// Block and wait for new messages and call ProcessMessages.
  WResult WaitForMessages(WTime timeout = WTime::MakeZero());

public:
  struct Event
  {
    const WProcessMessage* m_pMessage;
    // Set to true in a message handler to cancel the ProcessMessages function and return to the caller before all messages have been processed.
    mutable bool m_bInterruptMessageProcessing = false;
  };
  WEvent<const Event&> m_MessageEvent; ///< Will be sent from thread calling ProcessMessages or WaitForMessages.

private:
  void EnqueueMessage(WUniquePtr<WProcessMessage>&& msg);
  WUniquePtr<WProcessMessage> PopMessage();
  void ReceiveMessageData(WArrayPtr<const WUInt8> data);

private:
  WIpcChannel* m_pChannel = nullptr;
  WUInt64 m_uiSendChannelId = 0;    // UniqueID derived from channel address to correlate telemetry across processes.
  WUInt64 m_uiReceiveChannelId = 0; // UniqueID derived from channel address to correlate telemetry across processes.
  WAtomicInteger64 m_iSendMessageId = 0;

  WMutex m_IncomingQueueMutex;
  WDeque<WUniquePtr<WProcessMessage>> m_IncomingQueue;
};
