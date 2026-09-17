#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DependencyFile.h>
#include <Foundation/Strings/HashedString.h>
#include <Utilities/UtilitiesDLL.h>

using WConfigFileResourceHandle = WTypedResourceHandle<class WConfigFileResource>;

/// This resource loads config files containing key/value pairs
///
/// The config files usually use the file extension '.WConfig'.
///
/// The file format looks like this:
///
/// To declare a key/value pair for the first time, write its type, name and value:
///   int i = 1
///   float f = 2.3
///   bool b = false
///   string s = "hello"
///
/// To set a variable to a different value than before, it has to be marked with 'override':
///
///   override i = 4
///
/// The format supports C preprocessor features like #include, #define, #ifdef, etc.
/// This can be used to build hierarchical config files:
///
///   #include "BaseConfig.WConfig"
///   override int SomeValue = 7
///
/// It can also be used to define 'enum types':
///
///   #define SmallValue 3
///   #define BigValue 5
///   int MyValue = BigValue
///
/// Since resources can be reloaded at runtime, config resources are a convenient way to define game parameters
/// that you may want to tweak at any time.
/// Using C preprocessor logic (#define, #if, #else, etc) you can quickly select between different configuration sets.
///
/// Once loaded, accessing the data is very efficient.
class W_UTILITIES_DLL WConfigFileResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WConfigFileResource, WResource);

  W_RESOURCE_DECLARE_COMMON_CODE(WConfigFileResource);

public:
  WConfigFileResource();
  ~WConfigFileResource();

  /// Returns the 'int' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  WInt32 GetInt(WTempHashedString sName) const;

  /// Returns the 'float' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  float GetFloat(WTempHashedString sName) const;

  /// Returns the 'bool' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  bool GetBool(WTempHashedString sName) const;

  /// Returns the 'string' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  WStringView GetString(WTempHashedString sName) const;

  /// Returns the 'int' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  WInt32 GetInt(WTempHashedString sName, WInt32 iFallback) const;

  /// Returns the 'float' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  float GetFloat(WTempHashedString sName, float fFallback) const;

  /// Returns the 'bool' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  bool GetBool(WTempHashedString sName, bool bFallback) const;

  /// Returns the 'string' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  WStringView GetString(WTempHashedString sName, WStringView sFallback) const;

protected:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  friend class WConfigFileResourceLoader;

  WHashTable<WHashedString, WInt32> m_IntData;
  WHashTable<WHashedString, float> m_FloatData;
  WHashTable<WHashedString, WString> m_StringData;
  WHashTable<WHashedString, bool> m_BoolData;

  WDependencyFile m_RequiredFiles;
};


class W_UTILITIES_DLL WConfigFileResourceLoader : public WResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    WDefaultMemoryStreamStorage m_Storage;
    WMemoryStreamReader m_Reader;
    WDependencyFile m_RequiredFiles;

    WResult PrePropFileLocator(WStringView sCurAbsoluteFile, WStringView sIncludeFile, WPreprocessor::IncludeType incType, WStringBuilder& out_sAbsoluteFilePath);
  };

  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const WResource* pResource) const override;
};
