#include <ToolsFoundation/ToolsFoundationDLL.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER) && W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

#  include <ToolsFoundation/FileSystem/FileSystemModel.h>
#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/Algorithm/HashStream.h>
#  include <Foundation/Configuration/SubSystem.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Time/Stopwatch.h>
#  include <Foundation/Utilities/Progress.h>

W_IMPLEMENT_SINGLETON(WFileSystemModel);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, FileSystemModel)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WFileSystemModel);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WFileSystemModel* pDummy = WFileSystemModel::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  thread_local WHybridArray<WFileChangedEvent, 2, WStaticsAllocatorWrapper> g_PostponedFiles;
  thread_local bool g_bInFileBroadcast = false;
  thread_local WHybridArray<WFolderChangedEvent, 2, WStaticsAllocatorWrapper> g_PostponedFolders;
  thread_local bool g_bInFolderBroadcast = false;
} // namespace

WFolderChangedEvent::WFolderChangedEvent(const WDataDirPath& file, Type type)
  : m_Path(file)
  , m_Type(type)
{
}

WFileChangedEvent::WFileChangedEvent(const WDataDirPath& file, WFileStatus status, Type type)
  : m_Path(file)
  , m_Status(status)
  , m_Type(type)
{
}

bool WFileSystemModel::IsSameFile(const WStringView sAbsolutePathA, const WStringView sAbsolutePathB)
{
#  if (W_ENABLED(W_SUPPORTS_CASE_INSENSITIVE_PATHS))
  return sAbsolutePathA.IsEqual_NoCase(sAbsolutePathB);
#  else
  return sAbsolutePathA.IsEqual(sAbsolutePathB);
#  endif
}

////////////////////////////////////////////////////////////////////////
// WAssetFiles
////////////////////////////////////////////////////////////////////////

WFileSystemModel::WFileSystemModel()
  : m_SingletonRegistrar(this)
{
}

WFileSystemModel::~WFileSystemModel() = default;

void WFileSystemModel::Initialize(const WApplicationFileSystemConfig& fileSystemConfig, WFileSystemModel::FilesMap&& referencedFiles, WFileSystemModel::FoldersMap&& referencedFolders)
{
  {
    W_PROFILE_SCOPE("Initialize");
    W_LOCK(m_FilesMutex);
    m_FileSystemConfig = fileSystemConfig;

    m_ReferencedFiles = std::move(referencedFiles);
    m_ReferencedFolders = std::move(referencedFolders);

    WStringBuilder sDataDirPath;
    m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
    for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      if (WFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
      {
        WLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        m_DataDirRoots.PushBack({});
      }
      else
      {
        sDataDirPath.MakeCleanPath();
        sDataDirPath.Trim(nullptr, "/");

        m_DataDirRoots.PushBack(sDataDirPath);

        // The root should always be in the model so that every file's parent folder is present in the model.
        m_ReferencedFolders.FindOrAdd(WDataDirPath(sDataDirPath, m_DataDirRoots, i)).Value() = WFileStatus::Status::Valid;
      }
    }

    // Update data dir index and remove files no longer inside a data dir.
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid();)
    {
      const bool bValid = it.Key().UpdateDataDirInfos(m_DataDirRoots, it.Key().GetDataDirIndex());
      if (!bValid)
      {
        it = m_ReferencedFiles.Remove(it);
      }
      else
      {
        ++it;
      }
    }
    for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid();)
    {
      const bool bValid = it.Key().UpdateDataDirInfos(m_DataDirRoots, it.Key().GetDataDirIndex());
      if (!bValid)
      {
        it = m_ReferencedFolders.Remove(it);
      }
      else
      {
        ++it;
      }
    }

    m_pWatcher = W_DEFAULT_NEW(WFileSystemWatcher, m_FileSystemConfig);
    m_WatcherSubscription = m_pWatcher->m_Events.AddEventHandler(WMakeDelegate(&WFileSystemModel::OnAssetWatcherEvent, this));
    m_pWatcher->Initialize();
    m_bInitialized = true;
  }
  FireFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset);
}


void WFileSystemModel::Deinitialize(WFileSystemModel::FilesMap* out_pReferencedFiles, WFileSystemModel::FoldersMap* out_pReferencedFolders)
{
  {
    W_LOCK(m_FilesMutex);
    W_PROFILE_SCOPE("Deinitialize");
    m_pWatcher->m_Events.RemoveEventHandler(m_WatcherSubscription);
    m_pWatcher->Deinitialize();
    m_pWatcher.Clear();

    if (out_pReferencedFiles)
    {
      m_ReferencedFiles.Swap(*out_pReferencedFiles);
    }
    if (out_pReferencedFolders)
    {
      m_ReferencedFolders.Swap(*out_pReferencedFolders);
    }
    m_ReferencedFiles.Clear();
    m_ReferencedFolders.Clear();
    m_LockedFiles.Clear();
    m_FileSystemConfig = WApplicationFileSystemConfig();
    m_DataDirRoots.Clear();
    m_bInitialized = false;
  }
  FireFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset);
}

void WFileSystemModel::MainThreadTick()
{
  if (m_pWatcher)
    m_pWatcher->MainThreadTick();
}

const WFileSystemModel::LockedFiles WFileSystemModel::GetFiles() const
{
  return LockedFiles(m_FilesMutex, &m_ReferencedFiles);
}


const WFileSystemModel::LockedFolders WFileSystemModel::GetFolders() const
{
  return LockedFolders(m_FilesMutex, &m_ReferencedFolders);
}

void WFileSystemModel::NotifyOfChange(WStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return;

  W_ASSERT_DEV(WPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for directory iteration.");

  WStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  sPath.Trim(nullptr, "/");
  if (sPath.IsEmpty())
    return;
  WDataDirPath folder(sPath, m_DataDirRoots);

  // We ignore any changes outside the model's data dirs.
  if (!folder.IsValid())
    return;

  HandleSingleFile(std::move(folder), true);
}

void WFileSystemModel::CheckFileSystem()
{
  if (!m_bInitialized)
    return;

  W_PROFILE_SCOPE("CheckFileSystem");

  WUniquePtr<WProgressRange> range = nullptr;
  if (WThreadUtils::IsMainThread())
    range = W_DEFAULT_NEW(WProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  {
    SetAllStatusUnknown();

    // check every data directory
    for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); i++)
    {
      auto& dd = m_FileSystemConfig.m_DataDirs[i];
      if (WThreadUtils::IsMainThread())
        range->BeginNextStep(dd.m_sDataDirSpecialPath);
      if (!m_DataDirRoots[i].IsEmpty())
      {
        CheckFolder(m_DataDirRoots[i]);
      }
    }

    RemoveStaleFileInfos();
  }

  if (WThreadUtils::IsMainThread())
  {
    range = nullptr;
  }

  FireFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset);
}


WResult WFileSystemModel::FindFile(WStringView sPath, WFileStatus& out_stat) const
{
  if (!m_bInitialized)
    return W_FAILURE;

  W_LOCK(m_FilesMutex);
  WFileSystemModel::FilesMap::ConstIterator it;
  if (WPathUtils::IsAbsolutePath(sPath))
  {
    it = m_ReferencedFiles.Find(sPath);
  }
  else
  {
    // Data dir parent relative?
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      WStringBuilder sDataDir;
      WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
      sDataDir.PathParentDirectory();
      sDataDir.AppendPath(sPath);
      it = m_ReferencedFiles.Find(sDataDir);
      if (it.IsValid())
        break;
    }

    if (!it.IsValid())
    {
      // Data dir relative?
      for (const auto& dd : m_FileSystemConfig.m_DataDirs)
      {
        WStringBuilder sDataDir;
        WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
        sDataDir.AppendPath(sPath);
        it = m_ReferencedFiles.Find(sDataDir);
        if (it.IsValid())
          break;
      }
    }
  }

  if (it.IsValid())
  {
    out_stat = it.Value();
    return W_SUCCESS;
  }
  return W_FAILURE;
}


WResult WFileSystemModel::FindFile(WDelegate<bool(const WDataDirPath&, const WFileStatus&)> visitor) const
{
  if (!m_bInitialized)
    return W_FAILURE;

  W_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    if (visitor(it.Key(), it.Value()))
      return W_SUCCESS;
  }
  return W_FAILURE;
}


WResult WFileSystemModel::LinkDocument(WStringView sAbsolutePath, const WUuid& documentId)
{
  if (!m_bInitialized || !documentId.IsValid())
    return W_FAILURE;

  WDataDirPath filePath;
  WFileStatus fileStatus;
  {
    W_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      // Store status before updates so we can fire the unlink if a guid was already set.
      fileStatus = it.Value();
      it.Value().m_DocumentID = documentId;
      filePath = it.Key();
    }
    else
    {
      return W_FAILURE;
    }
  }

  if (fileStatus.m_DocumentID != documentId)
  {
    if (fileStatus.m_DocumentID.IsValid())
    {
      FireFileChangedEvent(filePath, fileStatus, WFileChangedEvent::Type::DocumentUnlinked);
    }
    fileStatus.m_DocumentID = documentId;
    FireFileChangedEvent(std::move(filePath), fileStatus, WFileChangedEvent::Type::DocumentLinked);
  }
  return W_SUCCESS;
}

WResult WFileSystemModel::UnlinkDocument(WStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return W_FAILURE;

  WDataDirPath filePath;
  WFileStatus fileStatus;
  bool bDocumentLinkChanged = false;
  {
    W_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bDocumentLinkChanged = it.Value().m_DocumentID.IsValid();
      fileStatus = it.Value();
      it.Value().m_DocumentID = WUuid::MakeInvalid();
      filePath = it.Key();
    }
    else
    {
      return W_FAILURE;
    }
  }

  if (bDocumentLinkChanged)
  {
    FireFileChangedEvent(std::move(filePath), fileStatus, WFileChangedEvent::Type::DocumentUnlinked);
  }
  return W_SUCCESS;
}

WResult WFileSystemModel::HashFile(WStringView sAbsolutePath, WFileStatus& out_stat)
{
  if (!m_bInitialized)
    return W_FAILURE;

  W_ASSERT_DEV(WPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for hashing.");

  WStringBuilder sAbsolutePath2(sAbsolutePath);
  sAbsolutePath2.MakeCleanPath();
  sAbsolutePath2.Trim("", "/");
  if (sAbsolutePath2.IsEmpty())
    return W_FAILURE;

  WDataDirPath file(sAbsolutePath2, m_DataDirRoots);

  WFileStats statDep;
  if (WOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
  {
    WLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
    return W_FAILURE;
  }

  // We ignore any changes outside the model's data dirs.
  if (file.IsValid())
  {
    {
      W_LOCK(m_FilesMutex);
      auto it = m_ReferencedFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // We can only hash files that are tracked.
    if (out_stat.m_Status == WFileStatus::Status::Unknown)
    {
      out_stat = HandleSingleFile(file, statDep, false);
      if (out_stat.m_Status == WFileStatus::Status::Unknown)
      {
        WLog::Error("Failed to hash file '{0}', update failed", sAbsolutePath2);
        return W_FAILURE;
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, WTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      WFileReader fileReader;
      if (fileReader.Open(sAbsolutePath2).Failed())
      {
        MarkFileLocked(sAbsolutePath2);
        WLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return W_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (WOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        WLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return W_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash = WFileSystemModel::HashFile(fileReader, nullptr);
      out_stat.m_Status = WFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      W_LOCK(m_FilesMutex);
      m_ReferencedFiles.Insert(file, out_stat);
    }
    return W_SUCCESS;
  }
  else
  {
    {
      W_LOCK(m_FilesMutex);
      auto it = m_TransiendFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, WTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      WFileReader modifiedFile;
      if (modifiedFile.Open(sAbsolutePath2).Failed())
      {
        WLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return W_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (WOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        WLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return W_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash = WFileSystemModel::HashFile(modifiedFile, nullptr);
      out_stat.m_Status = WFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      W_LOCK(m_FilesMutex);
      m_TransiendFiles.Insert(sAbsolutePath2, out_stat);
    }
    return W_SUCCESS;
  }
}


WUInt64 WFileSystemModel::HashFile(WStreamReader& ref_inputStream, WStreamWriter* pPassThroughStream)
{
  WHashStreamWriter64 hsw;

  FILESYSTEM_PROFILE("HashFile");
  WUInt8 cachedBytes[1024 * 10];

  while (true)
  {
    const WUInt64 uiRead = ref_inputStream.ReadBytes(cachedBytes, W_ARRAY_SIZE(cachedBytes));

    if (uiRead == 0)
      break;

    hsw.WriteBytes(cachedBytes, uiRead).AssertSuccess();

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(cachedBytes, uiRead).AssertSuccess();
  }

  return hsw.GetHashValue();
}

WResult WFileSystemModel::ReadDocument(WStringView sAbsolutePath, const WDelegate<void(const WFileStatus&, WStreamReader&)>& callback)
{
  if (!m_bInitialized)
    return W_FAILURE;

  WStringBuilder sAbsolutePath2(sAbsolutePath);
  sAbsolutePath2.MakeCleanPath();
  sAbsolutePath2.Trim(nullptr, "/");

  // try to read the asset file
  WFileReader file;
  if (file.Open(sAbsolutePath2) == W_FAILURE)
  {
    MarkFileLocked(sAbsolutePath2);
    WLog::Error("Failed to open file '{0}'", sAbsolutePath2);
    return W_FAILURE;
  }

  // Get model state.
  WFileStatus stat;
  {
    W_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath2);
    if (!it.IsValid())
      return W_FAILURE;

    stat = it.Value();
  }

  // Get current state.
  WFileStats statDep;
  if (WOSFile::GetFileStats(sAbsolutePath, statDep).Failed())
  {
    WLog::Error("Failed to retrieve file stats '{0}'", sAbsolutePath);
    return W_FAILURE;
  }

  WDefaultMemoryStreamStorage storage;
  WMemoryStreamReader MemReader(&storage);
  MemReader.SetDebugSourceInformation(sAbsolutePath);

  WMemoryStreamWriter MemWriter(&storage);
  stat.m_LastModified = statDep.m_LastModificationTime;
  stat.m_Status = WFileStatus::Status::Valid;
  stat.m_uiHash = WFileSystemModel::HashFile(file, &MemWriter);

  if (callback.IsValid())
  {
    callback(stat, MemReader);
  }

  bool bFileChanged = false;
  {
    // Update state. No need to compare timestamps we hold a lock on the file via the reader.
    W_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath2);
    if (it.IsValid())
    {
      bFileChanged = !it.Value().m_LastModified.Compare(stat.m_LastModified, WTimestamp::CompareMode::Identical);
      it.Value() = stat;
    }
    else
    {
      W_REPORT_FAILURE("A file was removed from the model while we had a lock on it.");
    }

    if (bFileChanged)
    {
      FireFileChangedEvent(it.Key(), stat, WFileChangedEvent::Type::FileChanged);
    }
  }

  return W_SUCCESS;
}

void WFileSystemModel::SetAllStatusUnknown()
{
  W_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = WFileStatus::Status::Unknown;
  }

  for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
  {
    it.Value() = WFileStatus::Status::Unknown;
  }
}


void WFileSystemModel::RemoveStaleFileInfos()
{
  WSet<WDataDirPath> unknownFiles;
  WSet<WDataDirPath> unknownFolders;
  {
    W_LOCK(m_FilesMutex);
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
    {
      // search for files that existed previously but have not been found anymore recently
      if (it.Value().m_Status == WFileStatus::Status::Unknown)
      {
        unknownFiles.Insert(it.Key());
      }
    }
    for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
    {
      // search for folders that existed previously but have not been found anymore recently
      if (it.Value() == WFileStatus::Status::Unknown)
      {
        unknownFolders.Insert(it.Key());
      }
    }
  }

  for (const WDataDirPath& file : unknownFiles)
  {
    HandleSingleFile(file, false);
  }
  for (const WDataDirPath& folders : unknownFolders)
  {
    HandleSingleFile(folders, false);
  }
}


void WFileSystemModel::CheckFolder(WStringView sAbsolutePath)
{
  WStringBuilder sAbsolutePath2 = sAbsolutePath;
  sAbsolutePath2.MakeCleanPath();
  W_ASSERT_DEV(WPathUtils::IsAbsolutePath(sAbsolutePath2), "Only absolute paths are supported for directory iteration.");
  sAbsolutePath2.Trim(nullptr, "/");

  if (sAbsolutePath2.IsEmpty())
    return;

  WDataDirPath folder(sAbsolutePath2, m_DataDirRoots);

  // We ignore any changes outside the model's data dirs.
  if (!folder.IsValid())
    return;

  bool bExists = false;
  {
    W_LOCK(m_FilesMutex);
    bExists = m_ReferencedFolders.Contains(folder);
  }
  if (!bExists)
  {
    // If the folder does not exist yet we call NotifyOfChange which handles add / removal recursively as well.
    NotifyOfChange(folder);
    return;
  }

  WFileSystemIterator iterator;
  iterator.StartSearch(sAbsolutePath2, WFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

  if (!iterator.IsValid())
    return;

  WStringBuilder sPath;

  WSet<WString> visitedFiles;
  WSet<WString> visitedFolders;
  visitedFolders.Insert(sAbsolutePath2);

  for (; iterator.IsValid(); iterator.Next())
  {
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sName);
    sPath.MakeCleanPath();
    if (iterator.GetStats().m_bIsDirectory)
      visitedFolders.Insert(sPath);
    else
      visitedFiles.Insert(sPath);

    WDataDirPath path(sPath, m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), iterator.GetStats(), false);
  }

  WDynamicArray<WString> missingFiles;
  WDynamicArray<WString> missingFolders;

  {
    W_LOCK(m_FilesMutex);

    // As we are using WCompareDataDirPath, entries of different casing interleave but we are only interested in the ones with matching casing so we skip the rest.
    for (auto it = m_ReferencedFiles.LowerBound(sAbsolutePath2.GetView()); it.IsValid(); ++it)
    {
      if (WPathUtils::IsSubPath(sAbsolutePath2, it.Key().GetAbsolutePath()) && !visitedFiles.Contains(it.Key().GetAbsolutePath()))
        missingFiles.PushBack(it.Key().GetAbsolutePath());
      if (!it.Key().GetAbsolutePath().StartsWith_NoCase(sAbsolutePath2))
        break;
    }

    for (auto it = m_ReferencedFolders.LowerBound(sAbsolutePath2.GetView()); it.IsValid(); ++it)
    {
      if (WPathUtils::IsSubPath(sAbsolutePath2, it.Key().GetAbsolutePath()) && !visitedFolders.Contains(it.Key().GetAbsolutePath()))
        missingFolders.PushBack(it.Key().GetAbsolutePath());
      if (!it.Key().GetAbsolutePath().StartsWith_NoCase(sAbsolutePath2))
        break;
    }
  }

  for (WString& sFile : missingFiles)
  {
    WDataDirPath path(std::move(sFile), m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), false);
  }

  // Delete sub-folders before parent folders.
  missingFolders.Sort([](const WString& lhs, const WString& rhs) -> bool
    { return WStringUtils::Compare(lhs, rhs) > 0; });
  for (WString& sFolder : missingFolders)
  {
    WDataDirPath path(std::move(sFolder), m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), false);
  }
}

void WFileSystemModel::OnAssetWatcherEvent(const WFileSystemWatcherEvent& e)
{
  switch (e.m_Type)
  {
    case WFileSystemWatcherEvent::Type::FileAdded:
    case WFileSystemWatcherEvent::Type::FileRemoved:
    case WFileSystemWatcherEvent::Type::FileChanged:
    case WFileSystemWatcherEvent::Type::DirectoryAdded:
    case WFileSystemWatcherEvent::Type::DirectoryRemoved:
      NotifyOfChange(e.m_sPath);
      break;
  }
}

WFileStatus WFileSystemModel::HandleSingleFile(WDataDirPath absolutePath, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile");

  WFileStats Stats;
  const WResult statCheck = WOSFile::GetFileStats(absolutePath, Stats);

#  if W_ENABLED(W_PLATFORM_WINDOWS)
  if (statCheck.Succeeded() && Stats.m_sName != WPathUtils::GetFileNameAndExtension(absolutePath))
  {
    // Casing has changed.
    WStringBuilder sCorrectCasingPath = absolutePath.GetAbsolutePath();
    sCorrectCasingPath.ChangeFileNameAndExtension(Stats.m_sName);
    WDataDirPath correctCasingPath(sCorrectCasingPath.GetView(), m_DataDirRoots, absolutePath.GetDataDirIndex());
    // Add new casing
    WFileStatus res = HandleSingleFile(std::move(correctCasingPath), Stats, bRecurseIntoFolders);
    // Remove old casing
    RemoveFileOrFolder(absolutePath, bRecurseIntoFolders);
    return res;
  }
#  endif

  if (statCheck.Failed())
  {
    RemoveFileOrFolder(absolutePath, bRecurseIntoFolders);
    return {};
  }

  return HandleSingleFile(std::move(absolutePath), Stats, bRecurseIntoFolders);
}


WFileStatus WFileSystemModel::HandleSingleFile(WDataDirPath absolutePath, const WFileStats& FileStat, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile2");

  if (FileStat.m_bIsDirectory)
  {
    WFileStatus status;
    status.m_Status = WFileStatus::Status::Valid;

    bool bExisted = false;
    {
      W_LOCK(m_FilesMutex);
      auto it = m_ReferencedFolders.FindOrAdd(absolutePath, &bExisted);
      it.Value() = WFileStatus::Status::Valid;
    }

    if (!bExisted)
    {
      FireFolderChangedEvent(absolutePath, WFolderChangedEvent::Type::FolderAdded);
      if (bRecurseIntoFolders)
        CheckFolder(absolutePath);
    }

    return status;
  }
  else
  {
    WFileStatus status;
    bool bExisted = false;
    bool bFileChanged = false;
    {
      W_LOCK(m_FilesMutex);
      auto it = m_ReferencedFiles.FindOrAdd(absolutePath, &bExisted);
      WFileStatus& value = it.Value();
      bFileChanged = !value.m_LastModified.Compare(FileStat.m_LastModificationTime, WTimestamp::CompareMode::Identical);
      if (bFileChanged)
      {
        value.m_uiHash = 0;
      }

      // If the state is unknown, we loaded it from the cache and need to fire FileChanged to update dependent systems.
      // #TODO_ASSET This behaviors should be changed once the asset cache is stored less lossy.
      bFileChanged |= value.m_Status == WFileStatus::Status::Unknown;
      // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
      value.m_Status = WFileStatus::Status::Valid;
      value.m_LastModified = FileStat.m_LastModificationTime;
      status = value;
    }

    if (!bExisted)
    {
      FireFileChangedEvent(absolutePath, status, WFileChangedEvent::Type::FileAdded);
    }
    else if (bFileChanged)
    {
      FireFileChangedEvent(absolutePath, status, WFileChangedEvent::Type::FileChanged);
    }
    return status;
  }
}

void WFileSystemModel::RemoveFileOrFolder(const WDataDirPath& absolutePath, bool bRecurseIntoFolders)
{
  WFileStatus fileStatus;
  bool bFileExisted = false;
  bool bFolderExisted = false;
  {
    W_LOCK(m_FilesMutex);
    if (auto it = m_ReferencedFiles.Find(absolutePath); it.IsValid())
    {
      bFileExisted = true;
      fileStatus = it.Value();
      m_ReferencedFiles.Remove(it);
    }
    if (auto it = m_ReferencedFolders.Find(absolutePath); it.IsValid())
    {
      bFolderExisted = true;
      m_ReferencedFolders.Remove(it);
    }
  }

  if (bFileExisted)
  {
    FireFileChangedEvent(absolutePath, fileStatus, WFileChangedEvent::Type::FileRemoved);
  }

  if (bFolderExisted)
  {
    if (bRecurseIntoFolders)
    {
      WSet<WDataDirPath> previouslyKnownFiles;
      {
        FILESYSTEM_PROFILE("FindReferencedFiles");
        W_LOCK(m_FilesMutex);
        auto itlowerBound = m_ReferencedFiles.LowerBound(absolutePath);
        while (itlowerBound.IsValid())
        {
          if (WPathUtils::IsSubPath(absolutePath, itlowerBound.Key().GetAbsolutePath()))
          {
            previouslyKnownFiles.Insert(itlowerBound.Key());
          }
          // As we are using WCompareDataDirPath, entries of different casing interleave but we are only interested in the ones with matching casing so we skip the rest.
          if (!itlowerBound.Key().GetAbsolutePath().StartsWith_NoCase(absolutePath.GetAbsolutePath()))
          {
            break;
          }
          ++itlowerBound;
        }
      }
      {
        FILESYSTEM_PROFILE("HandleRemovedFiles");
        for (const WDataDirPath& file : previouslyKnownFiles)
        {
          RemoveFileOrFolder(file, false);
        }
      }
    }
    FireFolderChangedEvent(absolutePath, WFolderChangedEvent::Type::FolderRemoved);
  }
}

void WFileSystemModel::MarkFileLocked(WStringView sAbsolutePath)
{
  W_LOCK(m_FilesMutex);
  auto it = m_ReferencedFiles.Find(sAbsolutePath);
  if (it.IsValid())
  {
    it.Value().m_Status = WFileStatus::Status::FileLocked;
    m_LockedFiles.Insert(sAbsolutePath);
  }
}

void WFileSystemModel::FireFileChangedEvent(const WDataDirPath& file, WFileStatus fileStatus, WFileChangedEvent::Type type)
{
  // We queue up all requests on a thread and only return once the list is empty. The reason for this is that:
  // A: We don't want to allow recursive event calling as it creates limbo states in the model and hard to debug bugs.
  // B: If a user calls NotifyOfChange, the function should only return if the event and any indirect events that were triggered by the event handlers have been processed.

  WFileChangedEvent& e = g_PostponedFiles.ExpandAndGetRef();
  e.m_Path = file;
  e.m_Status = fileStatus;
  e.m_Type = type;

  if (g_bInFileBroadcast)
  {
    return;
  }

  g_bInFileBroadcast = true;
  W_SCOPE_EXIT(g_bInFileBroadcast = false);

  for (WUInt32 i = 0; i < g_PostponedFiles.GetCount(); i++)
  {
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    WFileChangedEvent tempEvent = std::move(g_PostponedFiles[i]);
    m_FileChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFiles.Clear();
}

void WFileSystemModel::FireFolderChangedEvent(const WDataDirPath& file, WFolderChangedEvent::Type type)
{
  // See comment in FireFileChangedEvent.
  WFolderChangedEvent& e = g_PostponedFolders.ExpandAndGetRef();
  e.m_Path = file;
  e.m_Type = type;

  if (g_bInFolderBroadcast)
  {
    return;
  }

  g_bInFolderBroadcast = true;
  W_SCOPE_EXIT(g_bInFolderBroadcast = false);

  for (WUInt32 i = 0; i < g_PostponedFolders.GetCount(); i++)
  {
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    WFolderChangedEvent tempEvent = std::move(g_PostponedFolders[i]);
    m_FolderChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFolders.Clear();
}

#endif
