#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

class WRawMemoryStreamReader;

/// Compression modes for WArchive file entries
enum class WArchiveCompressionMode : WUInt8
{
  Uncompressed,
  Compressed_zstd,
  Compressed_zip,
};

/// Data for a single file entry in an WArchive file
class W_FOUNDATION_DLL WArchiveEntry
{
public:
  WUInt64 m_uiDataStartOffset = 0;      ///< Byte offset for where the file's (compressed) data stream starts in the WArchive
  WUInt64 m_uiUncompressedDataSize = 0; ///< Size of the original uncompressed data.
  WUInt64 m_uiStoredDataSize = 0;       ///< The amount of (compressed) bytes actually stored in the WArchive.
  WUInt32 m_uiPathStringOffset = 0;     ///< Byte offset into WArchiveTOC::m_AllPathStrings where the path string for this entry resides.
  WArchiveCompressionMode m_CompressionMode = WArchiveCompressionMode::Uncompressed;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

/// Helper class to store a hashed string for quick lookup in the archive TOC
///
/// Stores a hash of the lower case string for quick comparison.
/// Additionally stores an offset into the WArchiveTOC::m_AllPathStrings array for final validation, to prevent hash collisions.
/// The proper string lookup with hash collision check only works together with WArchiveLookupString, which has the necessary context
/// to index the WArchiveTOC::m_AllPathStrings array.
class W_FOUNDATION_DLL WArchiveStoredString
{
public:
  W_DECLARE_POD_TYPE();

  WArchiveStoredString() = default;

  WArchiveStoredString(WUInt64 uiLowerCaseHash, WUInt32 uiSrcStringOffset)
    : m_uiLowerCaseHash(WHashingUtils::StringHashTo32(uiLowerCaseHash))
    , m_uiSrcStringOffset(uiSrcStringOffset)
  {
  }

  WUInt32 m_uiLowerCaseHash;
  WUInt32 m_uiSrcStringOffset;
};

void operator<<(WStreamWriter& inout_stream, const WArchiveStoredString& value);
void operator>>(WStreamReader& inout_stream, WArchiveStoredString& value);

/// Helper class for looking up path strings in WArchiveTOC::FindEntry()
///
/// Only works together with WArchiveStoredString.
class WArchiveLookupString
{
  W_DISALLOW_COPY_AND_ASSIGN(WArchiveLookupString);

public:
  W_DECLARE_POD_TYPE();

  WArchiveLookupString(WUInt64 uiLowerCaseHash, WStringView sString, const WDynamicArray<WUInt8>& archiveAllPathStrings)
    : m_uiLowerCaseHash(WHashingUtils::StringHashTo32(uiLowerCaseHash))
    , m_sString(sString)
    , m_ArchiveAllPathStrings(archiveAllPathStrings)
  {
  }

  WUInt32 m_uiLowerCaseHash;
  WStringView m_sString;
  const WDynamicArray<WUInt8>& m_ArchiveAllPathStrings;
};

/// Functions to enable WHashTable to 1) store WArchiveStoredString and 2) lookup strings efficiently with a WArchiveLookupString
template <>
struct WHashHelper<WArchiveStoredString>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WArchiveStoredString& hs) { return hs.m_uiLowerCaseHash; }
  W_ALWAYS_INLINE static WUInt32 Hash(const WArchiveLookupString& hs) { return hs.m_uiLowerCaseHash; }

  W_ALWAYS_INLINE static bool Equal(const WArchiveStoredString& a, const WArchiveStoredString& b) { return a.m_uiSrcStringOffset == b.m_uiSrcStringOffset; }

  W_ALWAYS_INLINE static bool Equal(const WArchiveStoredString& a, const WArchiveLookupString& b)
  {
    // in case that we want to lookup a string using a WArchiveLookupString, we validate
    // that the stored string is actually equal to the lookup string, to enable handling of hash collisions
    return b.m_sString.IsEqual_NoCase(reinterpret_cast<const char*>(&b.m_ArchiveAllPathStrings[a.m_uiSrcStringOffset]));
  }
};

/// Table-of-contents for an WArchive file
class W_FOUNDATION_DLL WArchiveTOC
{
public:
  /// all files stored in the WArchive
  WDynamicArray<WArchiveEntry> m_Entries;
  /// allows to map a hashed string to the index of the file entry for the file path
  WHashTable<WArchiveStoredString, WUInt32> m_PathToEntryIndex;
  /// one large array holding all path strings for the file entries, to reduce allocations
  WDynamicArray<WUInt8> m_AllPathStrings;

  /// Returns the entry index for the given file or WInvalidIndex, if not found.
  WUInt32 FindEntry(WStringView sFile) const;

  WUInt32 AddPathString(WStringView sPathString);

  void RebuildPathToEntryHashes();

  WStringView GetEntryPathString(WUInt32 uiEntryIdx) const;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream, WUInt8 uiArchiveVersion);
};
