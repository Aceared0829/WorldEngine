#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/TraceWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Platform/PlatformDesc.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Utilities/CommandLineOptions.h>

WCommandLineOptionBool opt_DisableConsoleOutput("app", "-disableConsoleOutput", "Disables logging to the standard console window.", false);
WCommandLineOptionInt opt_TelemetryPort("app", "-TelemetryPort", "The network port over which telemetry is sent.", WTelemetry::s_uiPort);
WCommandLineOptionString opt_Profile("app", "-profile", "The platform profile to use.", "Default");

WString WGameApplicationBase::GetBaseDataDirectoryPath() const
{
  return ">sdk/Data/Base";
}

WString WGameApplicationBase::GetProjectDataDirectoryPath() const
{
  return ">project/";
}

void WGameApplicationBase::ExecuteInitFunctions()
{
  Init_PlatformProfile_SetPreferred();
  Init_ConfigureTelemetry();
  Init_FileSystem_SetSpecialDirs();
  Init_LoadRequiredPlugins();
  Init_ConfigureAssetManagement();
  Init_FileSystem_ConfigureDataDirs();
  Init_LoadWorldModuleConfig();
  Init_LoadProjectPlugins();
  Init_PlatformProfile_LoadForRuntime();
  Init_ConfigureTags();
  Init_ConfigureCVars();
  Init_SetupGraphicsDevice();
  Init_SetupDefaultResources();
}

void WGameApplicationBase::Init_PlatformProfile_SetPreferred()
{
  if (opt_Profile.IsOptionSpecified())
  {
    m_PlatformProfile.SetConfigName(opt_Profile.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));
  }
  else
  {
    m_PlatformProfile.SetConfigName(WPlatformDesc::GetThisPlatformDesc().GetName());

    const WStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".WProfile");

    if (!WFileSystem::ExistsFile(sRuntimeProfileFile))
    {
      WLog::Info("Platform profile '{}' doesn't exist, switching to 'Default'", m_PlatformProfile.GetConfigName());

      m_PlatformProfile.SetConfigName("Default");
    }
  }

  m_PlatformProfile.AddMissingConfigs();
}

void WGameApplicationBase::BaseInit_ConfigureLogging()
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  WGlobalLog::RemoveLogWriter(m_LogToVsID);
  WGlobalLog::RemoveLogWriter(m_LogToTracingID);

  if (!opt_DisableConsoleOutput.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    m_LogToConsoleID = WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
  }

  m_LogToVsID = WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
  m_LogToTracingID = WGlobalLog::AddLogWriter(WLogWriter::Tracing::LogMessageHandler);
#endif
}

void WGameApplicationBase::Init_ConfigureTelemetry()
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WTelemetry::s_uiPort = static_cast<WUInt16>(opt_TelemetryPort.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));
  WTelemetry::SetServerName(GetApplicationName());
  WTelemetry::CreateServer();
#endif
}

void WGameApplicationBase::Init_FileSystem_SetSpecialDirs()
{
  WFileSystem::SetSpecialDirectory("project", FindProjectDirectory());
}

void WGameApplicationBase::Init_ConfigureAssetManagement() {}

void WGameApplicationBase::Init_LoadRequiredPlugins()
{
  WPlugin::InitializeStaticallyLinkedPlugins();

#if W_ENABLED(W_PLATFORM_WINDOWS)
  WPlugin::LoadPlugin("XBoxControllerPlugin", WPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif
}

void WGameApplicationBase::Init_FileSystem_ConfigureDataDirs()
{
  // ">appdir/" and ">user/" are built-in special directories
  // see WFileSystem::ResolveSpecialDirectory

  const WStringBuilder sUserDataPath(">user/", GetApplicationName());

  WFileSystem::CreateDirectoryStructure(sUserDataPath).AssertSuccess();

  WString writableBinRoot = ">appdir/";
  WString shaderCacheRoot = ">sdk/Output/";

#if W_DISABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // On platforms where this is disabled, one can usually only write to the user directory
  // e.g. on mobile platforms
  writableBinRoot = sUserDataPath;
#endif

  WFileSystem::CreateDirectoryStructure(shaderCacheRoot).IgnoreResult();

  // for absolute paths, read-only
  WFileSystem::AddDataDirectory("", "GameApplicationBase", ":", WDataDirUsage::ReadOnly).AssertSuccess();

  // ":bin/" : writing to the binary directory
  WFileSystem::AddDataDirectory(writableBinRoot, "GameApplicationBase", "bin", WDataDirUsage::AllowWrites).AssertSuccess();

  // ":shadercache/" for reading and writing shader files
#if W_DISABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  WFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", WDataDirUsage::ReadOnly).AssertSuccess();
#else
  WFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", WDataDirUsage::AllowWrites).AssertSuccess();
#endif

  // ":appdata/" for reading and writing app user data
  WFileSystem::AddDataDirectory(sUserDataPath, "GameApplicationBase", "appdata", WDataDirUsage::AllowWrites).AssertSuccess();

  // ":base/" for reading the core engine files
  WFileSystem::AddDataDirectory(GetBaseDataDirectoryPath(), "GameApplicationBase", "base", WDataDirUsage::ReadOnly).IgnoreResult();

  {
    // the default config path is ":project/...", which would require the project data directory to be mounted already,
    // but that only happens further below, so the path has to be resolved to an absolute one instead
    // (the ":" data directory above is what makes absolute paths readable)
    WStringBuilder sConfigFile;
    WFileSystem::ResolveSpecialDirectory(GetProjectDataDirectoryPath(), sConfigFile).IgnoreResult();
    sConfigFile.AppendPath("RuntimeConfigs/DataDirectories.ddl");

    WApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load(sConfigFile);

    // get rid of duplicates that we already hard-coded above
    for (WUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const WString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") || name.IsEqual_NoCase("base"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    // ":project/" is deliberately NOT mounted before the config is applied, because its position *within* the
    // config matters. Data directories are searched back to front, and the editor lists the data directories of
    // the active plugin bundles before the project, so that a project can override plugin provided files
    // (e.g. ship its own version of a particle shader). Mounting the project up front would put every
    // directory from the config, including those, above it again.
    // The config entry is only patched up, so that the (virtual) GetProjectDataDirectoryPath() still decides
    // where the project is, and so that it stays read-only, which is what a shipped game wants.
    WUInt32 uiProjectIdx = WInvalidIndex;
    for (WUInt32 i = 0; i < appFileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      const WString name = appFileSystemConfig.m_DataDirs[i].m_sRootName;
      if (name.IsEqual_NoCase("project"))
      {
        uiProjectIdx = i;
        break;
      }
    }

    if (uiProjectIdx == WInvalidIndex)
    {
      // no config file at all, or one that doesn't mention the project directory
      WApplicationFileSystemConfig::DataDirConfig cfg;
      cfg.m_sRootName = "project";
      cfg.m_bHardCodedDependency = true;

      uiProjectIdx = appFileSystemConfig.m_DataDirs.GetCount();
      appFileSystemConfig.m_DataDirs.PushBack(cfg);
    }

    appFileSystemConfig.m_DataDirs[uiProjectIdx].m_sDataDirSpecialPath = GetProjectDataDirectoryPath();
    appFileSystemConfig.m_DataDirs[uiProjectIdx].m_bWritable = false;

    appFileSystemConfig.Apply();
  }

  {
    // We need the file system before we can start the html logger.

    WGlobalLog::RemoveLogWriter(m_LogToHTML);
    WStringBuilder sLogFile;
    sLogFile.SetFormat(":appdata/Log.htm");
    m_LogHTML.BeginLog(sLogFile, GetApplicationName());

    m_LogToHTML = WGlobalLog::AddLogWriter(WMakeDelegate(&WLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  }
}

void WGameApplicationBase::Init_LoadWorldModuleConfig()
{
  WWorldModuleConfig worldModuleConfig;
  worldModuleConfig.Load();
  worldModuleConfig.Apply();
}

void WGameApplicationBase::Init_LoadProjectPlugins()
{
  WApplicationPluginConfig appPluginConfig;
  appPluginConfig.Load();
  appPluginConfig.Apply();
}

void WGameApplicationBase::Init_PlatformProfile_LoadForRuntime()
{
  const WStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".WProfile");
  m_PlatformProfile.AddMissingConfigs();

  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile).IgnoreResult();
}

void WGameApplicationBase::Init_ConfigureTags()
{
  W_LOG_BLOCK("Reading Tags", "Tags.ddl");

  WStringView sFile = ":project/RuntimeConfigs/Tags.ddl";

  WFileReader file;
  if (file.Open(sFile).Failed())
  {
    WLog::Dev("'{}' does not exist", sFile);
    return;
  }

  WStringBuilder tmp;

  WOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
  {
    WLog::Error("Failed to parse DDL data in tags file");
    return;
  }

  const WOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const WOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const WOpenDdlReaderElement* pName = pTags->FindChildOfType(WOpenDdlPrimitiveType::String, "Name");

    if (!pName)
    {
      WLog::Error("Incomplete tag declaration!");
      continue;
    }

    tmp = pName->GetPrimitivesString()[0];
    WTagRegistry::GetGlobalRegistry().RegisterTag(tmp);
  }
}

void WGameApplicationBase::Init_ConfigureCVars()
{
  WCVar::SetStorageFolder(":appdata/CVars");
  WCVar::LoadCVars();
}

void WGameApplicationBase::Init_SetupDefaultResources()
{
  // continuously unload resources that are not in use anymore
  WResourceManager::SetAutoFreeUnused(WTime::MakeFromMicroseconds(100), WTime::MakeFromSeconds(10.0f));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void WGameApplicationBase::Deinit_UnloadPlugins()
{
  WPlugin::UnloadAllPlugins();
}

void WGameApplicationBase::Deinit_ShutdownLogging()
{
#if W_DISABLED(W_COMPILE_FOR_DEVELOPMENT)
  // during development, keep these loggers active
  WGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  WGlobalLog::RemoveLogWriter(m_LogToVsID);
  WGlobalLog::RemoveLogWriter(m_LogToTracingID);
#endif

  WGlobalLog::RemoveLogWriter(m_LogToHTML);
  m_LogHTML.EndLog();
}
