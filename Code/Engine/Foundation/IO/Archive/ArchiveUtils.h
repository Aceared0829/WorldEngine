#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class WStreamReader;
class WStreamWriter;
class WMemoryMappedFile;
class WArchiveTOC;
class WArchiveEntry;
class WRawMemoryStreamReader;

/// Utilities for working with WArchive files
namespace WArchiveUtils
{
  using FileWriteProgressCallback = WDelegate<bool(WUInt64, WUInt64)>;
  constexpr WUInt32 ArchiveHeaderSize = 16;
  constexpr WUInt32 ArchiveTOCMetaMaxFooterSize = 14 + 12; //< note that it's the MAX size, i.e. toc meta can be smaller

  struct TOCMeta
  {
    WUInt32 m_uiTocSize = 0;
    WUInt64 m_uiExpectedTocHash = 0;
    WUInt32 m_uiTocOffsetFromArchiveEnd = 0;
  };

  /// Returns a modifiable array of file extensions that the engine considers to be valid WArchive file extensions.
  ///
  /// By default it always contains 'WArchive'.
  /// Add or overwrite the values, if you want custom file extensions to be handled as WArchives.
  W_FOUNDATION_DLL WHybridArray<WString, 4, WStaticsAllocatorWrapper>& GetAcceptedArchiveFileExtensions();

  /// Checks case insensitive, whether the given extension is in the list of GetAcceptedArchiveFileExtensions().
  W_FOUNDATION_DLL bool IsAcceptedArchiveFileExtensions(WStringView sExtension);

  /// Writes the header that identifies the WArchive file and version to the stream
  W_FOUNDATION_DLL WResult WriteHeader(WStreamWriter& inout_stream);

  /// Reads the WArchive header. Returns success and the version, if the stream is a valid WArchive file.
  W_FOUNDATION_DLL WResult ReadHeader(WStreamReader& inout_stream, WUInt8& out_uiVersion);

  /// Writes the archive TOC to the stream. This must be the last thing in the stream, if ExtractTOC() is supposed to work.
  W_FOUNDATION_DLL WResult AppendTOC(WStreamWriter& inout_stream, const WArchiveTOC& toc);

  /// Deserializes the TOC meta from archive ending. Assumes the TOC is the very last data in the file.
  W_FOUNDATION_DLL WResult ExtractTOCMeta(WUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, TOCMeta& ref_tocMeta, WUInt8 uiArchiveVersion);

  /// Deserializes the TOC meta from the memory mapped file. Assumes the TOC is the very last data in the file.
  W_FOUNDATION_DLL WResult ExtractTOCMeta(const WMemoryMappedFile& memFile, TOCMeta& ref_tocMeta, WUInt8 uiArchiveVersion);

  /// Deserializes the TOC from from archive ending. Assumes the TOC is the very last data in the file and reads it from the back.
  W_FOUNDATION_DLL WResult ExtractTOC(WUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, WArchiveTOC& ref_toc, WUInt8 uiArchiveVersion);

  /// Deserializes the TOC from the memory mapped file. Assumes the TOC is the very last data in the file and reads it from the back.
  W_FOUNDATION_DLL WResult ExtractTOC(const WMemoryMappedFile& memFile, WArchiveTOC& ref_toc, WUInt8 uiArchiveVersion);

  /// Writes a single file entry to an WArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. The progress callback is executed for every couple of KB of data that were written.
  W_FOUNDATION_DLL WResult WriteEntry(WStreamWriter& inout_stream, WStringView sAbsSourcePath, WUInt32 uiPathStringOffset,
    WArchiveCompressionMode compression, WInt32 iCompressionLevel, WArchiveEntry& ref_tocEntry, WUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// Writes a single file entry to an WArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. Compression parameter indicate compression that the entry data already have applied.
  W_FOUNDATION_DLL WResult WriteEntryPreprocessed(WStreamWriter& inout_stream, WConstByteArrayPtr entryData, WUInt32 uiPathStringOffset,
    WArchiveCompressionMode compression, WUInt32 uiUncompressedEntryDataSize, WArchiveEntry& ref_tocEntry, WUInt64& inout_uiCurrentStreamPosition);

  /// Similar to WriteEntry, but if compression is enabled, checks that compression makes enough of a difference.
  /// If compression does not reduce file size enough, the file is stored uncompressed instead.
  W_FOUNDATION_DLL WResult WriteEntryOptimal(WStreamWriter& inout_stream, WStringView sAbsSourcePath, WUInt32 uiPathStringOffset,
    WArchiveCompressionMode compression, WInt32 iCompressionLevel, WArchiveEntry& ref_tocEntry, WUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// Configures \a memReader as a view into the data stored for \a entry in the archive file.
  ///
  /// The raw memory stream may be compressed or uncompressed. This only creates a view for the stored data, it does not interpret it.
  W_FOUNDATION_DLL void ConfigureRawMemoryStreamReader(
    const WArchiveEntry& entry, const void* pStartOfArchiveData, WRawMemoryStreamReader& ref_memReader);

  /// Creates a new stream reader which allows to read the uncompressed data for the given archive entry.
  ///
  /// Under the hood it may create different types of stream readers to uncompress or decode the data.
  W_FOUNDATION_DLL WUniquePtr<WStreamReader> CreateEntryReader(const WArchiveEntry& entry, const void* pStartOfArchiveData);

  W_FOUNDATION_DLL WResult ReadZipHeader(WStreamReader& inout_stream, WUInt8& out_uiVersion);
  W_FOUNDATION_DLL WResult ExtractZipTOC(const WMemoryMappedFile& memFile, WArchiveTOC& ref_toc);


} // namespace WArchiveUtils
