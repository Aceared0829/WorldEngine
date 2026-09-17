#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

WResult WProjectExport::ClearTargetFolder(const char* szAbsFolderPath)
{
  if (WOSFile::DeleteFolder(szAbsFolderPath).Failed())
  {
    WLog::Error("Target folder could not be removed:\n'{}'", szAbsFolderPath);
    return W_FAILURE;
  }

  if (WOSFile::CreateDirectoryStructure(szAbsFolderPath).Failed())
  {
    WLog::Error("Target folder could not be created:\n'{}'", szAbsFolderPath);
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WProjectExport::ScanFolder(WSet<WString>& out_Files, const char* szFolder, const WPathPatternFilter& filter, WAssetCurator* pCurator, WDynamicArray<WString>* pSceneFiles, const WPlatformProfile* pPlatformProfile)
{
  WStringBuilder sRootFolder = szFolder;
  sRootFolder.Trim("/\\");

  const WUInt32 uiRootFolderLength = sRootFolder.GetElementCount();

  WStringBuilder sAbsFilePath, sRelFilePath;

  WFileSystemIterator it;
  for (it.StartSearch(sRootFolder, WFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
  {
    if (WProgress::GetGlobalProgressbar()->WasCanceled())
    {
      WLog::Warning("Folder scanning canceled by user");
      return W_FAILURE;
    }

    it.GetStats().GetFullPath(sAbsFilePath);

    sRelFilePath = sAbsFilePath;
    sRelFilePath.Shrink(uiRootFolderLength, 0); // keep the slash at the front -> useful for the pattern filter

    WStringBuilder filterRule;

    if (!filter.PassesFilters(sRelFilePath, &filterRule))
    {
      if (it.GetStats().m_bIsDirectory)
      {
        WLog::Info(" Skipping folder '{}' - doesn't pass filter rule '{}'.", sRelFilePath, filterRule);
        it.SkipFolder();
      }
      else
      {
        WLog::Info(" Skipping file '{}' - doesn't pass filter rule '{}'.", sRelFilePath, filterRule);
        it.Next();
      }

      continue;
    }

    if (it.GetStats().m_bIsDirectory)
    {
      it.Next();
      continue;
    }

    if (pCurator)
    {
      auto asset = pCurator->FindSubAsset(sAbsFilePath);

      if (asset.isValid() && asset->m_bMainAsset)
      {
        // redirect to asset output
        WAssetDocumentManager* pAssetMan = WStaticCast<WAssetDocumentManager*>(asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_pManager);

        sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_Path, nullptr, pPlatformProfile);

        sRelFilePath.Prepend("AssetCache/");
        out_Files.Insert(sRelFilePath);

        if (pSceneFiles && asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName == "Scene")
        {
          pSceneFiles->PushBack(sRelFilePath);
        }

        for (const WString& outputTag : asset->m_pAssetInfo->m_Info->m_Outputs)
        {
          sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_Path, outputTag, pPlatformProfile);

          sRelFilePath.Prepend("AssetCache/");
          out_Files.Insert(sRelFilePath);
        }

        it.Next();
        continue;
      }
    }

    out_Files.Insert(sRelFilePath);
    it.Next();
  }

  return W_SUCCESS;
}

WResult WProjectExport::CopyFiles(const char* szSrcFolder, const char* szDstFolder, const WSet<WString>& files, WProgressRange* pProgressRange)
{
  WLog::Info("Source folder: {}", szSrcFolder);
  WLog::Info("Destination folder: {}", szDstFolder);

  WStringBuilder sSrc, sDst;

  for (auto itFile = files.GetIterator(); itFile.IsValid(); ++itFile)
  {
    if (WProgress::GetGlobalProgressbar()->WasCanceled())
    {
      WLog::Info("File copy operation canceled by user.");
      return W_FAILURE;
    }

    if (pProgressRange)
    {
      pProgressRange->BeginNextStep(itFile.Key());
    }

    sSrc.Set(szSrcFolder, "/", itFile.Key());
    sDst.Set(szDstFolder, "/", itFile.Key());

    if (WOSFile::CopyFile(sSrc, sDst).Succeeded())
    {
      WLog::Info(" Copied: {}", itFile.Key());
    }
    else
    {
      WLog::Error(" Copy failed: {}", itFile.Key());
    }
  }

  WLog::Success("Finished copying files to destination '{}'", szDstFolder);
  return W_SUCCESS;
}

WResult WProjectExport::GatherGeneratedAssetManagerFiles(WSet<WString>& out_Files)
{
  WTempHybridArray<WString, 4> addFiles;

  for (auto pMan : WDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = WDynamicCast<WAssetDocumentManager*>(pMan))
    {
      pAssMan->GetAdditionalOutputs(addFiles).AssertSuccess();

      for (const auto& file : addFiles)
      {
        out_Files.Insert(file);
      }

      addFiles.Clear();
    }
  }

  return W_SUCCESS;
}

WResult WProjectExport::CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile)
{
  if (WFileSystem::ExistsFile(szExpectedFile))
    return W_SUCCESS;

  WStringBuilder src;
  src.Set("#include <", szFallbackFile, ">\n\n\n[EXCLUDE]\n\n// TODO: add exclude patterns\n\n\n[INCLUDE]\n\n//TODO: add include patterns\n\n\n");

  WFileWriter file;
  if (file.Open(szExpectedFile).Failed())
  {
    WLog::Error("Failed to open '{}' for writing.", szExpectedFile);
    return W_FAILURE;
  }

  file.WriteBytes(src.GetData(), src.GetElementCount()).AssertSuccess();
  return W_SUCCESS;
}

WResult WProjectExport::ReadExportFilters(WPathPatternFilter& out_DataFilter, WPathPatternFilter& out_BinariesFilter, const WPlatformProfile* pPlatformProfile)
{
  WStringBuilder sDefine;
  sDefine.SetFormat("PLATFORM_PROFILE_{} 1", pPlatformProfile->GetConfigName());
  sDefine.ToUpper();

  WTempHybridArray<WString, 1> ppDefines;
  ppDefines.PushBack(sDefine);

  if (WProjectExport::CreateExportFilterFile(":project/ProjectData.WExportFilter", "CommonData.WExportFilter").Failed())
  {
    WLog::Error("The file 'ProjectData.WExportFilter' could not be created.");
    return W_FAILURE;
  }

  if (WProjectExport::CreateExportFilterFile(":project/ProjectBinaries.WExportFilter", "CommonBinaries.WExportFilter").Failed())
  {
    WLog::Error("The file 'ProjectBinaries.WExportFilter' could not be created.");
    return W_FAILURE;
  }

  if (out_DataFilter.ReadConfigFile("ProjectData.WExportFilter", ppDefines).Failed())
  {
    WLog::Error("The file 'ProjectData.WExportFilter' could not be read.");
    return W_FAILURE;
  }

  if (out_BinariesFilter.ReadConfigFile("ProjectBinaries.WExportFilter", ppDefines).Failed())
  {
    WLog::Error("The file 'ProjectBinaries.WExportFilter' could not be read.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WProjectExport::CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory)
{
  WApplicationFileSystemConfig cfg;

  WStringBuilder sPath;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    const auto& info = itDir.Value();

    if (info.m_sTargetDirRootName == "-")
      continue;

    sPath.Set(">sdk/", info.m_sTargetDirPath);

    auto& ddc = cfg.m_DataDirs.ExpandAndGetRef();
    ddc.m_sDataDirSpecialPath = sPath;
    ddc.m_sRootName = info.m_sTargetDirRootName;
  }

  sPath.Set(szTargetDirectory, "/Data/project/RuntimeConfigs/DataDirectories.ddl");

  if (cfg.Save(sPath).Failed())
  {
    WLog::Error("Failed to write DataDirectories.ddl file.");
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WProjectExport::GatherAssetLookupTableFiles(DirectoryMapping& mapping, const WApplicationFileSystemConfig& dirConfig, const WPlatformProfile* pPlatformProfile)
{
  WStringBuilder sDataDirPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    if (WFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      WLog::Error("Failed to resolve data directory path '{}'", dataDir.m_sDataDirSpecialPath);
      return W_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    WStringBuilder sAidltPath("AssetCache/", pPlatformProfile->GetConfigName(), ".WAidlt");

    mapping[sDataDirPath].m_Files.Insert(sAidltPath);
  }

  return W_SUCCESS;
}

WResult WProjectExport::ScanDataDirectories(DirectoryMapping& mapping, const WApplicationFileSystemConfig& dirConfig, const WPathPatternFilter& dataFilter, WDynamicArray<WString>* pSceneFiles, const WPlatformProfile* pPlatformProfile)
{
  WProgressRange progress("Scanning data directories", dirConfig.m_DataDirs.GetCount(), true);

  WUInt32 uiDataDirNumber = 1;

  WStringBuilder sDataDirPath, sDstPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    progress.BeginNextStep(dataDir.m_sDataDirSpecialPath);

    if (WFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      WLog::Error("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath);
      return W_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    WProjectExport::DataDirectory& ddInfo = mapping[sDataDirPath];

    if (!dataDir.m_sRootName.IsEmpty())
    {
      sDstPath.Set("Data/", dataDir.m_sRootName);

      ddInfo.m_sTargetDirRootName = dataDir.m_sRootName;
      ddInfo.m_sTargetDirPath = sDstPath;
    }
    else
    {
      sDstPath.SetFormat("Data/Extra{}", uiDataDirNumber);
      ++uiDataDirNumber;

      ddInfo.m_sTargetDirPath = sDstPath;
    }

    W_SUCCEED_OR_RETURN(WProjectExport::ScanFolder(ddInfo.m_Files, sDataDirPath, dataFilter, WAssetCurator::GetSingleton(), pSceneFiles, pPlatformProfile));
  }

  return W_SUCCESS;
}

WResult WProjectExport::CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory)
{
  WUInt32 uiTotalFiles = 0;
  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
    uiTotalFiles += itDir.Value().m_Files.GetCount();

  WProgressRange range("Copying files", uiTotalFiles, true);

  WLog::Info("Copying files to target directory '{}'", szTargetDirectory);

  WStringBuilder sTargetFolder;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    sTargetFolder.Set(szTargetDirectory, "/", itDir.Value().m_sTargetDirPath);

    if (WProjectExport::CopyFiles(itDir.Key(), sTargetFolder, itDir.Value().m_Files, &range).Failed())
      return W_FAILURE;
  }

  WLog::Success("Finished copying all files.");
  return W_SUCCESS;
}

WResult WProjectExport::GatherBinaries(DirectoryMapping& mapping, const WPathPatternFilter& filter)
{
  WStringBuilder sAppDir;
  sAppDir = WOSFile::GetApplicationDirectory();
  sAppDir.MakeCleanPath();
  sAppDir.Trim("/\\");

  WProjectExport::DataDirectory& ddInfo = mapping[sAppDir];
  ddInfo.m_sTargetDirPath = "Bin";
  ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

  if (WProjectExport::ScanFolder(ddInfo.m_Files, sAppDir, filter, nullptr, nullptr, nullptr).Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WString WProjectExport::FindCustomGameExecutable()
{
  if (!WFileSystem::ExistsFile(":project/Editor/CppProject.ddl"))
    return {};

  WCppSettings cppSettings;
  if (cppSettings.Load().Failed() || cppSettings.m_sPluginName.IsEmpty())
    return {};

  // this has to match the name of the application target that the C++ project generation creates
  WStringBuilder sExeName(cppSettings.m_sPluginName, "Game");

#if W_ENABLED(W_PLATFORM_WINDOWS)
  sExeName.Append(".exe");
#endif

  WStringBuilder sExePath = WOSFile::GetApplicationDirectory();
  sExePath.AppendPath(sExeName);
  sExePath.MakeCleanPath();

  if (!WOSFile::ExistsFile(sExePath))
  {
    WLog::Info("The project's game executable '{}' hasn't been built, exporting WPlayer instead.", sExeName);
    return {};
  }

  return sExeName;
}

WResult WProjectExport::CreateLaunchConfig(const WDynamicArray<WString>& sceneFiles, const char* szTargetDirectory, WStringView sCustomGameExecutable)
{
  auto WriteScript = [szTargetDirectory](WStringView sScriptName, WStringView sCommand) -> WResult
  {
    WStringBuilder sScriptPath;
    sScriptPath.SetFormat("{}/{}.bat", szTargetDirectory, sScriptName);

    WOSFile file;
    if (file.Open(sScriptPath, WFileOpenMode::Write).Failed())
    {
      WLog::Error("Couldn't create '{}'", sScriptPath);
      return W_FAILURE;
    }

    return file.Write(sCommand.GetStartPointer(), sCommand.GetElementCount());
  };

  if (!sCustomGameExecutable.IsEmpty())
  {
    // the game executable decides itself which scene to load (see WGameState::GetStartupOptions()),
    // so a script per scene would be pointless here
    WStringBuilder cmd;
    cmd.SetFormat("start Bin/{} -project \"Data/project\"", sCustomGameExecutable);

    WStringBuilder sScriptName("Launch ", WPathUtils::GetFileName(sCustomGameExecutable));
    return WriteScript(sScriptName, cmd);
  }

  for (const auto& sf : sceneFiles)
  {
    WStringBuilder cmd;
    cmd.SetFormat("start Bin/WPlayer.exe -project \"Data/project\" -scene \"{}\"", sf);

    WStringBuilder sScriptName("Launch ", WPathUtils::GetFileName(sf));
    W_SUCCEED_OR_RETURN(WriteScript(sScriptName, cmd));
  }

  return W_SUCCESS;
}

WResult WProjectExport::GatherGeneratedAssetFiles(WSet<WString>& out_Files, const char* szProjectDirectory)
{
  WStringBuilder sRoot(szProjectDirectory, "/AssetCache/Generated");

  WPathPatternFilter filter;
  WSet<WString> files;
  W_SUCCEED_OR_RETURN(ScanFolder(files, sRoot, filter, nullptr, nullptr, nullptr));

  WStringBuilder sFilePath;

  for (const auto& file : files)
  {
    sFilePath.Set("/AssetCache/Generated", file);
    out_Files.Insert(sFilePath);
  }

  return W_SUCCESS;
}

void WProjectExport::AddPackageDependenciesToFileList(DirectoryMapping& ref_fileList)
{
  auto knownAssets = WAssetCurator::GetSingleton()->GetKnownAssets();

  WStringBuilder sAbsPath, sRelPath;

  for (auto it = knownAssets->GetIterator(); it.IsValid(); ++it)
  {
    const WAssetInfo* pAssetInfo = it.Value();
    if (pAssetInfo->m_Info == nullptr)
      continue;

    for (const WString& dep : pAssetInfo->m_Info->m_PackageDependencies)
    {
      // GUIDs reference other assets which ScanFolder already handles via the asset curator
      if (WConversionUtils::IsStringUuid(dep))
        continue;

      if (WFileSystem::ResolvePath(dep, &sAbsPath, nullptr).Failed())
        continue;

      sAbsPath.MakeCleanPath();

      // Find which data directory this file belongs to and add it directly to that directory's file list
      for (auto itDir = ref_fileList.GetIterator(); itDir.IsValid(); ++itDir)
      {
        if (sAbsPath.StartsWith_NoCase(itDir.Key()))
        {
          sRelPath = sAbsPath;
          sRelPath.Shrink(itDir.Key().GetElementCount(), 0); // keeps the leading slash
          itDir.Value().m_Files.Insert(sRelPath);
          break;
        }
      }
    }
  }
}

WStatus WProjectExport::ExportProjectComplete(WStringView sTargetDirectory, const WProjectExportOptions& options, WStringBuilder* out_pLog)
{
  if (sTargetDirectory.IsEmpty())
    return WStatus("No target directory given.");

  if (!WPathUtils::IsAbsolutePath(sTargetDirectory))
    return WStatus(WFmt("The target directory '{}' is not an absolute path.", sTargetDirectory));

  const WString sTargetDir = sTargetDirectory;

  WLogSystemToBuffer logBuffer;
  WStatus result = WStatus(W_SUCCESS);

  {
    WLogSystemScope logScope(&logBuffer);

    if (options.m_bCompileCppPlugin)
    {
      // Does nothing if the project has no C++ code at all. Failing here would leave the export with
      // binaries that don't match the code, so it is not something to continue past.
      if (WCppProject::EnsureCppPluginReady().Failed())
      {
        result = WStatus("Building the project's C++ plugin failed.");
      }
    }

    if (result.Succeeded() && options.m_bTransformAssets)
    {
      const WStatus stat = WAssetCurator::GetSingleton()->TransformAllAssets();

      if (stat.Failed())
      {
        result = WStatus(WFmt("Transforming all assets failed: {}", stat.GetMessageString()));
      }
    }

    if (result.Succeeded())
    {
      if (WProjectExport::ExportProject(sTargetDir, WAssetCurator::GetSingleton()->GetActiveAssetProfile(), WQtEditorApp::GetSingleton()->GetFileSystemConfig(), options.m_bCreateLaunchScripts).Failed())
      {
        result = WStatus("Project export failed.");
      }
    }
  }

  if (out_pLog != nullptr)
  {
    *out_pLog = logBuffer.m_sBuffer;
  }

  // Written last, and only when the directory is there: a failure early on means ExportProject() never
  // got as far as creating it, and an otherwise empty folder containing just a log would look like a
  // half finished export.
  if (WOSFile::ExistsDirectory(sTargetDir))
  {
    WStringBuilder sLogFile(sTargetDir, "/ExportLog.txt");

    WOSFile file;
    if (file.Open(sLogFile, WFileOpenMode::Write).Succeeded())
    {
      file.Write(logBuffer.m_sBuffer.GetData(), logBuffer.m_sBuffer.GetElementCount()).IgnoreResult();
    }
    else
    {
      WLog::Warning("Failed to write the export log '{}'.", sLogFile);
    }
  }

  return result;
}

WResult WProjectExport::ExportProject(const char* szTargetDirectory, const WPlatformProfile* pPlatformProfile, const WApplicationFileSystemConfig& dataDirs, bool bCreateLaunchScripts)
{
  WProgressRange mainProgress("Export Project", 7, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.05f); // Generating special files
  mainProgress.SetStepWeighting(2, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 1.0f);  // Copying files
  mainProgress.SetStepWeighting(5, 0.01f); // Writing data directory config
  mainProgress.SetStepWeighting(6, 0.01f); // Finish up

  WStringBuilder sProjectRootDir;
  WTempHybridArray<WString, 16> sceneFiles;
  WProjectExport::DirectoryMapping fileList;

  WPathPatternFilter dataFilter;
  WPathPatternFilter binariesFilter;

  const WString sCustomGameExecutable = WProjectExport::FindCustomGameExecutable();

  // 0
  {
    mainProgress.BeginNextStep("Preparing output folder");
    W_SUCCEED_OR_RETURN(WProjectExport::ClearTargetFolder(szTargetDirectory));
  }

  // 0
  {
    WFileSystem::ResolveSpecialDirectory(">project", sProjectRootDir).AssertSuccess();
    sProjectRootDir.Trim("/\\");

    W_SUCCEED_OR_RETURN(WProjectExport::GatherAssetLookupTableFiles(fileList, dataDirs, pPlatformProfile));
    W_SUCCEED_OR_RETURN(WProjectExport::ReadExportFilters(dataFilter, binariesFilter, pPlatformProfile));
    W_SUCCEED_OR_RETURN(WProjectExport::GatherGeneratedAssetFiles(fileList[sProjectRootDir].m_Files, sProjectRootDir));
  }

  // 1
  {
    mainProgress.BeginNextStep("Generating special files");
    W_SUCCEED_OR_RETURN(WProjectExport::GatherGeneratedAssetManagerFiles(fileList[sProjectRootDir].m_Files));
  }

  // 2
  {
    mainProgress.BeginNextStep("Scanning data directories");
    W_SUCCEED_OR_RETURN(WProjectExport::ScanDataDirectories(fileList, dataDirs, dataFilter, &sceneFiles, pPlatformProfile));
    WProjectExport::AddPackageDependenciesToFileList(fileList);
  }

  // 3
  {
    // by default all DLLs are excluded by CommonBinaries.WExportFilter
    // we want to override this for all the runtime DLLs and indirect DLL dependencies
    // so we add those to the 'include filter'

    for (auto it : WQtEditorApp::GetSingleton()->GetPluginBundles().m_Plugins)
    {
      if (!it.Value().m_bSelected)
        continue;

      for (const auto& dep : it.Value().m_PackageDependencies)
      {
        binariesFilter.AddFilter(dep, true);
      }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
      for (const auto& dep : it.Value().m_PackageDependenciesDebug)
      {
        binariesFilter.AddFilter(dep, true);
      }
#elif W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      for (const auto& dep : it.Value().m_PackageDependenciesDev)
      {
        binariesFilter.AddFilter(dep, true);
      }
#else
      for (const auto& dep : it.Value().m_PackageDependenciesShipping)
      {
        binariesFilter.AddFilter(dep, true);
      }
#endif
      for (const auto& dep : it.Value().m_RuntimePlugins)
      {
        WStringBuilder tmp = dep;

#if W_ENABLED(W_PLATFORM_WINDOWS)
        tmp.Append(".dll");
#elif W_ENABLED(W_PLATFORM_LINUX)
        tmp.Append(".so");
#else
#  error "Platform not implemented"
#endif

        binariesFilter.AddFilter(tmp, true);
      }
    }

    if (!sCustomGameExecutable.IsEmpty())
    {
      // the export filters exclude all executables by default, so the project's own game executable
      // has to be added explicitly, and WPlayer isn't needed anymore
      WStringBuilder sPattern("/", sCustomGameExecutable);
      binariesFilter.AddFilter(sPattern, true);

      for (WUInt32 i = binariesFilter.m_IncludePatterns.GetCount(); i > 0; --i)
      {
        if (binariesFilter.m_IncludePatterns[i - 1].m_sString.IsEqual_NoCase("/WPlayer.exe"))
        {
          binariesFilter.m_IncludePatterns.RemoveAtAndCopy(i - 1);
        }
      }

      WLog::Info("Exporting '{}' as the game executable, WPlayer is not exported.", sCustomGameExecutable);
    }

    mainProgress.BeginNextStep("Gathering binaries");
    W_SUCCEED_OR_RETURN(WProjectExport::GatherBinaries(fileList, binariesFilter));
  }

  // 4
  {
    mainProgress.BeginNextStep("Copying files");
    W_SUCCEED_OR_RETURN(WProjectExport::CopyAllFiles(fileList, szTargetDirectory));
  }

  // 5
  {
    mainProgress.BeginNextStep("Writing data directory config");
    W_SUCCEED_OR_RETURN(WProjectExport::CreateDataDirectoryDDL(fileList, szTargetDirectory));
  }

  // 6
  {
    mainProgress.BeginNextStep("Finishing up");

    if (bCreateLaunchScripts)
    {
      W_SUCCEED_OR_RETURN(WProjectExport::CreateLaunchConfig(sceneFiles, szTargetDirectory, sCustomGameExecutable));
    }
  }

  return W_SUCCESS;
}
