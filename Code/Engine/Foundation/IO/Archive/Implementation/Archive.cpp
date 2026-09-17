#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Logging/Log.h>

void operator<<(WStreamWriter& inout_stream, const WArchiveStoredString& value)
{
  inout_stream << value.m_uiLowerCaseHash;
  inout_stream << value.m_uiSrcStringOffset;
}

void operator>>(WStreamReader& inout_stream, WArchiveStoredString& value)
{
  inout_stream >> value.m_uiLowerCaseHash;
  inout_stream >> value.m_uiSrcStringOffset;
}

WUInt32 WArchiveTOC::FindEntry(WStringView sFile) const
{
  WStringBuilder sLowerCasePath = sFile;
  sLowerCasePath.ToLower();

  WUInt32 uiIndex;

  WArchiveLookupString lookup(WHashingUtils::StringHash(sLowerCasePath.GetView()), sLowerCasePath, m_AllPathStrings);

  if (!m_PathToEntryIndex.TryGetValue(lookup, uiIndex))
    return WInvalidIndex;

  W_ASSERT_DEBUG(sFile.IsEqual_NoCase(GetEntryPathString(uiIndex)), "Hash table corruption detected.");
  return uiIndex;
}

WUInt32 WArchiveTOC::AddPathString(WStringView sPathString)
{
  const WUInt32 offset = m_AllPathStrings.GetCount();
  const WUInt32 numNewBytesNeeded = sPathString.GetElementCount() + 1;
  m_AllPathStrings.Reserve(m_AllPathStrings.GetCount() + numNewBytesNeeded);
  m_AllPathStrings.PushBackRange(WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(sPathString.GetStartPointer()), sPathString.GetElementCount()));
  m_AllPathStrings.PushBackUnchecked('\0');
  return offset;
}

void WArchiveTOC::RebuildPathToEntryHashes()
{
  const WUInt32 uiNumEntries = m_Entries.GetCount();
  m_PathToEntryIndex.Clear();
  m_PathToEntryIndex.Reserve(uiNumEntries);

  WStringBuilder sLowerCasePath;

  for (WUInt32 i = 0; i < uiNumEntries; i++)
  {
    const WUInt32 uiSrcStringOffset = m_Entries[i].m_uiPathStringOffset;
    WStringView sEntryString = GetEntryPathString(i);
    sLowerCasePath = sEntryString;
    sLowerCasePath.ToLower();

    // cut off the upper 32 bit, we don't need them here
    const WUInt32 uiLowerCaseHash = WHashingUtils::StringHashTo32(WHashingUtils::StringHash(sLowerCasePath.GetView()) & 0xFFFFFFFFllu);

    m_PathToEntryIndex.Insert(WArchiveStoredString(uiLowerCaseHash, uiSrcStringOffset), i);

    // Verify that the conversion worked
    W_ASSERT_DEBUG(FindEntry(sEntryString) == i, "Hashed path retrieval did not yield inserted index");
  }
}

WStringView WArchiveTOC::GetEntryPathString(WUInt32 uiEntryIdx) const
{
  return reinterpret_cast<const char*>(&m_AllPathStrings[m_Entries[uiEntryIdx].m_uiPathStringOffset]);
}

WResult WArchiveTOC::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(2);

  W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Entries));

  // write the hash of a known string to the archive, to detect hash function changes
  WUInt64 uiStringHash = WHashingUtils::StringHash("WArchive");
  inout_stream << uiStringHash;

  W_SUCCEED_OR_RETURN(inout_stream.WriteHashTable(m_PathToEntryIndex));

  W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_AllPathStrings));

  return W_SUCCESS;
}

struct WOldTempHashedString
{
  WUInt32 m_uiHash = 0;

  WResult Deserialize(WStreamReader& r)
  {
    r >> m_uiHash;
    return W_SUCCESS;
  }

  bool operator==(const WOldTempHashedString& rhs) const
  {
    return m_uiHash == rhs.m_uiHash;
  }
};

template <>
struct WHashHelper<WOldTempHashedString>
{
  static WUInt32 Hash(const WOldTempHashedString& value)
  {
    return value.m_uiHash;
  }

  static bool Equal(const WOldTempHashedString& a, const WOldTempHashedString& b) { return a == b; }
};

WResult WArchiveTOC::Deserialize(WStreamReader& inout_stream, WUInt8 uiArchiveVersion)
{
  W_ASSERT_ALWAYS(uiArchiveVersion <= 4, "Unsupported archive version {}", uiArchiveVersion);

  // we don't use the TOC version anymore, but the archive version instead
  const WTypeVersion version = inout_stream.ReadVersion(2);

  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Entries));

  bool bRecreateStringHashes = true;

  if (version == 1)
  {
    // read and discard the data, it is regenerated below
    WHashTable<WOldTempHashedString, WUInt32> m_PathToIndex;
    W_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(m_PathToIndex));
  }
  else
  {
    if (uiArchiveVersion >= 4)
    {
      // read the hash of a known string from the archive, to detect hash function changes
      WUInt64 uiStringHash = 0;
      inout_stream >> uiStringHash;

      if (uiStringHash == WHashingUtils::StringHash("WArchive"))
      {
        bRecreateStringHashes = false;
      }
    }

    W_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(m_PathToEntryIndex));
  }

  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_AllPathStrings));

  if (bRecreateStringHashes)
  {
    WLog::Info("Archive uses older string hashing, recomputing hashes.");

    // version 1 stores an older way for the path/hash -> entry lookup table, which is prone to hash collisions
    // in this case, rebuild the new hash table on the fly
    //
    // version 2 used MurmurHash
    // version 3 switched to 32 bit xxHash
    // version 4 switched to 64 bit hashes

    RebuildPathToEntryHashes();
  }

  // path strings mustn't be empty and must be zero-terminated
  if (m_AllPathStrings.IsEmpty() || m_AllPathStrings.PeekBack() != '\0')
  {
    WLog::Error("Archive is corrupt. Invalid string data.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WArchiveEntry::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_uiDataStartOffset;
  inout_stream << m_uiUncompressedDataSize;
  inout_stream << m_uiStoredDataSize;
  inout_stream << (WUInt8)m_CompressionMode;
  inout_stream << m_uiPathStringOffset;

  return W_SUCCESS;
}

WResult WArchiveEntry::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_uiDataStartOffset;
  inout_stream >> m_uiUncompressedDataSize;
  inout_stream >> m_uiStoredDataSize;
  WUInt8 uiCompressionMode = 0;
  inout_stream >> uiCompressionMode;
  m_CompressionMode = (WArchiveCompressionMode)uiCompressionMode;
  inout_stream >> m_uiPathStringOffset;

  return W_SUCCESS;
}
