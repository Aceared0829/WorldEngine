#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

W_CREATE_SIMPLE_TEST(IO, OSFile)
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

  const WUInt32 uiTextLen = sFileContent.GetElementCount();

  WStringBuilder sOutputFile = WTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO", "SubFolder");
  sOutputFile.AppendPath("OSFile_TestFile.txt");

  WStringBuilder sOutputFile2 = WTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile2.MakeCleanPath();
  sOutputFile2.AppendPath("IO", "SubFolder2");
  sOutputFile2.AppendPath("OSFile_TestFileCopy.txt");

  WStringBuilder sOutputFile3 = WTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile3.MakeCleanPath();
  sOutputFile3.AppendPath("IO", "SubFolder2", "SubSubFolder");
  sOutputFile3.AppendPath("RandomFile.txt");

  W_TEST_BLOCK(WTestBlock::Enabled, "Write File")
  {
    WOSFile f;
    W_TEST_BOOL(f.Open(sOutputFile.GetData(), WFileOpenMode::Write) == W_SUCCESS);
    W_TEST_BOOL(f.IsOpen());
    W_TEST_INT(f.GetFilePosition(), 0);
    W_TEST_INT(f.GetFileSize(), 0);

    for (WUInt32 i = 0; i < uiTextLen; ++i)
    {
      W_TEST_BOOL(f.Write(&sFileContent.GetData()[i], 1) == W_SUCCESS);
      W_TEST_INT(f.GetFilePosition(), i + 1);
      W_TEST_INT(f.GetFileSize(), i + 1);
    }

    W_TEST_INT(f.GetFilePosition(), uiTextLen);
    f.SetFilePosition(5, WFileSeekMode::FromStart);
    W_TEST_INT(f.GetFileSize(), uiTextLen);

    W_TEST_INT(f.GetFilePosition(), 5);
    // f.Close(); // The file should be closed automatically
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Append File")
  {
    WOSFile f;
    W_TEST_BOOL(f.Open(sOutputFile.GetData(), WFileOpenMode::Append) == W_SUCCESS);
    W_TEST_BOOL(f.IsOpen());
    W_TEST_INT(f.GetFilePosition(), uiTextLen);
    W_TEST_BOOL(f.Write(sFileContent.GetData(), uiTextLen) == W_SUCCESS);
    W_TEST_INT(f.GetFilePosition(), uiTextLen * 2);
    f.Close();
    W_TEST_BOOL(!f.IsOpen());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Read File")
  {
    const WUInt32 FS_MAX_PATH = 1024;
    char szTemp[FS_MAX_PATH];

    WOSFile f;
    W_TEST_BOOL(f.Open(sOutputFile.GetData(), WFileOpenMode::Read) == W_SUCCESS);
    W_TEST_BOOL(f.IsOpen());
    W_TEST_INT(f.GetFilePosition(), 0);

    W_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);
    W_TEST_INT(f.GetFilePosition(), uiTextLen * 2);

    W_TEST_BOOL(WMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    W_TEST_BOOL(WMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Copy File")
  {
    WOSFile::CopyFile(sOutputFile.GetData(), sOutputFile2.GetData()).IgnoreResult();

    WOSFile f;
    W_TEST_BOOL(f.Open(sOutputFile2.GetData(), WFileOpenMode::Read) == W_SUCCESS);

    const WUInt32 FS_MAX_PATH = 1024;
    char szTemp[FS_MAX_PATH];

    W_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);

    W_TEST_BOOL(WMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    W_TEST_BOOL(WMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadAll")
  {
    WOSFile f;
    W_TEST_BOOL(f.Open(sOutputFile, WFileOpenMode::Read) == W_SUCCESS);

    WDynamicArray<WUInt8> fileContent;
    const WUInt64 bytes = f.ReadAll(fileContent);

    W_TEST_INT(bytes, uiTextLen * 2);

    W_TEST_BOOL(WMemoryUtils::IsEqual(fileContent.GetData(), (const WUInt8*)sFileContent.GetData(), uiTextLen));

    f.Close();
  }

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  W_TEST_BLOCK(WTestBlock::Enabled, "File Stats")
  {
    WFileStats s;

    WStringBuilder dir = sOutputFile2.GetFileDirectory();

    W_TEST_BOOL(WOSFile::GetFileStats(sOutputFile2.GetData(), s) == W_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(WSIUnitOfTime::Microsecond));

    W_TEST_BOOL(WOSFile::GetFileStats(dir.GetData(), s) == W_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(WSIUnitOfTime::Microsecond));
  }

#  if (W_ENABLED(W_SUPPORTS_CASE_INSENSITIVE_PATHS) && W_ENABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS))
  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileCasing")
  {
    WStringBuilder dir = sOutputFile2;
    dir.ToLower();

#    if W_ENABLED(W_PLATFORM_WINDOWS)
    // On Windows the drive letter will always be turned upper case by WOSFile::GetFileCasing()
    // ensure that our input data ('ground truth') also uses an upper case drive letter
    auto driveLetterIterator = sOutputFile2.GetIteratorFront();
    const WUInt32 uiDriveLetter = WStringUtils::ToUpperChar(driveLetterIterator.GetCharacter());
    sOutputFile2.ChangeCharacter(driveLetterIterator, uiDriveLetter);
#    endif

    WStringBuilder sCorrected;
    W_TEST_BOOL(WOSFile::GetFileCasing(dir.GetData(), sCorrected) == W_SUCCESS);

    // On Windows the drive letter will always be made to upper case
    W_TEST_STRING(sCorrected.GetData(), sOutputFile2.GetData());
  }
#  endif // W_SUPPORTS_CASE_INSENSITIVE_PATHS && W_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif   // W_SUPPORTS_FILE_STATS

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

  W_TEST_BLOCK(WTestBlock::Enabled, "File Iterator")
  {
    WStringBuilder sOutputFolder = WFileSystem::GetSdkRootDirectory();
    sOutputFolder.AppendPath("Data/Base/*");

    WStringBuilder sFullPath;

    WUInt32 uiFolders = 0;
    WUInt32 uiFiles = 0;

    bool bSkipFolder = true;

    WFileSystemIterator it;
    for (it.StartSearch(sOutputFolder.GetData(), WFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      sFullPath = it.GetCurrentPath();
      sFullPath.AppendPath(it.GetStats().m_sName.GetData());

      it.GetStats();
      it.GetCurrentPath();

      if (it.GetStats().m_bIsDirectory)
      {
        ++uiFolders;
        bSkipFolder = !bSkipFolder;

        if (bSkipFolder)
        {
          it.SkipFolder(); // replaces the 'Next' call
          continue;
        }
      }
      else
      {
        ++uiFiles;
      }

      it.Next();
    }

// The binary folder will only have subdirectories on windows desktop
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
    W_TEST_BOOL(uiFolders > 0);
#  endif
    W_TEST_BOOL(uiFiles > 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "File Iterator (non recursive)")
  {
    WStringBuilder sOutputFolder = WFileSystem::GetSdkRootDirectory();
    sOutputFolder.AppendPath("Data/Base");

    WStringBuilder sFullPath;

    WUInt32 uiFolders = 0;
    WUInt32 uiFiles = 0;

    WFileSystemIterator it;
    for (it.StartSearch(sOutputFolder.GetData(), WFileSystemIteratorFlags::ReportFiles); it.IsValid();)
    {
      sFullPath = it.GetCurrentPath();
      sFullPath.AppendPath(it.GetStats().m_sName.GetData());

      it.GetStats();
      it.GetCurrentPath();

      if (it.GetStats().m_bIsDirectory)
      {
        ++uiFolders;
      }
      else
      {
        ++uiFiles;
      }

      W_TEST_BOOL(sFullPath.StartsWith(sOutputFolder));
      sFullPath.MakeRelativeTo(sOutputFolder).AssertSuccess();

      W_TEST_BOOL(!sFullPath.FindSubString("/")); // no sub path

      it.Next();
    }

    W_TEST_BOOL(uiFolders == 0);
    W_TEST_BOOL(uiFiles > 0);
  }

#endif

  W_TEST_BLOCK(WTestBlock::Enabled, "Delete File")
  {
    W_TEST_BOOL(WOSFile::DeleteFile(sOutputFile.GetData()) == W_SUCCESS);
    W_TEST_BOOL(WOSFile::DeleteFile(sOutputFile.GetData()) == W_SUCCESS);          // second time should still 'succeed'

    W_TEST_BOOL(WOSFile::DeleteFile(sOutputFile2.GetData()) == W_SUCCESS);
    W_TEST_BOOL(WOSFile::DeleteFile(sOutputFile2.GetData()) == W_SUCCESS);         // second time should still 'succeed'

    WOSFile f;
    W_TEST_BOOL(f.Open(sOutputFile.GetData(), WFileOpenMode::Read) == W_FAILURE);  // file should not exist anymore
    W_TEST_BOOL(f.Open(sOutputFile2.GetData(), WFileOpenMode::Read) == W_FAILURE); // file should not exist anymore
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCurrentWorkingDirectory")
  {
    WStringBuilder cwd = WOSFile::GetCurrentWorkingDirectory();

    W_TEST_BOOL(!cwd.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakePathAbsoluteWithCWD")
  {
    WStringBuilder cwd = WOSFile::GetCurrentWorkingDirectory();
    WStringBuilder path = WOSFile::MakePathAbsoluteWithCWD("sub/folder");

    W_TEST_BOOL(path.StartsWith(cwd));
    W_TEST_BOOL(path.EndsWith("/sub/folder"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExistsFile")
  {
    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile.GetData()) == false);
    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      WOSFile f;
      W_TEST_BOOL(f.Open(sOutputFile.GetData(), WFileOpenMode::Write) == W_SUCCESS);
    }

    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile.GetData()) == true);
    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      WOSFile f;
      W_TEST_BOOL(f.Open(sOutputFile2.GetData(), WFileOpenMode::Write) == W_SUCCESS);
    }

    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile.GetData()) == true);
    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile2.GetData()) == true);

    W_TEST_BOOL(WOSFile::DeleteFile(sOutputFile.GetData()) == W_SUCCESS);
    W_TEST_BOOL(WOSFile::DeleteFile(sOutputFile2.GetData()) == W_SUCCESS);

    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile.GetData()) == false);
    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    WStringBuilder sOutputFolder = WTestFramework::GetInstance()->GetAbsOutputPath();
    // We should not report folders as files
    W_TEST_BOOL(WOSFile::ExistsFile(sOutputFolder) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExistsDirectory")
  {
    // files are not folders
    W_TEST_BOOL(WOSFile::ExistsDirectory(sOutputFile.GetData()) == false);
    W_TEST_BOOL(WOSFile::ExistsDirectory(sOutputFile2.GetData()) == false);

    WStringBuilder sOutputFolder = WTestFramework::GetInstance()->GetAbsOutputPath();
    W_TEST_BOOL(WOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("IO");
    W_TEST_BOOL(WOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("SubFolder");
    W_TEST_BOOL(WOSFile::ExistsDirectory(sOutputFolder) == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetApplicationDirectory")
  {
    WStringView sAppDir = WOSFile::GetApplicationDirectory();
    W_TEST_BOOL(!sAppDir.IsEmpty());
  }

#if (W_ENABLED(W_SUPPORTS_FILE_ITERATORS) && W_ENABLED(W_SUPPORTS_FILE_STATS))

  W_TEST_BLOCK(WTestBlock::Enabled, "DeleteFolder")
  {
    {
      WOSFile f;
      W_TEST_BOOL(f.Open(sOutputFile3.GetData(), WFileOpenMode::Write) == W_SUCCESS);
    }

    WStringBuilder SubFolder2 = WTestFramework::GetInstance()->GetAbsOutputPath();
    SubFolder2.MakeCleanPath();
    SubFolder2.AppendPath("IO", "SubFolder2");

    W_TEST_BOOL(WOSFile::DeleteFolder(SubFolder2).Succeeded());
    W_TEST_BOOL(!WOSFile::ExistsDirectory(SubFolder2));
  }

#endif
}
