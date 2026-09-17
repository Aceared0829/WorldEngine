#include <Utilities/UtilitiesPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Utilities/Resources/ConfigFileResource.h>

static WConfigFileResourceLoader s_ConfigFileResourceLoader;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Utilties, ConfigFileResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WResourceManager::SetResourceTypeLoader<WConfigFileResource>(&s_ConfigFileResourceLoader);

    auto hFallback = WResourceManager::LoadResource<WConfigFileResource>("Empty.WConfig");
    WResourceManager::SetResourceTypeMissingFallback<WConfigFileResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WResourceManager::SetResourceTypeMissingFallback<WConfigFileResource>(WConfigFileResourceHandle());
    WResourceManager::SetResourceTypeLoader<WConfigFileResource>(nullptr);
    WConfigFileResource::CleanupDynamicPluginReferences();
  }

  W_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConfigFileResource, 1, WRTTIDefaultAllocator<WConfigFileResource>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_RESOURCE_IMPLEMENT_COMMON_CODE(WConfigFileResource);

WConfigFileResource::WConfigFileResource()
  : WResource(WResource::DoUpdate::OnAnyThread, 0)
{
}

WConfigFileResource::~WConfigFileResource() = default;

WInt32 WConfigFileResource::GetInt(WTempHashedString sName, WInt32 iFallback) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return iFallback;
}

WInt32 WConfigFileResource::GetInt(WTempHashedString sName) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  WStringView name = "<unknown>"_wsv;
  sName.LookupStringHash(name).IgnoreResult();
  WLog::Error("{}: 'int' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return 0;
}

float WConfigFileResource::GetFloat(WTempHashedString sName, float fFallback) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return fFallback;
}

float WConfigFileResource::GetFloat(WTempHashedString sName) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  WStringView name = "<unknown>"_wsv;
  sName.LookupStringHash(name).IgnoreResult();
  WLog::Error("{}: 'float' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return 0;
}

bool WConfigFileResource::GetBool(WTempHashedString sName, bool bFallback) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return bFallback;
}

bool WConfigFileResource::GetBool(WTempHashedString sName) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  WStringView name = "<unknown>"_wsv;
  sName.LookupStringHash(name).IgnoreResult();
  WLog::Error("{}: 'float' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return false;
}

WStringView WConfigFileResource::GetString(WTempHashedString sName, WStringView sFallback) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return sFallback;
}

WStringView WConfigFileResource::GetString(WTempHashedString sName) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  WStringView name = "<unknown>"_wsv;
  sName.LookupStringHash(name).IgnoreResult();
  WLog::Error("{}: 'string' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return "";
}

WResourceLoadDesc WConfigFileResource::UnloadData(Unload WhatToUnload)
{
  W_IGNORE_UNUSED(WhatToUnload);

  m_IntData.Clear();
  m_FloatData.Clear();
  m_StringData.Clear();
  m_BoolData.Clear();

  WResourceLoadDesc d;
  d.m_State = WResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

WResourceLoadDesc WConfigFileResource::UpdateContent(WStreamReader* Stream)
{
  WResourceLoadDesc d;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  d.m_State = WResourceState::Loaded;

  if (Stream == nullptr)
  {
    d.m_State = WResourceState::LoadedResourceMissing;
    return d;
  }

  m_RequiredFiles.ReadDependencyFile(*Stream).IgnoreResult();
  Stream->ReadHashTable(m_IntData).IgnoreResult();
  Stream->ReadHashTable(m_FloatData).IgnoreResult();
  Stream->ReadHashTable(m_StringData).IgnoreResult();
  Stream->ReadHashTable(m_BoolData).IgnoreResult();

  return d;
}

void WConfigFileResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = m_IntData.GetHeapMemoryUsage() + m_FloatData.GetHeapMemoryUsage() + m_StringData.GetHeapMemoryUsage() + m_BoolData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

WResult WConfigFileResourceLoader::LoadedData::PrePropFileLocator(WStringView sCurAbsoluteFile, WStringView sIncludeFile, WPreprocessor::IncludeType incType, WStringBuilder& out_sAbsoluteFilePath)
{
  WResult res = WPreprocessor::DefaultFileLocator(sCurAbsoluteFile, sIncludeFile, incType, out_sAbsoluteFilePath);

  m_RequiredFiles.AddFileDependency(out_sAbsoluteFilePath);

  return res;
}

WResourceLoadData WConfigFileResourceLoader::OpenDataStream(const WResource* pResource)
{
  W_PROFILE_SCOPE("ReadResourceFile");
  W_LOG_BLOCK("Load Config Resource", pResource->GetResourceID());

  WStringBuilder sConfig;

  WMap<WString, WInt32> intData;
  WMap<WString, float> floatData;
  WMap<WString, WString> stringData;
  WMap<WString, bool> boolData;

  LoadedData* pData = W_DEFAULT_NEW(LoadedData);
  pData->m_Reader.SetStorage(&pData->m_Storage);

  WPreprocessor preprop;

  // used to gather all the transitive file dependencies
  preprop.SetFileLocatorFunction(WMakeDelegate(&WConfigFileResourceLoader::LoadedData::PrePropFileLocator, pData));

  WLogSystemToBuffer errorBuffer;
  errorBuffer.SetLogLevel(WLogMsgType::WarningMsg);
  preprop.SetLogInterface(&errorBuffer);

  if (pResource->GetResourceID() == "Empty.WConfig")
  {
    // do nothing
  }
  else if (preprop.Process(pResource->GetResourceID(), sConfig, false, true, false).Succeeded())
  {
    sConfig.ReplaceAll("\r", "");
    sConfig.ReplaceAll("\n", ";");

    WTempHybridArray<WStringView, 32> lines;
    sConfig.Split(false, lines, ";");

    WStringBuilder key, value, line;

    for (WStringView tmp : lines)
    {
      line = tmp;
      line.Trim(" \t");

      if (line.IsEmpty())
        continue;

      const char* szAssign = line.FindSubString("=");

      if (szAssign == nullptr)
      {
        WLog::Error("Invalid line in config file: '{}'", tmp);
      }
      else
      {
        value = szAssign + 1;
        value.Trim(" ");

        line.SetSubString_FromTo(line.GetData(), szAssign);
        // do not use szAssign after this line anymore

        line.ReplaceAll("\t", " ");
        line.ReplaceAll("  ", " ");
        line.Trim(" ");

        const bool bOverride = line.TrimWordStart("override ");
        line.Trim(" ");

        if (line.StartsWith("int "))
        {
          key.SetSubString_FromTo(line.GetData() + 4, line.GetData() + line.GetElementCount());
          key.Trim(" ");

          if (bOverride && !intData.Contains(key))
            WLog::Error("Config 'int' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && intData.Contains(key))
            WLog::Error("Config 'int' key '{}' is not marked override, but exist already. Use 'override int' instead.", key);

          WInt32 val;
          if (WConversionUtils::StringToInt(value, val).Succeeded())
          {
            intData[key] = val;
          }
          else
          {
            WLog::Error("Failed to parse 'int' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("float "))
        {
          key.SetSubString_FromTo(line.GetData() + 6, line.GetData() + line.GetElementCount());
          key.Trim(" ");

          if (bOverride && !floatData.Contains(key))
            WLog::Error("Config 'float' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && floatData.Contains(key))
            WLog::Error("Config 'float' key '{}' is not marked override, but exist already. Use 'override float' instead.", key);

          double val;
          if (WConversionUtils::StringToFloat(value, val).Succeeded())
          {
            floatData[key] = (float)val;
          }
          else
          {
            WLog::Error("Failed to parse 'float' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("bool "))
        {
          key.SetSubString_FromTo(line.GetData() + 5, line.GetData() + line.GetElementCount());
          key.Trim(" ");

          if (bOverride && !boolData.Contains(key))
            WLog::Error("Config 'bool' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && boolData.Contains(key))
            WLog::Error("Config 'bool' key '{}' is not marked override, but exist already. Use 'override bool' instead.", key);

          bool val;
          if (WConversionUtils::StringToBool(value, val).Succeeded())
          {
            boolData[key] = val;
          }
          else
          {
            WLog::Error("Failed to parse 'bool' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("string "))
        {
          key.SetSubString_FromTo(line.GetData() + 7, line.GetData() + line.GetElementCount());
          key.Trim(" ");

          if (bOverride && !stringData.Contains(key))
            WLog::Error("Config 'string' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && stringData.Contains(key))
            WLog::Error("Config 'string' key '{}' is not marked override, but exist already. Use 'override string' instead.", key);

          if (!value.StartsWith("\"") || !value.EndsWith("\""))
          {
            WLog::Error("Failed to parse 'string' in config file: '{}'", tmp);
          }
          else
          {
            value.Shrink(1, 1);
            stringData[key] = value;
          }
        }
        else
        {
          WLog::Error("Invalid line in config file: '{}'", tmp);
        }
      }
    }
  }
  else
  {
    W_LOG_BLOCK("Invalid Config File", pResource->GetResourceIdOrDescription());
    WLog::Error(errorBuffer.m_sBuffer);
    // empty stream
    return {};
  }

  WResourceLoadData res;
  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  WFileStats stat;
  if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_sResourceDescription = stat.m_sName;
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }
#endif

  WMemoryStreamWriter writer(&pData->m_Storage);

  pData->m_RequiredFiles.StoreCurrentTimeStamp();
  pData->m_RequiredFiles.WriteDependencyFile(writer).IgnoreResult();
  writer.WriteMap(intData).IgnoreResult();
  writer.WriteMap(floatData).IgnoreResult();
  writer.WriteMap(stringData).IgnoreResult();
  writer.WriteMap(boolData).IgnoreResult();

  return res;
}

void WConfigFileResourceLoader::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  W_IGNORE_UNUSED(pResource);

  LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);

  W_DEFAULT_DELETE(pData);
}

bool WConfigFileResourceLoader::IsResourceOutdated(const WResource* pResource) const
{
  return static_cast<const WConfigFileResource*>(pResource)->m_RequiredFiles.HasAnyFileChanged();
}


W_STATICLINK_FILE(Utilities, Utilities_Resources_ConfigFileResource);
