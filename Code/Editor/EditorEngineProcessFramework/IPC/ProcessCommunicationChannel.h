#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class WIpcChannel;
class WProcessMessage;
class WIpcProcessMessageProtocol;
struct WIpcChannelEvent;

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WProcessCommunicationChannel
{
public:
  WProcessCommunicationChannel();
  ~WProcessCommunicationChannel();

  bool SendMessage(WProcessMessage* pMessage);

  /// Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and
  /// the waiting ends. If false is returned the wait for the message continues.
  using WaitForMessageCallback = WDelegate<bool(WProcessMessage*)>;
  WResult WaitForMessage(const WRTTI* pMessageType, WTime timeout, WaitForMessageCallback* pMessageCallack = nullptr);
  WResult WaitForConnection(WTime timeout);
  bool IsConnected() const;

  /// Returns true if any message was processed
  bool ProcessMessages();

  /// Blocks until a message arrives.
  ///
  /// \param timeout Zero blocks indefinitely. Pass a short one when the caller has other work that a
  ///        message is not going to wake it up for.
  void WaitForMessages(WTime timeout = WTime::MakeZero());

  struct Event
  {
    const WProcessMessage* m_pMessage;
    // Set to true in a message handler to cancel the ProcessMessages function and return to the caller before all messages have been processed.
    mutable bool m_bInterruptMessageProcessing = false;
  };

  WEvent<const Event&> m_Events;
  WEvent<const WIpcChannelEvent&, WMutex> m_IpcChannelEvents;

protected:
  void OnIpcProtocolEvent(const WIpcProcessMessageProtocol::Event& msg);
  void OnIpcChannelEvent(const WIpcChannelEvent& msg);
  WResult CreateAndConnectChannel(WInternal::NewInstance<WIpcChannel>&& channel);
  void DestroyChannel();
  WUniquePtr<WIpcProcessMessageProtocol> m_pProtocol;
  WUniquePtr<WIpcChannel> m_pChannel;
  const WRTTI* m_pFirstAllowedMessageType = nullptr;

private:
  WaitForMessageCallback m_WaitForMessageCallback;
  const WRTTI* m_pWaitForMessageType = nullptr;
};
