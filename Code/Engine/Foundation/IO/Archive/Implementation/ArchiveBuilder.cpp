#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

void WArchiveBuilder::AddFolder(WStringView sAbsFolderPath, WArchiveCompressionMode defaultMode /*= WArchiveCompressionMode::Uncompressed*/, InclusionCallback callback /*= InclusionCallback()*/)
{
#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)
  WFileSystemIterator fileIt;

  WStringBuilder sBasePath = sAbsFolderPath;
  sBasePath.MakeCleanPath();

  WStringBuilder fullPath;
  WStringBuilder relPath;

  for (fileIt.StartSearch(sBasePath, WFileSystemIteratorFlags::ReportFilesRecursive); fileIt.IsValid(); fileIt.Next())
  {
    const auto& stat = fileIt.GetStats();

    stat.GetFullPath(fullPath);
    relPath = fullPath;

    if (relPath.MakeRelativeTo(sBasePath).Succeeded())
    {
      WArchiveCompressionMode compression = defaultMode;
      WInt32 iCompressionLevel = 0;

      if (callback.IsValid())
      {
        switch (callback(fullPath))
        {
          case InclusionMode::Exclude:
            continue;

          case InclusionMode::Uncompressed:
            compression = WArchiveCompressionMode::Uncompressed;
            break;

#  ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
          case InclusionMode::Compress_zstd_fastest:
            compression = WArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<WInt32>(WCompressedStreamWriterZstd::Compression::Fastest);
            break;
          case InclusionMode::Compress_zstd_fast:
            compression = WArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<WInt32>(WCompressedStreamWriterZstd::Compression::Fast);
            break;
          case InclusionMode::Compress_zstd_average:
            compression = WArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<WInt32>(WCompressedStreamWriterZstd::Compression::Average);
            break;
          case InclusionMode::Compress_zstd_high:
            compression = WArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<WInt32>(WCompressedStreamWriterZstd::Compression::High);
            break;
          case InclusionMode::Compress_zstd_highest:
            compression = WArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<WInt32>(WCompressedStreamWriterZstd::Compression::Highest);
            break;
#  endif
        }
      }

      auto& e = m_Entries.ExpandAndGetRef();
      e.m_sAbsSourcePath = fullPath;
      e.m_sRelTargetPath = relPath;
      e.m_CompressionMode = compression;
      e.m_iCompressionLevel = iCompressionLevel;
    }
  }

#else
  W_IGNORE_UNUSED(sAbsFolderPath);
  W_IGNORE_UNUSED(defaultMode);
  W_IGNORE_UNUSED(callback);
  W_ASSERT_NOT_IMPLEMENTED;
#endif
}

WResult WArchiveBuilder::WriteArchive(WStringView sFile) const
{
  W_LOG_BLOCK("WriteArchive", sFile);

  WFileWriter file;
  if (file.Open(sFile, 1024 * 1024 * 16).Failed())
  {
    WLog::Error("Could not open file for writing archive to: '{}'", sFile);
    return W_FAILURE;
  }

  return WriteArchive(file);
}

WResult WArchiveBuilder::WriteArchive(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(WArchiveUtils::WriteHeader(inout_stream));

  WArchiveTOC toc;

  WStringBuilder sHashablePath;

  WUInt64 uiStreamSize = 0;
  const WUInt32 uiNumEntries = m_Entries.GetCount();

  WStopwatch sw;

  for (WUInt32 i = 0; i < uiNumEntries; ++i)
  {
    const SourceEntry& e = m_Entries[i];

    const WUInt32 uiPathStringOffset = toc.AddPathString(e.m_sRelTargetPath);

    sHashablePath = e.m_sRelTargetPath;
    sHashablePath.ToLower();

    toc.m_PathToEntryIndex[WArchiveStoredString(WHashingUtils::StringHash(sHashablePath), uiPathStringOffset)] = toc.m_Entries.GetCount();

    if (!WriteNextFileCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath))
      return W_FAILURE;

    WArchiveEntry& tocEntry = toc.m_Entries.ExpandAndGetRef();

    W_SUCCEED_OR_RETURN(WArchiveUtils::WriteEntryOptimal(inout_stream, e.m_sAbsSourcePath, uiPathStringOffset, e.m_CompressionMode, e.m_iCompressionLevel, tocEntry, uiStreamSize, WMakeDelegate(&WArchiveBuilder::WriteFileProgressCallback, this)));

    WriteFileResultCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath, tocEntry.m_uiUncompressedDataSize, tocEntry.m_uiStoredDataSize, sw.Checkpoint());
  }

  W_SUCCEED_OR_RETURN(WArchiveUtils::AppendTOC(inout_stream, toc));

  return W_SUCCESS;
}

bool WArchiveBuilder::WriteNextFileCallback(WUInt32 uiCurEntry, WUInt32 uiMaxEntries, WStringView sSourceFile) const
{
  W_IGNORE_UNUSED(uiCurEntry);
  W_IGNORE_UNUSED(uiMaxEntries);
  W_IGNORE_UNUSED(sSourceFile);
  return true;
}

bool WArchiveBuilder::WriteFileProgressCallback(WUInt64 bytesWritten, WUInt64 bytesTotal) const
{
  W_IGNORE_UNUSED(bytesWritten);
  W_IGNORE_UNUSED(bytesTotal);
  return true;
}
