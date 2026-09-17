#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

W_CREATE_SIMPLE_TEST(IO, DeferredFileWriter)
{
  W_TEST_BOOL(WFileSystem::AddDataDirectory("", "", ":", WDataDirUsage::AllowWrites) == W_SUCCESS);

  const WStringBuilder szOutputFolder = WTestFramework::GetInstance()->GetAbsOutputPath();
  WStringBuilder sOutputFolderResolved;
  WFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved).IgnoreResult();

  WStringBuilder sTempFile = sOutputFolderResolved;
  sTempFile.AppendPath("Temp.tmp");

  // make sure the file does not exist
  WFileSystem::DeleteFile(sTempFile);
  W_TEST_BOOL(!WFileSystem::ExistsFile(sTempFile));

  W_TEST_BLOCK(WTestBlock::Enabled, "DeferredFileWriter")
  {
    WDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (WUInt64 i = 0; i < 1'000'000; ++i)
    {
      writer << i;
    }

    // does not exist yet
    W_TEST_BOOL(!WFileSystem::ExistsFile(sTempFile));
  }

  // now it exists
  W_TEST_BOOL(WFileSystem::ExistsFile(sTempFile));

  // check content is correct
  {
    WFileReader reader;
    W_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (WUInt64 i = 0; i < 1'000'000; ++i)
    {
      WUInt64 v;
      reader >> v;
      W_TEST_BOOL(v == i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "DeferredFileWriter2")
  {
    WDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (WUInt64 i = 1; i < 100'000; ++i)
    {
      writer << i;
    }

    // does exist from earlier
    W_TEST_BOOL(WFileSystem::ExistsFile(sTempFile));

    // check content is as previous correct
    {
      WFileReader reader;
      W_TEST_BOOL(reader.Open(sTempFile).Succeeded());

      for (WUInt64 i = 0; i < 1'000'000; ++i)
      {
        WUInt64 v;
        reader >> v;
        W_TEST_BOOL(v == i);
      }
    }
  }

  // exist but now was overwritten
  W_TEST_BOOL(WFileSystem::ExistsFile(sTempFile));

  // check content is as previous correct
  {
    WFileReader reader;
    W_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (WUInt64 i = 1; i < 100'000; ++i)
    {
      WUInt64 v;
      reader >> v;
      W_TEST_BOOL(v == i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Discard")
  {
    WStringBuilder sTempFile2 = sOutputFolderResolved;
    sTempFile2.AppendPath("Temp2.tmp");
    {
      WDeferredFileWriter writer;
      writer.SetOutput(sTempFile2);
      writer << 10;
      writer.Discard();
    }
    W_TEST_BOOL(!WFileSystem::ExistsFile(sTempFile2));
  }

  WFileSystem::DeleteFile(sTempFile);
  WFileSystem::ClearAllDataDirectories();
}
