#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

WHybridArray<WString, 4, WStaticsAllocatorWrapper>& WArchiveUtils::GetAcceptedArchiveFileExtensions()
{
  static WHybridArray<WString, 4, WStaticsAllocatorWrapper> extensions;

  if (extensions.IsEmpty())
  {
    extensions.PushBack("WArchive");
  }

  return extensions;
}

bool WArchiveUtils::IsAcceptedArchiveFileExtensions(WStringView sExtension)
{
  for (const auto& ext : GetAcceptedArchiveFileExtensions())
  {
    if (sExtension.IsEqual_NoCase(ext.GetView()))
      return true;
  }

  return false;
}

WResult WArchiveUtils::WriteHeader(WStreamWriter& inout_stream)
{
  static_assert(16 == ArchiveHeaderSize);

  const char* szTag = "WEARCHIVE";
  W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(szTag, 10));

  const WUInt8 uiArchiveVersion = 4;

  // Version 2: Added end-of-file marker for file corruption (cutoff) detection
  // Version 3: HashedStrings changed from MurmurHash to xxHash
  // Version 4: use 64 Bit string hashes
  inout_stream << uiArchiveVersion;

  const WUInt8 uiPadding[5] = {0, 0, 0, 0, 0};
  W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(uiPadding, 5));

  return W_SUCCESS;
}

WResult WArchiveUtils::ReadHeader(WStreamReader& inout_stream, WUInt8& out_uiVersion)
{
  static_assert(16 == ArchiveHeaderSize);

  char szTag[10];
  if (inout_stream.ReadBytes(szTag, 10) != 10 || !WStringUtils::IsEqual(szTag, "WEARCHIVE"))
  {
    WLog::Error("Invalid or corrupted archive. Archive-marker not found.");
    return W_FAILURE;
  }

  out_uiVersion = 0;
  inout_stream >> out_uiVersion;

  if (out_uiVersion != 1 && out_uiVersion != 2 && out_uiVersion != 3 && out_uiVersion != 4)
  {
    WLog::Error("Unsupported archive version '{}'.", out_uiVersion);
    return W_FAILURE;
  }

  WUInt8 uiPadding[5] = {255, 255, 255, 255, 255};
  if (inout_stream.ReadBytes(uiPadding, 5) != 5)
  {
    WLog::Error("Invalid or corrupted archive. Missing header data.");
    return W_FAILURE;
  }

  const WUInt8 uiZeroPadding[5] = {0, 0, 0, 0, 0};

  if (WMemoryUtils::Compare<WUInt8>(uiPadding, uiZeroPadding, 5) != 0)
  {
    WLog::Error("Invalid or corrupted archive. Unexpected header data.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WArchiveUtils::WriteEntryPreprocessed(WStreamWriter& inout_stream, WConstByteArrayPtr entryData, WUInt32 uiPathStringOffset, WArchiveCompressionMode compression, WUInt32 uiUncompressedEntryDataSize, WArchiveEntry& ref_tocEntry, WUInt64& inout_uiCurrentStreamPosition)
{
  W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(entryData.GetPtr(), entryData.GetCount()));

  ref_tocEntry.m_uiPathStringOffset = uiPathStringOffset;
  ref_tocEntry.m_uiDataStartOffset = inout_uiCurrentStreamPosition;
  ref_tocEntry.m_uiUncompressedDataSize = uiUncompressedEntryDataSize;
  ref_tocEntry.m_uiStoredDataSize = entryData.GetCount();
  ref_tocEntry.m_CompressionMode = compression;

  inout_uiCurrentStreamPosition += entryData.GetCount();

  return W_SUCCESS;
}

WResult WArchiveUtils::WriteEntry(
  WStreamWriter& inout_stream, WStringView sAbsSourcePath, WUInt32 uiPathStringOffset, WArchiveCompressionMode compression,
  WInt32 iCompressionLevel, WArchiveEntry& inout_tocEntry, WUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  W_IGNORE_UNUSED(iCompressionLevel);

  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(sAbsSourcePath, 1024 * 1024));

  const WUInt64 uiMaxBytes = file.GetFileSize();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  constexpr WUInt32 uiMaxNumWorkerThreads = 12u;

  WUInt32 uiWorkerThreadCount;
  if (uiMaxBytes > WMath::MaxValue<WUInt32>())
  {
    uiWorkerThreadCount = uiMaxNumWorkerThreads;
  }
  else
  {
    constexpr WUInt32 uiBytesPerThread = 1024u * 1024u;
    uiWorkerThreadCount = WMath::Clamp((WUInt32)floor(uiMaxBytes / uiBytesPerThread), 1u, uiMaxNumWorkerThreads);
  }
#endif

  inout_tocEntry.m_uiPathStringOffset = uiPathStringOffset;
  inout_tocEntry.m_uiDataStartOffset = inout_uiCurrentStreamPosition;
  inout_tocEntry.m_uiUncompressedDataSize = 0;

  WStreamWriter* pWriter = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  WCompressedStreamWriterZstd zstdWriter;
#endif

  switch (compression)
  {
    case WArchiveCompressionMode::Uncompressed:
      break;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case WArchiveCompressionMode::Compressed_zstd:
    {
      zstdWriter.SetOutputStream(&inout_stream, uiWorkerThreadCount, (WCompressedStreamWriterZstd::Compression)iCompressionLevel);
      pWriter = &zstdWriter;
    }
    break;
#endif

    default:
      compression = WArchiveCompressionMode::Uncompressed;
      break;
  }

  inout_tocEntry.m_CompressionMode = compression;

  WUInt64 uiRead = 0;
  WDynamicArray<WUInt8> buf;
  buf.SetCountUninitialized(1024 * 32);
  while (true)
  {
    uiRead = file.ReadBytes(buf.GetData(), buf.GetCount());

    if (uiRead == 0)
      break;

    inout_tocEntry.m_uiUncompressedDataSize += uiRead;

    if (progress.IsValid())
    {
      if (!progress(inout_tocEntry.m_uiUncompressedDataSize, uiMaxBytes))
        return W_FAILURE;
    }

    W_SUCCEED_OR_RETURN(pWriter->WriteBytes(buf.GetData(), uiRead));
  }


  switch (compression)
  {
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case WArchiveCompressionMode::Compressed_zstd:
      W_SUCCEED_OR_RETURN(zstdWriter.FinishCompressedStream());
      inout_tocEntry.m_uiStoredDataSize = zstdWriter.GetWrittenBytes();
      break;
#endif

    case WArchiveCompressionMode::Uncompressed:
    default:
      inout_tocEntry.m_uiStoredDataSize = inout_tocEntry.m_uiUncompressedDataSize;
      break;
  }

  inout_uiCurrentStreamPosition += inout_tocEntry.m_uiStoredDataSize;

  return W_SUCCESS;
}

WResult WArchiveUtils::WriteEntryOptimal(WStreamWriter& inout_stream, WStringView sAbsSourcePath, WUInt32 uiPathStringOffset, WArchiveCompressionMode compression, WInt32 iCompressionLevel, WArchiveEntry& ref_tocEntry, WUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  if (compression == WArchiveCompressionMode::Uncompressed)
  {
    return WriteEntry(inout_stream, sAbsSourcePath, uiPathStringOffset, WArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
  }
  else
  {
    WDefaultMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);

    WUInt64 streamPos = inout_uiCurrentStreamPosition;
    W_SUCCEED_OR_RETURN(WriteEntry(writer, sAbsSourcePath, uiPathStringOffset, compression, iCompressionLevel, ref_tocEntry, streamPos, progress));

    if (ref_tocEntry.m_uiStoredDataSize * 12 >= ref_tocEntry.m_uiUncompressedDataSize * 10)
    {
      // less than 20% size saving -> go uncompressed
      return WriteEntry(inout_stream, sAbsSourcePath, uiPathStringOffset, WArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
    }
    else
    {
      auto res = storage.CopyToStream(inout_stream);
      inout_uiCurrentStreamPosition = streamPos;

      return res;
    }
  }
}

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

class WCompressedStreamReaderZstdWithSource : public WCompressedStreamReaderZstd
{
public:
  WRawMemoryStreamReader m_Source;
};

#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

class WCompressedStreamReaderZipWithSource : public WCompressedStreamReaderZip
{
public:
  WRawMemoryStreamReader m_Source;
};

#endif

WUniquePtr<WStreamReader> WArchiveUtils::CreateEntryReader(const WArchiveEntry& entry, const void* pStartOfArchiveData)
{
  WUniquePtr<WStreamReader> reader;

  switch (entry.m_CompressionMode)
  {
    case WArchiveCompressionMode::Uncompressed:
    {
      reader = W_DEFAULT_NEW(WRawMemoryStreamReader);
      WRawMemoryStreamReader* pRawReader = static_cast<WRawMemoryStreamReader*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, *pRawReader);
      break;
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case WArchiveCompressionMode::Compressed_zstd:
    {
      reader = W_DEFAULT_NEW(WCompressedStreamReaderZstdWithSource);
      WCompressedStreamReaderZstdWithSource* pRawReader = static_cast<WCompressedStreamReaderZstdWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source);
      break;
    }
#endif
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
    case WArchiveCompressionMode::Compressed_zip:
    {
      reader = W_DEFAULT_NEW(WCompressedStreamReaderZipWithSource);
      WCompressedStreamReaderZipWithSource* pRawReader = static_cast<WCompressedStreamReaderZipWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source, entry.m_uiStoredDataSize);
      break;
    }
#endif

    default:
      W_REPORT_FAILURE("Archive entry compression mode '{}' is not supported by WArchiveReader", (int)entry.m_CompressionMode);
      break;
  }

  return std::move(reader);
}

void WArchiveUtils::ConfigureRawMemoryStreamReader(const WArchiveEntry& entry, const void* pStartOfArchiveData, WRawMemoryStreamReader& ref_memReader)
{
  ref_memReader.Reset(WMemoryUtils::AddByteOffset(pStartOfArchiveData, static_cast<std::ptrdiff_t>(entry.m_uiDataStartOffset)), entry.m_uiStoredDataSize);
}

static const char* szEndMarker = "WEARCHIVE-END";

static WUInt32 GetEndMarkerSize(WUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return 0;

  return 14;
}

static WUInt32 GetTocMetaSize(WUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return sizeof(WUInt32); // TOC size

  return sizeof(WUInt32) /* TOC size */ + sizeof(WUInt64) /* TOC hash */;
}

struct TocMetaData
{
  WUInt32 m_uiSize = 0;
  WUInt64 m_uiHash = 0;
};

WResult WArchiveUtils::AppendTOC(WStreamWriter& inout_stream, const WArchiveTOC& toc)
{
  WDefaultMemoryStreamStorage storage;
  WMemoryStreamWriter writer(&storage);

  W_SUCCEED_OR_RETURN(toc.Serialize(writer));

  W_SUCCEED_OR_RETURN(storage.CopyToStream(inout_stream));

  TocMetaData tocMeta;

  WHashStreamWriter64 hashStream(tocMeta.m_uiSize);
  W_SUCCEED_OR_RETURN(storage.CopyToStream(hashStream));

  // Added in file version 2: hash of the TOC
  tocMeta.m_uiSize = storage.GetStorageSize32();
  tocMeta.m_uiHash = hashStream.GetHashValue();

  // append the TOC meta data
  inout_stream << tocMeta.m_uiSize;
  inout_stream << tocMeta.m_uiHash;

  // write an 'end' marker
  return inout_stream.WriteBytes(szEndMarker, 14);
}

static WResult VerifyEndMarker(WUInt64 uiArchiveDataSize, const void* pArchiveDataBuffer, WUInt8 uiArchiveVersion)
{
  const WUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);

  if (uiEndMarkerSize == 0)
  {
    return W_SUCCESS;
  }

  if (uiEndMarkerSize > uiArchiveDataSize)
  {
    WLog::Error("Archive is too small. End-marker not found.");
    return W_FAILURE;
  }

  const void* pStart = WMemoryUtils::AddByteOffset(pArchiveDataBuffer, static_cast<ptrdiff_t>(uiArchiveDataSize - uiEndMarkerSize));

  WRawMemoryStreamReader reader(pStart, uiEndMarkerSize);

  char szMarker[32] = "";
  if (reader.ReadBytes(szMarker, uiEndMarkerSize) != uiEndMarkerSize || !WStringUtils::IsEqual(szMarker, szEndMarker))
  {
    WLog::Error("Archive is corrupt or cut off. End-marker not found.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WArchiveUtils::ExtractTOCMeta(WUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, TOCMeta& ref_tocMeta, WUInt8 uiArchiveVersion)
{
  W_SUCCEED_OR_RETURN(VerifyEndMarker(uiArchiveEndingDataSize, pArchiveEndingDataBuffer, uiArchiveVersion));

  const WUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);
  const WUInt32 uiTocMetaSize = GetTocMetaSize(uiArchiveVersion);

  WUInt32 uiTocSize = 0;
  WUInt64 uiExpectedTocHash = 0;

  // read the TOC meta data
  {
    W_ASSERT_DEV(uiEndMarkerSize + uiTocMetaSize <= ArchiveTOCMetaMaxFooterSize, "");

    if (uiEndMarkerSize + uiTocMetaSize > uiArchiveEndingDataSize)
    {
      WLog::Error("Unable to extract Archive TOC. File size too small: {0}", WArgFileSize(uiArchiveEndingDataSize));
      return W_FAILURE;
    }

    const void* pTocMetaStart = WMemoryUtils::AddByteOffset(pArchiveEndingDataBuffer, static_cast<ptrdiff_t>(uiArchiveEndingDataSize - uiEndMarkerSize - uiTocMetaSize));

    WRawMemoryStreamReader tocMetaReader(pTocMetaStart, uiTocMetaSize);

    tocMetaReader >> uiTocSize;

    if (uiTocSize > 1024 * 1024 * 1024) // 1GB of TOC is enough for ~16M entries...
    {
      WLog::Error("Archive TOC is probably corrupted. Unreasonable TOC size: {0}", WArgFileSize(uiTocSize));
      return W_FAILURE;
    }

    if (uiArchiveVersion >= 2)
    {
      tocMetaReader >> uiExpectedTocHash;
    }
  }

  // output the result
  {
    ref_tocMeta = TOCMeta();
    ref_tocMeta.m_uiTocSize = uiTocSize;
    ref_tocMeta.m_uiExpectedTocHash = uiExpectedTocHash;
    ref_tocMeta.m_uiTocOffsetFromArchiveEnd = uiTocSize + uiTocMetaSize + uiEndMarkerSize;
  }

  return W_SUCCESS;
}

WResult WArchiveUtils::ExtractTOCMeta(const WMemoryMappedFile& memFile, TOCMeta& ref_tocMeta, WUInt8 uiArchiveVersion)
{
  return ExtractTOCMeta(memFile.GetFileSize(), memFile.GetReadPointer(), ref_tocMeta, uiArchiveVersion);
}

WResult WArchiveUtils::ExtractTOC(WUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, WArchiveTOC& ref_toc, WUInt8 uiArchiveVersion)
{
  // get toc meta
  TOCMeta tocMeta;
  if (ExtractTOCMeta(uiArchiveEndingDataSize, pArchiveEndingDataBuffer, tocMeta, uiArchiveVersion).Failed())
  {
    return W_FAILURE;
  }

  // verify meta is valid
  if (tocMeta.m_uiTocOffsetFromArchiveEnd > uiArchiveEndingDataSize)
  {
    WLog::Error("Archive TOC offset is corrupted.");
    return W_FAILURE;
  }

  // get toc data ptr
  const void* pTocStart = WMemoryUtils::AddByteOffset(pArchiveEndingDataBuffer, static_cast<ptrdiff_t>(uiArchiveEndingDataSize - tocMeta.m_uiTocOffsetFromArchiveEnd));

  // validate the TOC hash
  if (uiArchiveVersion >= 2)
  {
    const WUInt64 uiActualTocHash = WHashingUtils::xxHash64(pTocStart, tocMeta.m_uiTocSize);
    if (tocMeta.m_uiExpectedTocHash != uiActualTocHash)
    {
      WLog::Error("Archive TOC is corrupted. Hashes do not match.");
      return W_FAILURE;
    }
  }

  // read the actual TOC data
  {
    WRawMemoryStreamReader tocReader(pTocStart, tocMeta.m_uiTocSize);

    if (ref_toc.Deserialize(tocReader, uiArchiveVersion).Failed())
    {
      WLog::Error("Failed to deserialize WArchive TOC");
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

WResult WArchiveUtils::ExtractTOC(const WMemoryMappedFile& memFile, WArchiveTOC& ref_toc, WUInt8 uiArchiveVersion)
{
  return ExtractTOC(memFile.GetFileSize(), memFile.GetReadPointer(), ref_toc, uiArchiveVersion);
}

namespace ZipFormat
{
  constexpr WUInt32 EndOfCDMagicSignature = 0x06054b50;
  constexpr WUInt32 EndOfCDHeaderLength = 22;

  constexpr WUInt32 MaxCommentLength = 65535;
  constexpr WUInt64 MaxEndOfCDSearchLength = MaxCommentLength + EndOfCDHeaderLength;

  constexpr WUInt32 LocalFileMagicSignature = 0x04034b50;
  constexpr WUInt32 LocalFileHeaderLength = 30;

  constexpr WUInt32 CDFileMagicSignature = 0x02014b50;
  constexpr WUInt32 CDFileHeaderLength = 46;

  enum CompressionType
  {
    Uncompressed = 0,
    Deflate = 8,

  };

  struct EndOfCDHeader
  {
    WUInt32 signature;
    WUInt16 diskNumber;
    WUInt16 diskWithCD;
    WUInt16 diskEntries;
    WUInt16 totalEntries;
    WUInt32 cdSize;
    WUInt32 cdOffset;
    WUInt16 commentLength;
  };

  WStreamReader& operator>>(WStreamReader& inout_stream, EndOfCDHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.diskNumber >> ref_value.diskWithCD >> ref_value.diskEntries >> ref_value.totalEntries >> ref_value.cdSize;
    inout_stream >> ref_value.cdOffset >> ref_value.commentLength;
    W_ASSERT_DEBUG(ref_value.signature == EndOfCDMagicSignature, "ZIP: Corrupt end of central directory header.");
    return inout_stream;
  }

  struct CDFileHeader
  {
    WUInt32 signature;
    WUInt16 version;
    WUInt16 versionNeeded;
    WUInt16 flags;
    WUInt16 compression;
    WUInt16 modTime;
    WUInt16 modDate;
    WUInt32 crc32;
    WUInt32 compressedSize;
    WUInt32 uncompressedSize;
    WUInt16 fileNameLength;
    WUInt16 extraFieldLength;
    WUInt16 fileCommentLength;
    WUInt16 diskNumStart;
    WUInt16 internalAttr;
    WUInt32 externalAttr;
    WUInt32 offsetLocalHeader;
  };

  WStreamReader& operator>>(WStreamReader& inout_stream, CDFileHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.version >> ref_value.versionNeeded >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate;
    inout_stream >> ref_value.crc32 >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    inout_stream >> ref_value.fileCommentLength >> ref_value.diskNumStart >> ref_value.internalAttr >> ref_value.externalAttr >> ref_value.offsetLocalHeader;
    W_IGNORE_UNUSED(CDFileMagicSignature);
    W_ASSERT_DEBUG(ref_value.signature == CDFileMagicSignature, "ZIP: Corrupt central directory file entry header.");
    return inout_stream;
  }

  struct LocalFileHeader
  {
    WUInt32 signature;
    WUInt16 version;
    WUInt16 flags;
    WUInt16 compression;
    WUInt16 modTime;
    WUInt16 modDate;
    WUInt32 crc32;
    WUInt32 compressedSize;
    WUInt32 uncompressedSize;
    WUInt16 fileNameLength;
    WUInt16 extraFieldLength;
  };

  WStreamReader& operator>>(WStreamReader& inout_stream, LocalFileHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.version >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate >> ref_value.crc32;
    inout_stream >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    W_ASSERT_DEBUG(ref_value.signature == LocalFileMagicSignature, "ZIP: Corrupt local file entry header.");
    return inout_stream;
  }
}; // namespace ZipFormat

WResult WArchiveUtils::ReadZipHeader(WStreamReader& inout_stream, WUInt8& out_uiVersion)
{
  using namespace ZipFormat;

  WUInt32 header;
  inout_stream >> header;
  if (header == LocalFileMagicSignature)
  {
    out_uiVersion = 0;
    return W_SUCCESS;
  }
  return W_SUCCESS;
}

WResult WArchiveUtils::ExtractZipTOC(const WMemoryMappedFile& memFile, WArchiveTOC& ref_toc)
{
  using namespace ZipFormat;

  const WUInt8* pEndOfCDStart = nullptr;
  {
    // Find End of CD signature by searching from the end of the file.
    // As a comment can come after it we have to potentially walk max comment length backwards.
    const WUInt64 SearchEnd = memFile.GetFileSize() - WMath::Min(MaxEndOfCDSearchLength, memFile.GetFileSize());
    const WUInt8* pSearchEnd = static_cast<const WUInt8*>(memFile.GetReadPointer(SearchEnd, WMemoryMappedFile::OffsetBase::End));
    const WUInt8* pSearchStart = static_cast<const WUInt8*>(memFile.GetReadPointer(EndOfCDHeaderLength, WMemoryMappedFile::OffsetBase::End));
    while (pSearchStart >= pSearchEnd)
    {
      if (*reinterpret_cast<const WUInt32*>(pSearchStart) == EndOfCDMagicSignature)
      {
        pEndOfCDStart = pSearchStart;
        break;
      }
      pSearchStart--;
    }
    if (pEndOfCDStart == nullptr)
      return W_FAILURE;
  }

  WRawMemoryStreamReader tocReader(pEndOfCDStart, EndOfCDHeaderLength);
  EndOfCDHeader ecdHeader;
  tocReader >> ecdHeader;

  ref_toc.m_Entries.Reserve(ecdHeader.diskEntries);
  ref_toc.m_PathToEntryIndex.Reserve(ecdHeader.diskEntries);

  WStringBuilder sLowerCaseHash;
  WUInt64 uiEntryOffset = 0;
  for (WUInt16 uiEntry = 0; uiEntry < ecdHeader.diskEntries; ++uiEntry)
  {
    // First, read the current file's header from the central directory
    const void* pCdfStart = memFile.GetReadPointer(ecdHeader.cdOffset + uiEntryOffset, WMemoryMappedFile::OffsetBase::Start);
    WRawMemoryStreamReader cdfReader(pCdfStart, ecdHeader.cdSize - uiEntryOffset);
    CDFileHeader cdfHeader;
    cdfReader >> cdfHeader;

    if (cdfHeader.compression == CompressionType::Uncompressed || cdfHeader.compression == CompressionType::Deflate)
    {
      auto& entry = ref_toc.m_Entries.ExpandAndGetRef();
      entry.m_uiUncompressedDataSize = cdfHeader.uncompressedSize;
      entry.m_uiStoredDataSize = cdfHeader.compressedSize;
      entry.m_uiPathStringOffset = ref_toc.m_AllPathStrings.GetCount();
      entry.m_CompressionMode = cdfHeader.compression == CompressionType::Uncompressed ? WArchiveCompressionMode::Uncompressed : WArchiveCompressionMode::Compressed_zip;

      auto nameBuffer = WArrayPtr<const WUInt8>(static_cast<const WUInt8*>(pCdfStart) + CDFileHeaderLength, cdfHeader.fileNameLength);
      ref_toc.m_AllPathStrings.PushBackRange(nameBuffer);
      ref_toc.m_AllPathStrings.PushBack(0);
      const char* szName = reinterpret_cast<const char*>(ref_toc.m_AllPathStrings.GetData() + entry.m_uiPathStringOffset);
      sLowerCaseHash = szName;
      sLowerCaseHash.ToLower();
      ref_toc.m_PathToEntryIndex.Insert(WArchiveStoredString(WHashingUtils::StringHash(sLowerCaseHash), entry.m_uiPathStringOffset), ref_toc.m_Entries.GetCount() - 1);

      // Compute data stream start location. We need to skip past the local (and redundant) file header to find it.
      const void* pLfStart = memFile.GetReadPointer(cdfHeader.offsetLocalHeader, WMemoryMappedFile::OffsetBase::Start);
      WRawMemoryStreamReader lfReader(pLfStart, memFile.GetFileSize() - cdfHeader.offsetLocalHeader);
      LocalFileHeader lfHeader;
      lfReader >> lfHeader;
      entry.m_uiDataStartOffset = cdfHeader.offsetLocalHeader + LocalFileHeaderLength + lfHeader.fileNameLength + lfHeader.extraFieldLength;
    }
    // Compute next file header location.
    uiEntryOffset += CDFileHeaderLength + cdfHeader.fileNameLength + cdfHeader.extraFieldLength + cdfHeader.fileCommentLength;
  }

  return W_SUCCESS;
}
