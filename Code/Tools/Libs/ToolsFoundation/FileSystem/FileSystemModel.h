#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER) && W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/Configuration/Singleton.h>
#  include <Foundation/Threading/LockedObject.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <ToolsFoundation/FileSystem/DataDirPath.h>
#  include <ToolsFoundation/FileSystem/Declarations.h>

class WFileSystemWatcher;
struct WFileSystemWatcherEvent;
struct WFileStats;

/// Event fired by WFileSystemModel::m_FolderChangedEvents
struct W_TOOLSFOUNDATION_DLL WFolderChangedEvent
{
  enum class Type
  {
    None,
    FolderAdded,
    FolderRemoved,
    ModelReset, ///< Model was initialized or deinitialized.
  };

  WFolderChangedEvent() = default;
  WFolderChangedEvent(const WDataDirPath& file, Type type);

  WDataDirPath m_Path;
  Type m_Type = Type::None;
};

/// Event fired by WFileSystemModel::m_FileChangedEvents
struct W_TOOLSFOUNDATION_DLL WFileChangedEvent
{
  enum class Type
  {
    None,
    FileAdded,
    FileChanged,
    DocumentLinked,
    DocumentUnlinked,
    FileRemoved,
    ModelReset ///< Model was initialized or deinitialized.
  };

  WFileChangedEvent() = default;
  WFileChangedEvent(const WDataDirPath& file, WFileStatus status, Type type);

  WDataDirPath m_Path;
  WFileStatus m_Status;
  Type m_Type = Type::None;
};

/// A subsystem for tracking all files in a WApplicationFileSystemConfig.
///
/// Once Initialize is called with the WApplicationFileSystemConfig to track, the current state should be updated by calling CheckFileSystem() on a worker thread. This will trigger m_FolderChangedEvents and m_FileChangedEvents for all files / folders found in the data directories present in the config. Any future changes will be picked up by the WFileSystemWatcher created in Initialize.
/// For the system to work, the MainThreadTick function needs to be called at regular (e.g. frame) intervals.
/// The model also caches file hashes as well as allows files to be linked to document GUIDs for fast lookups.
class W_TOOLSFOUNDATION_DLL WFileSystemModel
{
  W_DECLARE_SINGLETON(WFileSystemModel);

public:
  using FilesMap = WMap<WDataDirPath, WFileStatus, WCompareDataDirPath>;
  using FoldersMap = WMap<WDataDirPath, WFileStatus::Status, WCompareDataDirPath>;

  using LockedFiles = WLockedObject<WMutex, const FilesMap>;
  using LockedFolders = WLockedObject<WMutex, const FoldersMap>;

public:
  /// Return true if the two paths point to the same file on disk. On different platforms the same strings can produce different results. This function assumes both paths are absolute and cleaned via WStringBuilder::MakeCleanPath.
  static bool IsSameFile(const WStringView sAbsolutePathA, const WStringView sAbsolutePathB);

  /// Computes the hash of the given file. Optionally passes the data stream through into another stream writer.
  static WUInt64 HashFile(WStreamReader& ref_inputStream, WStreamWriter* pPassThroughStream);

public:
  /// \name Setup
  ///@{

  WFileSystemModel();
  ~WFileSystemModel();

  /// Initializes the model for the given file system config.
  /// \param fileSystemConfig All data directories in this config will be tracked by the model.
  /// \param referencedFiles Restores the previous state of the file model. E.g. cached on disk. If the WFileStatus::Status is WFileStatus::Status::Unknown m_FileChangedEvents is guaranteed to be fired once the file is checked again, e.g. via CheckFileSystem or NotifyOfChange.
  /// \param referencedFolders Restores the previous state of the folder model. E.g. cached on disk.
  void Initialize(const WApplicationFileSystemConfig& fileSystemConfig, FilesMap&& referencedFiles, FoldersMap&& referencedFolders);

  /// Deinitialize the model.
  /// \param out_pReferencedFiles If set, filled with the current state of the file model so it can be cached, e.g. by storing it on disk.
  /// \param out_pReferencedFolders If set, filled with the current state of the folder model so it can be cached, e.g. by storing it on disk.
  void Deinitialize(FilesMap* out_pReferencedFiles = nullptr, FoldersMap* out_pReferencedFolders = nullptr);

  /// Needs to be called every frame to restart background tasks.
  void MainThreadTick();

  const WApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  WArrayPtr<const WString> GetDataDirectoryRoots() const { return m_DataDirRoots.GetArrayPtr(); }

  ///@}
  /// \name File / Folder Access
  ///@{

  /// Returns all files in the model.
  /// \return Returns the files and also a lock to the model.
  const LockedFiles GetFiles() const;

  /// Returns all folders in the model.
  /// \return Returns the folders and also a lock to the model.
  const LockedFolders GetFolders() const;

  /// Searches for a file in the model.
  /// \param sPath Absolute or relative path to a file to be searched for.
  /// \param stat Contains the current state of the file in the model if found.
  /// \return Returns W_SUCCESS if the file was found.
  WResult FindFile(WStringView sPath, WFileStatus& out_stat) const;

  /// Searches for the first file in the model that satisfies the given visitor function.
  /// \param visitor Called for every file in the model. If this functions returns true, the search is canceled and the function returns W_SUCCESS.
  /// \return Returns W_SUCCESS if the visitor returned true for a file.
  WResult FindFile(WDelegate<bool(const WDataDirPath&, const WFileStatus&)> visitor) const;

  ///@}
  /// \name File / Folder Updates
  ///@{

  /// Force checking the filesystem for changes to the given file or folder.
  /// This function will handle file add/remove/change as well as folder add/remove. If an existing folder should be checked for changes, use CheckFolder instead.
  /// \param sAbsolutePath File or folder to check for changes.
  void NotifyOfChange(WStringView sAbsolutePath);

  /// Check an existing folder recursively for changes.
  /// \param sAbsolutePath Absolute path to an existing folder in the model.
  void CheckFolder(WStringView sAbsolutePath);

  /// Updates all files and folders in the model by iterating over all data directories. This is very expensive and should be done on a worker thread.
  void CheckFileSystem();

  ///@}
  /// \name File Meta Operations
  ///@{

  /// Links a document Id to the given file. This allows for fast lookups whether a file is also a document.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param documentId The Id of the document that should be linked to the file.
  /// \return Returns W_SUCCESS if the file existed in the model and could be linked.
  WResult LinkDocument(WStringView sAbsolutePath, const WUuid& documentId);

  /// Unlinks a document from a file
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \return Returns W_SUCCESS if the file existed.
  WResult UnlinkDocument(WStringView sAbsolutePath);

  /// Creates a file reader to the given file. Will also link the document and hash it in a file-system-atomic operation.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param callback Called once the file was opened and hashed. The WFileStatus contains the up to date info for the file, including hash.
  /// \return Returns W_SUCCESS if the file existed and could be opened. Returns W_FAILURE if the file is not in the model or the file can't be opened for read access. On read failure, the file will be marked as locked.
  WResult ReadDocument(WStringView sAbsolutePath, const WDelegate<void(const WFileStatus&, WStreamReader&)>& callback);

  /// Returns an up-to-date hash for the given file. Will trigger m_FileChangedEvents if the file has been modified since the last check. Hashes are cached so in the best case this will just check the timestamp on disk against the model and then return the cached hash. This function will also work on files outside of the data directories.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param out_stat Contains the up to date info for the file, including hash.
  /// \return Returns W_SUCCESS if the file existed and could be opened. On failure, the file will be marked as locked.
  WResult HashFile(WStringView sAbsolutePath, WFileStatus& out_stat);

  ///@}

public:
  WCopyOnBroadcastEvent<const WFolderChangedEvent&, WMutex> m_FolderChangedEvents;
  WCopyOnBroadcastEvent<const WFileChangedEvent&, WMutex> m_FileChangedEvents;

private:
  void SetAllStatusUnknown();
  void RemoveStaleFileInfos();

  void OnAssetWatcherEvent(const WFileSystemWatcherEvent& e);
  WFileStatus HandleSingleFile(WDataDirPath absolutePath, bool bRecurseIntoFolders);
  WFileStatus HandleSingleFile(WDataDirPath absolutePath, const WFileStats& FileStat, bool bRecurseIntoFolders);

  void RemoveFileOrFolder(const WDataDirPath& absolutePath, bool bRecurseIntoFolders);

  void MarkFileLocked(WStringView sAbsolutePath);

  void FireFileChangedEvent(const WDataDirPath& file, WFileStatus fileStatus, WFileChangedEvent::Type type);
  void FireFolderChangedEvent(const WDataDirPath& file, WFolderChangedEvent::Type type);

private:
  // Immutable data after Initialize
  WApplicationFileSystemConfig m_FileSystemConfig;
  WDynamicArray<WString> m_DataDirRoots;
  WUniquePtr<WFileSystemWatcher> m_pWatcher;
  WEventSubscriptionID m_WatcherSubscription = {};

  // Actual file system data
  mutable WMutex m_FilesMutex;
  WAtomicBool m_bInitialized = false;

  FilesMap m_ReferencedFiles;                     // Absolute path to stat map
  FoldersMap m_ReferencedFolders;                 // Absolute path to status map
  WSet<WString> m_LockedFiles;
  WMap<WString, WFileStatus> m_TransiendFiles; // Absolute path to stat for files outside the data directories.
};

#endif
