#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>

/// Whether the remote interface is configured as a server or a client
enum class WRemoteMode
{
  None,   ///< Remote interface is shut down
  Server, ///< Remote interface acts as a server. Can connect with multiple clients
  Client  ///< Remote interface acts as a client. Can connect with exactly one server.
};

/// Mode for transmitting messages
///
/// Depending on the remote interface implementation, Unreliable may not be supported and revert to Reliable.
enum class WRemoteTransmitMode
{
  Reliable,   ///< Messages should definitely arrive at the target, if necessary they are send several times, until the target acknowledged it.
  Unreliable, ///< Messages are sent at most once, if they get lost, they are not resent. If it is known beforehand, that not receiver exists, they
              ///< are dropped without sending them at all.
};

/// Event type for connections
struct W_FOUNDATION_DLL WRemoteEvent
{
  enum Type
  {
    ConnectedToClient,      ///< brief Sent whenever a new connection to a client has been established.
    ConnectedToServer,      ///< brief Sent whenever a connection to the server has been established.
    DisconnectedFromClient, ///< Sent every time the connection to a client is dropped
    DisconnectedFromServer, ///< Sent when the connection to the server has been lost
  };

  Type m_Type;
  WUInt32 m_uiOtherAppID;
};

using WRemoteMessageHandler = WDelegate<void(WRemoteMessage&)>;

struct W_FOUNDATION_DLL WRemoteMessageQueue
{
  WRemoteMessageHandler m_MessageHandler;
  /// Messages are pushed into this container on arrival.
  WDeque<WRemoteMessage> m_MessageQueueIn;
  /// To flush the message queue, m_MessageQueueIn and m_MessageQueueOut are swapped.
  /// Thus new messages can arrive while we execute the event handler for each element
  /// in this container and then clear it.
  WDeque<WRemoteMessage> m_MessageQueueOut;
};

class W_FOUNDATION_DLL WRemoteInterface
{
public:
  virtual ~WRemoteInterface();

  /// Exposes the mutex that is internally used to secure multi-threaded access
  WMutex& GetMutex() const { return m_Mutex; }


  /// \name Connection
  ///@{

  /// Starts the remote interface as a server.
  ///
  /// \param uiConnectionToken Should be a unique sequence (e.g. 'EZPZ') to identify the purpose of this connection.
  /// Only server and clients with the same token will accept connections.
  /// \param uiPort The port over which the connection should run.
  /// \param bStartUpdateThread If true, a thread is started that will regularly call UpdateNetwork() and UpdatePingToServer().
  /// If false, this has to be called manually in regular intervals.
  WResult StartServer(WUInt32 uiConnectionToken, WStringView sAddress, bool bStartUpdateThread = true);

  /// Starts the network interface as a client. Tries to connect to the given address.
  ///
  /// This function immediately returns and no connection is guaranteed.
  /// \param uiConnectionToken Same as for StartServer()
  /// \param szAddress Could be a network address "127.0.0.1" or "localhost" or some other name that identifies the target, e.g. a named pipe.
  /// \param bStartUpdateThread Same as for StartServer()
  ///
  /// If this function succeeds, it still might not be connected to a server.
  /// Use WaitForConnectionToServer() to enforce a connection.
  WResult ConnectToServer(WUInt32 uiConnectionToken, WStringView sAddress, bool bStartUpdateThread = true);

  /// Can only be called after ConnectToServer(). Updates the network in a loop until a connection is established, or the time has run out.
  ///
  /// A timeout of exactly zero means to wait indefinitely.
  WResult WaitForConnectionToServer(WTime timeout = WTime::MakeFromSeconds(10));

  /// Closes the connection in an orderly fashion
  void ShutdownConnection();

  /// Whether the client is connected to a server
  bool IsConnectedToServer() const { return m_uiConnectedToServerWithID != 0; }

  /// Whether the server is connected to any client
  bool IsConnectedToClients() const { return m_iConnectionsToClients > 0; }

  /// Whether the client or server is connected its counterpart
  bool IsConnectedToOther() const { return IsConnectedToServer() || IsConnectedToClients(); }

  /// Whether the remote interface is inactive, a client or a server
  WRemoteMode GetRemoteMode() const { return m_RemoteMode; }

  /// The address through which the connection was started
  const WString& GetServerAddress() const { return m_sServerAddress; }

  /// Returns the own (random) application ID used to identify this instance
  WUInt32 GetApplicationID() const { return m_uiApplicationID; }

  /// Returns the connection token used to identify compatible servers/clients
  WUInt32 GetConnectionToken() const { return m_uiConnectionToken; }

  ///@}

  /// \name Server Information
  ///@{

  /// For the client to display the name of the server
  // const WString& GetServerInfoName() const { return m_ServerInfoName; }

  /// For the client to display the IP of the server
  const WString& GetServerInfoIP() const { return m_sServerInfoIP; }

  /// Some random identifier, that allows to determine after a reconnect, whether the connected instance is still the same server
  WUInt32 GetServerID() const { return m_uiConnectedToServerWithID; }

  /// Returns the current ping to the server
  WTime GetPingToServer() const { return m_PingToServer; }

  ///@}

  /// \name Updating the Remote Interface
  ///@{

  /// If no update thread was spawned, this should be called to process messages
  void UpdateRemoteInterface();

  /// If no update thread was spawned, this should be called by clients to determine the ping
  void UpdatePingToServer();

  ///@}

  /// \name Sending Messages
  ///@{

  /// Sends a reliable message without any data.
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(WUInt32 uiSystemID, WUInt32 uiMsgID);

  /// Sends a message, appends the given array of data
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(WRemoteTransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const WArrayPtr<const WUInt8>& data);

  void Send(WRemoteTransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const WContiguousMemoryStreamStorage& data);

  /// Sends a message, appends the given array of data
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(WRemoteTransmitMode tm, WUInt32 uiSystemID, WUInt32 uiMsgID, const void* pData = nullptr, WUInt32 uiDataBytes = 0);

  /// Sends an WRemoteMessage
  /// If it is a server, the message is broadcast to all clients.
  /// If it is a client, the message is only sent to the server.
  void Send(WRemoteTransmitMode tm, WRemoteMessage& ref_msg);

  ///@}

  /// \name Message Handling
  ///@{

  /// Registers a message handler that is executed for all incoming messages for the given system
  void SetMessageHandler(WUInt32 uiSystemID, WRemoteMessageHandler messageHandler);

  /// Registers a message handler that is executed for all incoming messages for systems for which there are no dedicated message handlers.
  void SetUnhandledMessageHandler(WRemoteMessageHandler messageHandler);

  /// Executes the message handler for all messages that have arrived for the given system
  WUInt32 ExecuteMessageHandlers(WUInt32 uiSystem);

  /// Executes all message handlers for all received messages
  WUInt32 ExecuteAllMessageHandlers();

  ///@}

  /// \name Events
  ///@{

  /// Broadcasts events about connections
  WEvent<const WRemoteEvent&> m_RemoteEvents;

  ///@}

protected:
  /// \name Implementation Details
  ///@{

  /// Derived classes have to implement this to start a network connection
  virtual WResult InternalCreateConnection(WRemoteMode mode, WStringView sServerAddress) = 0;

  /// Derived classes have to implement this to shutdown a network connection
  virtual void InternalShutdownConnection() = 0;

  /// Derived classes have to implement this to update
  virtual void InternalUpdateRemoteInterface() = 0;

  /// Derived classes have to implement this to get the ping to the server (client mode only)
  virtual WTime InternalGetPingToServer() = 0;

  /// Derived classes have to implement this to deliver messages to the server or client
  virtual WResult InternalTransmit(WRemoteTransmitMode tm, const WArrayPtr<const WUInt8>& data) = 0;

  /// Derived classes can override this to interpret an address differently
  virtual WResult DetermineTargetAddress(WStringView sConnectTo, WUInt32& out_IP, WUInt16& out_Port);

  /// Derived classes should update this when the information is available
  // WString m_ServerInfoName;
  /// Derived classes should update this when the information is available
  WString m_sServerInfoIP;

  /// Should be called by the implementation, when a server connection has been established
  void ReportConnectionToServer(WUInt32 uiServerID);
  /// Should be called by the implementation, when a client connection has been established
  void ReportConnectionToClient(WUInt32 uiApplicationID);
  /// Should be called by the implementation, when a server connection has been lost
  void ReportDisconnectedFromServer();
  /// Should be called by the implementation, when a client connection has been lost
  void ReportDisconnectedFromClient(WUInt32 uiApplicationID);
  /// Should be called by the implementation, when a message has arrived
  void ReportMessage(WUInt32 uiApplicationID, WUInt32 uiSystemID, WUInt32 uiMsgID, const WArrayPtr<const WUInt8>& data);

  ///@}


private:
  void StartUpdateThread();
  void StopUpdateThread();
  WResult Transmit(WRemoteTransmitMode tm, const WArrayPtr<const WUInt8>& data);
  WResult CreateConnection(WUInt32 uiConnectionToken, WRemoteMode mode, WStringView sServerAddress, bool bStartUpdateThread);
  WUInt32 ExecuteMessageHandlersForQueue(WRemoteMessageQueue& queue);

  mutable WMutex m_Mutex;
  class WRemoteThread* m_pUpdateThread = nullptr;
  WRemoteMode m_RemoteMode = WRemoteMode::None;
  WString m_sServerAddress;
  WTime m_PingToServer;
  WUInt32 m_uiApplicationID = 0; // sent when connecting to identify the sending instance
  WUInt32 m_uiConnectionToken = 0;
  WUInt32 m_uiConnectedToServerWithID = 0;
  WInt32 m_iConnectionsToClients = 0;
  WDynamicArray<WUInt8> m_TempSendBuffer;
  WHashTable<WUInt32, WRemoteMessageQueue> m_MessageQueues;
  WRemoteMessageHandler m_UnhandledMessageHandler;
};

/// The remote interface thread updates in regular intervals to keep the connection alive.
///
/// The thread does NOT call WRemoteInterface::ExecuteAllMessageHandlers(), so by default no message handlers are executed.
/// This has to be done manually by the application elsewhere.
class W_FOUNDATION_DLL WRemoteThread : public WThread
{
public:
  WRemoteThread();

  WRemoteInterface* m_pRemoteInterface = nullptr;
  volatile bool m_bKeepRunning = true;

private:
  virtual WUInt32 Run();
};
