#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>

/// Simple class to handle asset file headers (the very first bytes in all transformed asset files)
class W_FOUNDATION_DLL WAssetFileHeader
{
public:
  WAssetFileHeader();

  /// Reads the hash from file. If the file is outdated, the hash is set to 0xFFFFFFFFFFFFFFFF.
  WResult Read(WStreamReader& inout_stream);

  /// Writes the asset hash to file (plus a little version info)
  WResult Write(WStreamWriter& inout_stream) const;

  /// Checks whether the stored file contains the same hash.
  bool IsFileUpToDate(WUInt64 uiExpectedHash, WUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

  /// Returns the asset file hash
  WUInt64 GetFileHash() const { return m_uiHash; }

  /// Sets the asset file hash
  void SetFileHashAndVersion(WUInt64 uiHash, WUInt16 v)
  {
    m_uiHash = uiHash;
    m_uiVersion = v;
  }

  /// Returns the asset type version
  WUInt16 GetFileVersion() const { return m_uiVersion; }

  /// Returns the generator which was used to produce the asset file
  const WHashedString& GetGenerator() { return m_sGenerator; }

  /// Allows to set the generator string
  void SetGenerator(WStringView sGenerator) { m_sGenerator.Assign(sGenerator); }

private:
  // initialize to a 'valid' hash
  // this may get stored, unless someone sets the hash
  WUInt64 m_uiHash = 0;
  WUInt16 m_uiVersion = 0;
  WHashedString m_sGenerator;
};
