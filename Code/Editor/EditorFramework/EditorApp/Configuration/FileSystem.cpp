#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

void WQtEditorApp::AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName, bool bWriteable, WUInt32 uiInsertIndex)
{
  WStringBuilder sPath = szSdkRootRelativePath;
  sPath.MakeCleanPath();

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    if (dd.m_sDataDirSpecialPath == sPath)
    {
      dd.m_bHardCodedDependency = true;

      if (bWriteable)
        dd.m_bWritable = true;

      return;
    }
  }

  WApplicationFileSystemConfig::DataDirConfig cfg;
  cfg.m_sDataDirSpecialPath = sPath;
  cfg.m_bWritable = bWriteable;
  cfg.m_sRootName = szRootName;
  cfg.m_bHardCodedDependency = true;

  if (uiInsertIndex == WInvalidIndex || uiInsertIndex >= m_FileSystemConfig.m_DataDirs.GetCount())
    m_FileSystemConfig.m_DataDirs.PushBack(cfg);
  else
    m_FileSystemConfig.m_DataDirs.InsertAt(uiInsertIndex, cfg);
}

void WQtEditorApp::SetFileSystemConfig(const WApplicationFileSystemConfig& cfg)
{
  if (m_FileSystemConfig == cfg)
    return;

  m_FileSystemConfig = cfg;
  m_FileSystemConfig.CreateDataDirStubFiles().IgnoreResult();
  m_FileSystemConfig.Save().IgnoreResult();

  WQtEditorApp::GetSingleton()->AddReloadProjectRequiredReason("The data directory configuration has changed.");
}

void WQtEditorApp::SetupDataDirectories()
{
  W_PROFILE_SCOPE("SetupDataDirectories");
  WFileSystem::DetectSdkRootDirectory().IgnoreResult();

  WStringBuilder sPath = WToolsProject::GetSingleton()->GetProjectDirectory();

  WFileSystem::SetSpecialDirectory("project", sPath);

  sPath.AppendPath("RuntimeConfigs/DataDirectories.ddl");
  // we cannot use the default ":project/" path here, because that data directory will only be configured a few lines below
  // so instead we use the absolute path directly
  m_FileSystemConfig.Load(sPath);

  WEditorAppEvent e;
  e.m_Type = WEditorAppEvent::Type::BeforeApplyDataDirectories;
  m_Events.Broadcast(e);

  WQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base", false);

  // Remove stale plugin-bundle data directories (bundle no longer active, or the legacy shared mount) before re-injecting
  // the ones that are currently active. This self-heals when a plugin is disabled.
  {
    WSet<WString> knownBundleDirs;
    WQtEditorApp::GetSingleton()->GetAllKnownBundleDataDirectories(knownBundleDirs);

    WStringBuilder sLegacy = ">sdk/Data/Plugins";
    sLegacy.MakeCleanPath();
    knownBundleDirs.Insert(sLegacy);

    for (WUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      if (knownBundleDirs.Contains(m_FileSystemConfig.m_DataDirs[i - 1].m_sDataDirSpecialPath))
      {
        m_FileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }
  }

  // Inject the data directories declared by all currently active plugin bundles (mandatory/selected + transitive
  // requirements). These have to end up before ">project/", because data directories are searched back to front:
  // whatever is listed after the project shadows the project's own files, and a project must be able to override
  // plugin provided files (e.g. ship its own version of a particle shader).
  // Appending is not enough for that, because the project directory is usually already part of the config that was
  // just loaded, so the insert position has to be looked up.
  {
    WSet<WString> activeBundleDirs;
    WQtEditorApp::GetSingleton()->GetActiveBundleDataDirectories(activeBundleDirs);

    WStringBuilder sProjectDir = ">project/";
    sProjectDir.MakeCleanPath();

    WUInt32 uiInsertIndex = WInvalidIndex;
    for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      if (m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath == sProjectDir)
      {
        uiInsertIndex = i;
        break;
      }
    }

    for (const WString& sDir : activeBundleDirs)
    {
      WQtEditorApp::GetSingleton()->AddPluginDataDirDependency(sDir, nullptr, false, uiInsertIndex);

      // the loop above removed all bundle directories, so each of these really is an insert
      if (uiInsertIndex != WInvalidIndex)
        ++uiInsertIndex;
    }
  }

  WQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">project/", "project", true);

  // Tell the tools project that all data directories are ok to put documents in
  {
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sPath).Succeeded())
      {
        WToolsProject::GetSingleton()->AddAllowedDocumentRoot(sPath);
      }
    }
  }

  m_FileSystemConfig.Apply();
}

bool WQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute(WStringBuilder& ref_sPath, bool bCheckExists) const
{
  ref_sPath.MakeCleanPath();

  if (WPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (WPathUtils::IsRootedPath(ref_sPath))
  {
    WStringBuilder sAbsPath;
    if (WFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (WConversionUtils::IsStringUuid(ref_sPath))
  {
    WUuid guid = WConversionUtils::ConvertStringToUuid(ref_sPath);
    auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(guid);

    // m_pAssetInfo is null for a sub-asset the curator knows but holds no file information for, so it
    // has to be checked separately from the asset itself.
    if (pAsset == nullptr || pAsset->m_pAssetInfo == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  WStringBuilder sTemp, sFolder, sDataDirName;

  const char* szEnd = ref_sPath.FindSubString("/");
  if (szEnd)
  {
    sDataDirName.SetSubString_FromTo(ref_sPath.GetData(), szEnd);
  }
  else
  {
    sDataDirName = ref_sPath;
  }

  for (WUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    // only check data directories that start with the required name
    while (sTemp.EndsWith("/") || sTemp.EndsWith("\\"))
      sTemp.Shrink(0, 1);
    const WStringView folderName = sTemp.GetFileName();

    if (sDataDirName != folderName)
      continue;

    sTemp.PathParentDirectory(); // the secret sauce is here
    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (!bCheckExists || WOSFile::ExistsFile(sTemp) || WOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool WQtEditorApp::MakeDataDirectoryRelativePathAbsolute(WStringBuilder& ref_sPath) const
{
  if (WPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (WPathUtils::IsRootedPath(ref_sPath))
  {
    WStringBuilder sAbsPath;
    if (WFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (WConversionUtils::IsStringUuid(ref_sPath))
  {
    WUuid guid = WConversionUtils::ConvertStringToUuid(ref_sPath);
    auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(guid);

    // m_pAssetInfo is null for a sub-asset the curator knows but holds no file information for, so it
    // has to be checked separately from the asset itself.
    if (pAsset == nullptr || pAsset->m_pAssetInfo == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  WStringBuilder sTemp;

  for (WUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (WOSFile::ExistsFile(sTemp) || WOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool WQtEditorApp::MakeDataDirectoryRelativePathAbsolute(WString& ref_sPath) const
{
  WStringBuilder sTemp = ref_sPath;
  bool bRes = MakeDataDirectoryRelativePathAbsolute(sTemp);
  ref_sPath = sTemp;
  return bRes;
}

bool WQtEditorApp::MakePathDataDirectoryRelative(WStringBuilder& ref_sPath) const
{
  WStringBuilder sTemp;

  for (WUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(WFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool WQtEditorApp::MakePathDataDirectoryParentRelative(WStringBuilder& ref_sPath) const
{
  WStringBuilder sTemp;

  for (WUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      sTemp.PathParentDirectory();

      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(WFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool WQtEditorApp::MakePathDataDirectoryRelative(WString& ref_sPath) const
{
  WStringBuilder sTemp = ref_sPath;
  bool bRes = MakePathDataDirectoryRelative(sTemp);
  ref_sPath = sTemp;
  return bRes;
}
