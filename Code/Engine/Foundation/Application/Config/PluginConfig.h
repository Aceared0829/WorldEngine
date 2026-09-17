#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class W_FOUNDATION_DLL WApplicationPluginConfig
{
public:
  WApplicationPluginConfig();

  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/Plugins.ddl"_wsv;

  WResult Save(WStringView sConfigPath = s_sConfigFile) const;
  void Load(WStringView sConfigPath = s_sConfigFile);
  void Apply();

  struct W_FOUNDATION_DLL PluginConfig
  {
    bool operator<(const PluginConfig& rhs) const;

    WString m_sAppDirRelativePath;
    bool m_bLoadCopy = false;
  };

  bool AddPlugin(const PluginConfig& cfg);
  bool RemovePlugin(const PluginConfig& cfg);

  mutable WHybridArray<PluginConfig, 8> m_Plugins;
};


using WApplicationPluginConfig_PluginConfig = WApplicationPluginConfig::PluginConfig;

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WApplicationPluginConfig);
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WApplicationPluginConfig_PluginConfig);
