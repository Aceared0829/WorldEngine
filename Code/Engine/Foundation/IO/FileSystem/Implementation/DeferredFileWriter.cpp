#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

WDeferredFileWriter::WDeferredFileWriter()
  : m_Writer(&m_Storage)
{
}

void WDeferredFileWriter::SetOutput(WStringView sFileToWriteTo, bool bOnlyWriteIfDifferent)
{
  m_bOnlyWriteIfDifferent = bOnlyWriteIfDifferent;
  m_sOutputFile = sFileToWriteTo;
}

WResult WDeferredFileWriter::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  W_ASSERT_DEBUG(!m_sOutputFile.IsEmpty(), "Output file has not been configured");

  return m_Writer.WriteBytes(pWriteBuffer, uiBytesToWrite);
}

WResult WDeferredFileWriter::Close(bool* out_pWasWrittenTo /*= nullptr*/)
{
  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = false;
  }

  if (m_bAlreadyClosed)
    return W_SUCCESS;

  if (m_sOutputFile.IsEmpty())
    return W_FAILURE;

  m_bAlreadyClosed = true;

  if (m_bOnlyWriteIfDifferent)
  {
    WFileReader fileIn;
    if (fileIn.Open(m_sOutputFile).Succeeded() && fileIn.GetFileSize() == m_Storage.GetStorageSize64())
    {
      WUInt8 tmp1[1024 * 4];
      WUInt8 tmp2[1024 * 4];

      WMemoryStreamReader storageReader(&m_Storage);

      while (true)
      {
        const WUInt64 readBytes1 = fileIn.ReadBytes(tmp1, W_ARRAY_SIZE(tmp1));
        const WUInt64 readBytes2 = storageReader.ReadBytes(tmp2, W_ARRAY_SIZE(tmp2));

        if (readBytes1 != readBytes2)
          goto write_data;

        if (readBytes1 == 0)
          break;

        if (WMemoryUtils::RawByteCompare(tmp1, tmp2, WMath::SafeConvertToSizeT(readBytes1)) != 0)
          goto write_data;
      }

      // content is already the same as what we would write -> skip the write (do not modify file write date)
      return W_SUCCESS;
    }
  }

write_data:
  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(m_sOutputFile, 0)); // use the minimum cache size, we want to pass data directly through to disk

  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = true;
  }

  m_sOutputFile.Clear();
  return m_Storage.CopyToStream(file);
}

void WDeferredFileWriter::Discard()
{
  m_sOutputFile.Clear();
}
