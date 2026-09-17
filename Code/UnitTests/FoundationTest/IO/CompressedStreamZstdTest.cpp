#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

W_CREATE_SIMPLE_TEST(IO, CompressedStreamZstd)
{
  WDynamicArray<WUInt32> TestData;

  // create the test data
  // a repetition of a counting sequence that is getting longer and longer, ie:
  // 0, 0,1, 0,1,2, 0,1,2,3, 0,1,2,3,4, ...
  {
    TestData.SetCountUninitialized(1024 * 1024 * 8);

    const WUInt32 uiItems = TestData.GetCount();
    WUInt32 uiStartPos = 0;

    for (WUInt32 uiWrite = 1; uiWrite < uiItems; ++uiWrite)
    {
      uiWrite = WMath::Min(uiWrite, uiItems - uiStartPos);

      if (uiWrite == 0)
        break;

      for (WUInt32 i = 0; i < uiWrite; ++i)
      {
        TestData[uiStartPos + i] = i;
      }

      uiStartPos += uiWrite;
    }
  }


  WDefaultMemoryStreamStorage StreamStorage;

  WMemoryStreamWriter MemoryWriter(&StreamStorage);
  WMemoryStreamReader MemoryReader(&StreamStorage);

  WCompressedStreamReaderZstd CompressedReader;
  WCompressedStreamWriterZstd CompressedWriter;

  const float fExpectedCompressionRatio = 900.0f; // this is a guess that is based on the current input data and size

  W_TEST_BLOCK(WTestBlock::Enabled, "Compress Data")
  {
    CompressedWriter.SetOutputStream(&MemoryWriter, 0);

    bool bFlush = true;

    WUInt32 uiWrite = 1;
    for (WUInt32 i = 0; i < TestData.GetCount();)
    {
      uiWrite = WMath::Min<WUInt32>(uiWrite, TestData.GetCount() - i);

      W_TEST_BOOL(CompressedWriter.WriteBytes(&TestData[i], sizeof(WUInt32) * uiWrite) == W_SUCCESS);

      if (bFlush)
      {
        // this actually hurts compression rates
        W_TEST_BOOL(CompressedWriter.Flush() == W_SUCCESS);
      }

      bFlush = !bFlush;

      i += uiWrite;
      uiWrite += 17; // try different sizes to write
    }

    // flush all data
    CompressedWriter.FinishCompressedStream().AssertSuccess();

    const WUInt64 uiCompressed = CompressedWriter.GetCompressedSize();
    const WUInt64 uiUncompressed = CompressedWriter.GetUncompressedSize();
    const WUInt64 uiBytesWritten = CompressedWriter.GetWrittenBytes();

    W_TEST_INT(uiUncompressed, TestData.GetCount() * sizeof(WUInt32));
    W_TEST_BOOL(uiBytesWritten > uiCompressed);
    W_TEST_BOOL(uiBytesWritten < uiUncompressed);

    const float fRatio = (float)uiUncompressed / (float)uiCompressed;
    W_TEST_BOOL(fRatio >= fExpectedCompressionRatio);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Uncompress Data")
  {
    CompressedReader.SetInputStream(&MemoryReader);

    bool bSkip = false;
    WUInt32 uiStartPos = 0;

    WDynamicArray<WUInt32> TestDataRead = TestData; // initialize with identical data, makes comparing the skipped parts easier

    // read the data in blocks that get larger and larger
    for (WUInt32 iRead = 1; iRead < TestData.GetCount(); ++iRead)
    {
      WUInt32 iToRead = WMath::Min(iRead, TestData.GetCount() - uiStartPos);

      if (iToRead == 0)
        break;

      if (bSkip)
      {
        const WUInt64 uiReadFromStream = CompressedReader.SkipBytes(sizeof(WUInt32) * iToRead);
        W_TEST_BOOL(uiReadFromStream == sizeof(WUInt32) * iToRead);
      }
      else
      {
        // overwrite part we are going to read from the stream, to make sure it re-reads the correct data
        for (WUInt32 i = 0; i < iToRead; ++i)
        {
          TestDataRead[uiStartPos + i] = 0;
        }

        const WUInt64 uiReadFromStream = CompressedReader.ReadBytes(&TestDataRead[uiStartPos], sizeof(WUInt32) * iToRead);
        W_TEST_BOOL(uiReadFromStream == sizeof(WUInt32) * iToRead);
      }

      bSkip = !bSkip;

      uiStartPos += iToRead;
    }

    W_TEST_BOOL(TestData == TestDataRead);

    // test reading after the end of the stream
    for (WUInt32 i = 0; i < 1000; ++i)
    {
      WUInt32 uiTemp = 0;
      W_TEST_BOOL(CompressedReader.ReadBytes(&uiTemp, sizeof(WUInt32)) == 0);
    }
  }
}

#endif
