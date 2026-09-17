#pragma once

#include <Foundation/IO/Archive/Archive.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Types/Delegate.h>

/// Utility class to build an WArchive file from files/folders on disk
///
/// All functionality for writing an WArchive file is available through WArchiveUtils.
class W_FOUNDATION_DLL WArchiveBuilder
{
public:
  struct SourceEntry
  {
    WString m_sAbsSourcePath; ///< The source file to read
    WString m_sRelTargetPath; ///< Under which relative path to store it in the WArchive
    WArchiveCompressionMode m_CompressionMode = WArchiveCompressionMode::Uncompressed;
    WInt32 m_iCompressionLevel = 0;
  };

  // all the source files from disk that should be put into the WArchive
  WDeque<SourceEntry> m_Entries;

  enum class InclusionMode
  {
    Exclude,               ///< Do not add this file to the archive
    Uncompressed,          ///< Add the file to the archive, but do not even try to compress it
    Compress_zstd_fastest, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_fast,    ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_average, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_high,    ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
    Compress_zstd_highest, ///< Add the file and try out compression. If compression does not help, the file will end up uncompressed in the archive.
  };

  /// Custom decider whether to include a file into the archive
  using InclusionCallback = WDelegate<InclusionMode(WStringView)>;

  /// Iterates over all files in a folder and adds them to m_Entries for later.
  ///
  /// The callback can be used to exclude certain files or to deactivate compression on them.
  /// \note If no callback is given, the default is to store all files uncompressed!
  void AddFolder(WStringView sAbsFolderPath, WArchiveCompressionMode defaultMode = WArchiveCompressionMode::Uncompressed, InclusionCallback callback = InclusionCallback());

  /// Overwrites the given file with the archive
  WResult WriteArchive(WStringView sFile) const;

  /// Writes the previously gathered files to the file stream
  WResult WriteArchive(WStreamWriter& inout_stream) const;

protected:
  /// Override this to get a callback when the next file is being written to the output. Return 'true' to continue, 'false' to cancel the entire archive generation.
  virtual bool WriteNextFileCallback(WUInt32 uiCurEntry, WUInt32 uiMaxEntries, WStringView sSourceFile) const;

  /// Override this to get a progress report for writing a single file to the output
  virtual bool WriteFileProgressCallback(WUInt64 bytesWritten, WUInt64 bytesTotal) const;

  /// Override this to get a callback after a file has been processed. Gets additional information about the compression result and duration.
  virtual void WriteFileResultCallback(WUInt32 uiCurEntry, WUInt32 uiMaxEntries, WStringView sSourceFile, WUInt64 uiSourceSize, WUInt64 uiStoredSize, WTime duration) const
  {
    W_IGNORE_UNUSED(uiCurEntry);
    W_IGNORE_UNUSED(uiMaxEntries);
    W_IGNORE_UNUSED(sSourceFile);
    W_IGNORE_UNUSED(uiSourceSize);
    W_IGNORE_UNUSED(uiStoredSize);
    W_IGNORE_UNUSED(duration);
  }
};
