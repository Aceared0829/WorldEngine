#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

////////////////////////////////////////////////////////////////////////
// WAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

WAssetInfo::TransformState WAssetCurator::HashAsset(WUInt64 uiSettingsHash, const WHybridArray<WString, 16>& assetTransformDeps, const WHybridArray<WString, 16>& assetThumbnailDeps, const WHybridArray<WString, 16>& assetPackageDeps, WSet<WString>& missingTransformDeps, WSet<WString>& missingThumbnailDeps, WSet<WString>& missingPackageDeps, WUInt64& out_AssetHash, WUInt64& out_ThumbHash, WUInt64& out_PackageHash, bool bForce)
{
  CURATOR_PROFILE("HashAsset");
  WStringBuilder tmp;
  WAssetInfo::TransformState state = WAssetInfo::Unknown;
  {
    // hash of the main asset file
    out_AssetHash = uiSettingsHash;
    out_ThumbHash = uiSettingsHash;
    out_PackageHash = uiSettingsHash;

    // Iterate dependencies
    for (const auto& dep : assetTransformDeps)
    {
      WString sPath = dep;
      if (!AddAssetHash(sPath, WDependencyFlags::Transform, out_AssetHash, out_ThumbHash, out_PackageHash, bForce))
      {
        missingTransformDeps.Insert(sPath);
      }
    }

    for (const auto& dep : assetThumbnailDeps)
    {
      WString sPath = dep;
      if (!AddAssetHash(sPath, WDependencyFlags::Thumbnail, out_AssetHash, out_ThumbHash, out_PackageHash, bForce))
      {
        missingThumbnailDeps.Insert(sPath);
      }
    }

    for (const auto& dep : assetPackageDeps)
    {
      WString sPath = dep;
      if (!AddAssetHash(sPath, WDependencyFlags::Package, out_AssetHash, out_ThumbHash, out_PackageHash, bForce))
      {
        missingPackageDeps.Insert(sPath);
      }
    }
  }

  if (!missingThumbnailDeps.IsEmpty())
  {
    out_ThumbHash = 0;
    state = WAssetInfo::MissingThumbnailDependency;
  }
  if (!missingTransformDeps.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state = WAssetInfo::MissingTransformDependency;
  }
  if (!missingPackageDeps.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state = WAssetInfo::MissingPackageDependency;
  }

  return state;
}

bool WAssetCurator::AddAssetHash(WString& sPath, WBitflags<WDependencyFlags> dependencyType, WUInt64& out_AssetHash, WUInt64& out_ThumbHash, WUInt64& out_PackageHash, bool bForce)
{
  if (sPath.IsEmpty())
    return true;

  if (WConversionUtils::IsStringUuid(sPath))
  {
    const WUuid guid = WConversionUtils::ConvertStringToUuid(sPath);
    WUInt64 assetHash = 0;
    WUInt64 thumbHash = 0;
    WUInt64 packageHash = 0;
    WAssetInfo::TransformState state = UpdateAssetTransformState(guid, assetHash, thumbHash, packageHash, bForce);
    if (state == WAssetInfo::Unknown || state == WAssetInfo::MissingTransformDependency || state == WAssetInfo::MissingThumbnailDependency || state == WAssetInfo::MissingPackageDependency || state == WAssetInfo::CircularDependency)
    {
      WLog::Error("Failed to hash dependency asset '{0}'", sPath);
      return false;
    }

    for (WDependencyFlags::Enum dep : dependencyType)
    {
      switch (dep)
      {
        case WDependencyFlags::Thumbnail:
          out_ThumbHash += thumbHash;
          break;
        case WDependencyFlags::Transform:
          out_AssetHash += assetHash;
          break;
        case WDependencyFlags::Package:
          out_PackageHash += packageHash;
          break;
        default:
          break;
      }
    }
    return true;
  }

  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
  {
    if (sPath.EndsWith(".color"))
    {
      // TODO: detect non-file assets and skip already in dependency gather function.
      return true;
    }
    WLog::Error("Failed to make path absolute '{0}'", sPath);
    return false;
  }

  WFileStatus fileStatus;
  WResult res = WFileSystemModel::GetSingleton()->HashFile(sPath, fileStatus);
  if (res.Failed())
  {
    return false;
  }

  for (WDependencyFlags::Enum dep : dependencyType)
  {
    switch (dep)
    {
      case WDependencyFlags::Thumbnail:
        out_ThumbHash += fileStatus.m_uiHash;
        break;
      case WDependencyFlags::Transform:
        out_AssetHash += fileStatus.m_uiHash;
        break;
      case WDependencyFlags::Package:
        out_PackageHash += fileStatus.m_uiHash;
        break;
      default:
        break;
    }
  }
  return true;
}

static WResult PatchAssetGuid(WStringView sAbsFilePath, WUuid oldGuid, WUuid newGuid)
{
  const WDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (WDocumentManager::FindDocumentTypeFromPath(sAbsFilePath, true, pTypeDesc).Failed())
    return W_FAILURE;

  WUInt32 uiTries = 0;

  WStringBuilder sTemp;
  WStringBuilder sTempTarget = WOSFile::GetTempDataFolder();
  sTempTarget.AppendPath(WPathUtils::GetFileNameAndExtension(sAbsFilePath));
  sTempTarget.ChangeFileName(WConversionUtils::ToString(newGuid, sTemp));

  sTemp = sAbsFilePath;
  while (pTypeDesc->m_pManager->CloneDocument(sTemp, sTempTarget, newGuid).Failed())
  {
    if (uiTries >= 5)
      return W_FAILURE;

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(50 * (uiTries + 1)));
    uiTries++;
  }

  WResult res = WOSFile::CopyFile(sTempTarget, sAbsFilePath);
  WOSFile::DeleteFile(sTempTarget).IgnoreResult();
  return res;
}

WResult WAssetCurator::EnsureAssetInfoUpdated(const WDataDirPath& absFilePath, const WFileStatus& stat, bool bForce)
{
  CURATOR_PROFILE(absFilePath);

  WFileSystemModel* pFiles = WFileSystemModel::GetSingleton();

  // Read document info outside the lock
  WUniquePtr<WAssetInfo> pNewAssetInfo;
  W_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(absFilePath, stat, pNewAssetInfo));
  W_ASSERT_DEV(pNewAssetInfo != nullptr && pNewAssetInfo->m_Info != nullptr, "Info should be valid on success.");


  W_LOCK(m_CuratorMutex);
  const WUuid oldGuid = stat.m_DocumentID;
  // if it already has a valid GUID, an WAssetInfo object must exist
  const bool bNewAssetFile = !stat.m_DocumentID.IsValid(); // Under this current location the asset is not known.
  WUuid newGuid = pNewAssetInfo->m_Info->m_DocumentID;

  WAssetInfo* pCurrentAssetInfo = nullptr;
  // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
  m_KnownAssets.TryGetValue(pNewAssetInfo->m_Info->m_DocumentID, pCurrentAssetInfo);

  WEnum<WAssetExistanceState> newExistanceState = WAssetExistanceState::FileUnchanged;
  if (bNewAssetFile && pCurrentAssetInfo != nullptr)
  {
    WFileStats fsOldLocation;
    const bool IsSameFile = WFileSystemModel::IsSameFile(pNewAssetInfo->m_Path, pCurrentAssetInfo->m_Path);
    const WResult statCheckOldLocation = WOSFile::GetFileStats(pCurrentAssetInfo->m_Path, fsOldLocation);

    if (statCheckOldLocation.Succeeded() && !IsSameFile)
    {
      // DUPLICATED
      // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
      // That means we currently always adjust the GUID of the second, third, etc. file that we look at
      // even if we might know that changing another file makes more sense
      // This works well for when the editor is running and someone copies a file.

      WLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", pNewAssetInfo->m_Path.GetAbsolutePath(), pCurrentAssetInfo->m_Path.GetAbsolutePath());

      const WUuid mod = WUuid::MakeStableUuidFromString(absFilePath);
      WUuid replacementGuid = pNewAssetInfo->m_Info->m_DocumentID;
      replacementGuid.CombineWithSeed(mod);

      if (PatchAssetGuid(absFilePath, pNewAssetInfo->m_Info->m_DocumentID, replacementGuid).Failed())
      {
        WLog::Error("Failed to adjust GUID of asset: '{0}'", absFilePath);
        pFiles->NotifyOfChange(absFilePath);
        return W_FAILURE;
      }

      WLog::Warning("Adjusted GUID of asset to make it unique: '{0}'", absFilePath);

      // now let's try that again
      pFiles->NotifyOfChange(absFilePath);
      return W_SUCCESS;
    }
    else
    {
      // MOVED
      // Notify old location to removed stale entry.
      pFiles->UnlinkDocument(pCurrentAssetInfo->m_Path).IgnoreResult();
      pFiles->NotifyOfChange(pCurrentAssetInfo->m_Path);
      newExistanceState = WAssetExistanceState::FileMoved;
    }
  }

  // Guid changed, different asset found, mark old as deleted and add new one.
  if (!bNewAssetFile && oldGuid != pNewAssetInfo->m_Info->m_DocumentID)
  {
    // OVERWRITTEN
    SetAssetExistanceState(*m_KnownAssets[oldGuid], WAssetExistanceState::FileRemoved);
    RemoveAssetTransformState(oldGuid);
    newExistanceState = WAssetExistanceState::FileAdded;
  }

  if (pCurrentAssetInfo)
  {
    UntrackDependencies(pCurrentAssetInfo);
    pCurrentAssetInfo->Update(pNewAssetInfo);
    // Only update if it was not already set to not overwrite, e.g. FileMoved.
    if (newExistanceState == WAssetExistanceState::FileUnchanged)
      newExistanceState = WAssetExistanceState::FileModified;
  }
  else
  {
    pCurrentAssetInfo = pNewAssetInfo.Release();
    m_KnownAssets[newGuid] = pCurrentAssetInfo;
    newExistanceState = WAssetExistanceState::FileAdded;
  }

  TrackDependencies(pCurrentAssetInfo);
  CheckForCircularDependencies(pCurrentAssetInfo).IgnoreResult();
  UpdateAssetTransformState(newGuid, WAssetInfo::TransformState::Unknown);
  // Don't call SetAssetExistanceState on newly created assets as their data structure is initialized in UpdateSubAssets for the first time.
  if (newExistanceState != WAssetExistanceState::FileAdded)
  {
    SetAssetExistanceState(*pCurrentAssetInfo, newExistanceState);
  }

  if (UpdateSubAssets(*pCurrentAssetInfo).Succeeded())
  {
    InvalidateAssetTransformState(newGuid);
  }
  else
  {
    UpdateAssetTransformState(newGuid, WAssetInfo::TransformState::TransformError);
  }

  pFiles->LinkDocument(absFilePath, pCurrentAssetInfo->m_Info->m_DocumentID).AssertSuccess("Failed to link document in file system model");
  return W_SUCCESS;
}

void WAssetCurator::TrackDependencies(WAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_PackageDependencies, m_InversePackageDeps, m_UnresolvedPackageDeps, true);

  const WString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, "");
  auto it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const WString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, outputIt.Key());
    it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  }

  // Depending on the order of loading, dependencies might be unresolved until the dependency itself is loaded into the curator.
  // If pAssetInfo was previously an unresolved dependency, these two calls will update the inverse dep tables now that it can be resolved.
  UpdateUnresolvedTrackedFiles(m_InverseTransformDeps, m_UnresolvedTransformDeps);
  UpdateUnresolvedTrackedFiles(m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps);
  UpdateUnresolvedTrackedFiles(m_InversePackageDeps, m_UnresolvedPackageDeps);
}

void WAssetCurator::UntrackDependencies(WAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_PackageDependencies, m_InversePackageDeps, m_UnresolvedPackageDeps, false);

  const WString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, "");
  auto it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const WString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, outputIt.Key());
    it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  }
}

WResult WAssetCurator::CheckForCircularDependencies(WAssetInfo* pAssetInfo)
{
  WSet<WUuid> inverseHull;
  GenerateInverseTransitiveHull(pAssetInfo, inverseHull, true, true);

  WResult res = W_SUCCESS;
  for (const auto& sDep : pAssetInfo->m_Info->m_TransformDependencies)
  {
    if (WConversionUtils::IsStringUuid(sDep))
    {
      const WUuid guid = WConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = W_FAILURE;
      }
    }
  }

  for (const auto& sDep : pAssetInfo->m_Info->m_ThumbnailDependencies)
  {
    if (WConversionUtils::IsStringUuid(sDep))
    {
      const WUuid guid = WConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = W_FAILURE;
      }
    }
  }
  return res;
}

void WAssetCurator::UpdateTrackedFiles(const WUuid& assetGuid, const WSet<WString>& files, WMap<WString, WHybridArray<WUuid, 1>>& inverseTracker, WSet<std::tuple<WUuid, WUuid>>& unresolved, bool bAdd)
{
  for (const auto& dep : files)
  {
    WString sPath = dep;

    if (sPath.IsEmpty())
      continue;

    if (WConversionUtils::IsStringUuid(sPath))
    {
      const WUuid guid = WConversionUtils::ConvertStringToUuid(sPath);
      const WAssetInfo* pInfo = GetAssetInfo(guid);

      if (!bAdd)
      {
        unresolved.Remove(std::tuple<WUuid, WUuid>(assetGuid, guid));
        if (pInfo == nullptr)
          continue;
      }

      if (pInfo == nullptr && bAdd)
      {
        unresolved.Insert(std::tuple<WUuid, WUuid>(assetGuid, guid));
        continue;
      }

      sPath = pInfo->m_Path.GetAbsolutePath();
    }
    else
    {
      if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      {
        continue;
      }
    }
    auto it = inverseTracker.FindOrAdd(sPath);
    if (bAdd)
    {
      it.Value().PushBack(assetGuid);
    }
    else
    {
      it.Value().RemoveAndCopy(assetGuid);
    }
  }
}

void WAssetCurator::UpdateUnresolvedTrackedFiles(WMap<WString, WHybridArray<WUuid, 1>>& inverseTracker, WSet<std::tuple<WUuid, WUuid>>& unresolved)
{
  for (auto it = unresolved.GetIterator(); it.IsValid();)
  {
    auto& t = *it;
    const WUuid& assetGuid = std::get<0>(t);
    const WUuid& depGuid = std::get<1>(t);
    if (const WAssetInfo* pInfo = GetAssetInfo(depGuid))
    {
      WString sPath = pInfo->m_Path.GetAbsolutePath();
      auto itTracker = inverseTracker.FindOrAdd(sPath);
      itTracker.Value().PushBack(assetGuid);
      it = unresolved.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

WResult WAssetCurator::ReadAssetDocumentInfo(const WDataDirPath& absFilePath, const WFileStatus& stat, WUniquePtr<WAssetInfo>& out_assetInfo)
{
  CURATOR_PROFILE(szAbsFilePath);
  WFileSystemModel* pFiles = WFileSystemModel::GetSingleton();

  out_assetInfo = W_DEFAULT_NEW(WAssetInfo);
  out_assetInfo->m_Path = absFilePath;

  // figure out which manager should handle this asset type
  {
    const WDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo->m_pDocumentTypeDescriptor == nullptr)
    {
      if (WDocumentManager::FindDocumentTypeFromPath(absFilePath, false, pTypeDesc).Failed())
      {
        W_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo->m_pDocumentTypeDescriptor = static_cast<const WAssetDocumentTypeDescriptor*>(pTypeDesc);
    }
  }

  // Try cache first
  {
    WFileStatus cacheStat;
    WUniquePtr<WAssetDocumentInfo> docInfo;
    {
      W_LOCK(m_CachedAssetsMutex);
      auto itFile = m_CachedFiles.Find(absFilePath);
      auto itAsset = m_CachedAssets.Find(absFilePath);
      if (itAsset.IsValid() && itFile.IsValid())
      {
        docInfo = std::move(itAsset.Value());
        cacheStat = itFile.Value();
        m_CachedAssets.Remove(itAsset);
        m_CachedFiles.Remove(itFile);
      }
    }

    if (docInfo && cacheStat.m_LastModified.Compare(stat.m_LastModified, WTimestamp::CompareMode::Identical))
    {
      out_assetInfo->m_Info = std::move(docInfo);
      return W_SUCCESS;
    }
  }

  // try to read the asset file
  WStatus infoStatus(W_SUCCESS);
  WResult res = pFiles->ReadDocument(absFilePath, [&out_assetInfo, &infoStatus](const WFileStatus& stat, WStreamReader& ref_reader)
    { infoStatus = out_assetInfo->GetManager()->ReadAssetDocumentInfo(out_assetInfo->m_Info, ref_reader); });

  if (infoStatus.Failed())
  {
    WLog::Error("Failed to read asset document info for asset file '{0}'", absFilePath);
    return W_FAILURE;
  }

  W_ASSERT_DEV(out_assetInfo->m_Info != nullptr, "Info should be valid on suceess.");
  return res;
}

WResult WAssetCurator::UpdateSubAssets(WAssetInfo& assetInfo)
{
  CURATOR_PROFILE("UpdateSubAssets");
  if (assetInfo.m_ExistanceState == WAssetExistanceState::FileRemoved)
  {
    return W_SUCCESS;
  }

  if (assetInfo.m_ExistanceState == WAssetExistanceState::FileAdded)
  {
    auto& mainSub = m_KnownSubAssets[assetInfo.m_Info->m_DocumentID];
    mainSub.m_bMainAsset = true;
    mainSub.m_ExistanceState = WAssetExistanceState::FileAdded;
    mainSub.m_pAssetInfo = &assetInfo;
    mainSub.m_Data.m_Guid = assetInfo.m_Info->m_DocumentID;
    mainSub.m_Data.m_sSubAssetsDocumentTypeName = assetInfo.m_Info->m_sAssetsDocumentTypeName;
  }

  WStringBuilder tmp;
  WTempHybridArray<WLogEntry, 2> logEntries;

  {
    WTempHybridArray<WSubAssetData, 4> subAssets;
    {
      CURATOR_PROFILE("FillOutSubAssetList");
      assetInfo.GetManager()->FillOutSubAssetList(*assetInfo.m_Info.Borrow(), subAssets);
    }

    for (const WUuid& sub : assetInfo.m_SubAssets)
    {
      m_KnownSubAssets[sub].m_ExistanceState = WAssetExistanceState::FileRemoved;
      m_SubAssetChanged.Insert(sub);
    }

    for (const WSubAssetData& data : subAssets)
    {
      const auto itSub = m_KnownSubAssets.Find(data.m_Guid);
      const bool bExisted = itSub.IsValid();
      if (bExisted == assetInfo.m_SubAssets.Contains(data.m_Guid))
      {
        WSubAsset sub;
        sub.m_bMainAsset = false;
        sub.m_ExistanceState = bExisted ? WAssetExistanceState::FileModified : WAssetExistanceState::FileAdded;
        sub.m_pAssetInfo = &assetInfo;
        sub.m_Data = data;
        m_KnownSubAssets.Insert(data.m_Guid, sub);

        if (!bExisted)
        {
          assetInfo.m_SubAssets.Insert(sub.m_Data.m_Guid);
          m_SubAssetChanged.Insert(sub.m_Data.m_Guid);
        }
      }
      else
      {
        tmp.SetFormat("Sub-asset '{}' with GUID '{}' already exists in '{}'", data.m_sName, data.m_Guid, itSub.Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

        auto& log = logEntries.ExpandAndGetRef();
        log.m_Type = WLogMsgType::ErrorMsg;
        log.m_sMsg = tmp;
      }
    }

    for (auto it = assetInfo.m_SubAssets.GetIterator(); it.IsValid();)
    {
      if (m_KnownSubAssets[it.Key()].m_ExistanceState == WAssetExistanceState::FileRemoved)
      {
        it = assetInfo.m_SubAssets.Remove(it);
      }
      else
      {
        ++it;
      }
    }
  }

  if (logEntries.IsEmpty())
    return W_SUCCESS;

  assetInfo.m_LogEntries = logEntries;

  return W_FAILURE;
}

void WAssetCurator::RemoveAssetTransformState(const WUuid& assetGuid)
{
  W_LOCK(m_CuratorMutex);

  for (int i = 0; i < WAssetInfo::TransformState::COUNT; i++)
  {
    m_TransformState[i].Remove(assetGuid);
  }
  m_TransformStateStale.Remove(assetGuid);
}


void WAssetCurator::InvalidateAssetTransformState(const WUuid& assetGuid)
{
  W_LOCK(m_CuratorMutex);

  WSet<WUuid> hull;
  {
    WAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    {
      GenerateInverseTransitiveHull(pAssetInfo, hull, true, true);
    }
  }

  for (const WUuid& guid : hull)
  {
    WAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pAssetInfo))
    {
      // We do not set pAssetInfo->m_TransformState because that is user facing and
      // as after updating the state it might just be the same as before we instead add
      // it to the queue here to prevent flickering in the GUI.
      m_TransformStateStale.Insert(guid);
      // Increasing m_LastStateUpdate will ensure that asset hash/state computations
      // that are in flight will not be written back to the asset.
      pAssetInfo->m_LastStateUpdate++;
      pAssetInfo->m_AssetHash = 0;
      pAssetInfo->m_ThumbHash = 0;
      pAssetInfo->m_PackageHash = 0;
      pAssetInfo->ClearTransformInfoCache();
    }
  }
}

void WAssetCurator::UpdateAssetTransformState(const WUuid& assetGuid, WAssetInfo::TransformState state)
{
  W_LOCK(m_CuratorMutex);

  WAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    m_TransformStateStale.Remove(assetGuid);
    for (int i = 0; i < WAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Remove(assetGuid);
    }
    m_TransformState[state].Insert(assetGuid);

    const bool bStateChanged = pAssetInfo->m_TransformState != state;

    if (bStateChanged)
    {
      pAssetInfo->m_TransformState = state;
      m_SubAssetChanged.Insert(assetGuid);
      for (const auto& key : pAssetInfo->m_SubAssets)
      {
        m_SubAssetChanged.Insert(key);
      }
    }

    switch (state)
    {
      case WAssetInfo::TransformState::TransformError:
      {
        // Transform errors are unexpected and invalidate any previously computed
        // state of assets depending on this one.
        auto it = m_InverseTransformDeps.Find(pAssetInfo->m_Path);
        if (it.IsValid())
        {
          for (const WUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseThumbnailDeps.Find(pAssetInfo->m_Path);
        if (it2.IsValid())
        {
          for (const WUuid& guid : it2.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        // Invalidating a dependent also invalidates everything that dependent transitively depends on,
        // which can lead back to this asset. Being stale would make the update task recompute the state
        // we just recorded, and since the error is only known to the code that ran the transform, that
        // recomputation would silently drop it - the asset would fall back to NeedsTransform and the
        // curator would stop reporting it. Keep the error until something actually changes on disk.
        m_TransformStateStale.Remove(assetGuid);

        break;
      }

      case WAssetInfo::TransformState::Unknown:
      {
        InvalidateAssetTransformState(assetGuid);
        break;
      }

      case WAssetInfo::TransformState::UpToDate:
      {
        if (bStateChanged)
        {
          WString sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_Path);
          WQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);

          for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
          {
            WSubAsset* pSubAsset;
            if (m_KnownSubAssets.TryGetValue(subAssetUuid, pSubAsset))
            {
              sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);
              WQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);
            }
          }
        }
        break;
      }

      default:
        break;
    }
  }
}

void WAssetCurator::UpdateAssetTransformLog(const WUuid& assetGuid, WDynamicArray<WLogEntry>& logEntries)
{
  WAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    pAssetInfo->m_LogEntries.Clear();
    pAssetInfo->m_LogEntries.Swap(logEntries);
  }
}


void WAssetCurator::SetAssetExistanceState(WAssetInfo& assetInfo, WAssetExistanceState::Enum state)
{
  W_ASSERT_DEBUG(m_CuratorMutex.IsLocked(), "");

  // Only the main thread tick function is allowed to change from FileAdded / FileRenamed to FileModified to inform views.
  // A modified 'added' file is still added until the added state was addressed.
  auto IsModifiedAfterAddOrRename = [](WAssetExistanceState::Enum oldState, WAssetExistanceState::Enum newState) -> bool
  {
    return oldState == WAssetExistanceState::FileAdded && newState == WAssetExistanceState::FileModified ||
           oldState == WAssetExistanceState::FileMoved && newState == WAssetExistanceState::FileModified;
  };

  if (!IsModifiedAfterAddOrRename(assetInfo.m_ExistanceState, state))
    assetInfo.m_ExistanceState = state;

  for (WUuid subGuid : assetInfo.m_SubAssets)
  {
    auto& existanceState = GetSubAssetInternal(subGuid)->m_ExistanceState;
    if (!IsModifiedAfterAddOrRename(existanceState, state))
    {
      existanceState = state;
      m_SubAssetChanged.Insert(subGuid);
    }
  }

  auto& existanceState = GetSubAssetInternal(assetInfo.m_Info->m_DocumentID)->m_ExistanceState;
  if (!IsModifiedAfterAddOrRename(existanceState, state))
  {
    existanceState = state;
    m_SubAssetChanged.Insert(assetInfo.m_Info->m_DocumentID);
  }
}


////////////////////////////////////////////////////////////////////////
// WUpdateTask
////////////////////////////////////////////////////////////////////////

WUpdateTask::WUpdateTask(WOnTaskFinishedCallback onTaskFinished)
{
  ConfigureTask("WUpdateTask", WTaskNesting::Maybe, onTaskFinished);
}

WUpdateTask::~WUpdateTask() = default;

void WUpdateTask::Execute()
{
  WUuid assetGuid;
  {
    W_LOCK(WAssetCurator::GetSingleton()->m_CuratorMutex);
    if (!WAssetCurator::GetSingleton()->GetNextAssetToUpdate(assetGuid, m_sAssetPath))
      return;
  }

  const WDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  if (WDocumentManager::FindDocumentTypeFromPath(m_sAssetPath, false, pTypeDescriptor).Failed())
    return;

  WUInt64 uiAssetHash = 0;
  WUInt64 uiThumbHash = 0;
  WUInt64 uiPackageHash = 0;

  // Do not log update errors done on the background thread. Only if done explicitly on the main thread or the GUI will not be responsive
  // if the user deleted some base asset and everything starts complaining about it.
  WLogEntryDelegate logger([&](WLogEntry& ref_entry) -> void {}, WLogMsgType::All);
  WLogSystemScope logScope(&logger);

  WAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, WAssetCurator::GetSingleton()->GetActiveAssetProfile(), static_cast<const WAssetDocumentTypeDescriptor*>(pTypeDescriptor), uiAssetHash, uiThumbHash, uiPackageHash);
}
