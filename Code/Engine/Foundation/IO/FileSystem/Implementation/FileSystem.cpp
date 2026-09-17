#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringView.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FileSystem)

  ON_CORESYSTEMS_STARTUP
  {
    WFileSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WFileSystem::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WFileSystem::FileSystemData* WFileSystem::s_pData = nullptr;
WString WFileSystem::s_sSdkRootDir;
WMap<WString, WString> WFileSystem::s_SpecialDirectories;


void WFileSystem::RegisterDataDirectoryFactory(WDataDirFactory factory, float fPriority /*= 0*/)
{
  // This assert helps finding cases where the WFileSystem is used without W being properly initialized or already shutdown.
  // The code would crash below anyways but asserts are easier to see in automated testing on e.g. CI.
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  W_LOCK(s_pData->m_FsMutex);

  auto& data = s_pData->m_DataDirFactories.ExpandAndGetRef();
  data.m_Factory = factory;
  data.m_fPriority = fPriority;
}

WEventSubscriptionID WFileSystem::RegisterEventHandler(WEvent<const FileEvent&>::Handler handler)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_Event.AddEventHandler(handler);
}

void WFileSystem::UnregisterEventHandler(WEvent<const FileEvent&>::Handler handler)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(handler);
}

void WFileSystem::UnregisterEventHandler(WEventSubscriptionID subscriptionId)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(subscriptionId);
}

void WFileSystem::CleanUpRootName(WStringBuilder& sRoot)
{
  // this cleaning might actually make the root name empty
  // e.g. ":" becomes ""
  // which is intended to support passing through of absolute paths
  // ie. mounting the empty dir "" under the root ":" will allow to write directly to files using absolute paths

  while (sRoot.StartsWith(":"))
    sRoot.Shrink(1, 0);

  while (sRoot.EndsWith("/"))
    sRoot.Shrink(0, 1);

  sRoot.ToUpper();
}

WResult WFileSystem::AddDataDirectory(WStringView sDataDirectory, WStringView sGroup, WStringView sRootName, WDataDirUsage usage)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_ASSERT_DEV(usage != WDataDirUsage::AllowWrites || !sRootName.IsEmpty(), "A data directory must have a non-empty, unique name to be mounted for write access");

  WStringBuilder sPath = sDataDirectory;
  sPath.MakeCleanPath();

  if (!sPath.IsEmpty() && !sPath.EndsWith("/"))
    sPath.Append("/");

  WStringBuilder sCleanRootName = sRootName;
  CleanUpRootName(sCleanRootName);

  W_LOCK(s_pData->m_FsMutex);

  bool failed = false;
  if (FindDataDirectoryWithRoot(sCleanRootName) != nullptr)
  {
    WLog::Error("A data directory with root name '{0}' already exists.", sCleanRootName);
    failed = true;
  }

  if (!failed)
  {
    s_pData->m_DataDirFactories.Sort([](const auto& a, const auto& b)
      { return a.m_fPriority < b.m_fPriority; });

    // use the factory that was added last as the one with the highest priority -> allows to override already added factories
    for (WInt32 i = s_pData->m_DataDirFactories.GetCount() - 1; i >= 0; --i)
    {
      WDataDirectoryType* pDataDir = s_pData->m_DataDirFactories[i].m_Factory(sPath, sGroup, sRootName, usage);

      if (pDataDir != nullptr)
      {
        WDataDirectoryInfo dd;
        dd.m_Usage = usage;
        dd.m_pDataDirType = pDataDir;
        dd.m_sRootName = sCleanRootName;
        dd.m_sGroup = sGroup;

        s_pData->m_DataDirectories.PushBack(dd);

        {
          // Broadcast that a data directory was added
          FileEvent fe;
          fe.m_EventType = FileEventType::AddDataDirectorySucceeded;
          fe.m_sFileOrDirectory = sPath;
          fe.m_sOther = sCleanRootName;
          fe.m_pDataDir = pDataDir;
          s_pData->m_Event.Broadcast(fe);
        }

        WLog::Dev("Added Data Directory '{}' -> '{}'", sRootName, sDataDirectory);
        return W_SUCCESS;
      }
    }
  }

  {
    // Broadcast that adding a data directory failed
    FileEvent fe;
    fe.m_EventType = FileEventType::AddDataDirectoryFailed;
    fe.m_sFileOrDirectory = sPath;
    fe.m_sOther = sCleanRootName;
    s_pData->m_Event.Broadcast(fe);
  }

  WLog::Error("Adding Data Directory '{0}' failed.", WArgSensitive(sDataDirectory, "Path"));
  return W_FAILURE;
}


bool WFileSystem::RemoveDataDirectory(WStringView sRootName)
{
  WStringBuilder sCleanRootName = sRootName;
  CleanUpRootName(sCleanRootName);

  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_LOCK(s_pData->m_FsMutex);

  for (WUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    const auto& directory = s_pData->m_DataDirectories[i];

    if (directory.m_sRootName == sCleanRootName)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = directory.m_pDataDirType->GetDataDirectoryPath();
        fe.m_sOther = directory.m_sRootName;
        fe.m_pDataDir = directory.m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      directory.m_pDataDirType->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);

      return true;
    }
    else
      ++i;
  }

  return false;
}

WUInt32 WFileSystem::RemoveDataDirectoryGroup(WStringView sGroup)
{
  if (s_pData == nullptr)
    return 0;

  W_LOCK(s_pData->m_FsMutex);

  WUInt32 uiRemoved = 0;

  for (WUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    if (s_pData->m_DataDirectories[i].m_sGroup == sGroup)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirType->GetDataDirectoryPath();
        fe.m_sOther = s_pData->m_DataDirectories[i].m_sRootName;
        fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      ++uiRemoved;

      s_pData->m_DataDirectories[i].m_pDataDirType->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);
    }
    else
      ++i;
  }

  return uiRemoved;
}

void WFileSystem::ClearAllDataDirectories()
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  W_LOCK(s_pData->m_FsMutex);

  for (WInt32 i = s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    {
      // Broadcast that a data directory is about to be removed
      FileEvent fe;
      fe.m_EventType = FileEventType::RemoveDataDirectory;
      fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirType->GetDataDirectoryPath();
      fe.m_sOther = s_pData->m_DataDirectories[i].m_sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirType->RemoveDataDirectory();
  }

  s_pData->m_DataDirectories.Clear();
}

const WDataDirectoryInfo* WFileSystem::FindDataDirectoryWithRoot(WStringView sRootName)
{
  if (sRootName.IsEmpty())
    return nullptr;

  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_LOCK(s_pData->m_FsMutex);

  for (const auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_sRootName.IsEqual_NoCase(sRootName))
    {
      return &dd;
    }
  }

  return nullptr;
}

WUInt32 WFileSystem::GetNumDataDirectories()
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories.GetCount();
}

WDataDirectoryType* WFileSystem::GetDataDirectory(WUInt32 uiDataDirIndex)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories[uiDataDirIndex].m_pDataDirType;
}

const WDataDirectoryInfo& WFileSystem::GetDataDirectoryInfo(WUInt32 uiDataDirIndex)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories[uiDataDirIndex];
}

WStringView WFileSystem::GetDataDirRelativePath(WStringView sPath, WUInt32 uiDataDir)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_LOCK(s_pData->m_FsMutex);

  // if an absolute path is given, this will check whether the absolute path would fall into this data directory
  // if yes, the prefix path is removed and then only the relative path is given to the data directory type
  // otherwise the data directory would prepend its own path and thus create an invalid path to work with

  // first check the redirected directory
  const WString128& sRedDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirType->GetRedirectedDataDirectoryPath();

  if (!sRedDirPath.IsEmpty() && sPath.StartsWith_NoCase(sRedDirPath))
  {
    WStringView sRelPath(sPath.GetStartPointer() + sRedDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (WPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  // then check the original mount path
  const WString128& sDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirType->GetDataDirectoryPath();

  // If the data dir is empty we return the paths as is or the code below would remove the '/' in front of an
  // absolute path.
  if (!sDirPath.IsEmpty() && sPath.StartsWith_NoCase(sDirPath))
  {
    WStringView sRelPath(sPath.GetStartPointer() + sDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (WPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  return sPath;
}


WDataDirectoryInfo* WFileSystem::GetDataDirForRoot(const WString& sRoot)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_LOCK(s_pData->m_FsMutex);

  for (WInt32 i = (WInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_sRootName == sRoot)
      return &s_pData->m_DataDirectories[i];
  }

  return nullptr;
}


void WFileSystem::DeleteFile(WStringView sFile)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (WPathUtils::IsAbsolutePath(sFile))
  {
    WOSFile::DeleteFile(sFile).IgnoreResult();
    return;
  }

  WString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  W_ASSERT_DEV(!sRootName.IsEmpty(), "Files can only be deleted with a rooted path name.");

  if (sRootName.IsEmpty())
    return;

  W_LOCK(s_pData->m_FsMutex);

  for (WInt32 i = (WInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // do not delete data from directories that are mounted as read only
    if (s_pData->m_DataDirectories[i].m_Usage != WDataDirUsage::AllowWrites)
      continue;

    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    WStringView sRelPath = GetDataDirRelativePath(sFile, i);

    {
      // Broadcast that a file is about to be deleted
      // This can be used to check out files or mark them as deleted in a revision control system
      FileEvent fe;
      fe.m_EventType = FileEventType::DeleteFile;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      fe.m_sOther = sRootName;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirType->DeleteFile(sRelPath);
  }
}

bool WFileSystem::ExistsFile(WStringView sFile)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  WString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  W_LOCK(s_pData->m_FsMutex);

  for (WInt32 i = (WInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    WStringView sRelPath = GetDataDirRelativePath(sFile, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirType->ExistsFile(sRelPath, bOneSpecificDataDir))
      return true;
  }

  return false;
}


WResult WFileSystem::GetFileStats(WStringView sFileOrFolder, WFileStats& out_stats)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  W_LOCK(s_pData->m_FsMutex);

  if (sFileOrFolder.IsEmpty())
  {
    return W_FAILURE;
  }

  WString sRootName;
  sFileOrFolder = ExtractRootName(sFileOrFolder, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  for (WInt32 i = (WInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    WStringView sRelPath = GetDataDirRelativePath(sFileOrFolder, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirType->GetFileStats(sRelPath, bOneSpecificDataDir, out_stats).Succeeded())
      return W_SUCCESS;
  }

  return W_FAILURE;
}

WStringView WFileSystem::ExtractRootName(WStringView sPath, WString& rootName)
{
  WStringView root, path;
  WPathUtils::GetRootedPathParts(sPath, root, path);

  WStringBuilder rootUpr = root;
  rootUpr.ToUpper();
  rootName = rootUpr;
  return path;
}

WDataDirectoryReader* WFileSystem::GetFileReader(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  W_LOCK(s_pData->m_FsMutex);

  WString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  // clean up the path to get rid of ".." etc.
  WStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  // the last added data directory has the highest priority
  for (WInt32 i = (WInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // if a root is used, ignore all directories that do not have the same root name
    if (bOneSpecificDataDir && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    WStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType = FileEventType::OpenFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    // Let the data directory try to open the file.
    WDataDirectoryReader* pReader = s_pData->m_DataDirectories[i].m_pDataDirType->OpenFileToRead(sRelPath, FileShareMode, bOneSpecificDataDir);

    if (pReader != nullptr)
    {
      if (bAllowFileEvents)
      {
        // Broadcast that this file has been opened.
        FileEvent fe;
        fe.m_EventType = FileEventType::OpenFileSucceeded;
        fe.m_sFileOrDirectory = sRelPath;
        fe.m_sOther = sRootName;
        fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      return pReader;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that opening this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::OpenFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

WDataDirectoryWriter* WFileSystem::GetFileWriter(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  W_LOCK(s_pData->m_FsMutex);

  WString sRootName;

  if (!WPathUtils::IsAbsolutePath(sFile))
  {
    W_ASSERT_DEV(sFile.StartsWith(":"),
      "Only native absolute paths or rooted paths (starting with a colon and then the data dir root name) are allowed for "
      "writing to files. This path is neither: '{0}'",
      sFile);
    sFile = ExtractRootName(sFile, sRootName);
  }

  // clean up the path to get rid of ".." etc.
  WStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  // the last added data directory has the highest priority
  for (WInt32 i = (WInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_Usage != WDataDirUsage::AllowWrites)
      continue;

    // ignore all directories that have not the category that is currently requested
    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    WStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType = FileEventType::CreateFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
      s_pData->m_Event.Broadcast(fe);
    }

    WDataDirectoryWriter* pWriter = s_pData->m_DataDirectories[i].m_pDataDirType->OpenFileToWrite(sRelPath, FileShareMode);

    if (pWriter != nullptr)
    {
      if (bAllowFileEvents)
      {
        // Broadcast that this file has been created.
        FileEvent fe;
        fe.m_EventType = FileEventType::CreateFileSucceeded;
        fe.m_sFileOrDirectory = sRelPath;
        fe.m_sOther = sRootName;
        fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirType;
        s_pData->m_Event.Broadcast(fe);
      }

      return pWriter;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that creating this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::CreateFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

WResult WFileSystem::ResolvePath(WStringView sPath, WStringBuilder* out_pAbsolutePath, WStringBuilder* out_pDataDirRelativePath, const WDataDirectoryInfo** out_pDataDir /*= nullptr*/)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  W_LOCK(s_pData->m_FsMutex);

  WStringBuilder absPath, relPath;

  if (sPath.StartsWith(":"))
  {
    // writing is only allowed using rooted paths
    WString sRootName;
    ExtractRootName(sPath, sRootName);

    const WDataDirectoryInfo* pDataDir = GetDataDirForRoot(sRootName);

    if (pDataDir == nullptr)
      return W_FAILURE;

    if (out_pDataDir != nullptr)
      *out_pDataDir = pDataDir;

    relPath = sPath.GetShrunk(sRootName.GetCharacterCount() + 2);

    absPath = pDataDir->m_pDataDirType->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);
  }
  else if (WPathUtils::IsAbsolutePath(sPath))
  {
    absPath = sPath;
    absPath.MakeCleanPath();

    for (WUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
    {
      auto& dir = s_pData->m_DataDirectories[dd - 1];

      if (WPathUtils::IsSubPath(dir.m_pDataDirType->GetRedirectedDataDirectoryPath(), absPath))
      {
        if (out_pAbsolutePath)
          *out_pAbsolutePath = absPath;

        if (out_pDataDirRelativePath)
        {
          *out_pDataDirRelativePath = absPath;
          out_pDataDirRelativePath->MakeRelativeTo(dir.m_pDataDirType->GetRedirectedDataDirectoryPath()).IgnoreResult();
        }

        if (out_pDataDir)
          *out_pDataDir = &dir;

        return W_SUCCESS;
      }
    }

    return W_FAILURE;
  }
  else
  {
    // try to get a reader -> if we get one, the file does indeed exist
    WDataDirectoryReader* pReader = WFileSystem::GetFileReader(sPath, WFileShareMode::SharedReads, true);

    if (!pReader)
      return W_FAILURE;

    if (out_pDataDir != nullptr)
    {
      for (WUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
      {
        auto& dir = s_pData->m_DataDirectories[dd - 1];

        if (dir.m_pDataDirType == pReader->GetDataDirectory())
        {
          *out_pDataDir = &dir;
        }
      }
    }

    relPath = pReader->GetFilePath();

    absPath = pReader->GetDataDirectory()->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);

    pReader->Close();
  }

  if (out_pAbsolutePath)
    *out_pAbsolutePath = absPath;

  if (out_pDataDirRelativePath)
    *out_pDataDirRelativePath = relPath;

  return W_SUCCESS;
}

WResult WFileSystem::FindFolderWithSubPath(WStringBuilder& out_sResult, WStringView sStartDirectory, WStringView sSubPath, WStringView sRedirectionFileName /*= nullptr*/)
{
  WStringBuilder sStartDirAbs = sStartDirectory;
  sStartDirAbs.MakeCleanPath();

  // in this case the given path and the absolute path are different
  // but we want to return the same path format as is given
  // ie. if we get ":MyRoot\Bla" with "MyRoot" pointing to "C:\Game", then the result should be
  // ":MyRoot\blub", rather than "C:\Game\blub"
  if (sStartDirAbs.StartsWith(":"))
  {
    WStringBuilder abs;
    if (ResolvePath(sStartDirAbs, &abs, nullptr).Failed())
    {
      out_sResult.Clear();
      return W_FAILURE;
    }

    sStartDirAbs = abs;
  }

  out_sResult = sStartDirectory;
  out_sResult.MakeCleanPath();

  WStringBuilder FullPath, sRedirection;

  while (!out_sResult.IsEmpty())
  {
    sRedirection.Clear();

    if (!sRedirectionFileName.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirectionFileName);

      WOSFile f;
      if (f.Open(FullPath, WFileOpenMode::Read).Succeeded())
      {
        WDataBuffer db;
        f.ReadAll(db);
        sRedirection.Set(WStringView((const char*)db.GetData(), db.GetCount()));
      }
    }

    // first try with the redirection
    if (!sRedirection.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirection);
      FullPath.AppendPath(sSubPath);
      FullPath.MakeCleanPath();

      if (WOSFile::ExistsDirectory(FullPath) || WOSFile::ExistsFile(FullPath))
      {
        out_sResult.AppendPath(sRedirection);
        out_sResult.MakeCleanPath();
        return W_SUCCESS;
      }
    }

    // then try without the redirection
    FullPath = sStartDirAbs;
    FullPath.AppendPath(sSubPath);
    FullPath.MakeCleanPath();

    if (WOSFile::ExistsDirectory(FullPath) || WOSFile::ExistsFile(FullPath))
    {
      return W_SUCCESS;
    }

    out_sResult.PathParentDirectory();
    sStartDirAbs.PathParentDirectory();
  }

  return W_FAILURE;
}

bool WFileSystem::ResolveAssetRedirection(WStringView sPathOrAssetGuid, WStringBuilder& out_sRedirection)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_pDataDirType->ResolveAssetRedirection(sPathOrAssetGuid, out_sRedirection))
      return true;
  }

  out_sRedirection = sPathOrAssetGuid;
  return false;
}

WStringView WFileSystem::MigrateFileLocation(WStringView sOldLocation, WStringView sNewLocation)
{
  WStringBuilder sOldPathFull, sNewPathFull;

  if (ResolvePath(sOldLocation, &sOldPathFull, nullptr).Failed() || sOldPathFull.IsEmpty())
  {
    // if the old path could not be resolved, use the new path
    return sNewLocation;
  }

  ResolvePath(sNewLocation, &sNewPathFull, nullptr).AssertSuccess();

  if (!ExistsFile(sOldPathFull))
  {
    // old path doesn't exist -> use the new
    return sNewLocation;
  }

  // old path does exist -> deal with it

  if (ExistsFile(sNewPathFull))
  {
    // new path also exists -> delete the old one (in all data directories), use the new one
    DeleteFile(sOldLocation); // location, not full path
    return sNewLocation;
  }

  // new one doesn't exist -> try to move old to new
  if (WOSFile::MoveFileOrDirectory(sOldPathFull, sNewPathFull).Failed())
  {
    // if the old location exists, but we can't move the file, return the old location to use
    return sOldLocation;
  }

  // deletes the file in the old location in ALL data directories,
  // so that they can't interfere with the new file in the future
  DeleteFile(sOldLocation); // location, not full path

  // if we successfully moved the file to the new location, use the new location
  return sNewLocation;
}

void WFileSystem::ReloadAllExternalDataDirectoryConfigs()
{
  W_LOG_BLOCK("ReloadAllExternalDataDirectoryConfigs");

  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    dd.m_pDataDirType->ReloadExternalConfigs();
  }
}

void WFileSystem::Startup()
{
  s_pData = W_DEFAULT_NEW(FileSystemData);
}

void WFileSystem::Shutdown()
{
  {
    W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
    W_LOCK(s_pData->m_FsMutex);

    s_pData->m_DataDirFactories.Clear();

    ClearAllDataDirectories();
  }

  W_DEFAULT_DELETE(s_pData);
}

WResult WFileSystem::DetectSdkRootDirectory(WStringView sExpectedSubFolder /*= "Data/Base"*/)
{
  W_IGNORE_UNUSED(sExpectedSubFolder);

  if (!s_sSdkRootDir.IsEmpty())
    return W_SUCCESS;

  WStringBuilder sdkRoot;

#if W_ENABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  if (WFileSystem::FindFolderWithSubPath(sdkRoot, WOSFile::GetApplicationDirectory(), sExpectedSubFolder, "WSdkRoot.txt").Failed())
  {
    WLog::Error("Could not find SDK root. Application dir is '{0}'. Searched for parent with '{1}' sub-folder.", WOSFile::GetApplicationDirectory(), sExpectedSubFolder);
    return W_FAILURE;
  }
#else
  // mobile platforms
  sdkRoot = WOSFile::GetApplicationDirectory();
#endif

  WFileSystem::SetSdkRootDirectory(sdkRoot);
  return W_SUCCESS;
}

void WFileSystem::SetSdkRootDirectory(WStringView sSdkDir)
{
  WStringBuilder s = sSdkDir;
  s.MakeCleanPath();

  s_sSdkRootDir = s;
}

WStringView WFileSystem::GetSdkRootDirectory()
{
  W_ASSERT_DEV(!s_sSdkRootDir.IsEmpty(), "The project directory has not been set through 'WFileSystem::SetSdkRootDirectory'.");
  return s_sSdkRootDir;
}

void WFileSystem::SetSpecialDirectory(WStringView sName, WStringView sReplacement)
{
  WStringBuilder tmp = sName;
  tmp.ToLower();

  if (sReplacement.IsEmpty())
  {
    s_SpecialDirectories.Remove(tmp);
  }
  else
  {
    s_SpecialDirectories[tmp] = sReplacement;
    WLog::Dev("Setting special directory '{}' to '{}'", sName, sReplacement);
  }
}

WResult WFileSystem::ResolveSpecialDirectory(WStringView sDirectory, WStringBuilder& out_sPath)
{
  if (sDirectory.IsEmpty() || !sDirectory.StartsWith(">"))
  {
    out_sPath = sDirectory;
    return W_SUCCESS;
  }

  // skip the '>'
  sDirectory.ChopAwayFirstCharacterAscii();
  const char* szStart = sDirectory.GetStartPointer();

  const char* szEnd = sDirectory.FindSubString("/");

  if (szEnd == nullptr)
    szEnd = szStart + WStringUtils::GetStringElementCount(szStart);

  WStringBuilder sName;
  sName.SetSubString_FromTo(szStart, szEnd);
  sName.ToLower();

  const auto it = s_SpecialDirectories.Find(sName);
  if (it.IsValid())
  {
    out_sPath = it.Value();
    out_sPath.AppendPath(szEnd); // szEnd might be on \0 or a slash
    out_sPath.MakeCleanPath();
    return W_SUCCESS;
  }

  if (sName == "sdk")
  {
    sDirectory.Shrink(3, 0);
    out_sPath = GetSdkRootDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return W_SUCCESS;
  }

  if (sName == "user")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = WOSFile::GetUserDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return W_SUCCESS;
  }

  if (sName == "temp")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = WOSFile::GetTempDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return W_SUCCESS;
  }

  if (sName == "appdir")
  {
    sDirectory.Shrink(6, 0);
    out_sPath = WOSFile::GetApplicationDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return W_SUCCESS;
  }

  return W_FAILURE;
}


WMutex& WFileSystem::GetMutex()
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  return s_pData->m_FsMutex;
}

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

void WFileSystem::StartSearch(WFileSystemIterator& ref_iterator, WStringView sSearchTerm, WBitflags<WFileSystemIteratorFlags> flags /*= WFileSystemIteratorFlags::Default*/)
{
  W_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  W_LOCK(s_pData->m_FsMutex);

  WTempHybridArray<WString, 16> folders;
  WStringBuilder sDdPath, sRelPath;

  if (sSearchTerm.IsRootedPath())
  {
    const WStringView root = sSearchTerm.GetRootedPathRootName();

    const WDataDirectoryInfo* pDataDir = FindDataDirectoryWithRoot(root);
    if (pDataDir == nullptr)
      return;

    sSearchTerm.SetStartPosition(root.GetEndPointer());

    if (!sSearchTerm.IsEmpty())
    {
      // root name should be followed by a slash
      sSearchTerm.ChopAwayFirstCharacterAscii();
    }

    folders.PushBack(pDataDir->m_pDataDirType->GetRedirectedDataDirectoryPath().GetView());
  }
  else if (sSearchTerm.IsAbsolutePath())
  {
    for (WUInt32 idx = s_pData->m_DataDirectories.GetCount(); idx > 0; --idx)
    {
      const auto& dd = s_pData->m_DataDirectories[idx - 1];

      sDdPath = dd.m_pDataDirType->GetRedirectedDataDirectoryPath();

      sRelPath = sSearchTerm;

      if (!sDdPath.IsEmpty())
      {
        if (sRelPath.MakeRelativeTo(sDdPath).Failed())
          continue;

        // this would use "../" if necessary, which we don't want
        if (sRelPath.StartsWith(".."))
          continue;
      }

      sSearchTerm = sRelPath;

      folders.PushBack(sDdPath);
      break;
    }
  }
  else
  {
    for (WUInt32 idx = s_pData->m_DataDirectories.GetCount(); idx > 0; --idx)
    {
      const auto& dd = s_pData->m_DataDirectories[idx - 1];

      sDdPath = dd.m_pDataDirType->GetRedirectedDataDirectoryPath();

      folders.PushBack(sDdPath);
    }
  }

  ref_iterator.StartMultiFolderSearch(folders, sSearchTerm, flags);
}

#endif

WResult WFileSystem::CreateDirectoryStructure(WStringView sPath)
{
  WStringBuilder sRedir;
  W_SUCCEED_OR_RETURN(ResolveSpecialDirectory(sPath, sRedir));

  if (sRedir.IsRootedPath())
  {
    WFileSystem::ResolvePath(sRedir, &sRedir, nullptr).AssertSuccess();
  }

  if (!sRedir.IsAbsolutePath())
    return W_FAILURE;

  return WOSFile::CreateDirectoryStructure(sRedir);
}

W_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileSystem);
