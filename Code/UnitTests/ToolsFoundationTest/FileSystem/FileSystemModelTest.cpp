#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER) && W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <Foundation/Threading/ThreadUtils.h>
#  include <ToolsFoundation/FileSystem/FileSystemModel.h>


W_CREATE_SIMPLE_TEST_GROUP(FileSystem);

namespace
{
  WResult WtCreateFile(WStringView sPath)
  {
    WFileWriter FileOut;
    W_SUCCEED_OR_RETURN(FileOut.Open(sPath));
    W_SUCCEED_OR_RETURN(FileOut.WriteString("Test"));
    FileOut.Close();
    return W_SUCCESS;
  }
} // namespace

W_CREATE_SIMPLE_TEST(FileSystem, DataDirPath)
{
  const WStringView sFilePathView = "C:/Code/WorldEngine/Data/Samples/Testing Chambers/Objects/Barrel.WPrefab"_wsv;
  const WStringView sDataDirView = "C:/Code/WorldEngine/Data/Samples/Testing Chambers"_wsv;

  auto CheckIsValid = [&](const WDataDirPath& path)
  {
    W_TEST_BOOL(path.IsValid());
    WStringView sAbs = path.GetAbsolutePath();
    W_TEST_STRING(sAbs, sFilePathView);
    WStringView sDD = path.GetDataDir();
    W_TEST_STRING(sDD, sDataDirView);
    WStringView sPR = path.GetDataDirParentRelativePath();
    W_TEST_STRING(sPR, "Testing Chambers/Objects/Barrel.WPrefab");
    WStringView sR = path.GetDataDirRelativePath();
    W_TEST_STRING(sR, "Objects/Barrel.WPrefab");
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "Windows Path copy ctor")
  {
    WTempHybridArray<WString, 2> rootFolders;
    rootFolders.PushBack("C:/SomeOtherFolder/Folder");
    rootFolders.PushBack(sDataDirView);

    WDataDirPath path(sFilePathView, rootFolders);
    CheckIsValid(path);
    WUInt32 uiIndex = path.GetDataDirIndex();
    W_TEST_INT(uiIndex, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Linux Path move ctor")
  {
    WString sFilePathView = "/Code/WorldEngine/Data/Samples/Testing Chambers/Objects/Barrel.WPrefab"_wsv;
    WString sFilePath = sFilePathView;
    auto sDataDir = "/Code/WorldEngine/Data/Samples/Testing Chambers"_wsv;
    WTempHybridArray<WString, 2> rootFolders;
    rootFolders.PushBack(sDataDir);
    rootFolders.PushBack("/SomeOtherFolder/Folder");

    const char* szRawStringPtr = sFilePath.GetData();
    WDataDirPath path(std::move(sFilePath), rootFolders);
    W_TEST_BOOL(path.IsValid());
    WStringView sAbs = path.GetAbsolutePath();
    W_TEST_STRING(sAbs, sFilePathView);
    W_TEST_BOOL(szRawStringPtr == sAbs.GetStartPointer());
    WStringView sDD = path.GetDataDir();
    W_TEST_STRING(sDD, sDataDir);
    WStringView sPR = path.GetDataDirParentRelativePath();
    W_TEST_STRING(sPR, "Testing Chambers/Objects/Barrel.WPrefab");
    WStringView sR = path.GetDataDirRelativePath();
    W_TEST_STRING(sR, "Objects/Barrel.WPrefab");
    WUInt32 uiIndex = path.GetDataDirIndex();
    W_TEST_INT(uiIndex, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Path to DataDir Itself")
  {
    WString sDataDirView = (const char*)u8"/Code/WorldEngine/Data/Sämples/Testing Chämbers";
    WTempHybridArray<WString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    WDataDirPath path(sDataDirView.GetView(), rootFolders);
    W_TEST_BOOL(path.IsValid());
    WStringView sAbs = path.GetAbsolutePath();
    W_TEST_STRING(sAbs, sDataDirView);
    WStringView sDD = path.GetDataDir();
    W_TEST_STRING(sDD, sDataDirView);
    WStringView sPR = path.GetDataDirParentRelativePath();
    W_TEST_STRING(sPR, (const char*)u8"Testing Chämbers");
    WStringView sR = path.GetDataDirRelativePath();
    W_TEST_STRING(sR, "");
    WUInt32 uiIndex = path.GetDataDirIndex();
    W_TEST_INT(uiIndex, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move")
  {
    WTempHybridArray<WString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    WString sFilePath = sFilePathView;
    const char* szRawStringPtr = sFilePath.GetData();
    WDataDirPath path(std::move(sFilePath), rootFolders);
    CheckIsValid(path);

    WStringView sAbs = path.GetAbsolutePath();
    W_TEST_BOOL(szRawStringPtr == sAbs.GetStartPointer());

    WDataDirPath path2 = std::move(path);
    WStringView sAbs2 = path2.GetAbsolutePath();
    W_TEST_BOOL(szRawStringPtr == sAbs2.GetStartPointer());

    WDataDirPath path3(std::move(path2));
    WStringView sAbs3 = path3.GetAbsolutePath();
    W_TEST_BOOL(szRawStringPtr == sAbs3.GetStartPointer());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Rebuild")
  {
    WTempHybridArray<WString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    WDataDirPath path(sFilePathView, rootFolders);
    CheckIsValid(path);
    W_TEST_INT(path.GetDataDirIndex(), 0);

    WTempHybridArray<WString, 2> newRootFolders;
    newRootFolders.PushBack(sDataDirView);
    newRootFolders.PushBack("C:/Some/Other/DataDir");

    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    W_TEST_INT(path.GetDataDirIndex(), 0);

    newRootFolders.InsertAt(0, "C:/Some/Other/DataDir2");
    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    W_TEST_INT(path.GetDataDirIndex(), 1);

    newRootFolders.RemoveAtAndCopy(0);
    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    W_TEST_INT(path.GetDataDirIndex(), 0);

    newRootFolders.RemoveAtAndCopy(0);
    path.UpdateDataDirInfos(newRootFolders);
    W_TEST_BOOL(!path.IsValid());
    WStringView sAbs = path.GetAbsolutePath();
    W_TEST_STRING(sAbs, sFilePathView);
  }
}

void FileSystemModelTest()
{
  constexpr WUInt32 WAIT_LOOPS = 1000;

  WStringBuilder sOutputFolder = WTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("Model");
  sOutputFolder.MakeCleanPath();

  WStringBuilder sOutputFolderResolved;
  WFileSystem::ResolveSpecialDirectory(sOutputFolder, sOutputFolderResolved).IgnoreResult();

  WTempHybridArray<WString, 1> rootFolders;

  WApplicationFileSystemConfig fsConfig;
  WApplicationFileSystemConfig::DataDirConfig& dataDir = fsConfig.m_DataDirs.ExpandAndGetRef();
  dataDir.m_bWritable = true;
  dataDir.m_sDataDirSpecialPath = sOutputFolder;
  dataDir.m_sRootName = "output";
  rootFolders.PushBack(sOutputFolder);

  // Files
  WTempHybridArray<WFileChangedEvent, 2> fileEvents;
  WTempHybridArray<WTime, 2> fileEventTimestamps;
  WMutex fileEventLock;
  auto fileEvent = [&](const WFileChangedEvent& e)
  {
    W_LOCK(fileEventLock);
    fileEvents.PushBack(e);
    fileEventTimestamps.PushBack(WTime::Now());

    WFileStatus stat;
    switch (e.m_Type)
    {
      case WFileChangedEvent::Type::FileRemoved:
        W_TEST_BOOL(WFileSystemModel::GetSingleton()->FindFile(e.m_Path, stat).Failed());
        break;
      case WFileChangedEvent::Type::FileAdded:
      case WFileChangedEvent::Type::FileChanged:
      case WFileChangedEvent::Type::DocumentLinked:
        W_TEST_BOOL(WFileSystemModel::GetSingleton()->FindFile(e.m_Path, stat).Succeeded());
        break;

      case WFileChangedEvent::Type::ModelReset:
      default:
        break;
    }
  };
  WEventSubscriptionID fileId = WFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(fileEvent);

  // Folders
  WTempHybridArray<WFolderChangedEvent, 2> folderEvents;
  WTempHybridArray<WTime, 2> folderEventTimestamps;
  WMutex folderEventLock;
  auto folderEvent = [&](const WFolderChangedEvent& e)
  {
    W_LOCK(folderEventLock);
    folderEvents.PushBack(e);
    folderEventTimestamps.PushBack(WTime::Now());

    switch (e.m_Type)
    {
      case WFolderChangedEvent::Type::FolderAdded:
        W_TEST_BOOL(WFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_Path));
        break;
      case WFolderChangedEvent::Type::FolderRemoved:
        W_TEST_BOOL(!WFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_Path));
        break;
      case WFolderChangedEvent::Type::ModelReset:
      default:
        break;
    }
  };
  WEventSubscriptionID folderId = WFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(folderEvent);

  // Helper functions
  auto CompareFiles = [&](WArrayPtr<WFileChangedEvent> expected)
  {
    W_LOCK(fileEventLock);
    if (W_TEST_INT(expected.GetCount(), fileEvents.GetCount()))
    {
      for (WUInt32 i = 0; i < expected.GetCount(); i++)
      {
        W_TEST_INT((int)expected[i].m_Type, (int)fileEvents[i].m_Type);
        W_TEST_STRING(expected[i].m_Path, fileEvents[i].m_Path);
        W_TEST_BOOL(expected[i].m_Status.m_DocumentID == fileEvents[i].m_Status.m_DocumentID);
        // Ignore stats besudes GUID.
      }
    }
  };

  auto ClearFiles = [&]()
  {
    W_LOCK(fileEventLock);
    fileEvents.Clear();
    fileEventTimestamps.Clear();
  };

  auto CompareFolders = [&](WArrayPtr<WFolderChangedEvent> expected)
  {
    W_LOCK(folderEventLock);
    if (W_TEST_INT(expected.GetCount(), folderEvents.GetCount()))
    {
      for (WUInt32 i = 0; i < expected.GetCount(); i++)
      {
        W_TEST_INT((int)expected[i].m_Type, (int)folderEvents[i].m_Type);
        W_TEST_STRING(expected[i].m_Path, folderEvents[i].m_Path);
        // Ignore stats
      }
    }
  };

  auto ClearFolders = [&]()
  {
    W_LOCK(folderEventLock);
    folderEvents.Clear();
    folderEventTimestamps.Clear();
  };

  auto MakePath = [&](WStringView sPath)
  {
    return WDataDirPath(sPath, rootFolders);
  };


  W_TEST_BLOCK(WTestBlock::Enabled, "Startup")
  {
    WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::FolderType::Factory);

    W_TEST_RESULT(WOSFile::DeleteFolder(sOutputFolderResolved));
    W_TEST_RESULT(WFileSystem::CreateDirectoryStructure(sOutputFolderResolved));

    // for absolute paths
    W_TEST_BOOL(WFileSystem::AddDataDirectory("", "", ":", WDataDirUsage::AllowWrites) == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", WDataDirUsage::AllowWrites) == W_SUCCESS);

    WFileSystemModel::GetSingleton()->Initialize(fsConfig, {}, {});

    WFileChangedEvent expected[] = {WFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();

    WFolderChangedEvent expected2[] = {WFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset)};
    CompareFolders(WMakeArrayPtr(expected2));
    ClearFolders();

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);

    auto it = WFileSystemModel::GetSingleton()->GetFolders()->GetIterator();
    W_TEST_STRING(it.Key(), sOutputFolder);
    W_TEST_BOOL(it.Value() == WFileStatus::Status::Valid);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "git")
  {
    WStringBuilder sIndex(sOutputFolder);
    sIndex.AppendPath("index");
    WStringBuilder sLock(sOutputFolder);
    sLock.AppendPath("index.lock");

    W_TEST_RESULT(WtCreateFile(sIndex));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sIndex), {}, WFileChangedEvent::Type::FileAdded)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
    }

#  if W_ENABLED(W_PLATFORM_LINUX)
    // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
    // As we intend to swap the index and index.lock files later, we need to make sure the two files have sufficiently different modification dates so that the swap of the files is detected as a change to the original file.
    WThreadUtils::Sleep(WTime::MakeFromSeconds(1.0));
#  endif

    W_TEST_RESULT(WtCreateFile(sLock));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sLock), {}, WFileChangedEvent::Type::FileAdded)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
    }

    W_TEST_RESULT(WOSFile::DeleteFile(sIndex));
    W_TEST_RESULT(WOSFile::MoveFileOrDirectory(sLock, sIndex));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() >= 2)
        break;
    }

    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sIndex), {}, WFileChangedEvent::Type::FileChanged),
      WFileChangedEvent(MakePath(sLock), {}, WFileChangedEvent::Type::FileRemoved)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);

    // Cleanup test
    W_TEST_RESULT(WOSFile::DeleteFile(sIndex));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    ClearFiles();
    ClearFolders();
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Add file")
  {
    WStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    W_TEST_RESULT(WtCreateFile(sFilePath));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileAdded)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "modify file")
  {
    WStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    {
#  if W_ENABLED(W_PLATFORM_LINUX)
      // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
      WThreadUtils::Sleep(WTime::MakeFromSeconds(1.0));
#  endif
      WFileWriter FileOut;
      W_TEST_RESULT(FileOut.Open(sFilePath));
      W_TEST_RESULT(FileOut.WriteString("Test2"));
      W_TEST_RESULT(FileOut.Flush());
      FileOut.Close();
    }

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileChanged)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "rename file")
  {
    WStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile.txt");

    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("rootFile2.txt");

    W_TEST_RESULT(WOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sFilePathNew), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sFilePathOld), {}, WFileChangedEvent::Type::FileRemoved)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Add folder")
  {
    WStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder1");

    W_TEST_RESULT(WFileSystem::CreateDirectoryStructure(sFolderPath));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(folderEventLock);
      if (folderEvents.GetCount() > 0)
        break;
    }

    WFolderChangedEvent expected[] = {WFolderChangedEvent(MakePath(sFolderPath), WFolderChangedEvent::Type::FolderAdded)};
    CompareFolders(WMakeArrayPtr(expected));
    ClearFolders();
    CompareFiles({});

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "move file")
  {
    WStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile2.txt");

    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder1", "rootFile2.txt");

    W_TEST_RESULT(WOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sFilePathNew), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sFilePathOld), {}, WFileChangedEvent::Type::FileRemoved)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "move folder")
  {
    WStringBuilder sFolderPathOld(sOutputFolder);
    sFolderPathOld.AppendPath("Folder1");

    WStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("Folder1", "rootFile2.txt");

    WStringBuilder sFolderPathNew(sOutputFolder);
    sFolderPathNew.AppendPath("Folder12");

    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    W_TEST_RESULT(WOSFile::MoveFileOrDirectory(sFolderPathOld, sFolderPathNew));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      W_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 2 && folderEvents.GetCount() == 2)
        break;
    }

    {
      WFolderChangedEvent expected[] = {
        WFolderChangedEvent(MakePath(sFolderPathNew), WFolderChangedEvent::Type::FolderAdded),
        WFolderChangedEvent(MakePath(sFolderPathOld), WFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(WMakeArrayPtr(expected));
    }

    {
      WFileChangedEvent expected[] = {
        WFileChangedEvent(MakePath(sFilePathNew), {}, WFileChangedEvent::Type::FileAdded),
        WFileChangedEvent(MakePath(sFilePathOld), {}, WFileChangedEvent::Type::FileRemoved)};
      CompareFiles(WMakeArrayPtr(expected));
    }
    {
      W_LOCK(fileEventLock);
      W_LOCK(folderEventLock);
      // Check folder added before file
      W_TEST_BOOL(fileEventTimestamps[0] > folderEventTimestamps[0]);
      // Check file removed before folder
      W_TEST_BOOL(fileEventTimestamps[1] < folderEventTimestamps[1]);
    }

    ClearFolders();
    ClearFiles();

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HashFile")
  {
    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    WFileStatus status;
    W_TEST_RESULT(WFileSystemModel::GetSingleton()->HashFile(sFilePathNew, status));
    W_TEST_INT((WInt64)status.m_uiHash, (WInt64)10983861097202158394u);
  }

  WFileSystemModel::FilesMap referencedFiles;
  WFileSystemModel::FoldersMap referencedFolders;

  W_TEST_BLOCK(WTestBlock::Enabled, "Shutdown")
  {
    WFileSystemModel::GetSingleton()->Deinitialize(&referencedFiles, &referencedFolders);
    W_TEST_INT(referencedFiles.GetCount(), 1);
    W_TEST_INT(referencedFolders.GetCount(), 2);

    WFileChangedEvent expected[] = {WFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();

    WFolderChangedEvent expected2[] = {WFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset)};
    CompareFolders(WMakeArrayPtr(expected2));
    ClearFolders();
  }

  WStringBuilder sOutputFolder2 = sOutputFolderResolved;
  sOutputFolder2.ChangeFileNameAndExtension("Model2");

  W_TEST_BLOCK(WTestBlock::Enabled, "Startup Restore Model")
  {
    {
      // Add another data directory. This is now at the index of the old one, requiring the indices to be updated inside WFileSystemModel::Initialize.
      W_TEST_RESULT(WOSFile::DeleteFolder(sOutputFolder2));
      W_TEST_RESULT(WFileSystem::CreateDirectoryStructure(sOutputFolder2));
      WApplicationFileSystemConfig::DataDirConfig dataDir;
      dataDir.m_bWritable = true;
      dataDir.m_sDataDirSpecialPath = sOutputFolder2;
      dataDir.m_sRootName = "output2";

      rootFolders.InsertAt(0, sOutputFolder);
      fsConfig.m_DataDirs.InsertAt(0, dataDir);
    }

    WFileSystemModel::GetSingleton()->Initialize(fsConfig, std::move(referencedFiles), std::move(referencedFolders));

    WFileChangedEvent expected[] = {WFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();

    WFolderChangedEvent expected2[] = {WFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset)};
    CompareFolders(WMakeArrayPtr(expected2));
    ClearFolders();

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);

    // Check that files have be remapped.
    for (auto it : *WFileSystemModel::GetSingleton()->GetFiles())
    {
      W_TEST_INT(it.Key().GetDataDirIndex(), 1);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFiles")
  {
    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    WFileSystemModel::LockedFiles files = WFileSystemModel::GetSingleton()->GetFiles();
    W_TEST_INT(files->GetCount(), 1);
    auto it = files->GetIterator();
    W_TEST_STRING(it.Key(), sFilePathNew);
    W_TEST_BOOL(it.Value().m_LastModified.IsValid());
    W_TEST_INT((WInt64)it.Value().m_uiHash, (WInt64)10983861097202158394u);
    W_TEST_BOOL(it.Value().m_Status == WFileStatus::Status::Valid);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFolders")
  {
    WStringBuilder sFolder(sOutputFolder);
    sFolder.AppendPath("Folder12");

    WFileSystemModel::LockedFolders folders = WFileSystemModel::GetSingleton()->GetFolders();
    W_TEST_INT(folders->GetCount(), 3);
    auto it = folders->GetIterator();

    // WMap is sorted so the order is fixed.
    W_TEST_STRING(it.Key(), sOutputFolder);
    W_TEST_BOOL(it.Value() == WFileStatus::Status::Valid);

    it.Next();
    W_TEST_STRING(it.Key(), sFolder);
    W_TEST_BOOL(it.Value() == WFileStatus::Status::Valid);

    it.Next();
    W_TEST_STRING(it.Key(), sOutputFolder2);
    W_TEST_BOOL(it.Value() == WFileStatus::Status::Valid);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CheckFileSystem")
  {
    WStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder12");

    WStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder12", "rootFile2.txt");

    WFileSystemModel::GetSingleton()->CheckFileSystem();

    // #TODO_ASSET This FileChanged should be removed once the model is fixed to no longer require firing this after restoring the model from cache. See comment in WFileSystemModel::HandleSingleFile.
    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileChanged),
      WFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();

    WFolderChangedEvent expected2[] = {WFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset)};
    CompareFolders(WMakeArrayPtr(expected2));
    ClearFolders();

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "NotifyOfChange - File")
  {
    WStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");
    {
      W_TEST_RESULT(WtCreateFile(sFilePath));
      WFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileAdded)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    {
      W_TEST_RESULT(WOSFile::DeleteFile(sFilePath));
      WFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileRemoved)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    for (size_t i = 0; i < 15; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "NotifyOfChange - Folder")
  {
    WStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("AnotherFolder");
    {
      W_TEST_RESULT(WFileSystem::CreateDirectoryStructure(sFolderPath));
      WFileSystemModel::GetSingleton()->NotifyOfChange(sFolderPath);

      CompareFiles({});
      ClearFiles();
      WFolderChangedEvent expected[] = {WFolderChangedEvent(MakePath(sFolderPath), WFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(WMakeArrayPtr(expected));
      ClearFolders();
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 4);
    }

    {
      W_TEST_RESULT(WOSFile::DeleteFolder(sFolderPath));
      WFileSystemModel::GetSingleton()->NotifyOfChange(sFolderPath);

      CompareFiles({});
      ClearFiles();
      WFolderChangedEvent expected[] = {WFolderChangedEvent(MakePath(sFolderPath), WFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(WMakeArrayPtr(expected));
      ClearFolders();
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CheckFolder - File")
  {
    WStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder12", "subFile.txt");
    {
      W_TEST_RESULT(WtCreateFile(sFilePath));
      WFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileAdded)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    {
      W_TEST_RESULT(WOSFile::DeleteFile(sFilePath));
      WFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileRemoved)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CheckFolder - Folder")
  {
    WStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("YetAnotherFolder");
    WStringBuilder sFolderSubPath(sOutputFolder);
    sFolderSubPath.AppendPath("YetAnotherFolder", "SubFolder");
    {
      W_TEST_RESULT(WFileSystem::CreateDirectoryStructure(sFolderSubPath));
      WFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      CompareFiles({});
      ClearFiles();
      WFolderChangedEvent expected[] = {
        WFolderChangedEvent(MakePath(sFolderPath), WFolderChangedEvent::Type::FolderAdded),
        WFolderChangedEvent(MakePath(sFolderSubPath), WFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(WMakeArrayPtr(expected));
      ClearFolders();
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 5);
    }

    {
      W_TEST_RESULT(WOSFile::DeleteFolder(sFolderPath));
      WFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      CompareFiles({});
      ClearFiles();
      WFolderChangedEvent expected[] = {
        WFolderChangedEvent(MakePath(sFolderSubPath), WFolderChangedEvent::Type::FolderRemoved),
        WFolderChangedEvent(MakePath(sFolderPath), WFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(WMakeArrayPtr(expected));
      ClearFolders();
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadDocument")
  {
    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    WUuid docGuid = WUuid::MakeUuid();
    auto callback = [&](const WFileStatus& status, WStreamReader& ref_reader)
    {
      W_TEST_INT((WInt64)status.m_uiHash, (WInt64)10983861097202158394u);
      WFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, docGuid).IgnoreResult();
    };

    W_TEST_RESULT(WFileSystemModel::GetSingleton()->ReadDocument(sFilePathNew, callback));

    WFileStatus stat;
    stat.m_DocumentID = docGuid;
    WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePathNew), stat, WFileChangedEvent::Type::DocumentLinked)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "LinkDocument")
  {
    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    WUuid guid = WUuid::MakeUuid();
    WUuid guid2 = WUuid::MakeUuid();
    {
      W_TEST_RESULT(WFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));
      W_TEST_RESULT(WFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));
      W_TEST_RESULT(WFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid2));

      WFileStatus stat;
      stat.m_DocumentID = guid;
      WFileStatus stat2;
      stat2.m_DocumentID = guid2;

      WFileChangedEvent expected[] = {
        WFileChangedEvent(MakePath(sFilePathNew), stat, WFileChangedEvent::Type::DocumentLinked),
        WFileChangedEvent(MakePath(sFilePathNew), stat, WFileChangedEvent::Type::DocumentUnlinked),
        WFileChangedEvent(MakePath(sFilePathNew), stat2, WFileChangedEvent::Type::DocumentLinked)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
    }
    {
      W_TEST_RESULT(WFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));
      W_TEST_RESULT(WFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));

      WFileStatus stat2;
      stat2.m_DocumentID = guid2;

      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePathNew), stat2, WFileChangedEvent::Type::DocumentUnlinked)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
    }
    for (size_t i = 0; i < 15; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Change file casing")
  {
    WStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("Folder12", "rootFile2.txt");

    WStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "RootFile2.txt");

    W_TEST_RESULT(WOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    WFileChangedEvent expected[] = {
      WFileChangedEvent(MakePath(sFilePathNew), {}, WFileChangedEvent::Type::FileAdded),
      WFileChangedEvent(MakePath(sFilePathOld), {}, WFileChangedEvent::Type::FileRemoved)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Change folder casing")
  {
    WStringBuilder sFolderPathOld(sOutputFolder);
    sFolderPathOld.AppendPath("Folder12");

    WStringBuilder sFolderPathNew(sOutputFolder);
    sFolderPathNew.AppendPath("FOLDER12");

    W_TEST_RESULT(WOSFile::MoveFileOrDirectory(sFolderPathOld, sFolderPathNew));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2 && folderEvents.GetCount() == 2)
        break;
    }

    {
      WFolderChangedEvent expected[] = {
        WFolderChangedEvent(MakePath(sFolderPathNew), WFolderChangedEvent::Type::FolderAdded),
        WFolderChangedEvent(MakePath(sFolderPathOld), WFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(WMakeArrayPtr(expected));
      ClearFolders();
    }

    {
      WStringBuilder sFilePathOld(sOutputFolder);
      sFilePathOld.AppendPath("Folder12", "RootFile2.txt");
      WStringBuilder sFilePathNew(sOutputFolder);
      sFilePathNew.AppendPath("FOLDER12", "RootFile2.txt");

      WFileChangedEvent expected[] = {
        WFileChangedEvent(MakePath(sFilePathNew), {}, WFileChangedEvent::Type::FileAdded),
        WFileChangedEvent(MakePath(sFilePathOld), {}, WFileChangedEvent::Type::FileRemoved)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
    }

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "delete folder")
  {
    WStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("FOLDER12");

    WStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("FOLDER12", "RootFile2.txt");

    W_TEST_RESULT(WOSFile::DeleteFolder(sFolderPath));

    for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      WFileSystemModel::GetSingleton()->MainThreadTick();
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

      W_LOCK(fileEventLock);
      W_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 1 && folderEvents.GetCount() == 1)
        break;
    }

    {
      WFolderChangedEvent expected[] = {
        WFolderChangedEvent(MakePath(sFolderPath), WFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(WMakeArrayPtr(expected));
    }

    {
      WFileChangedEvent expected[] = {
        WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileRemoved)};
      CompareFiles(WMakeArrayPtr(expected));
    }

    {
      W_LOCK(fileEventLock);
      W_LOCK(folderEventLock);
      // Check file removed before folder.
      W_TEST_BOOL(fileEventTimestamps[0] < folderEventTimestamps[0]);
    }

    ClearFolders();
    ClearFiles();

    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  referencedFiles = {};
  referencedFolders = {};

  W_TEST_BLOCK(WTestBlock::Enabled, "Shutdown with cached files and folders")
  {
    {
      // Add a file to test data directories being removed.
      WStringBuilder sFilePath(sOutputFolder);
      sFilePath.AppendPath("rootFile.txt");

      W_TEST_RESULT(WtCreateFile(sFilePath));

      for (WUInt32 i = 0; i < WAIT_LOOPS; i++)
      {
        WFileSystemModel::GetSingleton()->MainThreadTick();
        WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

        W_LOCK(fileEventLock);
        if (fileEvents.GetCount() > 0)
          break;
      }

      WFileChangedEvent expected[] = {WFileChangedEvent(MakePath(sFilePath), {}, WFileChangedEvent::Type::FileAdded)};
      CompareFiles(WMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
    }

    WFileSystemModel::GetSingleton()->Deinitialize(&referencedFiles, &referencedFolders);
    W_TEST_INT(referencedFiles.GetCount(), 1);
    W_TEST_INT(referencedFolders.GetCount(), 2);

    WFileChangedEvent expected[] = {WFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();

    WFolderChangedEvent expected2[] = {WFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset)};
    CompareFolders(WMakeArrayPtr(expected2));
    ClearFolders();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Startup without data dirs")
  {
    fsConfig.m_DataDirs.Clear();

    WFileSystemModel::GetSingleton()->Initialize(fsConfig, std::move(referencedFiles), std::move(referencedFolders));
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    W_TEST_INT(WFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 0);

    WFileChangedEvent expected[] = {WFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();

    WFolderChangedEvent expected2[] = {WFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset)};
    CompareFolders(WMakeArrayPtr(expected2));
    ClearFolders();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Final shutdown")
  {
    WFileSystemModel::GetSingleton()->Deinitialize();
    WFileChangedEvent expected[] = {WFileChangedEvent({}, {}, WFileChangedEvent::Type::ModelReset)};
    CompareFiles(WMakeArrayPtr(expected));
    ClearFiles();

    WFolderChangedEvent expected2[] = {WFolderChangedEvent({}, WFolderChangedEvent::Type::ModelReset)};
    CompareFolders(WMakeArrayPtr(expected2));
    ClearFolders();

    WFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(fileId);
    WFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(folderId);
  }
}

W_CREATE_SIMPLE_TEST(FileSystem, FileSystemModel)
{
  FileSystemModelTest();
}

#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
W_CREATE_SIMPLE_TEST(FileSystem, FileSystemModelNonNTFS)
{
  auto* pForceNonNTFS = static_cast<WCVarBool*>(WCVar::FindCVarByName("Platform.DirectoryWatcher.ForceNonNTFS"));
  *pForceNonNTFS = true;
  FileSystemModelTest();
  *pForceNonNTFS = false;
}
#  endif

#endif
