#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Timestamp.h>

/// This class represents a set of files of which one wants to know when any one of them changes.
///
/// WDependencyFile stores a list of files that are the 'dependency set'. It can be serialized.
/// Through HasAnyFileChanged() one can detect whether any of the files has changed, since the last call to StoreCurrentTimeStamp().
/// The time stamp that is retrieved through StoreCurrentTimeStamp() will also be serialized.
class W_FOUNDATION_DLL WDependencyFile
{
public:
  WDependencyFile();

  /// Clears all files that were added with AddFileDependency()
  void Clear();

  /// Adds one file as a dependency to the list
  void AddFileDependency(WStringView sFile);

  /// Allows read access to all currently stored file dependencies
  const WHybridArray<WString, 16>& GetFileDependencies() const { return m_AssetTransformDependencies; }

  /// Writes the current state to a stream. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest
  /// file stamp
  WResult WriteDependencyFile(WStreamWriter& inout_stream) const;

  /// Reads the state from a stream. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was
  /// serialized.
  WResult ReadDependencyFile(WStreamReader& inout_stream);

  /// Writes the current state to a file. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest file
  /// stamp
  WResult WriteDependencyFile(WStringView sFile) const;

  /// Reads the state from a file. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was
  /// serialized.
  WResult ReadDependencyFile(WStringView sFile);

  /// Retrieves the current file time stamps from the filesystem and determines whether any file has changed since the last call to
  /// StoreCurrentTimeStamp() (or ReadDependencyFile())
  bool HasAnyFileChanged() const;

  /// Retrieves the current file time stamps from the filesystem and stores it for later comparison. This value is also serialized through
  /// WriteDependencyFile(), so it should be called before that, to store the latest state.
  void StoreCurrentTimeStamp();

private:
  static WResult RetrieveFileTimeStamp(WStringView sFile, WTimestamp& out_Result);

  WHybridArray<WString, 16> m_AssetTransformDependencies;
  WInt64 m_iMaxTimeStampStored = 0;
  WUInt64 m_uiSumTimeStampStored = 0;

  struct FileCheckCache
  {
    WTimestamp m_FileTimestamp;
    WTime m_LastCheck;
  };

  static WMutex s_FileTimestampsLock;
  static WMap<WString, FileCheckCache> s_FileTimestamps;
};
