#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Uuid.h>
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

struct WAssetInfo;
struct WAssetCuratorEvent;
struct WSubAsset;
class WQtAssetFilter;

/// Verdict of WQtAssetFilter::IsAssetFiltered() for a single item.
///
/// Everything but Visible means the item is not shown. The values other than Filtered name the single reason an item
/// was excluded, which allows counting how many items a specific switch is hiding. They are only reported for items
/// that pass every other filter.
enum class WAssetFilterResult : WUInt8
{
  Visible,           ///< The item passes all filters and is shown.
  Filtered,          ///< The item is excluded, either for several reasons or for one that has no dedicated switch.
  HiddenFolder,      ///< Excluded only because it resides in a hidden folder. \see WQtAssetBrowserFilter::SetShowItemsInHiddenFolders()
  NonAssetFile,      ///< Excluded only because it is a plain file and files are not shown. \see WQtAssetBrowserFilter::SetShowFiles()
  NonImportableFile, ///< Excluded only because it is a file that can't be imported and those are not shown. \see WQtAssetBrowserFilter::SetShowNonImportableFiles()
};

/// Interface class of the asset filter used to decide which items are shown in the asset browser.
class W_EDITORFRAMEWORK_DLL WQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit WQtAssetFilter(QObject* pParent);

  /// Decides whether the given item is shown.
  ///
  /// Returning WAssetFilterResult::HiddenFolder instead of WAssetFilterResult::Filtered is optional. It allows
  /// the model to count items that are only excluded because of the hidden-folder rule, so that their number can
  /// be reported to the user. Such items are not shown either way.
  virtual WAssetFilterResult IsAssetFiltered(WStringView sDataDirParentRelativePath, bool bIsFolder, const WSubAsset* pInfo) const = 0;
  virtual bool GetSortByRecentUse() const { return false; }

Q_SIGNALS:
  void FilterChanged();
};

/// Each item in the asset browser can be multiple things at the same time as described by these flags.
/// Retrieved via user role WQtAssetBrowserModel::UserRoles::ItemFlags.
struct W_EDITORFRAMEWORK_DLL WAssetBrowserItemFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    Folder = W_BIT(0),        // Any folder inside a data directory
    DataDirectory = W_BIT(1), // mutually exclusive with Folder
    File = W_BIT(2),          // any file, could also be an Asset
    Asset = W_BIT(3),         // main asset: mutually exclusive with SubAsset
    SubAsset = W_BIT(4),      // sub-asset (imaginary, not a File or Asset)
    Default = 0
  };

  struct Bits
  {
    StorageType Folder : 1;
    StorageType DataDirectory : 1;
    StorageType File : 1;
    StorageType Asset : 1;
    StorageType SubAsset : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WAssetBrowserItemFlags);

/// Model of the item view in the asset browser.
class W_EDITORFRAMEWORK_DLL WQtAssetBrowserModel : public QAbstractItemModel, public QEnableSharedFromThis<WQtAssetBrowserModel>
{
  Q_OBJECT
public:
  enum UserRoles
  {
    SubAssetGuid = Qt::UserRole + 0, // WUuid
    AssetGuid,                       // WUuid
    AbsolutePath,                    // QString
    RelativePath,                    // QString
    AssetIcon,                       // QIcon
    TransformState,                  // QString
    Importable,                      // bool
    ItemFlags,                       // WAssetBrowserItemFlags as int
  };

  WQtAssetBrowserModel(QObject* pParent, WQtAssetFilter* pFilter);
  ~WQtAssetBrowserModel();
  void Initialize();

  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

  WInt32 FindAssetIndex(const WUuid& assetGuid) const;
  WInt32 FindIndex(WStringView sAbsPath) const;

  /// Number of items that pass all filters, but are not displayed because of a single switch.
  WUInt32 GetNumExcludedItems(WAssetFilterResult reason) const;

public Q_SLOTS:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2);
  void ThumbnailInvalidated(QString sPath, WUInt32 uiImageID);
  void OnFileSystemUpdate();

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset) const;

  /// Emitted when the result of GetNumExcludedItems() changed for any reason.
  void ExcludedItemCountsChanged();

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
  virtual Qt::DropActions supportedDropActions() const override;

private:
  friend struct FileComparer;

  enum class AssetOp
  {
    Add,
    Remove,
    Updated,
  };

  struct VisibleEntry
  {
    WDataDirPath m_sAbsFilePath;
    WUuid m_Guid;
    WBitflags<WAssetBrowserItemFlags> m_Flags;
    mutable WUInt32 m_uiThumbnailID;
  };

  struct FsEvent
  {
    WFileChangedEvent m_FileEvent;
    WFolderChangedEvent m_FolderEvent;
  };

private:
  void AssetCuratorEventHandler(const WAssetCuratorEvent& e);

  /// Re-runs the filter for every asset that lists the given asset among its missing dependencies.
  /// Needed because a filter's verdict can depend on whether those dependencies resolve, which
  /// changes when an asset is removed without any event being sent for the dependents themselves.
  void ReEvaluateDependents(const WUuid& removedAssetGuid);

  void HandleEntry(const VisibleEntry& entry, AssetOp op);

  /// Records under which single-reason exclusion the given item currently falls, if any.
  ///
  /// Emits ExcludedItemCountsChanged() if this changed any of the counts, unless m_bSuppressExcludedItemSignal is set,
  /// in which case the emit is deferred to whoever set that flag.
  ///
  /// Set bKnownUntracked only when the caller can guarantee that the path is not currently recorded under any reason,
  /// which is the case while resetModel() refills the empty sets. It skips the scan that would drop the path from the
  /// other reasons, so passing it wrongly leaves the item counted twice.
  void TrackExcludedItem(const WDataDirPath& path, WAssetFilterResult reason, bool bKnownUntracked = false);

  void FileSystemFileEventHandler(const WFileChangedEvent& e);
  void FileSystemFolderEventHandler(const WFolderChangedEvent& e);
  void HandleFile(const WFileChangedEvent& e);
  void HandleFolder(const WFolderChangedEvent& e);

private:
  WQtAssetFilter* m_pFilter = nullptr;
  bool m_bIconMode = true;
  WSet<WString> m_ImportExtensions;
  WEventSubscriptionID m_FileChangedSubscription = 0;
  WEventSubscriptionID m_FolderChangedSubscription = 0;

  WMutex m_Mutex;
  WDynamicArray<FsEvent> m_QueuedFileSystemEvents;

  WDynamicArray<VisibleEntry> m_EntriesToDisplay;
  WSet<WUuid> m_DisplayedEntries;

  // One set of paths per single-reason exclusion, indexed by WAssetFilterResult.
  // Keyed by path rather than by GUID, because plain files that are no assets have no GUID.
  WMap<WAssetFilterResult, WSet<WString>> m_ExcludedItems;

  // Set while resetModel() is between beginResetModel() and endResetModel(). It stops TrackExcludedItem() from
  // emitting once per item, which would not only be wasteful but would also make listeners query rowCount() while
  // the model is in reset state.
  bool m_bSuppressExcludedItemSignal = false;
  bool m_bExcludedItemCountsChanged = false;

  QFileIconProvider m_IconProvider;
};
