#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#define W_CURATOR_CACHE_VERSION 2      // Change this to delete and re-gen all asset caches.
#define W_CURATOR_CACHE_FILE_VERSION 8 // Change this if for cache format changes.

W_IMPLEMENT_SINGLETON(WAssetCurator);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetCurator)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "FileSystemModel",
  "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WAssetCurator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WAssetCurator* pDummy = WAssetCurator::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

void WAssetInfo::Update(WUniquePtr<WAssetInfo>& rhs)
{
  // Don't update the existance state, it is handled via WAssetCurator::SetAssetExistanceState
  // m_ExistanceState = rhs->m_ExistanceState;
  m_TransformState = rhs->m_TransformState;
  m_pDocumentTypeDescriptor = rhs->m_pDocumentTypeDescriptor;
  m_Path = std::move(rhs->m_Path);
  m_Info = std::move(rhs->m_Info);

  m_AssetHash = rhs->m_AssetHash;
  m_ThumbHash = rhs->m_ThumbHash;
  m_PackageHash = rhs->m_PackageHash;
  m_MissingTransformDeps = std::move(rhs->m_MissingTransformDeps);
  m_MissingThumbnailDeps = std::move(rhs->m_MissingThumbnailDeps);
  m_MissingPackageDeps = std::move(rhs->m_MissingPackageDeps);
  m_CircularDependencies = std::move(rhs->m_CircularDependencies);
  // Don't copy m_SubAssets, we want to update it independently.

  // Not copied from rhs, which never has one: anything cached may belong to a previous transform.
  ClearTransformInfoCache();

  rhs = nullptr;
}

const WAssetInfoFile* WAssetInfo::GetTransformInfo(WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const
{
  // Nothing was transformed yet, so there is nothing to describe, and no path to read from either.
  if (m_pDocumentTypeDescriptor == nullptr || m_AssetHash == 0)
    return nullptr;

  // Resolved here, so that a null profile and an explicit pointer to the same profile share one entry.
  const WPlatformProfile* pProfile = WAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);

  TransformInfoCache* pEntry = nullptr;

  for (TransformInfoCache& cache : m_TransformInfoCache)
  {
    if (cache.m_pAssetProfile == pProfile && cache.m_sOutputTag == sOutputTag)
    {
      // A hit for the current hash. Otherwise the entry is stale and is overwritten below, rather than
      // added to, so that the cache cannot grow with every transform.
      if (cache.m_uiAssetHash == m_AssetHash)
        return cache.m_bValid ? &cache.m_Info : nullptr;

      pEntry = &cache;
      break;
    }
  }

  if (pEntry == nullptr)
  {
    pEntry = &m_TransformInfoCache.ExpandAndGetRef();
  }

  pEntry->m_uiAssetHash = m_AssetHash;
  pEntry->m_sOutputTag = sOutputTag;
  pEntry->m_pAssetProfile = pProfile;

  WAssetDocumentManager* pManager = static_cast<WAssetDocumentManager*>(m_pDocumentTypeDescriptor->m_pManager);

  // A failure is the normal case, so it is cached as well, otherwise every call would try to open a
  // file that is known not to be there.
  pEntry->m_bValid = pManager->ReadAssetInfoFile(pEntry->m_Info, m_pDocumentTypeDescriptor, m_Path, m_AssetHash, sOutputTag, pProfile).Succeeded();

  if (!pEntry->m_bValid)
  {
    pEntry->m_Info.Clear();
    return nullptr;
  }

  return &pEntry->m_Info;
}

WStringView WSubAsset::GetName() const
{
  if (m_bMainAsset)
    return WPathUtils::GetFileName(m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
  else
    return m_Data.m_sName;
}


void WSubAsset::GetSubAssetIdentifier(WStringBuilder& out_sPath) const
{
  out_sPath = m_pAssetInfo->m_Path.GetDataDirParentRelativePath();

  if (!m_bMainAsset)
  {
    out_sPath.Append("|", m_Data.m_sName);
  }
}

////////////////////////////////////////////////////////////////////////
// WAssetCurator Setup
////////////////////////////////////////////////////////////////////////

WAssetCurator::WAssetCurator()
  : m_SingletonRegistrar(this)
{
}

WAssetCurator::~WAssetCurator()
{
  W_ASSERT_DEBUG(m_KnownAssets.IsEmpty(), "Need to call Deinitialize before curator is deleted.");
}

void WAssetCurator::StartInitialize(const WApplicationFileSystemConfig& cfg)
{
  W_PROFILE_SCOPE("StartInitialize");

  {
    W_LOG_BLOCK("SetupAssetProfiles");

    SetupDefaultAssetProfiles();
    if (LoadAssetProfiles().Failed())
    {
      WLog::Warning("Asset profiles file does not exist or contains invalid data. Setting up default profiles.");
      SaveAssetProfiles().IgnoreResult();
      SaveRuntimeProfiles();
    }
  }

  ComputeAllDocumentManagerAssetProfileHashes();
  BuildFileExtensionSet(m_ValidAssetExtensions);

  m_bRunUpdateTask = true;
  m_FileSystemConfig = cfg;

  WFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(WMakeDelegate(&WAssetCurator::OnFileChangedEvent, this));
  WFileSystemModel::FilesMap referencedFiles;
  WFileSystemModel::FoldersMap referencedFolders;
  LoadCaches(referencedFiles, referencedFolders);
  // We postpone the WAssetFiles initialize to after we have loaded the cache. No events will be fired before initialize is called.
  WFileSystemModel::GetSingleton()->Initialize(m_FileSystemConfig, std::move(referencedFiles), std::move(referencedFolders));

  m_pAssetTableWriter = W_DEFAULT_NEW(WAssetTableWriter, m_FileSystemConfig);

  WSharedPtr<WDelegateTask<void>> pInitTask = W_DEFAULT_NEW(WDelegateTask<void>, "AssetCuratorUpdateCache", WTaskNesting::Never, [this]()
    {
      W_LOCK(m_CuratorMutex);

      m_CuratorMutex.Unlock();
      CheckFileSystem();
      m_CuratorMutex.Lock();

      // As we fired a AssetListReset in CheckFileSystem, set everything new to FileUnchanged or
      // we would fire an added call for every asset.
      for (auto it = m_KnownSubAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_ExistanceState == WAssetExistanceState::FileAdded)
        {
          it.Value().m_ExistanceState = WAssetExistanceState::FileUnchanged;
        }
      }
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == WAssetExistanceState::FileAdded)
        {
          it.Value()->m_ExistanceState = WAssetExistanceState::FileUnchanged;
        }
      }

      // Re-save caches after we made a full CheckFileSystem pass.
      WFileSystemModel::FilesMap referencedFiles;
      WFileSystemModel::FoldersMap referencedFolders;
      WFileSystemModel* pFiles = WFileSystemModel::GetSingleton();
      {
        referencedFiles = *pFiles->GetFiles();
        referencedFolders = *pFiles->GetFolders();
      }
      SaveCaches(referencedFiles, referencedFolders); //
    });
  pInitTask->ConfigureTask("Initialize Curator", WTaskNesting::Never);
  m_InitializeCuratorTaskID = WTaskSystem::StartSingleTask(pInitTask, WTaskPriority::FileAccessHighPriority);

  {
    WAssetCuratorEvent e;
    e.m_Type = WAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }
}

void WAssetCurator::WaitForInitialize()
{
  W_PROFILE_SCOPE("WaitForInitialize");
  WTaskSystem::WaitForGroup(m_InitializeCuratorTaskID);
  m_InitializeCuratorTaskID.Invalidate();

  W_LOCK(m_CuratorMutex);

  // Broadcast reset.
  {
    WAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = WAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
  // Write Asset tables must happen after the reset as that causes the writer to rebuilt its tables.
  WriteAssetTables(nullptr, true).IgnoreResult();
  // This needs to happen after tables are written so that some assets can load their dependencies.
  ProcessAllCoreAssets();
}

void WAssetCurator::Deinitialize()
{
  W_PROFILE_SCOPE("Deinitialize");

  SaveAssetProfiles().IgnoreResult();

  ShutdownUpdateTask();
  WAssetProcessor::GetSingleton()->StopProcessor(true);
  WFileSystemModel* pFiles = WFileSystemModel::GetSingleton();
  WFileSystemModel::FilesMap referencedFiles;
  WFileSystemModel::FoldersMap referencedFolders;
  pFiles->Deinitialize(&referencedFiles, &referencedFolders);
  SaveCaches(referencedFiles, referencedFolders);

  pFiles->m_FileChangedEvents.RemoveEventHandler(WMakeDelegate(&WAssetCurator::OnFileChangedEvent, this));
  pFiles = nullptr;
  m_pAssetTableWriter = nullptr;

  {
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      W_DEFAULT_DELETE(it.Value());
    }
    m_KnownSubAssets.Clear();
    m_KnownAssets.Clear();
    m_TransformStateStale.Clear();

    for (int i = 0; i < WAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Clear();
    }
  }

  // Broadcast reset.
  {
    WAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = WAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  ClearAssetProfiles();
}

void WAssetCurator::MainThreadTick(bool bTopLevel)
{
  CURATOR_PROFILE("MainThreadTick");

  static std::atomic<bool> bReentry = false;
  if (bReentry)
    return;

  if (WQtEditorApp::GetSingleton()->IsProgressBarProcessingEvents())
    return;

  bReentry = true;

  WFileSystemModel::GetSingleton()->MainThreadTick();

  W_LOCK(m_CuratorMutex);
  WTempHybridArray<WAssetInfo*, 32> deletedAssets;
  for (const WUuid& guid : m_SubAssetChanged)
  {
    WSubAsset* pInfo = GetSubAssetInternal(guid);
    WAssetCuratorEvent e;
    e.m_AssetGuid = guid;
    e.m_pInfo = pInfo;
    e.m_Type = WAssetCuratorEvent::Type::AssetUpdated;

    if (pInfo != nullptr)
    {
      if (pInfo->m_ExistanceState == WAssetExistanceState::FileAdded)
      {
        pInfo->m_ExistanceState = WAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = WAssetExistanceState::FileUnchanged;
        e.m_Type = WAssetCuratorEvent::Type::AssetAdded;
        m_Events.Broadcast(e);
      }
      else if (pInfo->m_ExistanceState == WAssetExistanceState::FileMoved)
      {
        if (pInfo->m_bMainAsset)
        {
          // Make sure the document knows that its underlying file was renamed.
          if (WDocument* pDoc = WDocumentManager::GetDocumentByGuid(guid))
            pDoc->DocumentRenamed(pInfo->m_pAssetInfo->m_Path);
        }

        pInfo->m_ExistanceState = WAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = WAssetExistanceState::FileUnchanged;
        e.m_Type = WAssetCuratorEvent::Type::AssetMoved;
        m_Events.Broadcast(e);
      }
      else if (pInfo->m_ExistanceState == WAssetExistanceState::FileRemoved)
      {
        // this is a bit tricky:
        // when the document is deleted on disk, it would be nicer not to close it (discarding modifications!)
        // instead we could set it as modified
        // but then when it was only moved or renamed that means we have another document with the same GUID
        // so once the user would save the now modified document, we would end up with two documents with the same GUID
        // so, for now, since this is probably a rare case anyway, we just close the document without asking
        if (pInfo->m_bMainAsset)
        {
          WDocumentManager::EnsureDocumentIsClosedInAllManagers(pInfo->m_pAssetInfo->m_Path);
          e.m_Type = WAssetCuratorEvent::Type::AssetRemoved;
          m_Events.Broadcast(e);

          deletedAssets.PushBack(pInfo->m_pAssetInfo);
        }
        m_KnownAssets.Remove(guid);
        m_KnownSubAssets.Remove(guid);
      }
      else // Either WAssetInfo::ExistanceState::FileModified or tranform changed
      {
        pInfo->m_ExistanceState = WAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = WAssetExistanceState::FileUnchanged;
        e.m_Type = WAssetCuratorEvent::Type::AssetUpdated;
        m_Events.Broadcast(e);
      }
    }
  }
  m_SubAssetChanged.Clear();

  // Delete file asset info after all the sub-assets have been handled (so no ref exist to it anymore).
  for (WAssetInfo* pInfo : deletedAssets)
  {
    W_DEFAULT_DELETE(pInfo);
  }

  RunNextUpdateTask();

  if (bTopLevel && !m_TransformState[WAssetInfo::TransformState::NeedsImport].IsEmpty())
  {
    const WUuid assetToImport = *m_TransformState[WAssetInfo::TransformState::NeedsImport].GetIterator();

    WAssetInfo* pInfo = GetAssetInfo(assetToImport);

    ProcessAsset(pInfo, nullptr, WTransformFlags::TriggeredManually);
    UpdateAssetTransformState(assetToImport, WAssetInfo::TransformState::Unknown);
  }

  if (bTopLevel && m_pAssetTableWriter)
    m_pAssetTableWriter->MainThreadTick();

  bReentry = false;
}

WDateTime WAssetCurator::GetLastFullTransformDate() const
{
  WStringBuilder path = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  WFileStats stat;
  if (WOSFile::GetFileStats(path, stat).Failed())
    return {};

  return WDateTime::MakeFromTimestamp(stat.m_LastModificationTime);
}

void WAssetCurator::StoreFullTransformDate()
{
  WStringBuilder path = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  WOSFile file;
  if (file.Open(path, WFileOpenMode::Write).Succeeded())
  {
    WDateTime date;
    date.SetFromTimestamp(WTimestamp::CurrentTimestamp()).AssertSuccess();

    path.SetFormat("{}", date);
    file.Write(path.GetData(), path.GetElementCount()).AssertSuccess();
  }
}

////////////////////////////////////////////////////////////////////////
// WAssetCurator High Level Functions
////////////////////////////////////////////////////////////////////////

WStatus WAssetCurator::TransformAllAssets(WBitflags<WTransformFlags> transformFlags, const WPlatformProfile* pAssetProfile)
{
  W_PROFILE_SCOPE("TransformAllAssets");

  WDynamicArray<WUuid> assets;
  {
    W_LOCK(m_CuratorMutex);
    assets.Reserve(m_KnownAssets.GetCount());
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      assets.PushBack(it.Key());
    }
  }
  WUInt32 uiNumStepsLeft = assets.GetCount();

  WUInt32 uiNumFailedSteps = 0;
  WProgressRange range("Transforming Assets", 1 + uiNumStepsLeft, true);
  for (const WUuid& assetGuid : assets)
  {
    if (range.WasCanceled())
      break;

    W_LOCK(m_CuratorMutex);

    WAssetInfo* pAssetInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
      continue;

    if (uiNumStepsLeft > 0)
    {
      // it can happen that the number of known assets changes while we are processing them
      // in this case the progress bar may assert that the number of steps completed is larger than
      // what was specified before
      // since this is a valid case, we just stop updating the progress bar, in case more assets are detected

      range.BeginNextStep(WPathUtils::GetFileNameAndExtension(pAssetInfo->m_Path.GetDataDirParentRelativePath()));
      --uiNumStepsLeft;
    }

    WTransformStatus res = ProcessAsset(pAssetInfo, pAssetProfile, transformFlags);
    if (res.Failed())
    {
      uiNumFailedSteps++;
      WLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_Path.GetDataDirParentRelativePath());
    }
  }

  TransformAssetsForSceneExport(pAssetProfile);

  range.BeginNextStep("Writing Lookup Tables");

  WriteAssetTables(pAssetProfile).IgnoreResult();

  StoreFullTransformDate();

  if (uiNumFailedSteps > 0)
    return WStatus(WFmt("Transform all assets failed on {0} assets.", uiNumFailedSteps));

  return WStatus(W_SUCCESS);
}

void WAssetCurator::ResaveAllAssets(WStringView sPrefixPath)
{
  WHashTable<WUuid, WAssetInfo*> resaveAssets;
  resaveAssets.Reserve(m_KnownAssets.GetCount());

  for (auto itAsset = m_KnownAssets.GetIterator(); itAsset.IsValid(); ++itAsset)
  {
    if (WPathUtils::IsSubPath(sPrefixPath, itAsset.Value()->m_Path.GetAbsolutePath()))
    {
      resaveAssets.Insert(itAsset.Key(), itAsset.Value());
    }
  }

  WProgressRange range("Re-saving Assets", 1 + resaveAssets.GetCount(), true);

  W_LOCK(m_CuratorMutex);

  WDynamicArray<WUuid> sortedAssets;
  sortedAssets.Reserve(resaveAssets.GetCount());

  WMap<WUuid, WSet<WUuid>> dependencies;

  WSet<WUuid> accu;

  for (auto itAsset = resaveAssets.GetIterator(); itAsset.IsValid(); ++itAsset)
  {
    auto it2 = dependencies.Insert(itAsset.Key(), WSet<WUuid>());
    for (const WString& dep : itAsset.Value()->m_Info->m_TransformDependencies)
    {
      if (WConversionUtils::IsStringUuid(dep))
      {
        it2.Value().Insert(WConversionUtils::ConvertStringToUuid(dep));
      }
    }
  }

  while (!dependencies.IsEmpty())
  {
    bool bDeadEnd = true;
    for (auto it = dependencies.GetIterator(); it.IsValid(); ++it)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(it.Value()))
      {
        sortedAssets.PushBack(it.Key());
        accu.Insert(it.Key());
        dependencies.Remove(it);
        bDeadEnd = false;
        break;
      }
    }

    if (bDeadEnd)
    {
      // Just take the next one in and hope for the best.
      auto it = dependencies.GetIterator();
      sortedAssets.PushBack(it.Key());
      accu.Insert(it.Key());
      dependencies.Remove(it);
    }
  }

  for (WUInt32 i = 0; i < sortedAssets.GetCount(); i++)
  {
    if (range.WasCanceled())
      break;

    WAssetInfo* pAssetInfo = GetAssetInfo(sortedAssets[i]);
    W_ASSERT_DEBUG(pAssetInfo, "Should not happen as data was derived from known assets list.");
    range.BeginNextStep(WPathUtils::GetFileNameAndExtension(pAssetInfo->m_Path.GetDataDirParentRelativePath()));

    auto res = ResaveAsset(pAssetInfo);
    if (res.Failed())
    {
      WLog::Error("{0} ({1})", res.GetMessageString(), pAssetInfo->m_Path.GetDataDirParentRelativePath());
    }
  }
}

WTransformStatus WAssetCurator::TransformAsset(const WUuid& assetGuid, WBitflags<WTransformFlags> transformFlags, const WPlatformProfile* pAssetProfile)
{
  WTransformStatus res;
  WStringBuilder sAbsPath;
  WStopwatch timer;
  const WAssetDocumentTypeDescriptor* pTypeDesc = nullptr;
  {
    W_LOCK(m_CuratorMutex);

    WAssetInfo* pInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
      return WTransformStatus("Transform failed, unknown asset.");

    sAbsPath = pInfo->m_Path;
    res = ProcessAsset(pInfo, pAssetProfile, transformFlags);

    // A manually triggered transform reports failures only through the log, leaving the transform state
    // at whatever it was before. The asset curator panel filters on that state, so a broken asset would
    // not be listed until something else invalidates it. Record the error here rather than in
    // ProcessAsset, which recurses into the dependencies and would blame them for a failure of this
    // asset. The background path does its own recording in WAssetProcessor once the result comes back
    // from the processor, so this only covers the manual case.
    if (res.Failed() && transformFlags.IsSet(WTransformFlags::TriggeredManually))
    {
      WDynamicArray<WLogEntry> logEntries;
      auto& entry = logEntries.ExpandAndGetRef();
      entry.m_sMsg = res.m_sMessage;
      entry.m_Type = WLogMsgType::ErrorMsg;

      UpdateAssetTransformLog(assetGuid, logEntries);
      UpdateAssetTransformState(assetGuid, WAssetInfo::TransformState::TransformError);
    }
  }
  if (pTypeDesc && transformFlags.IsAnySet(WTransformFlags::TriggeredManually))
  {
    // As this is triggered manually it is safe to save here as these are only run on the main thread.
    if (WDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsPath))
    {
      // some assets modify the document during transformation
      // make sure the state is saved, at least when the user actively executed the action
      pDoc->SaveDocument().LogFailure();
    }
  }
  WLog::Info("Transform asset time: {0}s", WArgF(timer.GetRunningTotal().GetSeconds(), 2));
  return res;
}

WTransformStatus WAssetCurator::CreateThumbnail(const WUuid& assetGuid)
{
  W_LOCK(m_CuratorMutex);

  WAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return WStatus("Create thumbnail failed, unknown asset.");

  return ProcessAsset(pInfo, nullptr, WTransformFlags::None);
}

void WAssetCurator::TransformAssetsForSceneExport(const WPlatformProfile* pAssetProfile /*= nullptr*/)
{
  W_PROFILE_SCOPE("Transform Special Assets");

  WSet<WTempHashedString> types;

  {
    auto& allDMs = WDocumentManager::GetAllDocumentManagers();
    for (auto& dm : allDMs)
    {
      if (WAssetDocumentManager* pADM = WDynamicCast<WAssetDocumentManager*>(dm))
      {
        pADM->GetAssetTypesRequiringTransformForSceneExport(types);
      }
    }
  }

  WSet<WUuid> assets;
  {
    WAssetCurator::WLockedAssetTable allAssets = GetKnownAssets();

    for (auto it : *allAssets)
    {
      if (types.Contains(it.Value()->m_Info->m_sAssetsDocumentTypeName))
      {
        assets.Insert(it.Value()->m_Info->m_DocumentID);
      }
    }
  }

  for (const auto& guid : assets)
  {
    // Ignore result
    TransformAsset(guid, WTransformFlags::TriggeredManually | WTransformFlags::ForceTransform, pAssetProfile);
  }
}

WResult WAssetCurator::WriteAssetTables(const WPlatformProfile* pAssetProfile, bool bForce)
{
  CURATOR_PROFILE("WriteAssetTables");
  W_LOG_BLOCK("WAssetCurator::WriteAssetTables");

  if (pAssetProfile == nullptr)
  {
    pAssetProfile = GetActiveAssetProfile();
  }

  return m_pAssetTableWriter->WriteAssetTables(pAssetProfile, bForce);
}


////////////////////////////////////////////////////////////////////////
// WAssetCurator Asset Access
////////////////////////////////////////////////////////////////////////

const WAssetCurator::WLockedSubAsset WAssetCurator::FindSubAsset(WStringView sPathOrGuid, bool bExhaustiveSearch) const
{
  CURATOR_PROFILE("FindSubAsset");
  W_LOCK(m_CuratorMutex);

  if (WConversionUtils::IsStringUuid(sPathOrGuid))
  {
    return GetSubAsset(WConversionUtils::ConvertStringToUuid(sPathOrGuid));
  }

  // Split into mainAsset|subAsset
  WStringBuilder mainAsset;
  WStringView subAsset;
  const char* szSeparator = sPathOrGuid.FindSubString("|");
  if (szSeparator != nullptr)
  {
    mainAsset.SetSubString_FromTo(sPathOrGuid.GetStartPointer(), szSeparator);
    subAsset = WStringView(szSeparator + 1);
  }
  else
  {
    mainAsset = sPathOrGuid;
  }
  mainAsset.MakeCleanPath();

  // Find mainAsset
  WFileStatus stat;
  WResult res = WFileSystemModel::GetSingleton()->FindFile(mainAsset, stat);

  // Did we find an asset?
  if (res == W_SUCCESS && stat.m_DocumentID.IsValid())
  {
    WAssetInfo* pAssetInfo = nullptr;
    m_KnownAssets.TryGetValue(stat.m_DocumentID, pAssetInfo);
    W_ASSERT_DEV(pAssetInfo != nullptr, "Files reference non-existant assset!");

    if (subAsset.IsValid())
    {
      for (const WUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto itSub = m_KnownSubAssets.Find(sub);
        if (itSub.IsValid() && subAsset.IsEqual_NoCase(itSub.Value().GetName()))
        {
          return WLockedSubAsset(m_CuratorMutex, &itSub.Value());
        }
      }
    }
    else
    {
      auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
      return WLockedSubAsset(m_CuratorMutex, &itSub.Value());
    }
  }

  if (!bExhaustiveSearch)
    return WLockedSubAsset();

  // TODO: This is the old slow code path that will find the longest substring match.
  // Should be removed or folded into FindBestMatchForFile once it's surely not needed anymore.

  auto FindAsset = [this](WStringView sPathView) -> WAssetInfo*
  {
    // try to find the 'exact' relative path
    // otherwise find the shortest possible path
    WUInt32 uiMinLength = 0xFFFFFFFF;
    WAssetInfo* pBestInfo = nullptr;

    if (sPathView.IsEmpty())
      return nullptr;

    const WStringBuilder sPath = sPathView;
    const WStringBuilder sPathWithSlash("/", sPath);

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_Path.GetDataDirParentRelativePath().EndsWith_NoCase(sPath))
      {
        // endswith -> could also be equal
        if (sPathView.IsEqual_NoCase(it.Value()->m_Path.GetDataDirParentRelativePath()))
        {
          // if equal, just take it
          return it.Value();
        }

        // need to check again with a slash to make sure we don't return something that is of an invalid type
        // this can happen where the user is allowed to type random paths
        if (it.Value()->m_Path.GetDataDirParentRelativePath().EndsWith_NoCase(sPathWithSlash))
        {
          const WUInt32 uiLength = it.Value()->m_Path.GetDataDirParentRelativePath().GetElementCount();
          if (uiLength < uiMinLength)
          {
            uiMinLength = uiLength;
            pBestInfo = it.Value();
          }
        }
      }
    }

    return pBestInfo;
  };

  szSeparator = sPathOrGuid.FindSubString("|");
  if (szSeparator != nullptr)
  {
    WStringBuilder mainAsset2;
    mainAsset2.SetSubString_FromTo(sPathOrGuid.GetStartPointer(), szSeparator);

    WStringView subAsset2(szSeparator + 1);
    if (WAssetInfo* pAssetInfo = FindAsset(mainAsset2))
    {
      for (const WUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto subIt = m_KnownSubAssets.Find(sub);
        if (subIt.IsValid() && subAsset2.IsEqual_NoCase(subIt.Value().GetName()))
        {
          return WLockedSubAsset(m_CuratorMutex, &subIt.Value());
        }
      }
    }
  }

  WStringBuilder sPath = sPathOrGuid;
  sPath.MakeCleanPath();
  if (sPath.IsAbsolutePath())
  {
    if (!WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
      return WLockedSubAsset();
  }

  if (WAssetInfo* pAssetInfo = FindAsset(sPath))
  {
    auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
    return WLockedSubAsset(m_CuratorMutex, &itSub.Value());
  }
  return WLockedSubAsset();
}

const WAssetCurator::WLockedSubAsset WAssetCurator::GetSubAsset(const WUuid& assetGuid) const
{
  W_LOCK(m_CuratorMutex);

  auto it = m_KnownSubAssets.Find(assetGuid);
  if (it.IsValid())
  {
    const WSubAsset* pAssetInfo = &(it.Value());
    return WLockedSubAsset(m_CuratorMutex, pAssetInfo);
  }
  return WLockedSubAsset();
}

const WAssetCurator::WLockedSubAssetTable WAssetCurator::GetKnownSubAssets() const
{
  return WLockedSubAssetTable(m_CuratorMutex, &m_KnownSubAssets);
}

const WAssetCurator::WLockedAssetTable WAssetCurator::GetKnownAssets() const
{
  return WLockedAssetTable(m_CuratorMutex, &m_KnownAssets);
}

WUInt64 WAssetCurator::GetAssetTransformHash(WUuid assetGuid)
{
  WUInt64 assetHash = 0;
  WUInt64 thumbHash = 0;
  WUInt64 packageHash = 0;
  WAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, packageHash, false);
  return assetHash;
}

WUInt64 WAssetCurator::GetAssetThumbnailHash(WUuid assetGuid)
{
  WUInt64 assetHash = 0;
  WUInt64 packageHash = 0;
  WUInt64 thumbHash = 0;
  WAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, packageHash, false);
  return thumbHash;
}

WAssetInfo::TransformState WAssetCurator::IsAssetUpToDate(const WUuid& assetGuid, const WPlatformProfile*, const WAssetDocumentTypeDescriptor* pTypeDescriptor, WUInt64& out_uiAssetHash, WUInt64& out_uiThumbHash, WUInt64& out_uiPackageHash, bool bForce)
{
  if (bForce)
  {
    WSet<WUuid> transitiveHull;
    GenerateTransitiveAssetHull(assetGuid, transitiveHull, WDependencyFlags::Transform | WDependencyFlags::Thumbnail | WDependencyFlags::Package);
    // Mark hull as not up to date
    W_LOCK(m_CuratorMutex);
    for (auto& asset : transitiveHull)
    {
      InvalidateAssetTransformState(asset);
      // UpdateAssetTransformState(asset, WAssetInfo::TransformState::Unknown);
    }
  }

  // Running this will update the state of every asset marked as Unknown in the transitive hull.
  return WAssetCurator::UpdateAssetTransformState(assetGuid, out_uiAssetHash, out_uiThumbHash, out_uiPackageHash, false);
}

void WAssetCurator::InvalidateAssetsWithTransformState(WAssetInfo::TransformState state)
{
  W_LOCK(m_CuratorMutex);

  WHashSet<WUuid> allWithState = m_TransformState[state];

  for (const auto& asset : allWithState)
  {
    InvalidateAssetTransformState(asset);
  }
}

WAssetInfo::TransformState WAssetCurator::UpdateAssetTransformState(WUuid assetGuid, WUInt64& out_AssetHash, WUInt64& out_ThumbHash, WUInt64& out_PackageHash, bool bForce)
{
  CURATOR_PROFILE("UpdateAssetTransformState");
  WStringBuilder sAbsAssetPath;
  {
    W_LOCK(m_CuratorMutex);
    // If assetGuid is a sub-asset, redirect to main asset.
    auto it = m_KnownSubAssets.Find(assetGuid);
    if (!it.IsValid())
    {
      return WAssetInfo::Unknown;
    }
    WAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    assetGuid = pAssetInfo->m_Info->m_DocumentID;
    sAbsAssetPath = pAssetInfo->m_Path;

    // Circular dependencies can change if any asset in the circle has changed (and potentially broken the circle). Thus, we need to call CheckForCircularDependencies again for every asset.
    if (!pAssetInfo->m_CircularDependencies.IsEmpty() && m_TransformStateStale.Contains(assetGuid))
    {
      pAssetInfo->m_CircularDependencies.Clear();
      if (CheckForCircularDependencies(pAssetInfo).Failed())
      {
        UpdateAssetTransformState(assetGuid, WAssetInfo::CircularDependency);
        out_AssetHash = 0;
        out_ThumbHash = 0;
        out_PackageHash = 0;
        return WAssetInfo::CircularDependency;
      }
    }

    // Setting an asset to unknown actually does not change the m_TransformState but merely adds it to the m_TransformStateStale list.
    // This is to prevent the user facing state to constantly fluctuate if something is tagged as modified but not actually changed (E.g. saving a
    // file without modifying the content). Thus we need to check for m_TransformStateStale as well as for the set state.
    if (!bForce && pAssetInfo->m_TransformState != WAssetInfo::Unknown && !m_TransformStateStale.Contains(assetGuid))
    {
      out_AssetHash = pAssetInfo->m_AssetHash;
      out_ThumbHash = pAssetInfo->m_ThumbHash;
      out_PackageHash = pAssetInfo->m_PackageHash;
      return pAssetInfo->m_TransformState;
    }
  }

  WFileSystemModel::GetSingleton()->NotifyOfChange(sAbsAssetPath);

  // Data to pull from the asset under the lock that is needed for update computation.
  WAssetDocumentManager* pManager = nullptr;
  const WAssetDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  WString sAssetFile;
  WUInt8 uiLastStateUpdate = 0;
  WUInt64 uiSettingsHash = 0;
  WTempHybridArray<WString, 16> transformDeps;
  WTempHybridArray<WString, 16> thumbnailDeps;
  WTempHybridArray<WString, 16> packageDeps;
  WTempHybridArray<WString, 16> outputs;
  WTempHybridArray<WString, 16> subAssetNames;

  // Lock asset and get all data needed for update computation.
  {
    CURATOR_PROFILE("CopyAssetData");
    W_LOCK(m_CuratorMutex);
    WAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    if (!pAssetInfo)
    {
      WStringBuilder tmp;
      WLog::Error("Asset with GUID {0} is unknown", WConversionUtils::ToString(assetGuid, tmp));
      return WAssetInfo::TransformState::Unknown;
    }
    pManager = pAssetInfo->GetManager();
    pTypeDescriptor = pAssetInfo->m_pDocumentTypeDescriptor;
    sAssetFile = pAssetInfo->m_Path;
    uiLastStateUpdate = pAssetInfo->m_LastStateUpdate;
    // The settings has combines both the file settings and the global profile settings.
    uiSettingsHash = pAssetInfo->m_Info->m_uiSettingsHash + pManager->GetAssetProfileHash();
    for (const WString& dep : pAssetInfo->m_Info->m_TransformDependencies)
    {
      transformDeps.PushBack(dep);
    }
    for (const WString& ref : pAssetInfo->m_Info->m_ThumbnailDependencies)
    {
      thumbnailDeps.PushBack(ref);
    }
    for (const WString& ref : pAssetInfo->m_Info->m_PackageDependencies)
    {
      packageDeps.PushBack(ref);
    }
    for (const WString& output : pAssetInfo->m_Info->m_Outputs)
    {
      outputs.PushBack(output);
    }
    for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
    {
      if (WSubAsset* pSubAsset = GetSubAssetInternal(subAssetUuid))
      {
        subAssetNames.PushBack(pSubAsset->m_Data.m_sName);
      }
    }
  }

  WAssetInfo::TransformState state = WAssetInfo::TransformState::Unknown;
  WSet<WString> missingTransformDeps;
  WSet<WString> missingThumbnailDeps;
  WSet<WString> missingPackageDeps;
  // Compute final state and hashes.
  {
    state = HashAsset(uiSettingsHash, transformDeps, thumbnailDeps, packageDeps, missingTransformDeps, missingThumbnailDeps, missingPackageDeps, out_AssetHash, out_ThumbHash, out_PackageHash, bForce);
    W_ASSERT_DEV(state == WAssetInfo::Unknown || state == WAssetInfo::MissingTransformDependency || state == WAssetInfo::MissingThumbnailDependency || state == WAssetInfo::MissingPackageDependency, "Unhandled case of HashAsset return value.");

    if (state == WAssetInfo::Unknown)
    {
      if (pManager->IsOutputUpToDate(sAssetFile, outputs, out_AssetHash, pTypeDescriptor))
      {
        state = WAssetInfo::TransformState::UpToDate;
        if (pTypeDescriptor->m_AssetDocumentFlags.IsAnySet(WAssetDocumentFlags::SupportsThumbnail | WAssetDocumentFlags::AutoThumbnailOnTransform))
        {
          if (!pManager->IsThumbnailUpToDate(sAssetFile, "", out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = pTypeDescriptor->m_AssetDocumentFlags.IsSet(WAssetDocumentFlags::AutoThumbnailOnTransform) ? WAssetInfo::TransformState::NeedsTransform : WAssetInfo::TransformState::NeedsThumbnail;
          }
        }
        else if (pTypeDescriptor->m_AssetDocumentFlags.IsAnySet(WAssetDocumentFlags::SubAssetsSupportThumbnail | WAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform))
        {
          for (const WString& subAssetName : subAssetNames)
          {
            if (!pManager->IsThumbnailUpToDate(sAssetFile, subAssetName, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
            {
              state = pTypeDescriptor->m_AssetDocumentFlags.IsSet(WAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform) ? WAssetInfo::TransformState::NeedsTransform : WAssetInfo::TransformState::NeedsThumbnail;
              break;
            }
          }
        }
      }
      else
      {
        state = WAssetInfo::TransformState::NeedsTransform;
      }
    }
  }

  {
    W_LOCK(m_CuratorMutex);
    WAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    if (pAssetInfo)
    {
      // Only update the state if the asset state remains unchanged since we gathered its data.
      // Otherwise the state we computed would already be stale. Return the data regardless
      // instead of waiting for a new computation as the case in which the value has actually changed
      // is very rare (asset modified between the two locks) in which case we will just create
      // an already stale transform / thumbnail which will be immediately replaced again.
      if (pAssetInfo->m_LastStateUpdate == uiLastStateUpdate)
      {
        UpdateAssetTransformState(assetGuid, state);
        pAssetInfo->m_AssetHash = out_AssetHash;
        pAssetInfo->m_ThumbHash = out_ThumbHash;
        pAssetInfo->m_PackageHash = out_PackageHash;
        pAssetInfo->m_MissingTransformDeps = std::move(missingTransformDeps);
        pAssetInfo->m_MissingThumbnailDeps = std::move(missingThumbnailDeps);
        pAssetInfo->m_MissingPackageDeps = std::move(missingPackageDeps);
        if (state == WAssetInfo::TransformState::UpToDate)
        {
          if (UpdateSubAssets(*pAssetInfo).Failed())
          {
            UpdateAssetTransformState(assetGuid, WAssetInfo::TransformError);
            state = WAssetInfo::TransformState::TransformError;
          }
        }
      }
    }
    else
    {
      WStringBuilder tmp;
      WLog::Error("Asset with GUID {0} is unknown", WConversionUtils::ToString(assetGuid, tmp));
      return WAssetInfo::TransformState::Unknown;
    }
    return state;
  }
}

void WAssetCurator::GetAssetTransformStats(WUInt32& out_uiNumAssets, WHybridArray<WUInt32, WAssetInfo::TransformState::COUNT>& out_count)
{
  W_LOCK(m_CuratorMutex);
  out_count.SetCountUninitialized(WAssetInfo::TransformState::COUNT);
  for (int i = 0; i < WAssetInfo::TransformState::COUNT; i++)
  {
    out_count[i] = m_TransformState[i].GetCount();
  }

  out_uiNumAssets = m_KnownAssets.GetCount();
}

WString WAssetCurator::FindDataDirectoryForAsset(WStringView sAbsoluteAssetPath) const
{
  WStringBuilder sAssetPath(sAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    WStringBuilder sDataDir;
    WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  W_REPORT_FAILURE("Could not find data directory for asset '{0}", sAbsoluteAssetPath);
  return WFileSystem::GetSdkRootDirectory();
}

void WAssetCurator::GetAllAssetsInFolder(WStringView sFolderPath, WDynamicArray<WString>& out_assetGuids) const
{
  W_LOCK(m_CuratorMutex);

  WStringBuilder sFolderPathClean = sFolderPath;
  sFolderPathClean.MakeCleanPath();

  if (!sFolderPathClean.EndsWith("/"))
    sFolderPathClean.Append("/");

  WStringBuilder sTemp;

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    const WAssetInfo* pAssetInfo = it.Value();
    const WString& sAssetPath = pAssetInfo->m_Path.GetAbsolutePath();

    sTemp = sAssetPath;
    sTemp.MakeCleanPath();

    if (sTemp.StartsWith_NoCase(sFolderPathClean))
    {
      WConversionUtils::ToString(pAssetInfo->m_Info->m_DocumentID, sTemp);
      out_assetGuids.PushBack(sTemp);
    }
  }
}

WResult WAssetCurator::FindBestMatchForFile(WStringBuilder& ref_sFile, WArrayPtr<WString> allowedFileExtensions) const
{
  // TODO: Merge with exhaustive search in FindSubAsset
  ref_sFile.MakeCleanPath();

  WStringBuilder testName = ref_sFile;

  for (const auto& ext : allowedFileExtensions)
  {
    testName.ChangeFileExtension(ext);

    if (WFileSystem::ExistsFile(testName))
    {
      ref_sFile = testName;
      goto found;
    }
  }

  testName = ref_sFile.GetFileNameAndExtension();

  if (testName.IsEmpty())
  {
    ref_sFile = "";
    return W_FAILURE;
  }

  if (WPathUtils::ContainsInvalidFilenameChars(testName))
  {
    // not much we can do here, if the filename is already invalid, we will probably not find it in out known files list

    WPathUtils::MakeValidFilename(testName, '_', ref_sFile);
    return W_FAILURE;
  }

  {
    W_LOCK(m_CuratorMutex);

    auto SearchFile = [this](WStringBuilder& ref_sName) -> bool
    {
      return WFileSystemModel::GetSingleton()->FindFile([&ref_sName](const WDataDirPath& file, const WFileStatus& stat)
                                                {
                                                  if (stat.m_Status != WFileStatus::Status::Valid)
                                                    return false;

                                                  if (file.GetAbsolutePath().EndsWith_NoCase(ref_sName))
                                                  {
                                                    ref_sName = file.GetAbsolutePath();
                                                    return true;
                                                  }
                                                  return false; //
                                                })
        .Succeeded();
    };

    // search for the full name
    {
      testName.Prepend("/"); // make sure to not find partial names

      for (const auto& ext : allowedFileExtensions)
      {
        testName.ChangeFileExtension(ext);

        if (SearchFile(testName))
          goto found;
      }
    }

    return W_FAILURE;
  }

found:
  if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(testName))
  {
    ref_sFile = testName;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WAssetCurator::FindAllUses(WUuid assetGuid, WSet<WUuid>& ref_uses, bool bTransitive) const
{
  W_LOCK(m_CuratorMutex);

  WSet<WUuid> todoList;
  todoList.Insert(assetGuid);

  auto GatherReferences = [&](const WMap<WString, WHybridArray<WUuid, 1>>& inverseTracker, const WStringBuilder& sAsset)
  {
    auto it = inverseTracker.Find(sAsset);
    if (it.IsValid())
    {
      for (const WUuid& guid : it.Value())
      {
        if (!ref_uses.Contains(guid))
          todoList.Insert(guid);

        ref_uses.Insert(guid);
      }
    }
  };

  WStringBuilder sCurrentAsset;
  do
  {
    auto itFirst = todoList.GetIterator();
    const WAssetInfo* pInfo = GetAssetInfo(itFirst.Key());
    todoList.Remove(itFirst);

    if (pInfo)
    {
      sCurrentAsset = pInfo->m_Path;
      GatherReferences(m_InverseTransformDeps, sCurrentAsset);
      GatherReferences(m_InverseThumbnailDeps, sCurrentAsset);
      GatherReferences(m_InversePackageDeps, sCurrentAsset);
    }
  } while (bTransitive && !todoList.IsEmpty());
}

void WAssetCurator::FindAllUses(WStringView sAbsolutePath, WSet<WUuid>& ref_uses) const
{
  W_LOCK(m_CuratorMutex);

  auto GatherReferences = [&](const WMap<WString, WHybridArray<WUuid, 1>>& inverseTracker)
  {
    if (auto it = inverseTracker.Find(sAbsolutePath); it.IsValid())
    {
      for (const WUuid& guid : it.Value())
      {
        ref_uses.Insert(guid);
      }
    }
  };

  GatherReferences(m_InverseTransformDeps);
  GatherReferences(m_InverseThumbnailDeps);
  GatherReferences(m_InversePackageDeps);
}

bool WAssetCurator::IsReferenced(WStringView sAbsolutePath) const
{
  W_LOCK(m_CuratorMutex);
  auto it = m_InverseTransformDeps.Find(sAbsolutePath);
  return it.IsValid() && !it.Value().IsEmpty();
}

////////////////////////////////////////////////////////////////////////
// WAssetCurator Manual and Automatic Change Notification
////////////////////////////////////////////////////////////////////////

void WAssetCurator::NotifyOfFileChange(WStringView sAbsolutePath)
{
  WStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  WFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
}

void WAssetCurator::NotifyOfAssetChange(const WUuid& assetGuid)
{
  InvalidateAssetTransformState(assetGuid);
}

void WAssetCurator::UpdateAssetLastAccessTime(const WUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (!it.IsValid())
    return;

  it.Value().m_LastAccess = WTime::Now();
}

void WAssetCurator::CheckFileSystem()
{
  W_PROFILE_SCOPE("CheckFileSystem");
  WStopwatch sw;

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  {
    W_LOCK(m_CuratorMutex);
    SetAllAssetStatusUnknown();
  }
  WFileSystemModel::GetSingleton()->CheckFileSystem();

  if (WThreadUtils::IsMainThread())
  {
    // Broadcast reset only if we are on the main thread.
    // Otherwise we are on the init task thread and the reset will be called on the main thread by WaitForInitialize.
    WAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = WAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  RestartUpdateTask();

  WLog::Debug("Asset Curator Refresh Time: {0} ms", WArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void WAssetCurator::NeedsReloadResources(const WUuid& assetGuid) const
{
  if (m_pAssetTableWriter)
  {
    m_pAssetTableWriter->NeedsReloadResource(assetGuid);

    WAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    {
      for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
      {
        m_pAssetTableWriter->NeedsReloadResource(subAssetUuid);
      }
    }
  }
}

void WAssetCurator::GenerateTransitiveHull(const WStringView sAssetOrPath, WSet<WString>& inout_deps, WBitflags<WDependencyFlags> dependencyTypes) const
{
  W_LOCK(m_CuratorMutex);

  WTempHybridArray<WString, 6> toDoList;
  if (WConversionUtils::IsStringUuid(sAssetOrPath))
  {
    inout_deps.Insert(sAssetOrPath);
    toDoList.PushBack(sAssetOrPath);
  }
  else
  {
    auto subAsset = FindSubAsset(sAssetOrPath);
    if (subAsset.isValid())
    {
      WStringBuilder sTmp;
      WConversionUtils::ToString(subAsset->m_pAssetInfo->m_Info->m_DocumentID, sTmp);
      inout_deps.Insert(sTmp);
      toDoList.PushBack(sTmp);
    }
    else
    {
      inout_deps.Insert(sAssetOrPath);
      toDoList.PushBack(sAssetOrPath);
    }
  }

  while (!toDoList.IsEmpty())
  {
    WString currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    if (WConversionUtils::IsStringUuid(currentAsset))
    {
      auto it = m_KnownSubAssets.Find(WConversionUtils::ConvertStringToUuid(currentAsset));
      WAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;

      if (dependencyTypes.IsSet(WDependencyFlags::Transform))
      {
        for (const WString& dep : pAssetInfo->m_Info->m_TransformDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
      if (dependencyTypes.IsSet(WDependencyFlags::Thumbnail))
      {
        for (const WString& dep : pAssetInfo->m_Info->m_ThumbnailDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
      if (dependencyTypes.IsSet(WDependencyFlags::Package))
      {
        for (const WString& dep : pAssetInfo->m_Info->m_PackageDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
    }
  }
}

void WAssetCurator::GenerateTransitiveAssetHull(const WUuid& assetGuid, WSet<WUuid>& inout_deps, WBitflags<WDependencyFlags> dependencyTypes)
{
  WTempHybridArray<WUuid, 6> toDoList;

  auto AddDependencies = [&](const WSet<WString>& dependencies)
  {
    for (const WString& dep : dependencies)
    {
      if (!WConversionUtils::IsStringUuid(dep))
        continue;
      WUuid guid = WConversionUtils::ConvertStringToUuid(dep);
      if (!inout_deps.Contains(guid))
      {
        inout_deps.Insert(guid);
        toDoList.PushBack(guid);
      }
    }
  };

  // Build the transitive hull of all assets under 'assetGuid'.
  inout_deps.Insert(assetGuid);
  toDoList.PushBack(assetGuid);
  WStringBuilder sAbsAssetPath;
  while (!toDoList.IsEmpty())
  {
    WUuid currentAsset = toDoList.PeekBack();
    toDoList.PopBack();
    {
      W_LOCK(m_CuratorMutex);
      auto it = m_KnownSubAssets.Find(currentAsset);
      if (!it.IsValid())
        continue;
      sAbsAssetPath = it.Value().m_pAssetInfo->m_Path;
    }
    // To make sure the dependencies of the asset are up-to-date, we need to check for modifications.
    // This must be done outside the lock to prevent deadlocks.
    WFileSystemModel::GetSingleton()->NotifyOfChange(sAbsAssetPath);

    W_LOCK(m_CuratorMutex);
    auto it = m_KnownSubAssets.Find(currentAsset);
    if (!it.IsValid())
      continue;

    WAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    if (dependencyTypes.IsSet(WDependencyFlags::Transform))
      AddDependencies(pAssetInfo->m_Info->m_TransformDependencies);
    if (dependencyTypes.IsSet(WDependencyFlags::Thumbnail))
      AddDependencies(pAssetInfo->m_Info->m_ThumbnailDependencies);
    if (dependencyTypes.IsSet(WDependencyFlags::Package))
      AddDependencies(pAssetInfo->m_Info->m_PackageDependencies);
  }
}

void WAssetCurator::GenerateSettingsHashMap(const WSet<WString>& deps, WBitflags<WDependencyFlags> dependencyType, WMap<WString, WUInt64>& out_settingsHashMap) const
{
  W_LOCK(m_CuratorMutex);

  for (const WString& sDepOrRef : deps)
  {
    WUInt64 uiAssetHash = 0;
    if (WConversionUtils::IsStringUuid(sDepOrRef))
    {
      auto it = m_KnownAssets.Find(WConversionUtils::ConvertStringToUuid(sDepOrRef));
      if (it.IsValid())
      {
        for (WDependencyFlags::Enum dep : dependencyType)
        {
          switch (dep)
          {
            case WDependencyFlags::Thumbnail:
              uiAssetHash = it.Value()->m_ThumbHash;
              break;
            case WDependencyFlags::Transform:
              uiAssetHash = it.Value()->m_AssetHash;
              break;
            case WDependencyFlags::Package:
              uiAssetHash = it.Value()->m_PackageHash;
              break;
            default:
              break;
          }
        }
      }
    }
    else
    {
      WStringBuilder sTmp = sDepOrRef;
      if (WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTmp))
      {
        WFileStatus fileStatus;
        WResult res = WFileSystemModel::GetSingleton()->HashFile(sTmp, fileStatus);
        uiAssetHash = res.Failed() ? 1 : fileStatus.m_uiHash;
      }
      else
      {
        uiAssetHash = 2;
      }
    }
    out_settingsHashMap.Insert(sDepOrRef, uiAssetHash);
  }
}

void WAssetCurator::GenerateInverseTransitiveHull(const WAssetInfo* pAssetInfo, WSet<WUuid>& inout_inverseDeps, bool bIncludeTransformDebs, bool bIncludeThumbnailDebs) const
{
  W_LOCK(m_CuratorMutex);

  WTempHybridArray<const WAssetInfo*, 6> toDoList;
  toDoList.PushBack(pAssetInfo);
  inout_inverseDeps.Insert(pAssetInfo->m_Info->m_DocumentID);

  while (!toDoList.IsEmpty())
  {
    const WAssetInfo* currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    if (bIncludeTransformDebs)
    {
      if (auto it = m_InverseTransformDeps.Find(currentAsset->m_Path.GetAbsolutePath()); it.IsValid())
      {
        for (const WUuid& asset : it.Value())
        {
          if (!inout_inverseDeps.Contains(asset))
          {
            WAssetInfo* pAssetInfo = nullptr;
            if (m_KnownAssets.TryGetValue(asset, pAssetInfo))
            {
              toDoList.PushBack(pAssetInfo);
              inout_inverseDeps.Insert(asset);
            }
          }
        }
      }
    }

    if (bIncludeThumbnailDebs)
    {
      if (auto it = m_InverseThumbnailDeps.Find(currentAsset->m_Path.GetAbsolutePath()); it.IsValid())
      {
        for (const WUuid& asset : it.Value())
        {
          if (!inout_inverseDeps.Contains(asset))
          {
            WAssetInfo* pAssetInfo = nullptr;
            if (m_KnownAssets.TryGetValue(asset, pAssetInfo))
            {
              toDoList.PushBack(pAssetInfo);
              inout_inverseDeps.Insert(asset);
            }
          }
        }
      }
    }
  }
}

void WAssetCurator::WriteDependencyDGML(const WUuid& guid, WStringView sOutputFile) const
{
  W_LOCK(m_CuratorMutex);

  WDGMLGraph graph;

  WSet<WString> deps;
  WStringBuilder sTemp;
  GenerateTransitiveHull(WConversionUtils::ToString(guid, sTemp), deps, WDependencyFlags::Transform | WDependencyFlags::Thumbnail);

  WHashTable<WString, WUInt32> nodeMap;
  nodeMap.Reserve(deps.GetCount());
  for (auto& dep : deps)
  {
    WDGMLGraph::NodeDesc nd;
    if (WConversionUtils::IsStringUuid(dep))
    {
      auto it = m_KnownSubAssets.Find(WConversionUtils::ConvertStringToUuid(dep));
      const WSubAsset& subAsset = it.Value();
      const WAssetInfo* pAssetInfo = subAsset.m_pAssetInfo;
      if (subAsset.m_bMainAsset)
      {
        nd.m_Color = WColor::Blue;
        sTemp.SetFormat("{}", pAssetInfo->m_Path.GetDataDirParentRelativePath());
      }
      else
      {
        nd.m_Color = WColor::AliceBlue;
        sTemp.SetFormat("{} | {}", pAssetInfo->m_Path.GetDataDirParentRelativePath(), subAsset.GetName());
      }
      nd.m_Shape = WDGMLGraph::NodeShape::Rectangle;
    }
    else
    {
      sTemp = dep;
      nd.m_Color = WColor::Orange;
      nd.m_Shape = WDGMLGraph::NodeShape::Rectangle;
    }
    WUInt32 uiGraphNode = graph.AddNode(sTemp, &nd);
    nodeMap.Insert(dep, uiGraphNode);
  }

  for (auto& node : deps)
  {
    WDGMLGraph::NodeDesc nd;
    if (WConversionUtils::IsStringUuid(node))
    {
      WUInt32 uiInputNode = *nodeMap.GetValue(node);

      auto it = m_KnownSubAssets.Find(WConversionUtils::ConvertStringToUuid(node));
      WAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;

      WMap<WUInt32, WString> connection;

      auto ExtendConnection = [&](const WString& sRef, WStringView sLabel)
      {
        WUInt32 uiOutputNode = *nodeMap.GetValue(sRef);
        sTemp = connection[uiOutputNode];
        if (sTemp.IsEmpty())
          sTemp = sLabel;
        else
          sTemp.AppendFormat(" | {}", sLabel);
        connection[uiOutputNode] = sTemp;
      };

      for (const WString& sRef : pAssetInfo->m_Info->m_TransformDependencies)
      {
        ExtendConnection(sRef, "Transform");
      }

      for (const WString& sRef : pAssetInfo->m_Info->m_ThumbnailDependencies)
      {
        ExtendConnection(sRef, "Thumbnail");
      }

      // This will make the graph very big, not recommended.
      /* for (const WString& ref : pAssetInfo->m_Info->m_PackageDependencies)
       {
         ExtendConnection(ref, "Package");
       }*/

      for (auto it : connection)
      {
        graph.AddConnection(uiInputNode, it.Key(), it.Value());
      }
    }
  }

  WDGMLGraphWriter::WriteGraphToFile(sOutputFile, graph).IgnoreResult();
}

WAssetCurator::ExportResult WAssetCurator::ExportAssets(WArrayPtr<WString> sources, WStringView sDestinationFolder, WBitflags<WDependencyFlags> includeDependencyTypes) const
{
  W_LOCK(m_CuratorMutex);

  ExportResult result;

  WSet<WString> allDependencies;

  for (const WString& source : sources)
  {
    GenerateTransitiveHull(source, allDependencies, includeDependencyTypes);
  }

  WStringBuilder sDestPath = sDestinationFolder;
  WStringBuilder sAbsPath;
  WStringBuilder sRelPath;
  WStringBuilder sTargetPath;
  WStringBuilder sTargetDir;

  for (const WString& dep : allDependencies)
  {
    sAbsPath.Clear();
    const WDataDirectoryInfo* ddi = nullptr;

    if (WConversionUtils::IsStringUuid(dep))
    {
      WUuid depGuid = WConversionUtils::ConvertStringToUuid(dep);
      const auto pDepAsset = GetSubAsset(depGuid);

      if (!pDepAsset.isValid())
        continue;

      sAbsPath = pDepAsset->m_pAssetInfo->m_Path.GetAbsolutePath();
      sRelPath = pDepAsset->m_pAssetInfo->m_Path.GetDataDirRelativePath();

      // the index in GetDataDirIndex() doesn't seem to match the index of WFileSystem
      // ddi = &WFileSystem::GetDataDirectoryInfo(pDepAsset->m_pAssetInfo->m_Path.GetDataDirIndex());

      if (WFileSystem::ResolvePath(sAbsPath, &sAbsPath, &sRelPath, &ddi).Failed())
        continue;
    }
    else
    {
      if (WFileSystem::ResolvePath(dep, &sAbsPath, &sRelPath, &ddi).Failed())
        continue;
    }

    if (ddi->m_sRootName == "BASE")
      continue;

    if (!WOSFile::ExistsFile(sAbsPath))
      continue;


    WStringBuilder sTargetPath = sDestPath;
    sTargetPath.AppendPath(sRelPath);

    if (WOSFile::CopyFile(sAbsPath, sTargetPath).Failed())
    {
      result.m_uiFailedFiles++;
      continue;
    }

    ++result.m_uiCopiedFiles;
  }

  return result;
}

////////////////////////////////////////////////////////////////////////
// WAssetCurator Processing
////////////////////////////////////////////////////////////////////////

WCommandLineOptionEnum opt_AssetThumbnails("_Editor", "-AssetThumbnails", "Whether to generate thumbnails for transformed assets.", "default = 0 | never = 1", 0);

WTransformStatus WAssetCurator::ProcessAsset(WAssetInfo* pAssetInfo, const WPlatformProfile* pAssetProfile, WBitflags<WTransformFlags> transformFlags)
{
  if (transformFlags.IsSet(WTransformFlags::ForceTransform))
    WLog::Dev("Asset transform forced.");

  const WAssetDocumentTypeDescriptor* pTypeDesc = pAssetInfo->m_pDocumentTypeDescriptor;
  WUInt64 uiHash = 0;
  WUInt64 uiThumbHash = 0;
  WUInt64 uiPackageHash = 0;
  WAssetInfo::TransformState state = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash, uiPackageHash);

  if (state == WAssetInfo::TransformState::CircularDependency)
  {
    return WTransformStatus(WFmt("Circular dependency for asset '{0}', can't transform.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  for (const auto& dep : pAssetInfo->m_Info->m_TransformDependencies)
  {
    WBitflags<WTransformFlags> transformFlagsDeps = transformFlags;
    transformFlagsDeps.Remove(WTransformFlags::ForceTransform);
    if (WAssetInfo* pInfo = GetAssetInfo(dep))
    {
      W_SUCCEED_OR_RETURN(ProcessAsset(pInfo, pAssetProfile, transformFlagsDeps));
    }
  }

  WTransformStatus resReferences;
  for (const auto& ref : pAssetInfo->m_Info->m_ThumbnailDependencies)
  {
    WBitflags<WTransformFlags> transformFlagsRefs = transformFlags;
    transformFlagsRefs.Remove(WTransformFlags::ForceTransform);
    if (WAssetInfo* pInfo = GetAssetInfo(ref))
    {
      resReferences = ProcessAsset(pInfo, pAssetProfile, transformFlagsRefs);
      if (resReferences.Failed())
        break;
    }
  }


  W_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<WAssetDocument>(), "Asset document does not derive from correct base class ('{0}')", pAssetInfo->m_Path.GetDataDirParentRelativePath());

  auto assetFlags = pTypeDesc->m_AssetDocumentFlags;

  // Skip assets that cannot be auto-transformed.
  {
    if (assetFlags.IsAnySet(WAssetDocumentFlags::DisableTransform))
      return WStatus(W_SUCCESS);

    if (!transformFlags.IsSet(WTransformFlags::TriggeredManually) && assetFlags.IsAnySet(WAssetDocumentFlags::OnlyTransformManually))
      return WStatus(W_SUCCESS);
  }

  // If references are not complete and we generate thumbnails on transform we can cancel right away.
  if (assetFlags.IsSet(WAssetDocumentFlags::AutoThumbnailOnTransform) && resReferences.Failed())
  {
    return resReferences;
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  {
    // Sanity check that transforming the dependencies did not change the asset's transform state.
    // In theory this can happen if an asset is transformed by multiple processes at the same time or changes to the file system are being made in the middle of the transform.
    // If this can be reproduced consistently, it is usually a bug in the dependency tracking or other part of the asset curator.
    WUInt64 uiHash2 = 0;
    WUInt64 uiThumbHash2 = 0;
    WUInt64 uiPackageHash2 = 0;
    WAssetInfo::TransformState state2 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash2, uiThumbHash2, uiPackageHash2);

    if (uiHash != uiHash2)
      return WTransformStatus(WFmt("Asset hash changed while processing dependencies from {} to {}", uiHash, uiHash2));
    if (uiThumbHash != uiThumbHash2)
      return WTransformStatus(WFmt("Asset thumbnail hash changed while processing dependencies from {} to {}", uiThumbHash, uiThumbHash2));
    if (uiPackageHash != uiPackageHash2)
      return WTransformStatus(WFmt("Asset package hash changed while processing dependencies from {} to {}", uiPackageHash, uiPackageHash2));
    if (state != state2)
      return WTransformStatus(WFmt("Asset state changed while processing dependencies from {} to {}", state, state2));
  }
#endif

  if (transformFlags.IsSet(WTransformFlags::ForceTransform))
  {
    state = WAssetInfo::NeedsTransform;
  }

  if (state == WAssetInfo::TransformState::UpToDate)
    return WStatus(W_SUCCESS);

  if (state == WAssetInfo::TransformState::MissingTransformDependency)
  {
    return WTransformStatus(WFmt("Missing dependency for asset '{0}', can't transform.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  // does the document already exist and is open ?
  bool bWasOpen = false;
  WDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_Path);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = WQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_Path.GetAbsolutePath(), WDocumentFlags::None);

  if (pDoc == nullptr)
    return WTransformStatus(WFmt("Could not open asset document '{0}'", pAssetInfo->m_Path.GetDataDirParentRelativePath()));

  W_SCOPE_EXIT(if (!pDoc->HasWindowBeenRequested() && !bWasOpen) pDoc->GetDocumentManager()->CloseDocument(pDoc););

  WTransformStatus ret;
  WAssetDocument* pAsset = static_cast<WAssetDocument*>(pDoc);
  if (state == WAssetInfo::TransformState::NeedsTransform || (state == WAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(WAssetDocumentFlags::AutoThumbnailOnTransform)) || (transformFlags.IsSet(WTransformFlags::TriggeredManually) && state == WAssetInfo::TransformState::NeedsImport))
  {
    ret = pAsset->TransformAsset(transformFlags, pAssetProfile);
    if (ret.Succeeded())
    {
      m_pAssetTableWriter->NeedsReloadResource(pAsset->GetGuid());

      for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
      {
        m_pAssetTableWriter->NeedsReloadResource(subAssetUuid);
      }
    }
  }

  if (state == WAssetInfo::TransformState::MissingPackageDependency)
  {
    return WTransformStatus(WFmt("Missing package dependency for asset '{0}'. Asset compromised.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  if (state == WAssetInfo::TransformState::MissingThumbnailDependency)
  {
    return WTransformStatus(WFmt("Missing thumbnail dependency for asset '{0}', can't create thumbnail.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  if (opt_AssetThumbnails.GetOptionValue(WCommandLineOption::LogMode::FirstTimeIfSpecified) != 1)
  {
    // skip thumbnail generation, if disabled globally

    if (ret.Succeeded() && assetFlags.IsSet(WAssetDocumentFlags::SupportsThumbnail) && !assetFlags.IsSet(WAssetDocumentFlags::AutoThumbnailOnTransform) && !resReferences.Failed())
    {
      // If the transformed succeeded, the asset should now be in the NeedsThumbnail state unless the thumbnail already exists in which case we are done or the transform made changes to the asset, e.g. a mesh imported new materials in which case we will revert to transform needed as our dependencies need transform. We simply skip the thumbnail generation in this case.
      WAssetInfo::TransformState state3 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash, uiPackageHash);
      if (state3 == WAssetInfo::TransformState::NeedsThumbnail)
      {
        ret = pAsset->CreateThumbnail();
      }
    }
  }

  return ret;
}


WStatus WAssetCurator::ResaveAsset(WAssetInfo* pAssetInfo)
{
  bool bWasOpen = false;
  WDocument* pDoc = pAssetInfo->GetManager()->GetDocumentByPath(pAssetInfo->m_Path);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = WQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_Path.GetAbsolutePath(), WDocumentFlags::None);

  if (pDoc == nullptr)
    return WStatus(WFmt("Could not open asset document '{0}'", pAssetInfo->m_Path.GetDataDirParentRelativePath()));

  WStatus ret = pDoc->SaveDocument(true);

  if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

  return ret;
}

WAssetInfo* WAssetCurator::GetAssetInfo(const WUuid& assetGuid)
{
  WAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;
  return nullptr;
}

const WAssetInfo* WAssetCurator::GetAssetInfo(const WUuid& assetGuid) const
{
  WAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;
  return nullptr;
}

WAssetInfo* WAssetCurator::GetAssetInfo(const WString& sAssetGuid)
{
  if (sAssetGuid.IsEmpty())
    return nullptr;

  if (WConversionUtils::IsStringUuid(sAssetGuid))
  {
    const WUuid guid = WConversionUtils::ConvertStringToUuid(sAssetGuid);

    WAssetInfo* pInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pInfo))
      return pInfo;
  }

  return nullptr;
}

WSubAsset* WAssetCurator::GetSubAssetInternal(const WUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

void WAssetCurator::BuildFileExtensionSet(WSet<WString>& AllExtensions)
{
  WStringBuilder sTemp;
  AllExtensions.Clear();

  const auto& assetTypes = WAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  WMap<WString, const WDocumentTypeDescriptor*> allDesc;
  for (auto it : assetTypes)
  {
    allDesc[WTranslate(it.Key())] = it.Value();
  }

  for (auto it : allDesc)
  {
    const auto desc = it.Value();

    if (desc->m_pManager->GetDynamicRTTI()->IsDerivedFrom<WAssetDocumentManager>())
    {
      sTemp = desc->m_sFileExtension;
      sTemp.ToLower();

      AllExtensions.Insert(sTemp);
    }
  }
}

void WAssetCurator::OnFileChangedEvent(const WFileChangedEvent& e)
{
  switch (e.m_Type)
  {
    case WFileChangedEvent::Type::DocumentLinked:
    case WFileChangedEvent::Type::DocumentUnlinked:
      break;
    case WFileChangedEvent::Type::FileAdded:
    case WFileChangedEvent::Type::FileChanged:
    {
      // If the asset was just added it is not tracked and thus no need to invalidate anything.
      if (e.m_Type == WFileChangedEvent::Type::FileChanged)
      {
        W_LOCK(m_CuratorMutex);
        WUuid guid0 = e.m_Status.m_DocumentID;
        if (guid0.IsValid())
          InvalidateAssetTransformState(guid0);

        auto it = m_InverseTransformDeps.Find(e.m_Path);
        if (it.IsValid())
        {
          for (const WUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseThumbnailDeps.Find(e.m_Path);
        if (it2.IsValid())
        {
          for (const WUuid& guid : it2.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }
      }

      // Assets should never be in an AssetCache folder.
      if (e.m_Path.GetAbsolutePath().FindSubString("/AssetCache/") != nullptr)
      {
        return;
      }

      // check that this is an asset type that we know
      WStringBuilder sExt = WPathUtils::GetFileExtension(e.m_Path);
      sExt.ToLower();
      if (!m_ValidAssetExtensions.Contains(sExt))
      {
        return;
      }

      EnsureAssetInfoUpdated(e.m_Path, e.m_Status).IgnoreResult();
    }
    break;
    case WFileChangedEvent::Type::FileRemoved:
    {
      W_LOCK(m_CuratorMutex);
      WUuid guid0 = e.m_Status.m_DocumentID;
      if (guid0.IsValid())
      {
        if (auto it = m_KnownAssets.Find(guid0); it.IsValid())
        {
          WAssetInfo* pAssetInfo = it.Value();
          W_ASSERT_DEBUG(WFileSystemModel::IsSameFile(e.m_Path, pAssetInfo->m_Path), "");
          UntrackDependencies(pAssetInfo);
          RemoveAssetTransformState(guid0);
          SetAssetExistanceState(*pAssetInfo, WAssetExistanceState::FileRemoved);
        }
      }
      auto it = m_InverseTransformDeps.Find(e.m_Path);
      if (it.IsValid())
      {
        for (const WUuid& guid : it.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }

      auto it2 = m_InverseThumbnailDeps.Find(e.m_Path);
      if (it2.IsValid())
      {
        for (const WUuid& guid : it2.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }
    }
    break;
    case WFileChangedEvent::Type::ModelReset:
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

void WAssetCurator::ProcessAllCoreAssets()
{
  W_PROFILE_SCOPE("ProcessAllCoreAssets");
  if (WQtUiServices::IsHeadless())
    return;

  // The 'Core Assets' are always transformed for the PC platform,
  // as they are needed to run the editor properly
  const WPlatformProfile* pAssetProfile = GetDevelopmentAssetProfile();

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    WStringBuilder sCoreCollectionPath;
    WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sCoreCollectionPath).IgnoreResult();

    WStringBuilder sName = sCoreCollectionPath.GetFileName();
    sName.Append(".WCollectionAsset");
    sCoreCollectionPath.AppendPath(sName);

    QFile coreCollection(sCoreCollectionPath.GetData());
    if (coreCollection.exists())
    {
      auto pSubAsset = FindSubAsset(sCoreCollectionPath);
      if (pSubAsset)
      {
        // prefer certain asset types over others, to ensure that thumbnail generation works
        WTempHybridArray<WTempHashedString, 4> transformOrder;
        transformOrder.PushBack(WTempHashedString("RenderPipeline"));
        transformOrder.PushBack(WTempHashedString());

        WTransformStatus resReferences(W_SUCCESS);

        for (const WTempHashedString& name : transformOrder)
        {
          for (const auto& ref : pSubAsset->m_pAssetInfo->m_Info->m_PackageDependencies)
          {
            if (WAssetInfo* pInfo = GetAssetInfo(ref))
            {
              if (name == WTempHashedString() || pInfo->m_Info->m_sAssetsDocumentTypeName == name)
              {
                resReferences = ProcessAsset(pInfo, pAssetProfile, WTransformFlags::TriggeredManually);
                if (resReferences.Failed())
                {
                  WLog::Error("Core asset '{}' of type '{}' failed transformation.", ref, pInfo->m_Info->m_sAssetsDocumentTypeName);
                }
              }
            }
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// WAssetCurator Update Task
////////////////////////////////////////////////////////////////////////

void WAssetCurator::RestartUpdateTask()
{
  W_LOCK(m_CuratorMutex);
  m_bRunUpdateTask = true;

  RunNextUpdateTask();
}

void WAssetCurator::ShutdownUpdateTask()
{
  {
    W_LOCK(m_CuratorMutex);
    m_bRunUpdateTask = false;
  }

  if (m_pUpdateTask)
  {
    WTaskSystem::WaitForGroup(m_UpdateTaskGroup);

    W_LOCK(m_CuratorMutex);
    m_pUpdateTask.Clear();
  }
}

bool WAssetCurator::GetNextAssetToUpdate(WUuid& guid, WStringBuilder& out_sAbsPath)
{
  W_LOCK(m_CuratorMutex);

  while (!m_TransformStateStale.IsEmpty())
  {
    auto it = m_TransformStateStale.GetIterator();
    guid = it.Key();

    auto pAssetInfo = GetAssetInfo(guid);

    // W_ASSERT_DEBUG(pAssetInfo != nullptr, "Non-existent assets should not have a tracked transform state.");

    if (pAssetInfo != nullptr)
    {
      out_sAbsPath = pAssetInfo->m_Path;
      return true;
    }
    else
    {
      WLog::Error("Non-existent assets ('{0}') should not have a tracked transform state.", guid);
      m_TransformStateStale.Remove(it);
    }
  }

  return false;
}

void WAssetCurator::OnUpdateTaskFinished(const WSharedPtr<WTask>& pTask)
{
  W_LOCK(m_CuratorMutex);

  RunNextUpdateTask();
}

void WAssetCurator::RunNextUpdateTask()
{
  W_LOCK(m_CuratorMutex);

  if (WQtEditorApp::GetSingleton()->IsInHeadlessMode())
    return;

  if (!m_bRunUpdateTask || (m_TransformStateStale.IsEmpty() && m_TransformState[WAssetInfo::TransformState::Unknown].IsEmpty()))
    return;

  if (m_pUpdateTask == nullptr)
  {
    m_pUpdateTask = W_DEFAULT_NEW(WUpdateTask, WMakeDelegate(&WAssetCurator::OnUpdateTaskFinished, this));
  }

  if (m_pUpdateTask->IsTaskFinished())
  {
    m_UpdateTaskGroup = WTaskSystem::StartSingleTask(m_pUpdateTask, WTaskPriority::FileAccess);
  }
}

////////////////////////////////////////////////////////////////////////
// WAssetCurator Check File System Helper
////////////////////////////////////////////////////////////////////////

void WAssetCurator::SetAllAssetStatusUnknown()
{
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    UpdateAssetTransformState(it.Key(), WAssetInfo::TransformState::Unknown);
  }
}

void WAssetCurator::LoadCaches(WFileSystemModel::FilesMap& out_referencedFiles, WFileSystemModel::FoldersMap& out_referencedFolders)
{
  W_PROFILE_SCOPE("LoadCaches");
  W_LOCK(m_CuratorMutex);

  WStopwatch sw;
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    WStringBuilder sDataDir;
    WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    WStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.WCache");

    WFileReader reader;
    if (reader.Open(sCacheFile).Succeeded())
    {
      WUInt32 uiCuratorCacheVersion = 0;
      WUInt32 uiFileVersion = 0;
      reader >> uiCuratorCacheVersion;
      reader >> uiFileVersion;

      if (uiCuratorCacheVersion != W_CURATOR_CACHE_VERSION)
      {
        // Do not purge cache on processors.
        if (!WQtUiServices::IsHeadless())
        {
          WStringBuilder sCacheDir = sDataDir;
          sCacheDir.AppendPath("AssetCache");

          QDir dir(sCacheDir.GetData());
          if (dir.exists())
          {
            dir.removeRecursively();
          }
        }
        continue;
      }

      if (uiFileVersion != W_CURATOR_CACHE_FILE_VERSION)
        continue;

      {
        W_PROFILE_SCOPE("Assets");
        WUInt32 uiAssetCount = 0;
        reader >> uiAssetCount;
        for (WUInt32 i = 0; i < uiAssetCount; i++)
        {
          WString sPath;
          reader >> sPath;

          const WRTTI* pType = nullptr;
          WAssetDocumentInfo* pEntry = static_cast<WAssetDocumentInfo*>(WReflectionSerializer::ReadObjectFromBinary(reader, pType));
          W_ASSERT_DEBUG(pEntry != nullptr && pType == WGetStaticRTTI<WAssetDocumentInfo>(), "Failed to deserialize WAssetDocumentInfo!");
          m_CachedAssets.Insert(sPath, WUniquePtr<WAssetDocumentInfo>(pEntry, WFoundation::GetDefaultAllocator()));

          WFileStatus stat;
          reader >> stat;
          m_CachedFiles.Insert(std::move(sPath), stat);
        }

        m_KnownAssets.Reserve(m_CachedAssets.GetCount());
        m_KnownSubAssets.Reserve(m_CachedAssets.GetCount());

        m_TransformState[WAssetInfo::Unknown].Reserve(m_CachedAssets.GetCount());
        m_TransformState[WAssetInfo::UpToDate].Reserve(m_CachedAssets.GetCount());
        m_SubAssetChanged.Reserve(m_CachedAssets.GetCount());
        m_TransformStateStale.Reserve(m_CachedAssets.GetCount());
        m_Updating.Reserve(m_CachedAssets.GetCount());
      }
      {
        W_PROFILE_SCOPE("Files");
        WUInt32 uiFileCount = 0;
        reader >> uiFileCount;
        for (WUInt32 i = 0; i < uiFileCount; i++)
        {
          WDataDirPath path;
          reader >> path;
          WFileStatus stat;
          reader >> stat;
          // We invalidate all asset guids as the current cache as stored on disk is missing various bits in the curator that requires the code to go through the found new asset init code on load again.
          stat.m_DocumentID = WUuid::MakeInvalid();
          out_referencedFiles.Insert(std::move(path), stat);
        }
      }

      {
        W_PROFILE_SCOPE("Folders");
        WUInt32 uiFolderCount = 0;
        reader >> uiFolderCount;
        for (WUInt32 i = 0; i < uiFolderCount; i++)
        {
          WDataDirPath path;
          reader >> path;
          WFileStatus::Status stat;
          reader >> (WUInt8&)stat;
          out_referencedFolders.Insert(std::move(path), stat);
        }
      }
    }
  }

  WLog::Debug("Asset Curator LoadCaches: {0} ms", WArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void WAssetCurator::SaveCaches(const WFileSystemModel::FilesMap& referencedFiles, const WFileSystemModel::FoldersMap& referencedFolders)
{
  W_PROFILE_SCOPE("SaveCaches");
  m_CachedAssets.Clear();
  m_CachedFiles.Clear();

  // Do not save cache on processors.
  if (WQtUiServices::IsHeadless())
    return;

  W_LOCK(m_CuratorMutex);
  const WUInt32 uiCuratorCacheVersion = W_CURATOR_CACHE_VERSION;

  WStopwatch sw;
  for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); i++)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i];

    WStringBuilder sDataDir;
    WFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    WStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.WCache");

    const WUInt32 uiFileVersion = W_CURATOR_CACHE_FILE_VERSION;
    WUInt32 uiAssetCount = 0;
    WUInt32 uiFileCount = 0;
    WUInt32 uiFolderCount = 0;

    {
      W_PROFILE_SCOPE("Count");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == WAssetExistanceState::FileUnchanged && it.Value()->m_Path.GetDataDirIndex() == i)
        {
          ++uiAssetCount;
        }
      }
      for (auto it = referencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status == WFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          ++uiFileCount;
        }
      }
      for (auto it = referencedFolders.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value() == WFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          ++uiFolderCount;
        }
      }
    }
    WDeferredFileWriter writer;
    writer.SetOutput(sCacheFile);

    writer << uiCuratorCacheVersion;
    writer << uiFileVersion;

    {
      W_PROFILE_SCOPE("Assets");
      writer << uiAssetCount;
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        const WAssetInfo* pAsset = it.Value();
        if (pAsset->m_ExistanceState == WAssetExistanceState::FileUnchanged && pAsset->m_Path.GetDataDirIndex() == i)
        {
          writer << pAsset->m_Path.GetAbsolutePath();
          WReflectionSerializer::WriteObjectToBinary(writer, WGetStaticRTTI<WAssetDocumentInfo>(), pAsset->m_Info.Borrow());
          const WFileStatus* pStat = referencedFiles.GetValue(it.Value()->m_Path);
          W_ASSERT_DEBUG(pStat != nullptr, "");
          writer << *pStat;
        }
      }
    }
    {
      W_PROFILE_SCOPE("Files");
      writer << uiFileCount;
      for (auto it = referencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        const WFileStatus& stat = it.Value();
        if (stat.m_Status == WFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          writer << it.Key();
          writer << stat;
        }
      }
    }
    {
      W_PROFILE_SCOPE("Folders");
      writer << uiFolderCount;
      for (auto it = referencedFolders.GetIterator(); it.IsValid(); ++it)
      {
        const WFileStatus::Status stat = it.Value();
        if (stat == WFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          writer << it.Key();
          writer << (WUInt8)stat;
        }
      }
    }

    writer.Close().IgnoreResult();
  }

  WLog::Debug("Asset Curator SaveCaches: {0} ms", WArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void WAssetCurator::ClearAssetCaches(WAssetDocumentManager::OutputReliability threshold)
{
  const bool bWasRunning = WAssetProcessor::GetSingleton()->GetProcessorState() == WAssetProcessor::ProcessorState::Running;

  if (bWasRunning)
  {
    // pause background asset processing while we delete files
    WAssetProcessor::GetSingleton()->StopProcessor(true);
  }

  {
    W_LOCK(m_CuratorMutex);

    WStringBuilder filePath;

    WSet<WString> keepAssets;
    WSet<WString> filesToDelete;

    // for all assets, gather their outputs and check which ones we want to keep
    // e.g. textures are perfectly reliable, and even when clearing the cache we can keep them, also because they cost a lot of time to regenerate
    for (auto it : m_KnownSubAssets)
    {
      const auto& subAsset = it.Value();
      auto pManager = subAsset.m_pAssetInfo->GetManager();
      if (pManager->GetAssetTypeOutputReliability() > threshold)
      {
        auto pDocumentTypeDescriptor = subAsset.m_pAssetInfo->m_pDocumentTypeDescriptor;
        const auto& path = subAsset.m_pAssetInfo->m_Path;

        // check additional outputs
        for (const auto& output : subAsset.m_pAssetInfo->m_Info->m_Outputs)
        {
          filePath = pManager->GetAbsoluteOutputFileName(pDocumentTypeDescriptor, path, output);
          filePath.MakeCleanPath();
          keepAssets.Insert(filePath);
        }

        filePath = pManager->GetAbsoluteOutputFileName(pDocumentTypeDescriptor, path, nullptr);
        filePath.MakeCleanPath();
        keepAssets.Insert(filePath);

        // and also keep the thumbnail
        filePath = pManager->GenerateResourceThumbnailPath(path, subAsset.m_Data.m_sName);
        filePath.MakeCleanPath();
        keepAssets.Insert(filePath);
      }
    }

    // iterate over all AssetCache folders in all data directories and gather the list of files for deletion
    WFileSystemIterator iter;
    for (WFileSystem::StartSearch(iter, "AssetCache/", WFileSystemIteratorFlags::ReportFilesRecursive); iter.IsValid(); iter.Next())
    {
      iter.GetStats().GetFullPath(filePath);
      filePath.MakeCleanPath();

      if (keepAssets.Contains(filePath))
        continue;

      filesToDelete.Insert(filePath);
    }

    for (const WString& file : filesToDelete)
    {
      WOSFile::DeleteFile(file).IgnoreResult();
    }
  }

  WAssetCurator::CheckFileSystem();

  WAssetCurator::ProcessAllCoreAssets();

  if (bWasRunning)
  {
    // restart background asset processing
    WAssetProcessor::GetSingleton()->StartProcessor();
  }
}

WUInt32 WAssetCurator::ReplaceAssetReferenceInObject(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, WStringView sOldReference, WStringView sNewReference, WDynamicArray<WString>& out_errors)
{
  WUInt32 uiReplacementCount = 0;

  const WRTTI* pType = pObject->GetTypeAccessor().GetType();
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (const WAbstractProperty* pProp : properties)
  {
    // Skip temporary properties
    if (pProp->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    // Check if this is an asset reference property
    const WAssetBrowserAttribute* pAssetAttr = pProp->GetAttributeByType<WAssetBrowserAttribute>();
    if (pAssetAttr == nullptr)
      continue;

    // Must be string type
    const auto propVarType = pProp->GetSpecificType()->GetVariantType();
    if (propVarType != WVariantType::String && propVarType != WVariantType::StringView)
      continue;

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
        {
          WVariant value;
          if (pAccessor->GetValue(pObject, pProp, value).Succeeded())
          {
            WString sValue = value.Get<WString>();
            if (sValue == sOldReference)
            {
              if (pAccessor->SetValue(pObject, pProp, WVariant(WString(sNewReference))).Succeeded())
              {
                uiReplacementCount++;
              }
              else
              {
                WStringBuilder sError;
                sError.SetFormat("Failed to replace property '{}'", pProp->GetPropertyName());
                out_errors.PushBack(sError);
              }
            }
          }
        }
      }
      break;

      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
        {
          WInt32 iCount = pAccessor->GetCount(pObject, pProp);

          for (WInt32 i = 0; i < iCount; ++i)
          {
            WVariant value;
            if (pAccessor->GetValue(pObject, pProp, value, i).Succeeded())
            {
              WString sValue = value.Get<WString>();
              if (sValue == sOldReference)
              {
                if (pAccessor->SetValue(pObject, pProp, WVariant(WString(sNewReference)), i).Succeeded())
                {
                  uiReplacementCount++;
                }
                else
                {
                  WStringBuilder sError;
                  sError.SetFormat("Failed to replace property '{}[{}]'", pProp->GetPropertyName(), i);
                  out_errors.PushBack(sError);
                }
              }
            }
          }
        }
      }
      break;

      case WPropertyCategory::Map:
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
        {
          WDynamicArray<WVariant> keys;
          if (pAccessor->GetKeys(pObject, pProp, keys).Succeeded())
          {
            for (const WVariant& key : keys)
            {
              WVariant value;
              if (pAccessor->GetValue(pObject, pProp, value, key).Succeeded())
              {
                WString sValue = value.Get<WString>();
                if (sValue == sOldReference)
                {
                  if (pAccessor->SetValue(pObject, pProp, WVariant(WString(sNewReference)), key).Succeeded())
                  {
                    uiReplacementCount++;
                  }
                  else
                  {
                    WStringBuilder sError;
                    sError.SetFormat("Failed to replace map property '{}[{}]'", pProp->GetPropertyName(), key.ConvertTo<WString>());
                    out_errors.PushBack(sError);
                  }
                }
              }
            }
          }
        }
      }
      break;

      default:
        break;
    }
  }

  // Process children recursively
  for (const WDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType() != nullptr &&
        pChild->GetParentPropertyType()->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;
    uiReplacementCount += ReplaceAssetReferenceInObject(pAccessor, pChild, sOldReference, sNewReference, out_errors);
  }

  return uiReplacementCount;
}

WUInt32 WAssetCurator::ReplaceAssetReferenceInDocument(WDocument* pDocument, WStringView sOldReference, WStringView sNewReference, WDynamicArray<WString>& out_errors)
{
  WObjectAccessorBase* pAccessor = pDocument->GetObjectAccessor();

  pAccessor->StartTransaction("Replace Asset Reference");

  WUInt32 uiReplacementCount = ReplaceAssetReferenceInObject(
    pAccessor,
    pDocument->GetObjectManager()->GetRootObject(),
    sOldReference,
    sNewReference,
    out_errors);

  if (uiReplacementCount > 0)
    pAccessor->FinishTransaction();
  else
    pAccessor->CancelTransaction();

  return uiReplacementCount;
}

WAssetCurator::ReplaceAssetResult WAssetCurator::ReplaceAssetReferenceInUses(WUuid assetToReplace, WStringView sOldReference, WStringView sNewReference)
{
  ReplaceAssetResult result;

  // Find all direct uses of this asset
  WSet<WUuid> uses;
  WAssetCurator::GetSingleton()->FindAllUses(assetToReplace, uses, false /* bTransitive */);

  for (const WUuid& useGuid : uses)
  {
    // Get the asset info to find the document path
    const WAssetCurator::WLockedSubAsset pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(useGuid);
    if (!pSubAsset.isValid())
    {
      result.m_Errors.PushBack("Could not find asset info for a referencing asset");
      result.m_uiDocumentsFailed++;
      continue;
    }

    WString sDocumentPath = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath();

    // Open the document (without requesting a window)
    WDocument* pDocument = WQtEditorApp::GetSingleton()->OpenDocument(sDocumentPath, WDocumentFlags::None);

    if (pDocument == nullptr)
    {
      WStringBuilder sError;
      sError.SetFormat("Failed to open document: {}", sDocumentPath);
      result.m_Errors.PushBack(sError);
      result.m_uiDocumentsFailed++;
      continue;
    }

    WDynamicArray<WString> docErrors;
    WUInt32 uiReplaced = ReplaceAssetReferenceInDocument(pDocument, sOldReference, sNewReference, docErrors);

    result.m_Errors.PushBackRange(docErrors);

    if (uiReplaced > 0)
    {
      // Save the document
      WStatus saveStatus = pDocument->SaveDocument(false);
      if (saveStatus.Failed())
      {
        WStringBuilder sError;
        sError.SetFormat("Failed to save document: {} - {}", sDocumentPath, saveStatus.GetMessageString());
        result.m_Errors.PushBack(sError);
        result.m_uiDocumentsFailed++;
      }
      else
      {
        result.m_uiDocumentsModified++;
        result.m_uiPropertiesReplaced += uiReplaced;
      }
    }
  }

  return result;
}
