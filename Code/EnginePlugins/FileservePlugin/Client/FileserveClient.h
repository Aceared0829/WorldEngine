#pragma once

#include <FileservePlugin/FileservePluginDLL.h>

#include <Core/Interfaces/RemoteToolingInterface.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

namespace WDataDirectory
{
  class FileserveType;
}

/// Singleton that represents the client side part of a fileserve connection
///
/// Whether the fileserve plugin will be enabled is controled by WFileserveClient::s_bEnableFileserve
/// By default this is on, but if switched off, the fileserve client functionality will be disabled.
/// WFileserveClient will also switch its functionality off, if the command line argument "-fs_off" is specified.
/// If a program knows that it always wants to switch file serving off, it should either simply not load the plugin at all,
/// or it can inject that command line argument through WCommandLineUtils. This should be done before application startup
/// and especially before any data directories get mounted.
///
/// The timeout for connecting to the server can be configured through the command line option "-fs_timeout seconds"
/// The server to connect to can be configured through command line option "-fs_server address".
/// The default address is "localhost:1042".
class W_FILESERVEPLUGIN_DLL WFileserveClient : public WRemoteToolingInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WFileserveClient, WRemoteToolingInterface);

public:
  WFileserveClient();
  ~WFileserveClient();

  /// WRemoteToolingInterface

  /// Returns the network connection interface.
  WRemoteInterface* GetRemoteInterface() override { return m_pNetwork.Borrow(); }

  /// Can be called at startup to go through multiple sources and search for a valid server address
  ///
  /// Ie. checks the command line, WFileserve.txt in different directories, etc.
  /// For every potential IP it checks whether a fileserve connection could be established (e.g. tries to connect and
  /// checks whether the server answers). If a valid connection is found, the IP is stored internally and W_SUCCESS is returned.
  /// Call GetServerConnectionAddress() to retrieve the address.
  ///
  /// \param timeout Specifies the timeout for checking whether a server can be reached.
  WResult SearchForServerAddress(WTime timeout = WTime::MakeFromSeconds(5));

  /// Waits for a Fileserver application to try to connect to this device and send its own information.
  ///
  /// This can be used when a device has no proper way to know the IP through which to connect to a Fileserver.
  /// Instead the device opens a server connection itself, and waits for the other side to try to connect to it.
  /// This typically means that a human has to manually input this device's IP on the host PC into the Fileserve application,
  /// thus enabling the exchange of connection information.
  /// Once this has happened, this function stores the valid server IP internally and returns with success.
  /// A subsequent call to EnsureConnected() should then succeed.
  WResult WaitForServerInfo(WTime timeout = WTime::MakeFromSeconds(60.0 * 5));

  /// Stores the current connection info to a text file in the user data folder.
  WResult SaveCurrentConnectionInfoToDisk() const;

  /// Allows to disable the file serving functionality. Should be called before mounting data directories.
  ///
  /// Also achieved through the command line argument "-fs_off"
  static void DisabledFileserveClient() { s_bEnableFileserve = false; }

  /// Returns the address through which the Fileserve client tried to connect with the server last.
  const char* GetServerConnectionAddress() { return m_sServerConnectionAddress; }

  /// Can be called to ensure a fileserve connection. Otherwise automatically called when a data directory is mounted.
  ///
  /// The timeout defines how long the code will wait for a connection.
  /// Positive numbers are a regular timeout.
  /// A zero timeout means the application will wait indefinitely.
  /// A negative number means to either wait that time, or whatever was specified through the command-line.
  /// The timeout can be specified with the command line switch "-fs_timeout X" (in seconds).
  WResult EnsureConnected(WTime timeout = WTime::MakeFromSeconds(-5));

  /// Needs to be called regularly to update the network. By default this is automatically called when the global event
  /// 'GameApp_UpdatePlugins' is fired, which is done by WGameApplication.
  void UpdateClient();

  /// Adds an address that should be tried for connecting with the server.
  void AddServerAddressToTry(WStringView sAddress);

private:
  friend class WDataDirectory::FileserveType;

  /// True by default, can
  static bool s_bEnableFileserve;

  struct FileCacheStatus
  {
    WInt64 m_TimeStamp = 0;
    WUInt64 m_FileHash = 0;
    WTime m_LastCheck;
  };

  struct DataDir
  {
    // WString m_sRootName;
    // WString m_sPathOnClient;
    WString m_sMountPoint;
    bool m_bMounted = false;

    WMap<WString, FileCacheStatus> m_CacheStatus;
  };

  void DeleteFile(WUInt16 uiDataDir, WStringView sFile);
  WUInt16 MountDataDirectory(WStringView sDataDir, WStringView sRootName);
  void UnmountDataDirectory(WUInt16 uiDataDir);
  static void ComputeDataDirMountPoint(WStringView sDataDir, WStringBuilder& out_sMountPoint);
  void BuildPathInCache(const char* szFile, const char* szMountPoint, WStringBuilder* out_pAbsPath, WStringBuilder* out_pFullPathMeta) const;
  void GetFullDataDirCachePath(const char* szDataDir, WStringBuilder& out_sFullPath, WStringBuilder& out_sFullPathMeta) const;
  void NetworkMsgHandler(WRemoteMessage& msg);
  void HandleFileTransferMsg(WRemoteMessage& msg);
  void HandleFileTransferFinishedMsg(WRemoteMessage& msg);
  static void WriteMetaFile(WStringBuilder sCachedMetaFile, WInt64 iFileTimeStamp, WUInt64 uiFileHash);
  void WriteDownloadToDisk(WStringBuilder sCachedFile);
  WResult DownloadFile(WUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, WStringBuilder* out_pFullPath);
  void DetermineCacheStatus(WUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const;
  void UploadFile(WUInt16 uiDataDirID, const char* szFile, const WDynamicArray<WUInt8>& fileContent);
  void InvalidateFileCache(WUInt16 uiDataDirID, WStringView sFile, WUInt64 uiHash);
  static WResult TryReadFileserveConfig(const char* szFile, WStringBuilder& out_Result);
  WResult TryConnectWithFileserver(const char* szAddress, WTime timeout) const;
  void FillFileStatusCache(const char* szFile);
  void ShutdownConnection();
  void ClearState();

  mutable WMutex m_Mutex;
  mutable WString m_sServerConnectionAddress;
  WString m_sFileserveCacheFolder;
  WString m_sFileserveCacheMetaFolder;
  bool m_bDownloading = false;
  bool m_bFailedToConnect = false;
  bool m_bWaitingForUploadFinished = false;
  WUuid m_CurFileRequestGuid;
  WStringBuilder m_sCurFileRequest;
  WUniquePtr<WRemoteInterface> m_pNetwork;
  WDynamicArray<WUInt8> m_Download;
  WTime m_CurrentTime;
  WHybridArray<WString, 4> m_TryServerAddresses;

  WMap<WString, WUInt16> m_FileDataDir;
  WHybridArray<DataDir, 8> m_MountedDataDirs;
};
