#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#if W_ENABLED(W_SUPPORTS_LONG_PATHS)
#  define LongPath                                                                                                                                   \
    "AVeryLongSubFolderPathNameThatShouldExceedThePathLengthLimitOnPlatformsLikeWindowsWhereOnly260CharactersAreAllowedOhNoesIStillNeedMoreThisIsNo" \
    "tLongEnoughAaaaaaaaaaaaaaahhhhStillTooShortAaaaaaaaaaaaaaaaaaaaaahImBoredNow"
#else
#  define LongPath "AShortPathBecaueThisPlatformDoesntSupportLongOnes"
#endif

W_CREATE_SIMPLE_TEST(IO, FileSystem)
{
  WStringBuilder sFileContent = "Lyrics to Taste The Cake:\n\
Turret: Who's there?\n\
Turret: Is anyone there?\n\
Turret: I see you.\n\
\n\
Chell rises from a stasis inside of a glass box\n\
She isn't greeted by faces,\n\
Only concrete and clocks.\n\
...";

  WStringBuilder szOutputFolder = WTestFramework::GetInstance()->GetAbsOutputPath();
  szOutputFolder.MakeCleanPath();

  WStringBuilder sOutputFolderResolved;
  WFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved).IgnoreResult();

  WStringBuilder sOutputFolder1 = szOutputFolder;
  sOutputFolder1.AppendPath("IO", "SubFolder");
  WStringBuilder sOutputFolder1Resolved;
  WFileSystem::ResolveSpecialDirectory(sOutputFolder1, sOutputFolder1Resolved).IgnoreResult();

  WStringBuilder sOutputFolder2 = szOutputFolder;
  sOutputFolder2.AppendPath("IO", "SubFolder2");
  WStringBuilder sOutputFolder2Resolved;
  WFileSystem::ResolveSpecialDirectory(sOutputFolder2, sOutputFolder2Resolved).IgnoreResult();

  W_TEST_BLOCK(WTestBlock::Enabled, "Setup Data Dirs")
  {
    // adding the same factory three times would actually not make a difference
    WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::FolderType::Factory);
    WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::FolderType::Factory);
    WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::FolderType::Factory);

    // WFileSystem::ClearAllDataDirectoryFactories();

    WFileSystem::RegisterDataDirectoryFactory(WDataDirectory::FolderType::Factory);

    // for absolute paths
    W_TEST_BOOL(WFileSystem::AddDataDirectory("", "", ":", WDataDirUsage::AllowWrites) == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(szOutputFolder, "Clear", "output", WDataDirUsage::AllowWrites) == W_SUCCESS);

    WStringBuilder sTempFile = sOutputFolder1Resolved;
    sTempFile.AppendPath(LongPath);
    sTempFile.AppendPath("Temp.tmp");

    WFileWriter TempFile;
    W_TEST_BOOL(TempFile.Open(sTempFile) == W_SUCCESS);
    TempFile.Close();

    sTempFile = sOutputFolder2Resolved;
    sTempFile.AppendPath("Temp.tmp");

    W_TEST_BOOL(TempFile.Open(sTempFile) == W_SUCCESS);
    TempFile.Close();

    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder1, "Clear", "output1", WDataDirUsage::AllowWrites) == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2, "Clear") == W_SUCCESS);

    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", WDataDirUsage::AllowWrites) == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == W_SUCCESS);

    W_TEST_INT(WFileSystem::RemoveDataDirectoryGroup("Remove"), 3);

    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", WDataDirUsage::AllowWrites) == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == W_SUCCESS);

    WFileSystem::ClearAllDataDirectories();

    W_TEST_INT(WFileSystem::RemoveDataDirectoryGroup("Remove"), 0);
    W_TEST_INT(WFileSystem::RemoveDataDirectoryGroup("Clear"), 0);

    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder1, "", "output1", WDataDirUsage::AllowWrites) == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2) == W_SUCCESS);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Add / Remove Data Dirs")
  {
    W_TEST_BOOL(WFileSystem::AddDataDirectory("", "xyz-rooted", "xyz", WDataDirUsage::AllowWrites) == W_SUCCESS);

    W_TEST_BOOL(WFileSystem::FindDataDirectoryWithRoot("xyz") != nullptr);

    W_TEST_BOOL(WFileSystem::RemoveDataDirectory("xyz") == true);

    W_TEST_BOOL(WFileSystem::FindDataDirectoryWithRoot("xyz") == nullptr);

    W_TEST_BOOL(WFileSystem::RemoveDataDirectory("xyz") == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Write File")
  {
    WFileWriter FileOut;

    WStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    W_TEST_BOOL(FileOut.Open(":output1/FileSystemTest.txt") == W_SUCCESS);

    W_TEST_STRING(FileOut.GetFilePathRelative(), "FileSystemTest.txt");
    W_TEST_STRING(FileOut.GetFilePathAbsolute(), sAbs);

    W_TEST_INT(FileOut.GetFileSize(), 0);

    W_TEST_BOOL(FileOut.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()) == W_SUCCESS);

    FileOut.Flush().IgnoreResult();
    W_TEST_INT(FileOut.GetFileSize(), sFileContent.GetElementCount());

    FileOut.Close();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Read File")
  {
    WFileReader FileIn;

    WStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    W_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == W_SUCCESS);

    W_TEST_STRING(FileIn.GetFilePathRelative(), "FileSystemTest.txt");
    W_TEST_STRING(FileIn.GetFilePathAbsolute(), sAbs);

    W_TEST_INT(FileIn.GetFileSize(), sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    W_TEST_INT(FileIn.ReadBytes(szTemp, 1024 * 2), sFileContent.GetElementCount());

    W_TEST_BOOL(WMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

#if W_DISABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)

  W_TEST_BLOCK(WTestBlock::Enabled, "Read File (Absolute Path)")
  {
    WFileReader FileIn;

    WStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    W_TEST_BOOL(FileIn.Open(sAbs) == W_SUCCESS);

    W_TEST_STRING(FileIn.GetFilePathRelative(), "FileSystemTest.txt");
    W_TEST_STRING(FileIn.GetFilePathAbsolute(), sAbs);

    W_TEST_INT(FileIn.GetFileSize(), sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    W_TEST_INT(FileIn.ReadBytes(szTemp, 1024 * 2), sFileContent.GetElementCount());

    W_TEST_BOOL(WMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

#endif

  W_TEST_BLOCK(WTestBlock::Enabled, "Delete File / Exists File")
  {
    {
      W_TEST_BOOL(WFileSystem::ExistsFile(":output1/FileSystemTest.txt"));
      WFileSystem::DeleteFile(":output1/FileSystemTest.txt");
      W_TEST_BOOL(!WFileSystem::ExistsFile("FileSystemTest.txt"));

      WFileReader FileIn;
      W_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == W_FAILURE);
    }

    // very long path names
    {
      WStringBuilder sTempFile = ":output1";
      sTempFile.AppendPath(LongPath);
      sTempFile.AppendPath("Temp.tmp");

      WFileWriter TempFile;
      W_TEST_BOOL(TempFile.Open(sTempFile) == W_SUCCESS);
      TempFile.Close();

      W_TEST_BOOL(WFileSystem::ExistsFile(sTempFile));
      WFileSystem::DeleteFile(sTempFile);
      W_TEST_BOOL(!WFileSystem::ExistsFile(sTempFile));

      WFileReader FileIn;
      W_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == W_FAILURE);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileStats")
  {
    const char* szPath = ":output1/" LongPath "/FileSystemTest.txt";

    // Create file
    {
      WFileWriter FileOut;
      WStringBuilder sAbs = sOutputFolder1Resolved;
      sAbs.AppendPath("FileSystemTest.txt");
      W_TEST_BOOL(FileOut.Open(szPath) == W_SUCCESS);
      FileOut.WriteBytes("Test", 4).IgnoreResult();
    }

    WFileStats stat;

    W_TEST_BOOL(WFileSystem::GetFileStats(szPath, stat).Succeeded());

    W_TEST_BOOL(!stat.m_bIsDirectory);
    W_TEST_STRING(stat.m_sName, "FileSystemTest.txt");
    W_TEST_INT(stat.m_uiFileSize, 4);

    WFileSystem::DeleteFile(szPath);
    W_TEST_BOOL(WFileSystem::GetFileStats(szPath, stat).Failed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ResolvePath")
  {
    WStringBuilder sRel, sAbs;

    W_TEST_BOOL(WFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == W_SUCCESS);

    WStringBuilder sExpectedAbs = sOutputFolder1Resolved;
    sExpectedAbs.AppendPath("FileSystemTest2.txt");

    W_TEST_STRING(sAbs, sExpectedAbs);
    W_TEST_STRING(sRel, "FileSystemTest2.txt");

    // create a file in the second dir
    {
      W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", WDataDirUsage::AllowWrites) == W_SUCCESS);

      {
        WFileWriter FileOut;
        W_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == W_SUCCESS);
      }

      W_TEST_INT(WFileSystem::RemoveDataDirectoryGroup("Remove"), 1);
    }

    // find the path to an existing file
    {
      W_TEST_BOOL(WFileSystem::ResolvePath("FileSystemTest2.txt", &sAbs, &sRel) == W_SUCCESS);

      sExpectedAbs = sOutputFolder2Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      W_TEST_STRING(sAbs, sExpectedAbs);
      W_TEST_STRING(sRel, "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      W_TEST_BOOL(WFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == W_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      W_TEST_STRING(sAbs, sExpectedAbs);
      W_TEST_STRING(sRel, "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      W_TEST_BOOL(WFileSystem::ResolvePath(":output1/SubSub/FileSystemTest2.txt", &sAbs, &sRel) == W_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("SubSub/FileSystemTest2.txt");

      W_TEST_STRING(sAbs, sExpectedAbs);
      W_TEST_STRING(sRel, "SubSub/FileSystemTest2.txt");
    }

    WFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    WFileSystem::DeleteFile(":output2/FileSystemTest2.txt");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindFolderWithSubPath")
  {
    W_TEST_BOOL(WFileSystem::AddDataDirectory(szOutputFolder, "remove", "toplevel", WDataDirUsage::AllowWrites) == W_SUCCESS);
    W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder2, "remove", "output2", WDataDirUsage::AllowWrites) == W_SUCCESS);

    WStringBuilder StartPath;
    WStringBuilder SubPath;
    WStringBuilder result, expected;

    // make sure this exists
    {
      WFileWriter FileOut;
      W_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == W_SUCCESS);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("DoesNotExist");

      W_TEST_BOOL(WFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Failed());
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("SubFolder2");
      expected.Set(sOutputFolderResolved, "/IO/");

      W_TEST_BOOL(WFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      W_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub");
      SubPath.Set("IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/");

      W_TEST_BOOL(WFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      W_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub");
      SubPath.Set("IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/");

      W_TEST_BOOL(WFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      W_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("SubFolder2/FileSystemTest2.txt");
      expected.Set(sOutputFolderResolved, "/IO/");

      W_TEST_BOOL(WFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      W_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(":toplevel/IO/SubFolder");
      SubPath.Set("IO/SubFolder2");
      expected.Set(":toplevel/");

      W_TEST_BOOL(WFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      W_TEST_STRING(result, expected);
    }

    WFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    WFileSystem::DeleteFile(":output2/FileSystemTest2.txt");

    WFileSystem::RemoveDataDirectoryGroup("remove");
  }
}
