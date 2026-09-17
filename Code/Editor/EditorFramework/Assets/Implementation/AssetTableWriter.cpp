#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

WResult WAssetTable::WriteAssetTable()
{
  W_PROFILE_SCOPE("WriteAssetTable");

  WStringBuilder sTemp;
  WString sResourcePath;

  {
    for (auto& man : WAssetDocumentManager::GetAllDocumentManagers())
    {
      if (!man->GetDynamicRTTI()->IsDerivedFrom<WAssetDocumentManager>())
        continue;

      WAssetDocumentManager* pManager = static_cast<WAssetDocumentManager*>(man);

      // allow to add fully custom entries
      pManager->AddEntriesToAssetTable(m_sDataDir, m_pProfile, WMakeDelegate(&WAssetTable::AddManagerResource, this));
    }
  }

  if (m_bReset)
  {
    m_GuidToPath.Clear();
    WAssetCurator::WLockedSubAssetTable allSubAssetsLocked = WAssetCurator::GetSingleton()->GetKnownSubAssets();

    for (auto it = allSubAssetsLocked->GetIterator(); it.IsValid(); ++it)
    {
      sTemp = it.Value().m_pAssetInfo->m_Path.GetAbsolutePath();

      // ignore all assets that are not located in this data directory
      if (!sTemp.IsPathBelowFolder(m_sDataDir))
        continue;

      Update(it.Value());
    }
    m_bReset = false;
  }

  // We don't write anything on a background process as the main editor process will have already written any dirty tables before sending an RPC request. We still want to engine process to reload any potential changes though and be able to check which resources to reload so the tables are kept up to date in memory.
  if (WQtEditorApp::GetSingleton()->IsBackgroundMode())
    return W_SUCCESS;

  WDeferredFileWriter file;
  file.SetOutput(m_sTargetFile);

  auto Write = [](const WString& sGuid, const WString& sPath, WDeferredFileWriter& ref_file)
  {
    ref_file.WriteBytes(sGuid.GetData(), sGuid.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes(";", 1).IgnoreResult();
    ref_file.WriteBytes(sPath.GetData(), sPath.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes("\n", 1).IgnoreResult();
  };

  for (auto it = m_GuidToManagerResource.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value().m_sPath, file);
  }

  for (auto it = m_GuidToPath.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value(), file);
  }

  if (file.Close().Failed())
  {
    WLog::Error("Failed to open asset lookup table file '{0}'", m_sTargetFile);
    return W_FAILURE;
  }

  m_bDirty = false;
  return W_SUCCESS;
}

void WAssetTable::Remove(const WSubAsset& subAsset)
{
  WStringBuilder sTemp;
  WConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);
  m_GuidToPath.Remove(sTemp);
  m_bDirty = true;
}

void WAssetTable::Update(const WSubAsset& subAsset)
{
  WStringBuilder sTemp;
  WAssetDocumentManager* pManager = subAsset.m_pAssetInfo->GetManager();
  WString sEntry = pManager->GetAssetTableEntry(&subAsset, m_sDataDir, m_pProfile);

  // It is valid to write no asset table entry, if no redirection is required. This is used by decal assets for instance.
  if (!sEntry.IsEmpty())
  {
    WConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);

    m_GuidToPath[sTemp] = sEntry;
  }
  m_bDirty = true;
}

void WAssetTable::AddManagerResource(WStringView sGuid, WStringView sPath, WStringView sType)
{
  m_GuidToManagerResource[sGuid] = ManagerResource{sPath, sType};
}

WAssetTableWriter::WAssetTableWriter(const WApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
  m_DataDirToAssetTables.SetCount(m_FileSystemConfig.m_DataDirs.GetCount());

  WStringBuilder sDataDirPath;

  m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
  for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
  {
    if (WFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      WLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
      m_DataDirRoots.PushBack({});
    }
    else
    {
      m_DataDirRoots.PushBack(sDataDirPath);
    }
  }

  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WAssetTableWriter::AssetCuratorEvents, this));
}

WAssetTableWriter::~WAssetTableWriter()
{
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WAssetTableWriter::AssetCuratorEvents, this));
}

void WAssetTableWriter::MainThreadTick()
{
  // We must flush any pending table changes before triggering resource reloads.
  // If no resource reload is scheduled, we can just wait for the timer to run out to flush the changes.
  //
  if (m_bTablesDirty && (WTime::Now() > m_NextTableFlush || m_bNeedToReloadResources))
  {
    m_bTablesDirty = false;
    if (WriteAssetTables(WAssetCurator::GetSingleton()->GetActiveAssetProfile(), false).Failed())
    {
      WLog::Error("Failed to write asset tables");
    }
  }

  if (m_bNeedToReloadResources)
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = WAssetCurator::GetSingleton()->GetKnownSubAssets();
    W_LOCK(m_AssetTableMutex);

    bool bReloadManagerResources = false;
    const WPlatformProfile* pCurrentProfile = WAssetCurator::GetSingleton()->GetActiveAssetProfile();
    for (const ReloadResource& reload : m_ReloadResources)
    {
      if (WAssetTable* pTable = GetAssetTable(reload.m_uiDataDirIndex, pCurrentProfile))
      {
        if (pTable->m_GuidToPath.Contains(reload.m_sResource))
        {
          WReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = reload.m_sResource;
          msg2.m_sResourceType = reload.m_sType;
          WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
        else if (WPathUtils::IsAbsolutePath(reload.m_sResource))
        {
          if (reload.m_uiDataDirIndex >= m_DataDirRoots.GetCount())
            continue;

          WStringBuilder sTempPath = reload.m_sResource;
          if (sTempPath.MakeRelativeTo(m_DataDirRoots[reload.m_uiDataDirIndex]).Failed())
            continue;

          WReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = sTempPath;
          msg2.m_sResourceType = reload.m_sType;
          WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
        else
        {
          // If an asset is not represented by a resource in the table we assume it is represented by a manager resource.
          // Currently we don't know how these relate, e.g. we don't know all "Decal" assets are represented by the "{ ProjectDecalAtlas }" resource. Therefore, we just reload all manager resources.
          bReloadManagerResources = true;
        }
      }
    }
    m_ReloadResources.Clear();

    if (bReloadManagerResources)
    {
      for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        WAssetTable* pTable = GetAssetTable(i, pCurrentProfile);
        for (auto it : pTable->m_GuidToManagerResource)
        {
          WReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = it.Key();
          msg2.m_sResourceType = it.Value().m_sType;
          WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
      }
    }

    // This forces the deletion of cached render data.
    WSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadResources";
    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
    m_bNeedToReloadResources = false;
  }
}

void WAssetTableWriter::NeedsReloadResource(const WUuid& assetGuid)
{
  WAssetCurator::WLockedSubAsset asset = WAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (asset.isValid())
  {
    W_LOCK(m_AssetTableMutex);
    m_bNeedToReloadResources = true;
    WString sDocType = asset->m_Data.m_sSubAssetsDocumentTypeName.GetString();
    WStringBuilder sGuid;
    WConversionUtils::ToString(assetGuid, sGuid);
    const WUInt32 uiDataDirIndex = FindDataDir(*asset);

    if (asset->m_bMainAsset)
    {
      const WPlatformProfile* pProfile = WAssetCurator::GetSingleton()->GetActiveAssetProfile();
      const WAssetDocumentManager* pManager = asset->m_pAssetInfo->GetManager();
      const WAssetDocumentTypeDescriptor* pDocTypeDesc = asset->m_pAssetInfo->m_pDocumentTypeDescriptor;
      const WSet<WString>& outputs = asset->m_pAssetInfo->m_Info->m_Outputs;
      for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
      {
        // Additional outputs are not written to the asset table, so we assume they are referenced by a relative path in the runtime. We store an absolute path here though so that we can detect additional outputs inside the MainThreadTick function where we flush the resource reloads.
        const WString sTargetFile = pManager->GetAbsoluteOutputFileName(pDocTypeDesc, asset->m_pAssetInfo->m_Path.GetAbsolutePath(), it.Key(), pProfile);
        const WStringView sDocumentType = pManager->GetOutputDocumentType(pDocTypeDesc, it.Key(), pProfile);
        m_ReloadResources.PushBack({uiDataDirIndex, sTargetFile, sDocumentType});
      }
    }
    m_ReloadResources.PushBack({uiDataDirIndex, sGuid, sDocType});
  }
}

WResult WAssetTableWriter::WriteAssetTables(const WPlatformProfile* pAssetProfile, bool bForce)
{
  CURATOR_PROFILE("WriteAssetTables");
  W_LOG_BLOCK("WAssetCurator::WriteAssetTables");
  W_ASSERT_DEV(pAssetProfile != nullptr, "WriteAssetTables: pAssetProfile must be set.");

  WResult res = W_SUCCESS;
  bool bAnyChanged = false;
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = WAssetCurator::GetSingleton()->GetKnownSubAssets();
    W_LOCK(m_AssetTableMutex);

    WStringBuilder sd;

    for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      WAssetTable* table = GetAssetTable(i, pAssetProfile);
      if (!table)
      {
        WLog::Error("WriteAssetTables: The data dir '{}' with path '{}' could not be resolved", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        res = W_FAILURE;
        continue;
      }

      bAnyChanged |= (table->m_bReset || table->m_bDirty);
      if (!bForce && !table->m_bDirty && !table->m_bReset)
        continue;

      if (table->WriteAssetTable().Failed())
        res = W_FAILURE;
    }
  }

  if (bAnyChanged && pAssetProfile == WAssetCurator::GetSingleton()->GetActiveAssetProfile())
  {
    WSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadAssetLUT";
    msg.m_sPayload = pAssetProfile->GetConfigName();
    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  m_NextTableFlush = WTime::Now() + WTime::MakeFromSeconds(1.5);
  return res;
}

void WAssetTableWriter::AssetCuratorEvents(const WAssetCuratorEvent& e)
{
  W_LOCK(m_AssetTableMutex);

  const WPlatformProfile* pProfile = WAssetCurator::GetSingleton()->GetActiveAssetProfile();
  switch (e.m_Type)
  {
    // #TODO Are asset table entries static or do they change with the asset?
    /*case WAssetCuratorEvent::Type::AssetUpdated:
      if (e.m_pInfo->m_pAssetInfo->m_TransformState == WAssetInfo::TransformState::Unknown)
        return;
      [[fallthrough]];*/
    case WAssetCuratorEvent::Type::AssetAdded:
    case WAssetCuratorEvent::Type::AssetMoved:
    {
      WUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (WAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Update(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case WAssetCuratorEvent::Type::AssetRemoved:
    {
      WUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (WAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Remove(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case WAssetCuratorEvent::Type::AssetListReset:
      for (WUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        for (auto it : m_DataDirToAssetTables[i])
        {
          it.Value()->m_bReset = true;
        }
      }
      m_bTablesDirty = true;
      break;
    case WAssetCuratorEvent::Type::ActivePlatformChanged:
      if (WriteAssetTables(pProfile, false).Failed())
      {
        WLog::Error("Failed to write asset tables");
      }
      break;
    default:
      break;
  }
}

WAssetTable* WAssetTableWriter::GetAssetTable(WUInt32 uiDataDirIndex, const WPlatformProfile* pAssetProfile)
{
  auto it = m_DataDirToAssetTables[uiDataDirIndex].Find(pAssetProfile);
  if (!it.IsValid())
  {
    if (m_DataDirRoots[uiDataDirIndex].IsEmpty())
      return nullptr;

    WUniquePtr<WAssetTable> table = W_DEFAULT_NEW(WAssetTable);
    table->m_pProfile = pAssetProfile;
    table->m_sDataDir = m_DataDirRoots[uiDataDirIndex];

    WStringBuilder sFinalPath(m_DataDirRoots[uiDataDirIndex], "/AssetCache/", pAssetProfile->GetConfigName(), ".WAidlt");
    sFinalPath.MakeCleanPath();
    table->m_sTargetFile = sFinalPath;

    it = m_DataDirToAssetTables[uiDataDirIndex].Insert(pAssetProfile, std::move(table));
  }
  return it.Value().Borrow();
}

WUInt32 WAssetTableWriter::FindDataDir(const WSubAsset& asset)
{
  return asset.m_pAssetInfo->m_Path.GetDataDirIndex();
}
