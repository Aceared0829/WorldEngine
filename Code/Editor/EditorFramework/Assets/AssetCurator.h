#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>
#include <ToolsFoundation/FileSystem/Declarations.h>

#include <tuple>

class WUpdateTask;
class WTask;
class WAssetDocumentManager;
class WDirectoryWatcher;
struct WFileStats;
class WAssetProcessorLog;
class WFileSystemWatcher;
class WAssetTableWriter;
struct WFileChangedEvent;
class WFileSystemModel;
class WObjectAccessorBase;


#if 0 // Define to enable extensive curator profile scopes
#  define CURATOR_PROFILE(szName) W_PROFILE_SCOPE(szName)

#else
#  define CURATOR_PROFILE(Name)

#endif

/// Custom mutex that allows to profile the time in the curator lock.
class WCuratorMutex : public WMutex
{
public:
  void Lock()
  {
    CURATOR_PROFILE("WCuratorMutex");
    WMutex::Lock();
  }

  void Unlock() { WMutex::Unlock(); }
};

struct W_EDITORFRAMEWORK_DLL WAssetInfo
{
  WAssetInfo() = default;
  void Update(WUniquePtr<WAssetInfo>& rhs);

  WAssetDocumentManager* GetManager() { return static_cast<WAssetDocumentManager*>(m_pDocumentTypeDescriptor->m_pManager); }

  enum TransformState : WUInt8
  {
    Unknown = 0,
    UpToDate,
    NeedsImport,
    NeedsTransform,
    NeedsThumbnail,
    TransformError,
    MissingTransformDependency,
    MissingThumbnailDependency,
    MissingPackageDependency,
    CircularDependency,
    COUNT,
  };

  WUInt8 m_LastStateUpdate = 0; ///< Changes every time m_TransformState is modified. Used to detect stale computations done outside the lock.
  WAssetExistanceState::Enum m_ExistanceState = WAssetExistanceState::FileAdded;
  TransformState m_TransformState = TransformState::Unknown;
  WUInt64 m_AssetHash = 0;      ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  WUInt64 m_ThumbHash = 0;      ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  WUInt64 m_PackageHash = 0;    ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.

  WDynamicArray<WLogEntry> m_LogEntries;

  const WAssetDocumentTypeDescriptor* m_pDocumentTypeDescriptor = nullptr;
  WDataDirPath m_Path;

  WUniquePtr<WAssetDocumentInfo> m_Info;

  WSet<WString> m_MissingTransformDeps;
  WSet<WString> m_MissingThumbnailDeps;
  WSet<WString> m_MissingPackageDeps;
  WSet<WString> m_CircularDependencies;

  WSet<WUuid> m_SubAssets; ///< Main asset uses the same GUID as this (see m_Info), but is NOT stored in m_SubAssets

  /// Returns the values that the last transform recorded. \see WAssetInfoFile
  ///
  /// Returns nullptr when the asset type recorded nothing, when the asset was never transformed, or when the file on
  /// disk belongs to an older transform.
  ///
  /// Only call this while holding the WAssetCurator lock (see WLockedSubAsset and WLockedAssetTable).
  const WAssetInfoFile* GetTransformInfo(WStringView sOutputTag = {}, const WPlatformProfile* pAssetProfile = nullptr) const;

  /// Drops what GetTransformInfo() cached.
  void ClearTransformInfoCache() { m_TransformInfoCache.Clear(); }

private:
  // Filled on demand by GetTransformInfo(). Mutable, because reading a file lazily is not a logical change to the asset.
  // Keyed by everything that selects a different file: an asset type that transforms per profile has one file per profile.
  struct TransformInfoCache
  {
    WUInt64 m_uiAssetHash = 0;
    WString m_sOutputTag;
    const WPlatformProfile* m_pAssetProfile = nullptr;
    bool m_bValid = false; ///< Whether a file was found. A miss is cached too, so it is not retried on every call.
    WAssetInfoFile m_Info;
  };

  // A deque, so that adding an entry does not invalidate pointers that GetTransformInfo() handed out earlier.
  mutable WDeque<TransformInfoCache> m_TransformInfoCache;

  W_DISALLOW_COPY_AND_ASSIGN(WAssetInfo);
};

/// Information about an asset or sub-asset.
struct W_EDITORFRAMEWORK_DLL WSubAsset
{
  WStringView GetName() const;
  void GetSubAssetIdentifier(WStringBuilder& out_sPath) const;

  WAssetExistanceState::Enum m_ExistanceState = WAssetExistanceState::FileAdded;
  WAssetInfo* m_pAssetInfo = nullptr;
  WTime m_LastAccess;
  bool m_bMainAsset = true;

  WSubAssetData m_Data;
};



struct WAssetCuratorEvent
{
  enum class Type
  {
    AssetAdded,
    AssetRemoved,
    AssetMoved,
    AssetUpdated,
    AssetListReset,
    ActivePlatformChanged,
  };

  WUuid m_AssetGuid;
  const WSubAsset* m_pInfo;
  Type m_Type;
};

class W_EDITORFRAMEWORK_DLL WAssetCurator
{
  W_DECLARE_SINGLETON(WAssetCurator);

public:
  WAssetCurator();
  ~WAssetCurator();

  /// \name Setup
  ///@{

  /// Starts init task. Need to call WaitForInitialize to finish before loading docs.
  void StartInitialize(const WApplicationFileSystemConfig& cfg);
  /// Waits for init task to finish.
  void WaitForInitialize();
  void Deinitialize();

  void MainThreadTick(bool bTopLevel);

  ///@}
  /// \name Asset Platform Configurations
  ///@{

public:
  /// The main platform on which development happens. E.g. "Default".
  ///
  /// TODO: review this concept
  const WPlatformProfile* GetDevelopmentAssetProfile() const;

  /// The currently active target platform for asset processing.
  const WPlatformProfile* GetActiveAssetProfile() const;

  /// Returns the index of the currently active asset platform configuration
  WUInt32 GetActiveAssetProfileIndex() const;

  /// Returns WInvalidIndex if no config with the given name exists. Name comparison is case insensitive.
  WUInt32 FindAssetProfileByName(const char* szPlatform);

  WUInt32 GetNumAssetProfiles() const;

  /// Always returns a valid config. E.g. even if WInvalidIndex is passed in, it will fall back to the default config (at index 0).
  const WPlatformProfile* GetAssetProfile(WUInt32 uiIndex) const;

  /// Always returns a valid config. E.g. even if WInvalidIndex is passed in, it will fall back to the default config (at index 0).
  WPlatformProfile* GetAssetProfile(WUInt32 uiIndex);

  /// Adds a new profile. The name should be set afterwards to a unique name.
  WPlatformProfile* CreateAssetProfile();

  /// Deletes the given asset profile, if possible.
  ///
  /// The function fails when the given profile is the main profile (at index 0),
  /// or it is the currently active profile.
  WResult DeleteAssetProfile(WPlatformProfile* pProfile);

  /// Switches the currently active asset target platform.
  ///
  /// Broadcasts WAssetCuratorEvent::Type::ActivePlatformChanged on change.
  void SetActiveAssetProfileByIndex(WUInt32 uiIndex, bool bForceReevaluation = false);

  /// Saves the current asset configurations. Returns failure if the output file could not be written to.
  WResult SaveAssetProfiles();

  void SaveRuntimeProfiles();

  ///@}
  /// \name Asset Reference Replacement
  ///@{

  /// Result of a bulk asset reference replacement operation.
  struct ReplaceAssetResult
  {
    WUInt32 m_uiDocumentsModified = 0;
    WUInt32 m_uiDocumentsFailed = 0;
    WUInt32 m_uiPropertiesReplaced = 0;
    WDynamicArray<WString> m_Errors;
  };

  /// Recursively replaces asset references in all properties of a document object and its children.
  ///
  /// \return The number of properties that were successfully replaced.
  WUInt32 ReplaceAssetReferenceInObject(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, WStringView sOldReference, WStringView sNewReference, WDynamicArray<WString>& out_errors);

  /// Replaces all asset references in a document, wrapped in a transaction.
  ///
  /// \return The number of properties that were successfully replaced.
  WUInt32 ReplaceAssetReferenceInDocument(WDocument* pDocument, WStringView sOldReference, WStringView sNewReference, WDynamicArray<WString>& out_errors);

  /// Replaces asset references in all documents that directly use the specified asset.
  ///
  /// Opens each referencing document, replaces all occurrences, and saves it.
  /// Documents are opened without a window. Errors are collected but do not abort the operation.
  ReplaceAssetResult ReplaceAssetReferenceInUses(WUuid assetToReplace, WStringView sOldReference, WStringView sNewReference);

private:
  void ClearAssetProfiles();
  void SetupDefaultAssetProfiles();
  WResult LoadAssetProfiles();
  void ComputeAllDocumentManagerAssetProfileHashes();

  WHybridArray<WPlatformProfile*, 8> m_AssetProfiles;

  ///@}
  /// \name High Level Functions
  ///@{

public:
  WDateTime GetLastFullTransformDate() const;
  void StoreFullTransformDate();

  /// Transforms all assets and writes the lookup tables. If the given platform is empty, the active platform is used.
  ///
  /// Pass WTransformFlags::TriggeredManually to make sure that all assets get transformed,
  /// even the ones that should not be transformed by background processors (mainly scenes).
  WStatus TransformAllAssets(WBitflags<WTransformFlags> transformFlags = WTransformFlags::TriggeredManually, const WPlatformProfile* pAssetProfile = nullptr);
  WTransformStatus TransformAsset(const WUuid& assetGuid, WBitflags<WTransformFlags> transformFlags, const WPlatformProfile* pAssetProfile = nullptr);
  WTransformStatus CreateThumbnail(const WUuid& assetGuid);

  void ResaveAllAssets(WStringView sPrefixPath);

  /// Some assets are not automatically updated by the asset dependency detection (mainly Collections) because of their transitive data dependencies.
  /// So we must update them when the user does something 'significant' like doing TransformAllAssets or a scene export.
  void TransformAssetsForSceneExport(const WPlatformProfile* pAssetProfile = nullptr);

  /// Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  WResult WriteAssetTables(const WPlatformProfile* pAssetProfile = nullptr, bool bForce = false);

  ///@}
  /// \name Asset Access
  ///@{
  using WLockedSubAsset = WLockedObject<WMutex, const WSubAsset>;

  /// Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be a stringyfied asset GUID or a relative or absolute path. The function will try all possibilities.
  /// If no asset can be found, an empty/invalid WAssetInfo is returned.
  /// If bExhaustiveSearch is set the function will go through all known assets and find the closest match.
  const WLockedSubAsset FindSubAsset(WStringView sPathOrGuid, bool bExhaustiveSearch = false) const;

  /// Same as GetAssteInfo, but wraps the return value into a WLockedSubAsset struct
  const WLockedSubAsset GetSubAsset(const WUuid& assetGuid) const;

  using WLockedSubAssetTable = WLockedObject<WMutex, const WHashTable<WUuid, WSubAsset>>;

  /// Returns the table of all known assets in a locked structure
  const WLockedSubAssetTable GetKnownSubAssets() const;

  using WLockedAssetTable = WLockedObject<WMutex, const WHashTable<WUuid, WAssetInfo*>>;

  /// Returns the table of all known assets in a locked structure
  const WLockedAssetTable GetKnownAssets() const;

  /// Computes the transform hash for the asset and its transform dependencies. Returns 0 if anything went wrong.
  WUInt64 GetAssetTransformHash(WUuid assetGuid);

  /// Computes the thumbnail hash for the asset and its thumbnail dependencies. Returns 0 if anything went wrong.
  WUInt64 GetAssetThumbnailHash(WUuid assetGuid);

  WAssetInfo::TransformState IsAssetUpToDate(const WUuid& assetGuid, const WPlatformProfile* pAssetProfile, const WAssetDocumentTypeDescriptor* pTypeDescriptor, WUInt64& out_uiAssetHash, WUInt64& out_uiThumbHash, WUInt64& out_uiPackageHash, bool bForce = false);
  /// Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(WUInt32& out_uiNumAssets, WHybridArray<WUInt32, WAssetInfo::TransformState::COUNT>& out_count);

  /// Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  WString FindDataDirectoryForAsset(WStringView sAbsoluteAssetPath) const;

  /// Collects all main asset GUIDs located within the specified folder path.
  ///
  /// Searches through all known assets and adds the string representation of their GUID
  /// to the output array if their absolute path starts with the given folder path.
  /// Only main assets are included (not sub-assets).
  void GetAllAssetsInFolder(WStringView sFolderPath, WDynamicArray<WString>& out_assetGuids) const;

  /// Uses knowledge about all existing files on disk to find the best match for a file. Very slow.
  ///
  /// \param sFile
  ///   File name (may include a path) to search for. Will be modified both on success and failure to give a 'reasonable' result.
  WResult FindBestMatchForFile(WStringBuilder& ref_sFile, WArrayPtr<WString> allowedFileExtensions) const;

  /// Finds all assets that reference the given asset, as a transform, thumbnail or package dependency.
  ///
  /// \param assetGuid
  ///   The asset to find use cases for.
  /// \param ref_uses
  ///   List of assets that use 'assetGuid'. Any previous content of the set is not removed.
  /// \param bTransitive
  ///   If set, will also find indirect uses of the asset.
  void FindAllUses(WUuid assetGuid, WSet<WUuid>& ref_uses, bool bTransitive) const;

  /// Returns all assets that reference a file, as a transform, thumbnail or package dependency. Use this to e.g. figure out which assets still reference a .tga file in the project.
  ///
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \param ref_uses List of assets that use 'sAbsolutePath'. Any previous content of the set is not removed.
  void FindAllUses(WStringView sAbsolutePath, WSet<WUuid>& ref_uses) const;

  /// Returns whether a file is referenced, i.e. used for transforming an asset. Use this to e.g. figure out whether a .tga file is still in use by any asset.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \return True, if at least one asset references the given file.
  bool IsReferenced(WStringView sAbsolutePath) const;


  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

  /// Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfFileChange(WStringView sAbsolutePath);
  /// Allows to tell the system to re-evaluate an assets status.
  void NotifyOfAssetChange(const WUuid& assetGuid);
  void UpdateAssetLastAccessTime(const WUuid& assetGuid);

  /// Checks file system for any changes. Call in case the file system watcher does not pick up certain changes.
  void CheckFileSystem();

  void NeedsReloadResources(const WUuid& assetGuid) const;

  void InvalidateAssetsWithTransformState(WAssetInfo::TransformState state);

  ///@}
  /// \name Utilities
  ///@{

  /// Generates one transitive hull for all the dependencies that are enabled. The set will contain dependencies that are reachable via any combination of enabled reference types.
  void GenerateTransitiveHull(const WStringView sAssetOrPath, WSet<WString>& inout_deps, WBitflags<WDependencyFlags> dependencyTypes) const;

  /// Generates one transitive hull for all the asset dependencies that are enabled. The set will contain dependencies that are reachable via any combination of enabled reference types.
  void GenerateTransitiveAssetHull(const WUuid& assetGuid, WSet<WUuid>& inout_deps, WBitflags<WDependencyFlags> dependencyTypes);

  /// Copies each value of deps into out_SettingsHashMap and fills the value of assets with the hash value of the dependencyType. For files, the file hash is used.
  void GenerateSettingsHashMap(const WSet<WString>& deps, WBitflags<WDependencyFlags> dependencyType, WMap<WString, WUInt64>& out_settingsHashMap) const;

  /// Generates one inverse transitive hull for all the types dependencies that are enabled. The set will contain inverse dependencies that can reach the given asset (pAssetInfo) via any combination of the enabled reference types. As only assets can have dependencies, the inverse hull is always just asset GUIDs.
  void GenerateInverseTransitiveHull(const WAssetInfo* pAssetInfo, WSet<WUuid>& inout_inverseDeps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false) const;

  /// Generates a DGML graph of all transform and thumbnail dependencies.
  void WriteDependencyDGML(const WUuid& guid, WStringView sOutputFile) const;

  struct ExportResult
  {
    WUInt32 m_uiCopiedFiles = 0;
    WUInt32 m_uiFailedFiles = 0;
  };

  /// Exports assets and their dependencies to a destination folder.
  ///
  /// Takes an array of source paths (asset GUIDs as strings or file paths) and exports them
  /// along with all their dependencies to the destination folder. Files are copied preserving
  /// their relative paths from the data directories. Each file is copied only once, even if
  /// it appears in multiple dependency trees.
  ExportResult ExportAssets(WArrayPtr<WString> sources, WStringView sDestinationFolder, WBitflags<WDependencyFlags> includeDependencyTypes = WDependencyFlags::Transform | WDependencyFlags::Thumbnail | WDependencyFlags::Package) const;

  ///@}

public:
  WEvent<const WAssetCuratorEvent&> m_Events;

private:
  /// \name Processing
  ///@{

  WTransformStatus ProcessAsset(WAssetInfo* pAssetInfo, const WPlatformProfile* pAssetProfile, WBitflags<WTransformFlags> transformFlags);
  WStatus ResaveAsset(WAssetInfo* pAssetInfo);
  /// Returns the asset info for the asset with the given GUID or nullptr if no such asset exists.
  WAssetInfo* GetAssetInfo(const WUuid& assetGuid);
  const WAssetInfo* GetAssetInfo(const WUuid& assetGuid) const;

  WSubAsset* GetSubAssetInternal(const WUuid& assetGuid);

  /// Returns the asset info for the asset with the given (stringyfied) GUID or nullptr if no such asset exists.
  WAssetInfo* GetAssetInfo(const WString& sAssetGuid);

  void OnFileChangedEvent(const WFileChangedEvent& e);

  /// Some assets are vital for the engine to run. Each data directory can contain a [DataDirName].WCollectionAsset
  ///   that has all its references transformed before any other documents are loaded.
  void ProcessAllCoreAssets();

  ///@}
  /// \name Update Task
  ///@{

  void RestartUpdateTask();
  void ShutdownUpdateTask();

  bool GetNextAssetToUpdate(WUuid& out_guid, WStringBuilder& out_sAbsPath);
  void OnUpdateTaskFinished(const WSharedPtr<WTask>& pTask);
  void RunNextUpdateTask();

  ///@}
  /// \name Asset Hashing and Status Updates (AssetUpdates.cpp)
  ///@{

  bool AddAssetHash(WString& sPath, WBitflags<WDependencyFlags> dependencyType, WUInt64& out_AssetHash, WUInt64& out_ThumbHash, WUInt64& out_PackageHash, bool bForce);
  WAssetInfo::TransformState HashAsset(WUInt64 uiSettingsHash, const WHybridArray<WString, 16>& assetTransformDeps, const WHybridArray<WString, 16>& assetThumbnailDeps, const WHybridArray<WString, 16>& assetPackageDeps, WSet<WString>& missingTransformDeps, WSet<WString>& missingThumbnailDeps, WSet<WString>& missingPackageDeps, WUInt64& out_AssetHash, WUInt64& out_ThumbHash, WUInt64& out_PackageHash, bool bForce);

  WResult EnsureAssetInfoUpdated(const WDataDirPath& absFilePath, const WFileStatus& stat, bool bForce = false);
  void TrackDependencies(WAssetInfo* pAssetInfo);
  void UntrackDependencies(WAssetInfo* pAssetInfo);
  WResult CheckForCircularDependencies(WAssetInfo* pAssetInfo);
  void UpdateTrackedFiles(const WUuid& assetGuid, const WSet<WString>& files, WMap<WString, WHybridArray<WUuid, 1>>& inverseTracker, WSet<std::tuple<WUuid, WUuid>>& unresolved, bool bAdd);
  void UpdateUnresolvedTrackedFiles(WMap<WString, WHybridArray<WUuid, 1>>& inverseTracker, WSet<std::tuple<WUuid, WUuid>>& unresolved);
  WResult ReadAssetDocumentInfo(const WDataDirPath& absFilePath, const WFileStatus& stat, WUniquePtr<WAssetInfo>& assetInfo);
  WResult UpdateSubAssets(WAssetInfo& assetInfo);

  void RemoveAssetTransformState(const WUuid& assetGuid);
  void InvalidateAssetTransformState(const WUuid& assetGuid);

  WAssetInfo::TransformState UpdateAssetTransformState(WUuid assetGuid, WUInt64& out_AssetHash, WUInt64& out_ThumbHash, WUInt64& out_PackageHash, bool bForce);
  void UpdateAssetTransformState(const WUuid& assetGuid, WAssetInfo::TransformState state);
  void UpdateAssetTransformLog(const WUuid& assetGuid, WDynamicArray<WLogEntry>& logEntries);
  void SetAssetExistanceState(WAssetInfo& assetInfo, WAssetExistanceState::Enum state);

  ///@}
  /// \name Check File System Helper
  ///@{
  void SetAllAssetStatusUnknown();
  void LoadCaches(WMap<WDataDirPath, WFileStatus, WCompareDataDirPath>& out_referencedFiles, WMap<WDataDirPath, WFileStatus::Status, WCompareDataDirPath>& out_referencedFolders);
  void SaveCaches(const WMap<WDataDirPath, WFileStatus, WCompareDataDirPath>& referencedFiles, const WMap<WDataDirPath, WFileStatus::Status, WCompareDataDirPath>& referencedFolders);
  static void BuildFileExtensionSet(WSet<WString>& AllExtensions);

  ///@}
  /// \name Utilities
  ///@{

public:
  /// Deletes all files in all asset caches, except for the asset outputs that exceed the threshold.
  ///
  /// -> OutputReliability::Perfect -> deletes everything
  /// -> OutputReliability::Good -> keeps the 'Perfect' files
  /// -> OutputReliability::Unknown -> keeps the 'Good' and 'Perfect' files
  void ClearAssetCaches(WAssetDocumentManager::OutputReliability threshold);

  ///@}

private:
  friend class WUpdateTask;
  friend class WAssetProcessor;
  friend class WEditorProcessorProcess;

  mutable WCuratorMutex m_CuratorMutex; // Global lock
  WTaskGroupID m_InitializeCuratorTaskID;

  WUInt32 m_uiActiveAssetProfile = 0;

  // Actual data stored in the curator
  WHashTable<WUuid, WAssetInfo*> m_KnownAssets;
  WHashTable<WUuid, WSubAsset> m_KnownSubAssets;

  // Derived dependency lookup tables
  WMap<WString, WHybridArray<WUuid, 1>> m_InverseTransformDeps; // [Absolute path -> asset Guid]
  WMap<WString, WHybridArray<WUuid, 1>> m_InverseThumbnailDeps; // [Absolute path -> asset Guid]
  WMap<WString, WHybridArray<WUuid, 1>> m_InversePackageDeps;   // [Absolute path -> asset Guid]
  WSet<std::tuple<WUuid, WUuid>> m_UnresolvedTransformDeps;      ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  WSet<std::tuple<WUuid, WUuid>> m_UnresolvedThumbnailDeps;
  WSet<std::tuple<WUuid, WUuid>> m_UnresolvedPackageDeps;

  // State caches
  WHashSet<WUuid> m_TransformState[WAssetInfo::TransformState::COUNT];
  WHashSet<WUuid> m_SubAssetChanged; ///< Flushed in main thread tick
  WHashSet<WUuid> m_TransformStateStale;
  WHashSet<WUuid> m_Updating;

  // Serialized cache
  mutable WCuratorMutex m_CachedAssetsMutex; ///< Only locks m_CachedAssets
  WMap<WString, WUniquePtr<WAssetDocumentInfo>> m_CachedAssets;
  WMap<WString, WFileStatus> m_CachedFiles;

  // Immutable data after StartInitialize
  WApplicationFileSystemConfig m_FileSystemConfig;
  WUniquePtr<WAssetTableWriter> m_pAssetTableWriter;
  WSet<WString> m_ValidAssetExtensions;

  // Update task
  bool m_bRunUpdateTask = false;
  WSharedPtr<WUpdateTask> m_pUpdateTask;
  WTaskGroupID m_UpdateTaskGroup;
};

class WUpdateTask final : public WTask
{
public:
  WUpdateTask(WOnTaskFinishedCallback onTaskFinished);
  ~WUpdateTask();

private:
  WStringBuilder m_sAssetPath;

  virtual void Execute() override;
};
