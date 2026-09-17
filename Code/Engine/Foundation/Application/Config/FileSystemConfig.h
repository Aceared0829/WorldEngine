#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class W_FOUNDATION_DLL WApplicationFileSystemConfig
{
public:
  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/DataDirectories.ddl"_wsv;

  WResult Save(WStringView sPath = s_sConfigFile);
  void Load(WStringView sPath = s_sConfigFile);

  /// Sets up the data directories that were configured or loaded into this object
  void Apply();

  /// Removes all data directories that were set up by any call to WApplicationFileSystemConfig::Apply()
  static void Clear();

  WResult CreateDataDirStubFiles();

  struct DataDirConfig
  {
    WString m_sDataDirSpecialPath;
    WString m_sRootName;
    bool m_bWritable;            ///< Whether the directory is going to be mounted for writing
    bool m_bHardCodedDependency; ///< If set to true, this indicates that it may not be removed by the user (in a config dialog)

    DataDirConfig()
    {
      m_bWritable = false;
      m_bHardCodedDependency = false;
    }

    bool operator==(const DataDirConfig& rhs) const
    {
      return m_bWritable == rhs.m_bWritable && m_sDataDirSpecialPath == rhs.m_sDataDirSpecialPath && m_sRootName == rhs.m_sRootName;
    }
  };

  bool operator==(const WApplicationFileSystemConfig& rhs) const { return m_DataDirs == rhs.m_DataDirs; }

  WHybridArray<DataDirConfig, 4> m_DataDirs;
};


using WApplicationFileSystemConfig_DataDirConfig = WApplicationFileSystemConfig::DataDirConfig;

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WApplicationFileSystemConfig);
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WApplicationFileSystemConfig_DataDirConfig);
