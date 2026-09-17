#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class WChunkStreamWriter;
class WChunkStreamReader;

//////////////////////////////////////////////////////////////////////////

/// Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class W_CORE_DLL WProfileConfigData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WProfileConfigData, WReflectedClass);

public:
  WProfileConfigData();
  ~WProfileConfigData();

  virtual void SaveRuntimeData(WChunkStreamWriter& inout_stream) const;
  virtual void LoadRuntimeData(WChunkStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

/// Stores platform-specific configuration data for asset processing and runtime settings.
///
/// A platform profile contains multiple configuration objects (WProfileConfigData) that store
/// settings for different aspects like asset transforms, rendering options, etc. Each profile
/// targets a specific platform and maintains a modification counter for change tracking.
class W_CORE_DLL WPlatformProfile final : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WPlatformProfile, WReflectedClass);

public:
  WPlatformProfile();
  ~WPlatformProfile();

  void SetConfigName(WStringView sName) { m_sName = sName; }
  WStringView GetConfigName() const { return m_sName; }

  void SetTargetPlatform(WStringView sPlatform) { m_sTargetPlatform = sPlatform; }
  WStringView GetTargetPlatform() const { return m_sTargetPlatform; }

  void Clear();
  void AddMissingConfigs();

  template <typename TYPE>
  const TYPE* GetTypeConfig() const
  {
    return static_cast<const TYPE*>(GetTypeConfig(WGetStaticRTTI<TYPE>()));
  }

  template <typename TYPE>
  TYPE* GetTypeConfig()
  {
    return static_cast<TYPE*>(GetTypeConfig(WGetStaticRTTI<TYPE>()));
  }

  const WProfileConfigData* GetTypeConfig(const WRTTI* pRtti) const;
  WProfileConfigData* GetTypeConfig(const WRTTI* pRtti);

  WResult SaveForRuntime(WStringView sFile) const;
  WResult LoadForRuntime(WStringView sFile);

  /// Returns a number indicating when the profile counter changed last. By storing and comparing this value, other code can update their state if necessary.
  WUInt32 GetLastModificationCounter() const { return m_uiLastModificationCounter; }


private:
  WUInt32 m_uiLastModificationCounter = 0;
  WString m_sName;
  WString m_sTargetPlatform = "Windows";
  WDynamicArray<WProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////
