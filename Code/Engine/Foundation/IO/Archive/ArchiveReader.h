#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Types/UniquePtr.h>

class WRawMemoryStreamReader;
class WStreamReader;

/// A utility class for reading from WArchive files
class W_FOUNDATION_DLL WArchiveReader
{
public:
  /// Opens the given file and validates that it is a valid archive file.
  WResult OpenArchive(WStringView sPath);

  /// Returns the table-of-contents for the previously opened archive.
  const WArchiveTOC& GetArchiveTOC();

  /// Extracts the given entry to the target folder.
  ///
  /// Calls ExtractFileProgressCallback() to report progress.
  WResult ExtractFile(WUInt32 uiEntryIdx, WStringView sTargetFolder) const;

  /// Extracts all files to the target folder.
  ///
  /// Calls ExtractNextFileCallback() for every file that is being extracted.
  WResult ExtractAllFiles(WStringView sTargetFolder) const;

  /// Sets up \a memReader for reading the raw (potentially compressed) data that is stored for the given entry in the archive.
  void ConfigureRawMemoryStreamReader(WUInt32 uiEntryIdx, WRawMemoryStreamReader& ref_memReader) const;

  /// Creates a reader that will decompress the given file entry.
  WUniquePtr<WStreamReader> CreateEntryReader(WUInt32 uiEntryIdx) const;

protected:
  /// Called by ExtractAllFiles() for progress reporting. Return false to abort.
  virtual bool ExtractNextFileCallback(WUInt32 uiCurEntry, WUInt32 uiMaxEntries, WStringView sSourceFile) const;

  /// Called by ExtractFile() for progress reporting. Return false to abort.
  virtual bool ExtractFileProgressCallback(WUInt64 bytesWritten, WUInt64 bytesTotal) const;

  WMemoryMappedFile m_MemFile;
  WArchiveTOC m_ArchiveTOC;
  WUInt8 m_uiArchiveVersion = 0;
  const void* m_pDataStart = nullptr;
  WUInt64 m_uiMemFileSize = 0;
};
