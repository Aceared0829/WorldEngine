#pragma once

#include <FileservePlugin/FileservePluginDLL.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

enum class WFileserveFileState
{
  None = 0,
  NonExistant = 1,
  NonExistantEither = 2,
  SameTimestamp = 3,
  SameHash = 4,
  Different = 5,
};

class W_FILESERVEPLUGIN_DLL WFileserveClientContext
{
public:
  struct DataDir
  {
    WString m_sRootName;
    WString m_sPathOnClient;
    WString m_sPathOnServer;
    WString m_sMountPoint;
    bool m_bMounted = false;
  };

  struct FileStatus
  {
    WInt64 m_iTimestamp = -1;
    WUInt64 m_uiHash = 0;
    WUInt64 m_uiFileSize = 0;
  };

  WFileserveFileState GetFileStatus(WUInt16& inout_uiDataDirID, const char* szRequestedFile, FileStatus& inout_status,
    WDynamicArray<WUInt8>& out_fileContent, bool bForceThisDataDir) const;

  bool m_bLostConnection = false;
  WUInt32 m_uiApplicationID = 0;
  WHybridArray<DataDir, 8> m_MountedDataDirs;
};
