#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/DependencyFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

enum class WDependencyFileVersion : WUInt8
{
  Version0 = 0,
  Version1,
  Version2, ///< added 'sum' time

  ENUM_COUNT,
  Current = ENUM_COUNT - 1,
};

WMutex WDependencyFile::s_FileTimestampsLock;
WMap<WString, WDependencyFile::FileCheckCache> WDependencyFile::s_FileTimestamps;

WDependencyFile::WDependencyFile()
{
  Clear();
}

void WDependencyFile::Clear()
{
  m_iMaxTimeStampStored = 0;
  m_uiSumTimeStampStored = 0;
  m_AssetTransformDependencies.Clear();
}

void WDependencyFile::AddFileDependency(WStringView sFile)
{
  if (sFile.IsEmpty())
    return;

  m_AssetTransformDependencies.PushBack(sFile);
}

void WDependencyFile::StoreCurrentTimeStamp()
{
  W_LOG_BLOCK("WDependencyFile::StoreCurrentTimeStamp");

  m_iMaxTimeStampStored = 0;
  m_uiSumTimeStampStored = 0;

#if W_DISABLED(W_SUPPORTS_FILE_STATS)
  WLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return;
#endif

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    WTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const WInt64 time = ts.GetInt64(WSIUnitOfTime::Second);
    m_iMaxTimeStampStored = WMath::Max<WInt64>(m_iMaxTimeStampStored, time);
    m_uiSumTimeStampStored += (WUInt64)time;
  }
}

bool WDependencyFile::HasAnyFileChanged() const
{
#if W_DISABLED(W_SUPPORTS_FILE_STATS)
  WLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return true;
#endif

  WUInt64 uiSumTs = 0;

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    WTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const WInt64 time = ts.GetInt64(WSIUnitOfTime::Second);

    if (time > m_iMaxTimeStampStored)
    {
      WLog::Dev("Detected file change in '{0}' (TimeStamp {1} > MaxTimeStamp {2})", WArgSensitive(sFile, "File"),
        ts.GetInt64(WSIUnitOfTime::Second), m_iMaxTimeStampStored);
      return true;
    }

    uiSumTs += (WUInt64)time;
  }

  if (uiSumTs != m_uiSumTimeStampStored)
  {
    WLog::Dev("Detected file change, but exact file is not known.");
    return true;
  }

  return false;
}

WResult WDependencyFile::WriteDependencyFile(WStreamWriter& inout_stream) const
{
  inout_stream << (WUInt8)WDependencyFileVersion::Current;

  inout_stream << m_iMaxTimeStampStored;
  inout_stream << m_uiSumTimeStampStored;
  inout_stream << m_AssetTransformDependencies.GetCount();

  for (const auto& sFile : m_AssetTransformDependencies)
    inout_stream << sFile;

  return W_SUCCESS;
}

WResult WDependencyFile::ReadDependencyFile(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = (WUInt8)WDependencyFileVersion::Version0;
  inout_stream >> uiVersion;

  if (uiVersion > (WUInt8)WDependencyFileVersion::Current)
  {
    WLog::Error("Dependency file has incorrect file version ({0})", uiVersion);
    return W_FAILURE;
  }

  W_ASSERT_DEV(uiVersion <= (WUInt8)WDependencyFileVersion::Current, "Invalid file version {0}", uiVersion);

  inout_stream >> m_iMaxTimeStampStored;

  if (uiVersion >= (WUInt8)WDependencyFileVersion::Version2)
  {
    inout_stream >> m_uiSumTimeStampStored;
  }

  WUInt32 count = 0;
  inout_stream >> count;
  m_AssetTransformDependencies.SetCount(count);

  for (WUInt32 i = 0; i < m_AssetTransformDependencies.GetCount(); ++i)
    inout_stream >> m_AssetTransformDependencies[i];

  return W_SUCCESS;
}

WResult WDependencyFile::RetrieveFileTimeStamp(WStringView sFile, WTimestamp& out_Result)
{
#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  W_LOCK(s_FileTimestampsLock);
  bool bExisted = false;
  auto it = s_FileTimestamps.FindOrAdd(sFile, &bExisted);

  if (!bExisted || it.Value().m_LastCheck + WTime::MakeFromSeconds(2.0) < WTime::Now())
  {
    it.Value().m_LastCheck = WTime::Now();

    WFileStats stats;
    if (WFileSystem::GetFileStats(sFile, stats).Failed())
    {
      WLog::Error("Could not query the file stats for '{0}'", WArgSensitive(sFile, "File"));
      return W_FAILURE;
    }

    it.Value().m_FileTimestamp = stats.m_LastModificationTime;
  }

  out_Result = it.Value().m_FileTimestamp;

#else

  out_Result = WTimestamp::MakeFromInt(0, WSIUnitOfTime::Second);
  WLog::Warning("Trying to retrieve a file time stamp on a platform that does not support it (file: '{0}')", WArgSensitive(sFile, "File"));

#endif

  return out_Result.IsValid() ? W_SUCCESS : W_FAILURE;
}

WResult WDependencyFile::WriteDependencyFile(WStringView sFile) const
{
  W_LOG_BLOCK("WDependencyFile::WriteDependencyFile", sFile);

  WFileWriter file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  return WriteDependencyFile(file);
}

WResult WDependencyFile::ReadDependencyFile(WStringView sFile)
{
  W_LOG_BLOCK("WDependencyFile::ReadDependencyFile", sFile);

  WFileReader file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  return ReadDependencyFile(file);
}
