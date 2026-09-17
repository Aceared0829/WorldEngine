#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Logging/Log.h>

WResult WArchiveReader::OpenArchive(WStringView sPath)
{
#if W_ENABLED(W_SUPPORTS_MEMORY_MAPPED_FILE)
  W_LOG_BLOCK("OpenArchive", sPath);

  W_SUCCEED_OR_RETURN(m_MemFile.Open(sPath, WMemoryMappedFile::Mode::ReadOnly));
  m_uiMemFileSize = m_MemFile.GetFileSize();

  // validate the archive
  {
    WRawMemoryStreamReader reader(m_MemFile.GetReadPointer(), m_MemFile.GetFileSize());

    WStringView extension = WPathUtils::GetFileExtension(sPath);

    if (WArchiveUtils::IsAcceptedArchiveFileExtensions(extension))
    {
      W_SUCCEED_OR_RETURN(WArchiveUtils::ReadHeader(reader, m_uiArchiveVersion));

      m_pDataStart = m_MemFile.GetReadPointer(WArchiveUtils::ArchiveHeaderSize, WMemoryMappedFile::OffsetBase::Start);

      W_SUCCEED_OR_RETURN(WArchiveUtils::ExtractTOC(m_MemFile, m_ArchiveTOC, m_uiArchiveVersion));
    }
#  ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
    else if (extension == "zip" || extension == "apk")
    {
      W_SUCCEED_OR_RETURN(WArchiveUtils::ReadZipHeader(reader, m_uiArchiveVersion));
      if (m_uiArchiveVersion != 0)
      {
        WLog::Error("Unknown zip version '{}'", m_uiArchiveVersion);
        return W_FAILURE;
      }
      m_pDataStart = m_MemFile.GetReadPointer(0, WMemoryMappedFile::OffsetBase::Start);

      if (WArchiveUtils::ExtractZipTOC(m_MemFile, m_ArchiveTOC).Failed())
      {
        WLog::Error("Failed to deserialize zip TOC");
        return W_FAILURE;
      }
    }
#  endif
    else
    {
      WLog::Error("Unknown archive file extension '{}'", extension);
      return W_FAILURE;
    }
  }

  // validate the entries
  {
    const WUInt32 uiMaxPathString = m_ArchiveTOC.m_AllPathStrings.GetCount();
    const WUInt64 uiValidSize = m_uiMemFileSize - uiMaxPathString;

    for (const auto& e : m_ArchiveTOC.m_Entries)
    {
      if (e.m_uiDataStartOffset + e.m_uiStoredDataSize > uiValidSize)
      {
        WLog::Error("Archive is corrupt. Invalid entry data range.");
        return W_FAILURE;
      }

      if (e.m_uiUncompressedDataSize < e.m_uiStoredDataSize)
      {
        WLog::Error("Archive is corrupt. Invalid compression info.");
        return W_FAILURE;
      }

      if (e.m_uiPathStringOffset >= uiMaxPathString)
      {
        WLog::Error("Archive is corrupt. Invalid entry path-string offset.");
        return W_FAILURE;
      }
    }
  }

  return W_SUCCESS;
#else
  W_IGNORE_UNUSED(sPath);
  W_REPORT_FAILURE("Memory mapped files are unsupported on this platform.");
  return W_FAILURE;
#endif
}

const WArchiveTOC& WArchiveReader::GetArchiveTOC()
{
  return m_ArchiveTOC;
}

WResult WArchiveReader::ExtractAllFiles(WStringView sTargetFolder) const
{
  W_LOG_BLOCK("ExtractAllFiles", sTargetFolder);

  const WUInt32 numEntries = m_ArchiveTOC.m_Entries.GetCount();

  for (WUInt32 e = 0; e < numEntries; ++e)
  {
    const char* szPath = reinterpret_cast<const char*>(&m_ArchiveTOC.m_AllPathStrings[m_ArchiveTOC.m_Entries[e].m_uiPathStringOffset]);

    if (!ExtractNextFileCallback(e + 1, numEntries, szPath))
      return W_FAILURE;

    W_SUCCEED_OR_RETURN(ExtractFile(e, sTargetFolder));
  }

  return W_SUCCESS;
}

void WArchiveReader::ConfigureRawMemoryStreamReader(WUInt32 uiEntryIdx, WRawMemoryStreamReader& ref_memReader) const
{
  WArchiveUtils::ConfigureRawMemoryStreamReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart, ref_memReader);
}

WUniquePtr<WStreamReader> WArchiveReader::CreateEntryReader(WUInt32 uiEntryIdx) const
{
  return WArchiveUtils::CreateEntryReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart);
}

WResult WArchiveReader::ExtractFile(WUInt32 uiEntryIdx, WStringView sTargetFolder) const
{
  WStringView sFilePath = m_ArchiveTOC.GetEntryPathString(uiEntryIdx);
  const WUInt64 uiMaxSize = m_ArchiveTOC.m_Entries[uiEntryIdx].m_uiUncompressedDataSize;

  WUniquePtr<WStreamReader> pReader = CreateEntryReader(uiEntryIdx);

  WStringBuilder sOutputFile = sTargetFolder;
  sOutputFile.AppendPath(sFilePath);

  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sOutputFile));

  WUInt8 uiTemp[1024 * 8];

  WUInt64 uiRead = 0;
  WUInt64 uiReadTotal = 0;
  while (true)
  {
    uiRead = pReader->ReadBytes(uiTemp, W_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    W_SUCCEED_OR_RETURN(file.WriteBytes(uiTemp, uiRead));

    uiReadTotal += uiRead;

    if (!ExtractFileProgressCallback(uiReadTotal, uiMaxSize))
      return W_FAILURE;
  }

  W_ASSERT_DEV(uiReadTotal == uiMaxSize, "Failed to read entire file");

  return W_SUCCESS;
}

bool WArchiveReader::ExtractNextFileCallback(WUInt32 uiCurEntry, WUInt32 uiMaxEntries, WStringView sSourceFile) const
{
  W_IGNORE_UNUSED(uiCurEntry);
  W_IGNORE_UNUSED(uiMaxEntries);
  W_IGNORE_UNUSED(sSourceFile);
  return true;
}

bool WArchiveReader::ExtractFileProgressCallback(WUInt64 bytesWritten, WUInt64 bytesTotal) const
{
  W_IGNORE_UNUSED(bytesWritten);
  W_IGNORE_UNUSED(bytesTotal);
  return true;
}
