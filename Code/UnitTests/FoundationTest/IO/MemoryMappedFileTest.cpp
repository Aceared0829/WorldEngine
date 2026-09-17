#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/OSFile.h>

#if W_ENABLED(W_SUPPORTS_MEMORY_MAPPED_FILE)

W_CREATE_SIMPLE_TEST(IO, MemoryMappedFile)
{
  WStringBuilder sOutputFile = WTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO");
  sOutputFile.AppendPath("MemoryMappedFile.dat");

  const WUInt32 uiFileSize = 1024 * 1024 * 16; // * 4

  // generate test data
  {
    WOSFile file;
    if (!W_TEST_BOOL_MSG(file.Open(sOutputFile, WFileOpenMode::Write).Succeeded(), "File for memory mapping could not be created"))
      return;

    WDynamicArray<WUInt32> data;
    data.SetCountUninitialized(uiFileSize);

    for (WUInt32 i = 0; i < uiFileSize; ++i)
    {
      data[i] = i;
    }

    file.Write(data.GetData(), data.GetCount() * sizeof(WUInt32)).IgnoreResult();
    file.Close();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Memory map for writing")
  {
    WMemoryMappedFile memFile;

    if (!W_TEST_BOOL_MSG(memFile.Open(sOutputFile, WMemoryMappedFile::Mode::ReadWrite).Succeeded(), "Memory mapping a file failed"))
      return;

    W_TEST_BOOL(memFile.GetWritePointer() != nullptr);
    W_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(WUInt32));

    WUInt32* ptr = static_cast<WUInt32*>(memFile.GetWritePointer());

    for (WUInt32 i = 0; i < uiFileSize; ++i)
    {
      W_TEST_INT(ptr[i], i);
      ptr[i] = ptr[i] + 1;
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Memory map for reading")
  {
    WMemoryMappedFile memFile;

    if (!W_TEST_BOOL_MSG(memFile.Open(sOutputFile, WMemoryMappedFile::Mode::ReadOnly).Succeeded(), "Memory mapping a file failed"))
      return;

    W_TEST_BOOL(memFile.GetReadPointer() != nullptr);
    W_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(WUInt32));

    const WUInt32* ptr = static_cast<const WUInt32*>(memFile.GetReadPointer());

    for (WUInt32 i = 0; i < uiFileSize; ++i)
    {
      W_TEST_INT(ptr[i], i + 1);
    }

    // try to map it a second time
    WMemoryMappedFile memFile2;

    if (!W_TEST_BOOL_MSG(memFile2.Open(sOutputFile, WMemoryMappedFile::Mode::ReadOnly).Succeeded(), "Memory mapping a file twice failed"))
      return;
  }

  WOSFile::DeleteFile(sOutputFile).IgnoreResult();
}
#endif
