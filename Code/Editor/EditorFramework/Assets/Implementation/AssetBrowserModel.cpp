#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

WQtAssetFilter::WQtAssetFilter(QObject* pParent)
  : QObject(pParent)
{
}

////////////////////////////////////////////////////////////////////////
// WQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct FileComparer
{
  FileComparer(WQtAssetBrowserModel* pModel, const WHashTable<WUuid, WSubAsset>& allAssets)
    : m_pModel(pModel)
    , m_AllAssets(allAssets)
  {
    m_bSortByRecentlyUsed = m_pModel->m_pFilter->GetSortByRecentUse();
  }

  bool Less(const WQtAssetBrowserModel::VisibleEntry& a, const WQtAssetBrowserModel::VisibleEntry& b) const
  {
    if (a.m_Flags.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory) != b.m_Flags.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory))
      return a.m_Flags.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory);

    if (a.m_Flags.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory))
    {
      return WCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
    }

    const WSubAsset* pInfoA = nullptr;
    m_AllAssets.TryGetValue(a.m_Guid, pInfoA);

    const WSubAsset* pInfoB = nullptr;
    m_AllAssets.TryGetValue(b.m_Guid, pInfoB);

    WStringView sSortA;
    WStringView sSortB;
    if (pInfoA && !pInfoA->m_bMainAsset)
    {
      sSortA = pInfoA->GetName();
    }
    else
    {
      sSortA = a.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }
    if (pInfoB && !pInfoB->m_bMainAsset)
    {
      sSortB = pInfoB->GetName();
    }
    else
    {
      sSortB = b.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }

    if (m_bSortByRecentlyUsed)
    {
      if (pInfoA && pInfoB)
      {
        if (pInfoA->m_LastAccess != pInfoB->m_LastAccess)
        {
          return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
        }
      }
      else if (pInfoA && pInfoA->m_LastAccess.IsPositive())
      {
        return true;
      }
      else if (pInfoB && pInfoB->m_LastAccess.IsPositive())
      {
        return false;
      }

      // in all other cases, fall through and do the file name comparison
    }

    WInt32 iValue = sSortA.Compare_NoCase(sSortB);
    if (iValue == 0)
    {
      if (!pInfoA && !pInfoB)
        return WCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
      else if (pInfoA && pInfoB)
        return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
      else
        return pInfoA == nullptr;
    }
    return iValue < 0;
  }

  W_ALWAYS_INLINE bool operator()(const WQtAssetBrowserModel::VisibleEntry& a, const WQtAssetBrowserModel::VisibleEntry& b) const
  {
    return Less(a, b);
  }

  WQtAssetBrowserModel* m_pModel = nullptr;
  const WHashTable<WUuid, WSubAsset>& m_AllAssets;
  bool m_bSortByRecentlyUsed = false;
};

WQtAssetBrowserModel::WQtAssetBrowserModel(QObject* pParent, WQtAssetFilter* pFilter)
  : QAbstractItemModel(pParent)
  , m_pFilter(pFilter)
{
}

WQtAssetBrowserModel::~WQtAssetBrowserModel()
{
  WFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(m_FileChangedSubscription);
  WFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(m_FolderChangedSubscription);
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void WQtAssetBrowserModel::Initialize()
{
  W_ASSERT_DEBUG(m_pFilter != nullptr, "WQtAssetBrowserModel requires a valid filter.");
  connect(m_pFilter, &WQtAssetFilter::FilterChanged, this, [this]()
    { resetModel(); });

  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);

  W_VERIFY(connect(WQtImageCache::GetSingleton(), &WQtImageCache::ImageLoaded, this, &WQtAssetBrowserModel::ThumbnailLoaded) != nullptr,
    "signal/slot connection failed");
  W_VERIFY(connect(WQtImageCache::GetSingleton(), &WQtImageCache::ImageInvalidated, this, &WQtAssetBrowserModel::ThumbnailInvalidated) != nullptr,
    "signal/slot connection failed");

  QWeakPointer<WQtAssetBrowserModel> pWeak = sharedFromThis();
  m_FileChangedSubscription = WFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler([pWeak](const WFileChangedEvent& e)
    {
      if (QSharedPointer<WQtAssetBrowserModel> strong = pWeak.toStrongRef())
      {
        strong->FileSystemFileEventHandler(e);
      } });
  m_FolderChangedSubscription = WFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler([pWeak](const WFolderChangedEvent& e)
    {
      if (QSharedPointer<WQtAssetBrowserModel> strong = pWeak.toStrongRef())
      {
        strong->FileSystemFolderEventHandler(e);
      } });
  WAssetDocumentGenerator::GetSupportsFileTypes(m_ImportExtensions);
}

void WQtAssetBrowserModel::AssetCuratorEventHandler(const WAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case WAssetCuratorEvent::Type::AssetAdded:
    case WAssetCuratorEvent::Type::AssetMoved:
    case WAssetCuratorEvent::Type::AssetUpdated:
    {
      VisibleEntry ve;
      ve.m_Guid = e.m_AssetGuid;
      ve.m_sAbsFilePath = e.m_pInfo->m_pAssetInfo->m_Path;
      ve.m_Flags = WAssetBrowserItemFlags::File;
      if (ve.m_Guid.IsValid())
      {
        ve.m_Flags |= WAssetBrowserItemFlags::Asset;
      }
      HandleEntry(ve, AssetOp::Updated);
      break;
    }
    case WAssetCuratorEvent::Type::AssetRemoved:
    {
      // A filter's verdict can depend on assets other than the one being filtered: the asset curator
      // panel hides an asset whose missing dependencies all resolve to known assets, on the grounds
      // that those are reported instead. Removing an asset can therefore change the verdict for
      // everything that depends on it, without their own transform state changing - which means no
      // event is sent for them. Re-evaluate those here, or they keep a stale verdict until the model
      // is rebuilt from scratch.
      ReEvaluateDependents(e.m_AssetGuid);
      break;
    }
    case WAssetCuratorEvent::Type::AssetListReset:
    {
      m_ImportExtensions.Clear();
      WAssetDocumentGenerator::GetSupportsFileTypes(m_ImportExtensions);
      break;
    }
    default:
      break;
  }
}

void WQtAssetBrowserModel::ReEvaluateDependents(const WUuid& removedAssetGuid)
{
  WStringBuilder sRemovedGuid;
  WConversionUtils::ToString(removedAssetGuid, sRemovedGuid);

  WTempHybridArray<VisibleEntry, 8> toReEvaluate;

  {
    WAssetCurator::WLockedAssetTable allAssetsLocked = WAssetCurator::GetSingleton()->GetKnownAssets();

    for (auto it : *allAssetsLocked)
    {
      const WAssetInfo* pAssetInfo = it.Value();

      auto references = [&](const WSet<WString>& deps) -> bool
      {
        return deps.Contains(sRemovedGuid);
      };

      if (!references(pAssetInfo->m_MissingTransformDeps) && !references(pAssetInfo->m_MissingThumbnailDeps) && !references(pAssetInfo->m_MissingPackageDeps))
        continue;

      auto& ve = toReEvaluate.ExpandAndGetRef();
      ve.m_Guid = it.Key();
      ve.m_sAbsFilePath = pAssetInfo->m_Path;
      ve.m_Flags = WAssetBrowserItemFlags::File | WAssetBrowserItemFlags::Asset;
    }
  }

  for (const VisibleEntry& ve : toReEvaluate)
  {
    HandleEntry(ve, AssetOp::Updated);
  }
}


WInt32 WQtAssetBrowserModel::FindAssetIndex(const WUuid& assetGuid) const
{
  if (!m_DisplayedEntries.Contains(assetGuid))
    return -1;

  for (WUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}


WInt32 WQtAssetBrowserModel::FindIndex(WStringView sAbsPath) const
{
  for (WUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_sAbsFilePath.GetAbsolutePath() == sAbsPath)
    {
      return i;
    }
  }
  return -1;
}

void WQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  m_EntriesToDisplay.Clear();
  m_DisplayedEntries.Clear();
  m_ExcludedItems.Clear();

  m_bSuppressExcludedItemSignal = true;
  m_bExcludedItemCountsChanged = false;

  // Get Curator Mutex first to prevent deadlocks
  WAssetCurator::WLockedSubAssetTable AllAssetsLocked = WAssetCurator::GetSingleton()->GetKnownSubAssets();
  const WHashTable<WUuid, WSubAsset>& AllAssets = *(AllAssetsLocked.operator->());

  auto allFiles = WFileSystemModel::GetSingleton()->GetFiles();
  auto allFolders = WFileSystemModel::GetSingleton()->GetFolders();

  for (const auto& folder : *allFolders)
  {
    if (m_pFilter->IsAssetFiltered(folder.Key().GetDataDirParentRelativePath(), true, nullptr) != WAssetFilterResult::Visible)
      continue;

    auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
    entry.m_Flags = folder.Key().GetDataDirRelativePath().IsEmpty() ? WAssetBrowserItemFlags::DataDirectory : WAssetBrowserItemFlags::Folder;
    entry.m_sAbsFilePath = folder.Key();
  }

  for (const auto& file : *allFiles)
  {
    if (file.Value().m_DocumentID.IsValid())
    {
      auto mainAsset = WAssetCurator::GetSingleton()->GetSubAsset(file.Value().m_DocumentID);

      if (!mainAsset)
        continue;

      const WAssetFilterResult mainResult = m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*mainAsset));
      if (mainResult == WAssetFilterResult::Visible)
      {
        auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid = file.Value().m_DocumentID;
        entry.m_Flags = WAssetBrowserItemFlags::File | WAssetBrowserItemFlags::Asset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }
      else
      {
        TrackExcludedItem(file.Key(), mainResult, true);
      }

      for (const auto& subAssetGuid : mainAsset->m_pAssetInfo->m_SubAssets)
      {
        auto subAsset = WAssetCurator::GetSingleton()->GetSubAsset(subAssetGuid);

        if (subAsset)
        {
          const WAssetFilterResult subResult = m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*subAsset));

          // Sub-assets are not tracked as excluded items: they share the path of their main asset, which is already
          // accounted for above, and being assets they can never fall under the file-related exclusions.
          if (subResult != WAssetFilterResult::Visible)
            continue;
        }

        auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid = subAssetGuid;
        entry.m_Flags |= WAssetBrowserItemFlags::SubAsset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }
    }
    else
    {
      const WAssetFilterResult result = m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, nullptr);

      if (result != WAssetFilterResult::Visible)
      {
        TrackExcludedItem(file.Key(), result, true);
        continue;
      }

      auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
      entry.m_sAbsFilePath = file.Key();
      entry.m_Flags = WAssetBrowserItemFlags::File;
    }
  }


  FileComparer cmp(this, AllAssets);
  m_EntriesToDisplay.Sort(cmp);

  endResetModel();

  m_bSuppressExcludedItemSignal = false;

  if (m_bExcludedItemCountsChanged)
  {
    m_bExcludedItemCountsChanged = false;
    Q_EMIT ExcludedItemCountsChanged();
  }
}

void WQtAssetBrowserModel::TrackExcludedItem(const WDataDirPath& path, WAssetFilterResult reason, bool bKnownUntracked /*= false*/)
{
  // The lookups below run directly off the view, so no string is allocated unless the path actually has to be inserted.
  const WStringView sPath = path.GetDataDirParentRelativePath();
  bool bChanged = false;

  if (!bKnownUntracked)
  {
    // an item can only ever fall under one reason at a time, so drop it from all the others
    for (auto it : m_ExcludedItems)
    {
      if (it.Key() != reason)
      {
        bChanged |= it.Value().Remove(sPath);
      }
    }
  }

  if (reason != WAssetFilterResult::Visible && reason != WAssetFilterResult::Filtered)
  {
    WSet<WString>& items = m_ExcludedItems[reason];

    if (!items.Contains(sPath))
    {
      items.Insert(sPath);
      bChanged = true;
    }
  }

  if (bChanged)
  {
    if (m_bSuppressExcludedItemSignal)
      m_bExcludedItemCountsChanged = true;
    else
      Q_EMIT ExcludedItemCountsChanged();
  }
}

WUInt32 WQtAssetBrowserModel::GetNumExcludedItems(WAssetFilterResult reason) const
{
  auto it = m_ExcludedItems.Find(reason);
  return it.IsValid() ? it.Value().GetCount() : 0;
}

void WQtAssetBrowserModel::HandleEntry(const VisibleEntry& entry, AssetOp op)
{
  auto subAsset = WAssetCurator::GetSingleton()->GetSubAsset(entry.m_Guid);

  const WAssetFilterResult filterResult = m_pFilter->IsAssetFiltered(entry.m_sAbsFilePath.GetDataDirParentRelativePath(), entry.m_Flags.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory), subAsset.Borrow());

  // Sub-assets share the path of their main asset, which is tracked in its own right, so tracking them too would
  // overwrite that entry. A removed item is gone no matter what the filter says about it, so it is dropped from the
  // bookkeeping rather than recorded under a verdict computed for an item that no longer exists.
  if (!entry.m_Flags.IsSet(WAssetBrowserItemFlags::SubAsset))
  {
    TrackExcludedItem(entry.m_sAbsFilePath, op == AssetOp::Remove ? WAssetFilterResult::Visible : filterResult);
  }

  if (filterResult != WAssetFilterResult::Visible)
  {
    if (!m_DisplayedEntries.Contains(entry.m_Guid))
    {
      return;
    }

    // Filtered but still exists, remove it.
    op = AssetOp::Remove;
  }

  WAssetCurator::WLockedSubAssetTable AllAssetsLocked = WAssetCurator::GetSingleton()->GetKnownSubAssets();
  const WHashTable<WUuid, WSubAsset>& AllAssets = *AllAssetsLocked.Borrow();

  FileComparer cmp(this, AllAssets);
  VisibleEntry* pLB = std::lower_bound(begin(m_EntriesToDisplay), end(m_EntriesToDisplay), entry, cmp);
  WUInt32 uiInsertIndex = pLB - m_EntriesToDisplay.GetData();
  // TODO: Due to sorting issues the above can fail (we need to add a sorting model on top of this as we use mutable data (name) for sorting.
  if (uiInsertIndex >= m_EntriesToDisplay.GetCount())
  {
    for (WUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); i++)
    {
      VisibleEntry& displayEntry = m_EntriesToDisplay[i];
      if (!cmp.Less(displayEntry, entry) && !cmp.Less(entry, displayEntry))
      {
        uiInsertIndex = i;
        pLB = &displayEntry;
        break;
      }
    }
  }

  if (op == AssetOp::Add)
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_EntriesToDisplay.InsertAt(uiInsertIndex, entry);
    if (entry.m_Guid.IsValid())
      m_DisplayedEntries.Insert(entry.m_Guid);
    endInsertRows();
  }
  else if (op == AssetOp::Remove)
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_EntriesToDisplay.RemoveAtAndCopy(uiInsertIndex);
      if (entry.m_Guid.IsValid())
        m_DisplayedEntries.Remove(entry.m_Guid);
      endRemoveRows();
    }
  }
  else // Updated
  {
    // Updated entries can cause the filter function `IsAssetFiltered` to change its result, e.g. the transform issues list in the curator panel shows assets that were updated from a healthy state to an error state. Thus, updated entries could be missing in the list at this point, so we need to first check if the item already exists:
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      // Already exists
      QModelIndex idx = index(uiInsertIndex, 0);
      Q_EMIT dataChanged(idx, idx);
    }
    else
    {
      // Item not found. Do an exhaustive search in case the name was changed in which the order is no longer the same.
      WInt32 oldIndex = FindAssetIndex(entry.m_Guid);
      if (oldIndex != -1)
      {
        // Order (most likely name) has changed, remove old entry and insert new one
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_EntriesToDisplay.RemoveAtAndCopy(oldIndex);
        m_DisplayedEntries.Remove(entry.m_Guid);
        endRemoveRows();
      }
      // Reinsert the updated entry.
      HandleEntry(entry, AssetOp::Add);
    }
  }
}


////////////////////////////////////////////////////////////////////////
// WQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void WQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2)
{
  const WUuid guid(userData1.toULongLong(), userData2.toULongLong());

  for (WUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void WQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, WUInt32 uiImageID)
{
  for (WUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void WQtAssetBrowserModel::OnFileSystemUpdate()
{
  WDynamicArray<FsEvent> events;

  {
    W_LOCK(m_Mutex);
    events.Swap(m_QueuedFileSystemEvents);
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type == WFileChangedEvent::Type::ModelReset)
    {
      resetModel();
      return;
    }
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type != WFileChangedEvent::Type::None)
      HandleFile(e.m_FileEvent);
    else
      HandleFolder(e.m_FolderEvent);
  }
}

QVariant WQtAssetBrowserModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const WInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (WInt32)m_EntriesToDisplay.GetCount())
    return QVariant();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  // Common properties shared among all item types.
  switch (iRole)
  {
    case WQtAssetBrowserModel::UserRoles::ItemFlags:
      return (int)entry.m_Flags.GetValue();
    case WQtAssetBrowserModel::UserRoles::Importable:
    {
      if (entry.m_Flags.IsSet(WAssetBrowserItemFlags::File) && !entry.m_Flags.IsSet(WAssetBrowserItemFlags::Asset))
      {
        WStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        return bImportable;
      }
      return false;
    }
    case WQtAssetBrowserModel::UserRoles::RelativePath:
      return WMakeQString(entry.m_sAbsFilePath.GetDataDirParentRelativePath());
    case WQtAssetBrowserModel::UserRoles::AbsolutePath:
      return WMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
  }

  if (entry.m_Flags.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory))
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        WStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();

        return WMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        WStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
        return WMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return WMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case WQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        return WQtUiServices::GetCachedIconResource(entry.m_Flags.IsSet(WAssetBrowserItemFlags::DataDirectory) ? ":/EditorFramework/Icons/DataDirectory.svg" : ":/EditorFramework/Icons/Folder.svg");
      }
    }
  }
  else if (!entry.m_Guid.IsValid()) // Normal file
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return WMakeQString(entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension());
      }
      break;

      case Qt::EditRole:
      {
        WStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileName(); // remove the file extension
        return WMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return WMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case WQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        WStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        const bool bIsReferenced = WAssetCurator::GetSingleton()->IsReferenced(entry.m_sAbsFilePath.GetAbsolutePath());
        if (bImportable)
        {
          return WQtUiServices::GetCachedIconResource(bIsReferenced ? ":/EditorFramework/Icons/ImportedFile.svg" : ":/EditorFramework/Icons/ImportableFile.svg");
        }
        return WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Document.svg");
      }

      case Qt::DecorationRole:
      {
        QFileInfo fi(WMakeQString(entry.m_sAbsFilePath));
        return m_IconProvider.icon(fi);
      }
    }
  }
  else if (entry.m_Guid.IsValid()) // Asset or sub-asset
  {
    const WUuid AssetGuid = entry.m_Guid;
    const WAssetCurator::WLockedSubAsset pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

    // this can happen when a file was just changed on disk, e.g. got deleted
    if (pSubAsset == nullptr)
      return QVariant();

    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        WStringBuilder sFilename = pSubAsset->GetName();
        return WMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        if (entry.m_Flags.IsSet(WAssetBrowserItemFlags::Asset))
        {
          // Don't allow changing extensions of assets
          WStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileName();
          return WMakeQString(sFilename);
        }
      }
      break;

      case Qt::ToolTipRole:
      {
        WStringBuilder sToolTip = pSubAsset->GetName();
        sToolTip.Append("\n", pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
        sToolTip.Append("\nTransform State: ");
        switch (pSubAsset->m_pAssetInfo->m_TransformState)
        {
          case WAssetInfo::Unknown:
            sToolTip.Append("Unknown");
            break;
          case WAssetInfo::UpToDate:
            sToolTip.Append("Up To Date");
            break;
          case WAssetInfo::NeedsTransform:
            sToolTip.Append("Needs Transform");
            break;
          case WAssetInfo::NeedsThumbnail:
            sToolTip.Append("Needs Thumbnail");
            break;
          case WAssetInfo::TransformError:
            sToolTip.Append("Transform Error");
            break;
          case WAssetInfo::MissingTransformDependency:
            sToolTip.Append("Missing Transform Dependency");
            break;
          case WAssetInfo::MissingPackageDependency:
            sToolTip.Append("Missing Package Dependency");
            break;
          case WAssetInfo::MissingThumbnailDependency:
            sToolTip.Append("Missing Thumbnail Dependency");
            break;
          case WAssetInfo::CircularDependency:
            sToolTip.Append("Circular Dependency");
            break;
          default:
            break;
        }

        // What the last transform measured, e.g. the triangle count and size of a mesh.
        if (pSubAsset->m_bMainAsset)
        {
          if (const WAssetInfoFile* pInfo = pSubAsset->m_pAssetInfo->GetTransformInfo())
          {
            pSubAsset->m_pAssetInfo->GetManager()->AppendAssetInfoSummary(sToolTip, *pInfo);
          }
        }

        return QString::fromUtf8(sToolTip, sToolTip.GetElementCount());
      }
      case Qt::DecorationRole:
      {
        if (m_bIconMode)
        {
          WString sThumbnailPath = pSubAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);

          WUInt64 uiUserData1, uiUserData2;
          AssetGuid.GetValues(uiUserData1, uiUserData2);

          const QPixmap* pThumbnailPixmap = WQtImageCache::GetSingleton()->QueryPixmapForType(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName,
            sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &entry.m_uiThumbnailID);

          return *pThumbnailPixmap;
        }
        else
        {
          return WQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, WColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, WColorScheme::CategoryColorUsage::OverlayIcon));
        }
      }
      break;

      case UserRoles::SubAssetGuid:
        return QVariant::fromValue(pSubAsset->m_Data.m_Guid);
      case UserRoles::AssetGuid:
        return QVariant::fromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
      case UserRoles::AssetIcon:
        return WQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, WColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, WColorScheme::CategoryColorUsage::OverlayIcon));
      case UserRoles::TransformState:
        return (int)pSubAsset->m_pAssetInfo->m_TransformState;
    }
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }
  return QVariant();
}

bool WQtAssetBrowserModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  if (!index.isValid())
    return false;

  if (iRole != Qt::EditRole)
    return false;

  const WInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (WInt32)m_EntriesToDisplay.GetCount())
    return false;

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];
  const bool bIsAsset = entry.m_Guid.IsValid();
  if (entry.m_Flags.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::File))
  {
    const WString& sAbsPath = entry.m_sAbsFilePath.GetAbsolutePath();
    emit editingFinished(WMakeQString(sAbsPath), value.toString(), bIsAsset);

    return true;
  }

  return false;
}

Qt::ItemFlags WQtAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  const WInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (WInt32)m_EntriesToDisplay.GetCount())
    return Qt::ItemFlags();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (entry.m_Flags.IsAnySet(WAssetBrowserItemFlags::File | WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::Asset))
  {
    flags |= Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
  }

  if (entry.m_Flags.IsAnySet(WAssetBrowserItemFlags::SubAsset))
  {
    flags |= Qt::ItemIsDragEnabled;
  }

  return flags;
}

QVariant WQtAssetBrowserModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Files");
    }
  }
  return QVariant();
}

QModelIndex WQtAssetBrowserModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn);
}

QModelIndex WQtAssetBrowserModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int WQtAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int)m_EntriesToDisplay.GetCount();
}

int WQtAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList WQtAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/WEditor.AssetGuid";
  types << "application/WEditor.files";
  return types;
}

QMimeData* WQtAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QString sGuids;
  QList<QUrl> urls;
  WTempHybridArray<QString, 1> guids;
  WTempHybridArray<QString, 1> files;

  WStringBuilder tmp;

  for (WUInt32 i = 0; i < (WUInt32)indexes.size(); ++i)
  {
    QString sGuid(WConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<WUuid>(), tmp).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();
    guids.PushBack(sGuid);
    if (i == 0)
      sGuids += sPath;
    else
      sGuids += "\n" + sPath;

    files.PushBack(sPath);
    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << guids;

  QByteArray encodedData2;
  QDataStream stream2(&encodedData2, QIODevice::WriteOnly);
  stream2 << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/WEditor.AssetGuid", encodedData);
  mimeData->setData("application/WEditor.files", encodedData2);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}

Qt::DropActions WQtAssetBrowserModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::LinkAction;
}

void WQtAssetBrowserModel::FileSystemFileEventHandler(const WFileChangedEvent& e)
{
  bool bFire = false;

  {
    W_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FileEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void WQtAssetBrowserModel::FileSystemFolderEventHandler(const WFolderChangedEvent& e)
{
  bool bFire = false;

  {
    W_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FolderEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void WQtAssetBrowserModel::HandleFile(const WFileChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Guid = e.m_Status.m_DocumentID;
  ve.m_sAbsFilePath = e.m_Path;
  ve.m_Flags = WAssetBrowserItemFlags::File;
  if (ve.m_Guid.IsValid())
  {
    ve.m_Flags |= WAssetBrowserItemFlags::Asset;
  }

  switch (e.m_Type)
  {
    case WFileChangedEvent::Type::ModelReset:
      resetModel();
      return;

    case WFileChangedEvent::Type::FileAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case WFileChangedEvent::Type::DocumentLinked:
    {
      ve.m_Guid = WUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case WFileChangedEvent::Type::DocumentUnlinked:
    {
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = WUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case WFileChangedEvent::Type::FileRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}

void WQtAssetBrowserModel::HandleFolder(const WFolderChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Flags = WAssetBrowserItemFlags::Folder;
  ve.m_sAbsFilePath = e.m_Path;

  switch (e.m_Type)
  {
    case WFolderChangedEvent::Type::FolderAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case WFolderChangedEvent::Type::FolderRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}
