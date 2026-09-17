#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#if (W_ENABLED(W_SUPPORTS_FILE_ITERATORS) && W_ENABLED(W_SUPPORTS_FILE_STATS) && defined(BUILDSYSTEM_HAS_ARCHIVE_TOOL))

W_CREATE_SIMPLE_TEST(IO, Archive)
{
  WStringBuilder sOutputFolder = WTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("ArchiveTest");
  sOutputFolder.MakeCleanPath();

  // make sure it is empty
  WOSFile::DeleteFolder(sOutputFolder).IgnoreResult();
  WOSFile::CreateDirectoryStructure(sOutputFolder).IgnoreResult();

  if (!W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", WDataDirUsage::AllowWrites).Succeeded()))
    return;

  const char* szTestData = "TestData";
  const char* szUnpackedData = "Unpacked";

  // write a couple of files for packaging
  const char* szFileList[] = {
    "File1.txt",
    "FolderA/File2.jpg",         // should get stored uncompressed
    "FolderB/File3.txt",
    "FolderA/FolderC/File4.zip", // should get stored uncompressed
    "FolderA/FolderD/File5.txt",
    "File6.txt",
  };

  const WUInt32 uiMinFileSize = 1024 * 128;


  W_TEST_BLOCK(WTestBlock::Enabled, "Generate Data")
  {
    WUInt64 uiValue = 0;

    WStringBuilder fileName;

    for (WUInt32 uiFileIdx = 0; uiFileIdx < W_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      fileName.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);

      WFileWriter file;
      if (!W_TEST_BOOL(file.Open(fileName).Succeeded()))
        return;

      for (WUInt32 i = 0; i < uiMinFileSize * uiFileIdx; ++i)
      {
        file << uiValue;
        ++uiValue;
      }
    }
  }

  const WStringBuilder sArchiveFolder(sOutputFolder, "/", szTestData);
  const WStringBuilder sUnpackFolder(sOutputFolder, "/", szUnpackedData);
  const WStringBuilder sArchiveFile(sOutputFolder, "/", szTestData, ".WArchive");

  WStringBuilder pathToArchiveTool = WCommandLineUtils::GetGlobalInstance()->GetParameter(0);
  pathToArchiveTool.PathParentDirectory();
  pathToArchiveTool.AppendPath("WArchiveTool");
#  if W_ENABLED(W_PLATFORM_WINDOWS)
  pathToArchiveTool.Append(".exe");
#  endif
  W_TEST_BLOCK(WTestBlock::Enabled, "Create a Package")
  {

    WProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack(sArchiveFolder);

    WInt32 iReturnValue = 1;

    WProcess ArchiveToolProc;
    if (!W_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()))
      return;

    W_TEST_INT(iReturnValue, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Unpack the Package")
  {

    WProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack("-unpack");
    opt.m_Arguments.PushBack(sArchiveFile);
    opt.m_Arguments.PushBack("-out");
    opt.m_Arguments.PushBack(sUnpackFolder);

    WInt32 iReturnValue = 1;

    WProcess ArchiveToolProc;
    if (!W_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()))
      return;

    W_TEST_INT(iReturnValue, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Compare unpacked data")
  {
    WUInt64 uiValue = 0;

    WStringBuilder sFileSrc;
    WStringBuilder sFileDst;

    for (WUInt32 uiFileIdx = 0; uiFileIdx < W_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(sOutputFolder, "/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(sOutputFolder, "/", szUnpackedData, "/", szFileList[uiFileIdx]);

      W_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Mount as Data Dir")
  {
    if (!W_TEST_BOOL(WFileSystem::AddDataDirectory(sArchiveFile, "Clear", "archive", WDataDirUsage::ReadOnly) == W_SUCCESS))
      return;

    WStringBuilder sFileSrc;
    WStringBuilder sFileDst;

    // test opening multiple files in parallel and keeping them open
    WFileReader readers[W_ARRAY_SIZE(szFileList)];
    for (WUInt32 uiFileIdx = 0; uiFileIdx < W_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);
      W_TEST_BOOL(readers[uiFileIdx].Open(sFileDst).Succeeded());

      // advance the reader a bit
      W_TEST_INT(readers[uiFileIdx].SkipBytes(uiMinFileSize * uiFileIdx), uiMinFileSize * uiFileIdx);
    }

    for (WUInt32 uiFileIdx = 0; uiFileIdx < W_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);

      W_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }

    // mount a second time
    if (!W_TEST_BOOL(WFileSystem::AddDataDirectory(sArchiveFile, "Clear", "archive2", WDataDirUsage::ReadOnly) == W_SUCCESS))
      return;
  }

  WFileSystem::RemoveDataDirectoryGroup("Clear");
}

#endif
