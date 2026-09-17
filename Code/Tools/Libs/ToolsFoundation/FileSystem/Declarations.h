#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Uuid.h>

#if 0 // Define to enable extensive file system profile scopes
#  define FILESYSTEM_PROFILE(szName) W_PROFILE_SCOPE(szName)

#else
#  define FILESYSTEM_PROFILE(Name)

#endif

/// Information about a single file on disk. The file might be a document or any other file found in the data directories.
struct W_TOOLSFOUNDATION_DLL WFileStatus
{
  enum class Status : WUInt8
  {
    Unknown,    ///< Since the file has been tagged as 'Unknown' it has not been encountered again on disk (yet). Use internally to find stale entries in the model.
    FileLocked, ///< The file is locked, i.e. reading is currently not possible. Try again at a later date.
    Valid       ///< The file exists on disk.
  };

  WFileStatus() = default;

  WTimestamp m_LastModified;
  WUInt64 m_uiHash = 0;
  WUuid m_DocumentID; ///< If the file is linked to a document, the GUID is valid, otherwise not.
  Status m_Status = Status::Unknown;
};
W_DECLARE_REFLECTABLE_TYPE(W_TOOLSFOUNDATION_DLL, WFileStatus);

W_ALWAYS_INLINE WStreamWriter& operator<<(WStreamWriter& inout_stream, const WFileStatus& value)
{
  inout_stream.WriteBytes(&value, sizeof(WFileStatus)).IgnoreResult();
  return inout_stream;
}

W_ALWAYS_INLINE WStreamReader& operator>>(WStreamReader& inout_stream, WFileStatus& ref_value)
{
  inout_stream.ReadBytes(&ref_value, sizeof(WFileStatus));
  return inout_stream;
}
