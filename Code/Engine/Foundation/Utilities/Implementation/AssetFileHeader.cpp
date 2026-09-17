#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>

static const char* g_szAssetTag = "WEAsset";

WAssetFileHeader::WAssetFileHeader() = default;

enum WAssetFileHeaderVersion : WUInt8
{
  Version1 = 1,
  Version2,
  Version3,

  VersionCount,
  VersionCurrent = VersionCount - 1
};

WResult WAssetFileHeader::Write(WStreamWriter& inout_stream) const
{
  W_ASSERT_DEBUG(m_uiHash != 0xFFFFFFFFFFFFFFFF, "Cannot write an invalid hash to file");

  // 8 Bytes for identification + version
  W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(g_szAssetTag, 7));

  const WUInt8 uiVersion = WAssetFileHeaderVersion::VersionCurrent;
  inout_stream << uiVersion;

  // 8 Bytes for the hash
  inout_stream << m_uiHash;
  // 2 for the type version
  inout_stream << m_uiVersion;

  inout_stream << m_sGenerator;
  return W_SUCCESS;
}

WResult WAssetFileHeader::Read(WStreamReader& inout_stream)
{
  // initialize to 'invalid'
  m_uiHash = 0xFFFFFFFFFFFFFFFF;
  m_uiVersion = 0;

  char szTag[8] = {0};
  if (inout_stream.ReadBytes(szTag, 7) < 7)
  {
    W_REPORT_FAILURE("The stream does not contain a valid asset file header");
    return W_FAILURE;
  }

  szTag[7] = '\0';

  // invalid asset file ... this is not going to end well
  W_ASSERT_DEBUG(WStringUtils::IsEqual(szTag, g_szAssetTag), "The stream does not contain a valid asset file header");

  if (!WStringUtils::IsEqual(szTag, g_szAssetTag))
    return W_FAILURE;

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  WUInt64 uiHash = 0;
  inout_stream >> uiHash;

  // future version?
  W_ASSERT_DEV(uiVersion <= WAssetFileHeaderVersion::VersionCurrent, "Unknown asset header version {0}", uiVersion);

  if (uiVersion >= WAssetFileHeaderVersion::Version2)
  {
    inout_stream >> m_uiVersion;
  }

  if (uiVersion >= WAssetFileHeaderVersion::Version3)
  {
    inout_stream >> m_sGenerator;
  }

  // older version? set the hash to 'invalid'
  if (uiVersion != WAssetFileHeaderVersion::VersionCurrent)
    return W_FAILURE;

  m_uiHash = uiHash;

  return W_SUCCESS;
}
