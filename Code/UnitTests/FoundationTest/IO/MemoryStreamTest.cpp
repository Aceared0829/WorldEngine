#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/MemoryStream.h>

W_CREATE_SIMPLE_TEST_GROUP(IO);

W_CREATE_SIMPLE_TEST(IO, MemoryStream)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Memory Stream Reading / Writing")
  {
    WDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    WMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    WMemoryStreamWriter StreamWriter(&StreamStorage);

    // Temp read pointer
    WUInt8* pPointer = reinterpret_cast<WUInt8*>(0x41); // Should crash when accessed

    // Try reading from an empty stream (should not crash, just return 0 bytes read)
    WUInt64 uiBytesRead = StreamReader.ReadBytes(pPointer, 128);

    W_TEST_BOOL(uiBytesRead == 0);


    // Now try writing data to the stream and reading it back
    WUInt32 uiData[1024];
    for (WUInt32 i = 0; i < 1024; i++)
      uiData[i] = rand();

    // Calculate the hash so we can reuse the array
    const WUInt32 uiHashBeforeWriting = WHashingUtils::xxHash32(uiData, sizeof(WUInt32) * 1024);

    // Write the data
    W_TEST_BOOL(StreamWriter.WriteBytes(reinterpret_cast<const WUInt8*>(uiData), sizeof(WUInt32) * 1024) == W_SUCCESS);

    W_TEST_BOOL(StreamWriter.GetByteCount64() == sizeof(WUInt32) * 1024);
    W_TEST_BOOL(StreamWriter.GetByteCount64() == StreamReader.GetByteCount64());
    W_TEST_BOOL(StreamWriter.GetByteCount64() == StreamStorage.GetStorageSize64());


    // Clear the array for the read back
    WMemoryUtils::ZeroFill(uiData, 1024);

    uiBytesRead = StreamReader.ReadBytes(reinterpret_cast<WUInt8*>(uiData), sizeof(WUInt32) * 1024);

    W_TEST_BOOL(uiBytesRead == sizeof(WUInt32) * 1024);

    const WUInt32 uiHashAfterReading = WHashingUtils::xxHash32(uiData, sizeof(WUInt32) * 1024);

    W_TEST_BOOL(uiHashAfterReading == uiHashBeforeWriting);

    // Modify data and test the Rewind() functionality of the writer
    uiData[0] = 0x42;
    uiData[1] = 0x23;

    const WUInt32 uiHashOfModifiedData = WHashingUtils::xxHash32(uiData, sizeof(WUInt32) * 4); // Only test the first 4 elements now

    StreamWriter.SetWritePosition(0);

    StreamWriter.WriteBytes(uiData, sizeof(WUInt32) * 4).IgnoreResult();

    // Clear the array for the read back
    WMemoryUtils::ZeroFill(uiData, 4);

    // Test the rewind of the reader as well
    StreamReader.SetReadPosition(0);

    uiBytesRead = StreamReader.ReadBytes(uiData, sizeof(WUInt32) * 4);

    W_TEST_BOOL(uiBytesRead == sizeof(WUInt32) * 4);

    const WUInt32 uiHashAfterReadingOfModifiedData = WHashingUtils::xxHash32(uiData, sizeof(WUInt32) * 4);

    W_TEST_BOOL(uiHashAfterReadingOfModifiedData == uiHashOfModifiedData);

    // Test skipping
    StreamReader.SetReadPosition(0);

    StreamReader.SkipBytes(sizeof(WUInt32));

    WUInt32 uiTemp;

    uiBytesRead = StreamReader.ReadBytes(&uiTemp, sizeof(WUInt32));

    W_TEST_BOOL(uiBytesRead == sizeof(WUInt32));

    // We skipped over the first 0x42 element, so this should be 0x23
    W_TEST_BOOL(uiTemp == 0x23);

    // Skip more bytes than available
    WUInt64 uiBytesSkipped = StreamReader.SkipBytes(0xFFFFFFFFFF);

    W_TEST_BOOL(uiBytesSkipped < 0xFFFFFFFFFF);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Raw Memory Stream Reading")
  {
    WDynamicArray<WUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      OrigStorage[i] = i % 256;
    }

    {
      WRawMemoryStreamReader reader(OrigStorage);

      WDynamicArray<WUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<WUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      W_TEST_BOOL(OrigStorage == CopyStorage);
    }

    {
      WRawMemoryStreamReader reader(OrigStorage.GetData() + 510, 490);

      WDynamicArray<WUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<WUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      W_TEST_BOOL(OrigStorage != CopyStorage);

      for (WUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }

    {
      WRawMemoryStreamReader reader(OrigStorage.GetData(), 1000);
      reader.SkipBytes(510);

      WDynamicArray<WUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(490);
      reader.ReadBytes(CopyStorage.GetData(), 490);

      W_TEST_BOOL(OrigStorage != CopyStorage);

      for (WUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Raw Memory Stream Writing")
  {
    WDynamicArray<WUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    WRawMemoryStreamWriter writer0;
    W_TEST_INT(writer0.GetNumWrittenBytes(), 0);
    W_TEST_INT(writer0.GetStorageSize(), 0);

    WRawMemoryStreamWriter writer(OrigStorage.GetData(), OrigStorage.GetCount());

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      writer << static_cast<WUInt8>(i % 256);

      W_TEST_INT(writer.GetNumWrittenBytes(), i + 1);
      W_TEST_INT(writer.GetStorageSize(), 1000);
    }

    for (WUInt32 i = 0; i < 1000; ++i)
    {
      W_TEST_INT(OrigStorage[i], i % 256);
    }

    {
      WRawMemoryStreamWriter writer2(OrigStorage);
      W_TEST_INT(writer2.GetNumWrittenBytes(), 0);
      W_TEST_INT(writer2.GetStorageSize(), 1000);
    }
  }
}

W_CREATE_SIMPLE_TEST(IO, LargeMemoryStream)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Large Memory Stream Reading / Writing")
  {
    WDefaultMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    const WUInt8 pattern[] = {11, 10, 27, 4, 14, 3, 21, 6};

    WUInt64 uiSize = 0;
    constexpr WUInt64 bytesToTest = 0x8000000llu; // tested with up to 8 GB, but that just takes too long

    // writes n gigabyte
    for (WUInt32 n = 0; n < 8; ++n)
    {
      // writes one gigabyte
      for (WUInt32 gb = 0; gb < 1024; ++gb)
      {
        // writes one megabyte
        for (WUInt32 mb = 0; mb < 1024 * 1024 / W_ARRAY_SIZE(pattern); ++mb)
        {
          writer.WriteBytes(pattern, W_ARRAY_SIZE(pattern)).IgnoreResult();
          uiSize += W_ARRAY_SIZE(pattern);

          if (uiSize == bytesToTest)
            goto check;
        }
      }
    }

  check:
    W_TEST_BOOL(uiSize == bytesToTest);
    W_TEST_BOOL(writer.GetWritePosition() == bytesToTest);
    uiSize = 0;

    // reads n gigabyte
    for (WUInt32 n = 0; n < 8; ++n)
    {
      // reads one gigabyte
      for (WUInt32 gb = 0; gb < 1024; ++gb)
      {
        // reads one megabyte
        for (WUInt32 mb = 0; mb < 1024 * 1024 / W_ARRAY_SIZE(pattern); ++mb)
        {
          WUInt8 pattern2[W_ARRAY_SIZE(pattern)];

          const WUInt64 uiRead = reader.ReadBytes(pattern2, W_ARRAY_SIZE(pattern));

          if (uiRead != W_ARRAY_SIZE(pattern))
          {
            W_TEST_BOOL(uiRead == 0);
            W_TEST_BOOL(uiSize == bytesToTest);
            goto endTest;
          }

          uiSize += uiRead;

          if (WMemoryUtils::RawByteCompare(pattern, pattern2, W_ARRAY_SIZE(pattern)) != 0)
          {
            W_TEST_BOOL_MSG(false, "Memory read comparison failed.");
            goto endTest;
          }
        }
      }
    }

  endTest:;
    W_TEST_BOOL(reader.GetReadPosition() == bytesToTest);
  }
}
