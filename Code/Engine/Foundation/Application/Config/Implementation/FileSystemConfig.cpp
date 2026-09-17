#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WApplicationFileSystemConfig, WNoBase, 1, WRTTIDefaultAllocator<WApplicationFileSystemConfig>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("DataDirs", m_DataDirs),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WApplicationFileSystemConfig_DataDirConfig, WNoBase, 1, WRTTIDefaultAllocator<WApplicationFileSystemConfig_DataDirConfig>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RelativePath", m_sDataDirSpecialPath),
    W_MEMBER_PROPERTY("Writable", m_bWritable),
    W_MEMBER_PROPERTY("RootName", m_sRootName),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WApplicationFileSystemConfig::Save(WStringView sPath)
{
  WFileWriter file;
  if (file.Open(sPath).Failed())
    return W_FAILURE;

  WOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::Compliant);

  for (WUInt32 i = 0; i < m_DataDirs.GetCount(); ++i)
  {
    writer.BeginObject("DataDir");

    WOpenDdlUtils::StoreString(writer, m_DataDirs[i].m_sDataDirSpecialPath, "Path");
    WOpenDdlUtils::StoreString(writer, m_DataDirs[i].m_sRootName, "RootName");
    WOpenDdlUtils::StoreBool(writer, m_DataDirs[i].m_bWritable, "Writable");

    writer.EndObject();
  }

  return W_SUCCESS;
}

void WApplicationFileSystemConfig::Load(WStringView sPath)
{
  W_LOG_BLOCK("WApplicationFileSystemConfig::Load()");

  m_DataDirs.Clear();

  WFileReader file;
  if (file.Open(sPath).Failed())
  {
    WLog::Dev("File-system config file '{0}' does not exist.", sPath);
    return;
  }

  WOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    WLog::Error("Failed to parse file-system config file '{0}'", sPath);
    return;
  }

  const WOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const WOpenDdlReaderElement* pDirs = pTree->GetFirstChild(); pDirs != nullptr; pDirs = pDirs->GetSibling())
  {
    if (!pDirs->IsCustomType("DataDir"))
      continue;

    DataDirConfig cfg;
    cfg.m_bWritable = false;

    const WOpenDdlReaderElement* pPath = pDirs->FindChildOfType(WOpenDdlPrimitiveType::String, "Path");
    const WOpenDdlReaderElement* pRoot = pDirs->FindChildOfType(WOpenDdlPrimitiveType::String, "RootName");
    const WOpenDdlReaderElement* pWrite = pDirs->FindChildOfType(WOpenDdlPrimitiveType::Bool, "Writable");

    if (pPath)
      cfg.m_sDataDirSpecialPath = pPath->GetPrimitivesString()[0];
    if (pRoot)
      cfg.m_sRootName = pRoot->GetPrimitivesString()[0];
    if (pWrite)
      cfg.m_bWritable = pWrite->GetPrimitivesBool()[0];

    /// \todo Temp fix for backwards compatibility
    {
      if (cfg.m_sRootName == "project")
      {
        cfg.m_sDataDirSpecialPath = ">project/";
      }
      else if (cfg.m_sDataDirSpecialPath.StartsWith_NoCase(":project/"))
      {
        WStringBuilder temp(">project/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath.GetData() + 9);
        cfg.m_sDataDirSpecialPath = temp;
      }
      else if (cfg.m_sDataDirSpecialPath.StartsWith_NoCase(":sdk/"))
      {
        WStringBuilder temp(">sdk/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath.GetData() + 5);
        cfg.m_sDataDirSpecialPath = temp;
      }
      else if (!cfg.m_sDataDirSpecialPath.StartsWith_NoCase(">sdk/"))
      {
        WStringBuilder temp(">sdk/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath);
        cfg.m_sDataDirSpecialPath = temp;
      }
    }

    m_DataDirs.PushBack(cfg);
  }
}

void WApplicationFileSystemConfig::Apply()
{
  W_LOG_BLOCK("WApplicationFileSystemConfig::Apply");

  // WStringBuilder s;

  // Make sure previous calls to Apply do not accumulate
  Clear();

  for (const auto& var : m_DataDirs)
  {
    // if (WFileSystem::ResolveSpecialDirectory(var.m_sDataDirSpecialPath, s).Succeeded())
    {
      WFileSystem::AddDataDirectory(var.m_sDataDirSpecialPath, "AppFileSystemConfig", var.m_sRootName, (!var.m_sRootName.IsEmpty() && var.m_bWritable) ? WDataDirUsage::AllowWrites : WDataDirUsage::ReadOnly).IgnoreResult();
    }
  }
}


void WApplicationFileSystemConfig::Clear()
{
  WFileSystem::RemoveDataDirectoryGroup("AppFileSystemConfig");
}

WResult WApplicationFileSystemConfig::CreateDataDirStubFiles()
{
  W_LOG_BLOCK("WApplicationFileSystemConfig::CreateDataDirStubFiles");

  WStringBuilder s;
  WResult res = W_SUCCESS;

  for (const auto& var : m_DataDirs)
  {
    if (WFileSystem::ResolveSpecialDirectory(var.m_sDataDirSpecialPath, s).Failed())
    {
      WLog::Error("Failed to get special directory '{0}'", var.m_sDataDirSpecialPath);
      res = W_FAILURE;
      continue;
    }

    s.AppendPath("DataDir.WManifest");

    WOSFile file;
    if (file.Open(s, WFileOpenMode::Write).Failed())
    {
      WLog::Error("Failed to create stub file '{0}'", s);
      res = W_FAILURE;
    }
  }

  return W_SUCCESS;
}



W_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_FileSystemConfig);
