#pragma once

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

class WRemoteMessage;

struct WFileserverEvent
{
  enum class Type
  {
    None,
    ServerStarted,
    ServerStopped,
    ClientConnected,
    ClientReconnected, // connected again after a disconnect
    ClientDisconnected,
    MountDataDir,
    MountDataDirFailed,
    UnmountDataDir,
    FileDownloadRequest,
    FileDownloading,
    FileDownloadFinished,
    FileDeleteRequest,
    FileUploadRequest,
    FileUploading,
    FileUploadFinished,
    AreYouThereRequest,
    LogCustomActivity,
  };

  Type m_Type = Type::None;
  WUInt32 m_uiClientID = 0;
  const char* m_szName = nullptr;
  const char* m_szPath = nullptr;
  const char* m_szRedirectedPath = nullptr;
  WUInt32 m_uiSizeTotal = 0;
  WUInt32 m_uiSentTotal = 0;
  WFileserveFileState m_FileState = WFileserveFileState::None;
};

/// A file server allows to serve files from a host PC to another process that is potentially on another device.
///
/// This is mostly useful for mobile devices, that do not have access to the data on the development machine.
/// Typically every change to a file would require packaging the app and deploying it to the device again.
/// Fileserve allows to only deploy a very lean application and instead get all asset data directly from a host PC.
/// This also allows to modify data on the PC and reload the data in the running application without delay.
///
/// A single file server can serve multiple clients. However, to mount "special directories" (see WFileSystem) the server
/// needs to know what local path to map them to (it uses the configuration on WFileSystem).
/// That means it cannot serve two clients that require different settings for the same special directory.
///
/// The port on which the server connects to clients can be configured through the command line option "-fs_port X"
class W_FILESERVEPLUGIN_DLL WFileserver
{
  W_DECLARE_SINGLETON(WFileserver);

public:
  WFileserver();

  /// Starts listening for client connections. Uses the configured port.
  void StartServer();

  /// Disconnects all clients.
  void StopServer();

  /// Has to be executed regularly to serve clients and keep the connection alive.
  bool UpdateServer();

  /// Whether the server was started.
  bool IsServerRunning() const;

  /// Overrides the current port setting. May only be called when the server is currently not running.
  void SetPort(WUInt16 uiPort);

  /// Returns the currently set port. If the command line option "-fs_port X" was used, this will return that value, otherwise the default is
  /// 1042.
  WUInt16 GetPort() const { return m_uiPort; }

  /// The server broadcasts events about its activity
  WEvent<const WFileserverEvent&> m_Events;

  /// Broadcasts to all clients that they should reload their resources
  void BroadcastReloadResourcesCommand();

  static WResult SendConnectionInfo(
    const char* szClientAddress, WUInt16 uiMyPort, const WArrayPtr<WStringBuilder>& myIPs, WTime timeout = WTime::MakeFromSeconds(10));

  using ClientMessageHandler = WDelegate<void(WFileserveClientContext&, WRemoteMessage&, WRemoteInterface&, WDelegate<void(const char*)>)>;

  void SetCustomMessageHandler(WUInt32 uiSystemID, ClientMessageHandler handler);

private:
  void NetworkEventHandler(const WRemoteEvent& e);
  WFileserveClientContext& DetermineClient(WRemoteMessage& msg);
  void NetworkMsgHandler(WRemoteMessage& msg);
  void UnknownNetworkMsgHandler(WRemoteMessage& msg);
  void HandleMountRequest(WFileserveClientContext& client, WRemoteMessage& msg);
  void HandleUnmountRequest(WFileserveClientContext& client, WRemoteMessage& msg);
  void HandleFileRequest(WFileserveClientContext& client, WRemoteMessage& msg);
  void HandleDeleteFileRequest(WFileserveClientContext& client, WRemoteMessage& msg);
  void HandleUploadFileHeader(WFileserveClientContext& client, WRemoteMessage& msg);
  void HandleUploadFileTransfer(WFileserveClientContext& client, WRemoteMessage& msg);
  void HandleUploadFileFinished(WFileserveClientContext& client, WRemoteMessage& msg);
  void LogCustomActivity(const char* szText);

  WHashTable<WUInt32, WFileserveClientContext> m_Clients;
  WUniquePtr<WRemoteInterface> m_pNetwork;
  WDynamicArray<WUInt8> m_SendToClient;   // ie. 'downloads' from server to client
  WDynamicArray<WUInt8> m_SentFromClient; // ie. 'uploads' from client to server
  WStringBuilder m_sCurFileUpload;
  WUuid m_FileUploadGuid;
  WUInt32 m_uiFileUploadSize;
  WUInt16 m_uiPort = 1042;
  WMap<WUInt32, ClientMessageHandler> m_CustomMessageHandlers;
};
