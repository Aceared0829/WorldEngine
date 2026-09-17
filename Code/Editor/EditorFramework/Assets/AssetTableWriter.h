#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Threading/TaskSystem.h>


struct WAssetCuratorEvent;
class WTask;
struct WAssetInfo;

/// Asset table class. Persistent cache for an asset table.
///
/// The following assumptions need to be true for this cache to work:
/// 1. WAssetDocumentManager::AddEntriesToAssetTable does never change over time
/// 2. WAssetDocumentManager::GetAssetTableEntry never changes over the lifetime of an asset.
struct WAssetTable
{
  struct ManagerResource
  {
    WString m_sPath;
    WString m_sType;
  };

  WString m_sDataDir;
  WString m_sTargetFile;
  const WPlatformProfile* m_pProfile = nullptr;
  bool m_bDirty = true;
  bool m_bReset = true;
  WMap<WString, ManagerResource> m_GuidToManagerResource;
  WMap<WString, WString> m_GuidToPath;

  WResult WriteAssetTable();
  void Remove(const WSubAsset& subAsset);
  void Update(const WSubAsset& subAsset);
  void AddManagerResource(WStringView sGuid, WStringView sPath, WStringView sType);
};

/// Keeps track of all asset tables and their state as well as reloading modified resources.
class W_EDITORFRAMEWORK_DLL WAssetTableWriter
{
public:
  WAssetTableWriter(const WApplicationFileSystemConfig& fileSystemConfig);
  ~WAssetTableWriter();

  /// Needs to be called every frame. Handles update delays to allow compacting multiple changes.
  void MainThreadTick();

  /// Marks an asset that needs to be reloaded in the engine process.
  /// The requests are batched and sent out via MainThreadTick.
  void NeedsReloadResource(const WUuid& assetGuid);

  /// Writes the asset table for each data dir for the given asset profile.
  WResult WriteAssetTables(const WPlatformProfile* pAssetProfile, bool bForce);

private:
  void AssetCuratorEvents(const WAssetCuratorEvent& e);
  WAssetTable* GetAssetTable(WUInt32 uiDataDirIndex, const WPlatformProfile* pAssetProfile);
  WUInt32 FindDataDir(const WSubAsset& asset);

private:
  struct ReloadResource
  {
    WUInt32 m_uiDataDirIndex;
    WString m_sResource;
    WString m_sType;
  };

private:
  WApplicationFileSystemConfig m_FileSystemConfig;
  WDynamicArray<WString> m_DataDirRoots;

  mutable WCuratorMutex m_AssetTableMutex;
  bool m_bTablesDirty = true;
  bool m_bNeedToReloadResources = false;
  WTime m_NextTableFlush;
  WDynamicArray<ReloadResource> m_ReloadResources;
  WDeque<WMap<const WPlatformProfile*, WUniquePtr<WAssetTable>>> m_DataDirToAssetTables;
};
