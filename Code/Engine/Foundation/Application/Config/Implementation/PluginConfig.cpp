#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WApplicationPluginConfig, WNoBase, 1, WRTTIDefaultAllocator<WApplicationPluginConfig>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Plugins", m_Plugins),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WApplicationPluginConfig_PluginConfig, WNoBase, 1, WRTTIDefaultAllocator<WApplicationPluginConfig_PluginConfig>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RelativePath", m_sAppDirRelativePath),
    W_MEMBER_PROPERTY("LoadCopy", m_bLoadCopy),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool WApplicationPluginConfig::PluginConfig::operator<(const PluginConfig& rhs) const
{
  return m_sAppDirRelativePath < rhs.m_sAppDirRelativePath;
}

bool WApplicationPluginConfig::AddPlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (WUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      return false;
    }
  }

  m_Plugins.PushBack(cfg);
  return true;
}

bool WApplicationPluginConfig::RemovePlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (WUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      m_Plugins.RemoveAtAndSwap(i);
      return true;
    }
  }

  return false;
}

WApplicationPluginConfig::WApplicationPluginConfig() = default;

WResult WApplicationPluginConfig::Save(WStringView sPath) const
{
  m_Plugins.Sort();

  WDeferredFileWriter file;
  file.SetOutput(sPath, true);

  WOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::Compliant);

  for (WUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    writer.BeginObject("Plugin");

    WOpenDdlUtils::StoreString(writer, m_Plugins[i].m_sAppDirRelativePath, "Path");
    WOpenDdlUtils::StoreBool(writer, m_Plugins[i].m_bLoadCopy, "LoadCopy");

    writer.EndObject();
  }

  return file.Close();
}

void WApplicationPluginConfig::Load(WStringView sPath)
{
  W_LOG_BLOCK("WApplicationPluginConfig::Load()");

  m_Plugins.Clear();

  WFileReader file;
  if (file.Open(sPath).Failed())
  {
    WLog::Warning("Could not open plugins config file '{0}'", sPath);
    return;
  }

  WOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    WLog::Error("Failed to parse plugins config file '{0}'", sPath);
    return;
  }

  const WOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const WOpenDdlReaderElement* pPlugin = pTree->GetFirstChild(); pPlugin != nullptr; pPlugin = pPlugin->GetSibling())
  {
    if (!pPlugin->IsCustomType("Plugin"))
      continue;

    PluginConfig cfg;

    const WOpenDdlReaderElement* pPath = pPlugin->FindChildOfType(WOpenDdlPrimitiveType::String, "Path");
    const WOpenDdlReaderElement* pCopy = pPlugin->FindChildOfType(WOpenDdlPrimitiveType::Bool, "LoadCopy");

    if (pPath)
    {
      cfg.m_sAppDirRelativePath = pPath->GetPrimitivesString()[0];
    }

    if (pCopy)
    {
      cfg.m_bLoadCopy = pCopy->GetPrimitivesBool()[0];
    }

    // this prevents duplicates
    AddPlugin(cfg);
  }
}

void WApplicationPluginConfig::Apply()
{
  W_LOG_BLOCK("WApplicationPluginConfig::Apply");

  for (const auto& var : m_Plugins)
  {
    WBitflags<WPluginLoadFlags> flags;
    flags.AddOrRemove(WPluginLoadFlags::LoadCopy, var.m_bLoadCopy);
    flags.AddOrRemove(WPluginLoadFlags::CustomDependency, false);

    WPlugin::LoadPlugin(var.m_sAppDirRelativePath, flags).IgnoreResult();
  }
}



W_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_PluginConfig);
