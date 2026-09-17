#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

class WIpcChannel;
class WMessageLoop;

/// Event data for WIpcChannel::m_Events
struct W_FOUNDATION_DLL WIpcChannelEvent
{
  enum Type
  {
    Disconnected, ///< Server or client are in a dorment state.
    Connecting,   ///< The server is listening for clients or the client is trying to find the server.
    Connected,    ///< Client and server are connected to each other.
    NewMessages,  ///< Sent when a new messages have been received or when disconnected to wake up any thread waiting for messages.
  };

  WIpcChannelEvent() = default;

  WIpcChannelEvent(Type type, WIpcChannel* pChannel)
    : m_Type(type)
    , m_pChannel(pChannel)
  {
  }

  Type m_Type = NewMessages;
  WIpcChannel* m_pChannel = nullptr;
};



/// Base class for a communication channel between processes.
///
///  The channel allows for byte blobs to be send back and forth between two processes.
///  A client should only try to connect to a server once the server has changed to ConnectionState::Connecting as this indicates the server is ready to be conneccted to.
///
///  Use WIpcChannel:::CreatePipeChannel to create an IPC pipe instance.
///  To send more complex messages accross, you can create a WIpcProcessMessageProtocol on top of the channel.
class W_FOUNDATION_DLL WIpcChannel
{
public:
  struct Mode
  {
    using StorageType = WUInt8;
    enum Enum
    {
      Server,
      Client,
      Default = Server
    };
  };

  struct ConnectionState
  {
    using StorageType = WUInt8;
    enum Enum
    {
      Disconnected,
      Connecting, ///< In case of the server, this state indicates that the server is ready to be connected to.
      Connected,
      Default = Disconnected
    };
  };

  virtual ~WIpcChannel();

  /// Creates an IPC communication channel using pipes.
  /// \param szAddress Name of the pipe, must be unique on a system and less than 200 characters.
  /// \param mode Whether to run in client or server mode.
  static WInternal::NewInstance<WIpcChannel> CreatePipeChannel(WStringView sAddress, Mode::Enum mode);

  static WInternal::NewInstance<WIpcChannel> CreateNetworkChannel(WStringView sAddress, Mode::Enum mode);

  WEnum<Mode> GetMode() const { return m_Mode; }
  WStringView GetAddress() const { return m_sAddress; }

  /// Connects async. Returns whether the state was changed from Disconnected to Connecting.
  WResult Connect();
  /// Disconnect async. On completion, m_Events will be broadcasted.
  void Disconnect();
  /// Returns whether we have a connection.
  bool IsConnected() const { return m_ConnectionState == ConnectionState::Connected; }
  /// Returns the current state of the connection.
  WEnum<ConnectionState> GetConnectionState() const { return WEnum<ConnectionState>(m_ConnectionState); }

  /// Sends a message. pMsg can be destroyed after the call.
  bool Send(WArrayPtr<const WUInt8> data);

  using ReceiveCallback = WDelegate<void(WArrayPtr<const WUInt8> message)>;
  void SetReceiveCallback(ReceiveCallback callback);

  /// Block and wait for new messages and call ProcessMessages.
  WResult WaitForMessages(WTime timeout);

public:
  WEvent<const WIpcChannelEvent&, WMutex> m_Events; ///< Will be sent from any thread.

protected:
  WIpcChannel(WStringView sAddress, Mode::Enum mode);

  /// Override this and return true, if the surrounding infrastructure should call the 'Tick()' function.
  virtual bool RequiresRegularTick() { return false; }
  /// Can implement regular updates, e.g. for polling network state.
  virtual void Tick() {}

  /// Called on worker thread after Connect was called.
  virtual void InternalConnect() = 0;
  /// Called on worker thread after Disconnect was called.
  virtual void InternalDisconnect() = 0;
  /// Called on worker thread to sent pending messages.
  virtual void InternalSend() = 0;
  /// Called by Send to determine whether the message loop need to be woken up.
  virtual bool NeedWakeup() const = 0;

  /// Sets the connection state and calls LogAndBroadcastConnectionState.
  void SetConnectionState(WEnum<ConnectionState> state);
  /// Implementation needs to call this when new data has been received.
  ///  data can be invalidated after the function.
  void ReceiveData(WArrayPtr<const WUInt8> data);
  void FlushPendingOperations();

private:
  void LogAndBroadcastConnectionState(WEnum<ConnectionState> previousState, WEnum<ConnectionState> currentState);

protected:
  enum Constants : WUInt32
  {
    HEADER_SIZE = 8,                     ///< Magic value and size WUint32
    MAGIC_VALUE = 'USED',                ///< Magic value
    MAX_MESSAGE_SIZE = 1024 * 1024 * 16, ///< Arbitrary message size limit
  };

  friend class WMessageLoop;
  WThreadID m_ThreadId = 0;

  WAtomicInteger<ConnectionState::Enum> m_ConnectionState = ConnectionState::Disconnected;

  // Setup in ctor
  WString m_sAddress;
  const WEnum<Mode> m_Mode;
  WMessageLoop* m_pOwner = nullptr;

  // Mutex locked
  WMutex m_OutputQueueMutex;
  WDeque<WContiguousMemoryStreamStorage> m_OutputQueue;

  // Only accessed from worker thread
  WDynamicArray<WUInt8> m_MessageAccumulator; ///< Message is assembled in here

  // Mutex locked
  WMutex m_ReceiveCallbackMutex;
  ReceiveCallback m_ReceiveCallback;
  WThreadSignal m_IncomingMessages;
};
