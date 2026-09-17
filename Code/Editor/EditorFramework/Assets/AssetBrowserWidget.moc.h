#pragma once

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserWidget.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>



class WQtToolBarActionMapView;
class WQtAssetBrowserFilter;
class WQtAssetBrowserModel;
struct WAssetCuratorEvent;
class WQtAssetBrowserModel;

class W_EDITORFRAMEWORK_DLL WQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  WQtAssetBrowserWidget(QWidget* pParent);
  ~WQtAssetBrowserWidget();

  enum class Mode
  {
    Browser,
    AssetPicker,
    FilePicker,
  };

  void SetMode(Mode mode);
  void SetSelectedAsset(WUuid preselectedAsset);
  void SetSelectedFile(WStringView sAbsPath);
  void ShowOnlyTheseTypeFilters(WStringView sFilters);
  void UseFileExtensionFilters(WStringView sFileExtensions);
  void SetRequiredTag(WStringView sRequiredTag);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  void dragEnterEvent(QDragEnterEvent* pEvent) override;
  void dragMoveEvent(QDragMoveEvent* pEvent) override;
  void dragLeaveEvent(QDragLeaveEvent* pEvent) override;
  void dropEvent(QDropEvent* pEvent) override;

  WQtAssetBrowserModel* GetAssetBrowserModel() { return m_Model.data(); }
  const WQtAssetBrowserModel* GetAssetBrowserModel() const { return m_Model.data(); }
  WQtAssetBrowserFilter* GetAssetBrowserFilter() { return m_pFilter; }
  const WQtAssetBrowserFilter* GetAssetBrowserFilter() const { return m_pFilter; }

Q_SIGNALS:
  void ItemChosen(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags);
  void ItemSelected(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags);
  void ItemCleared();

private Q_SLOTS:
  void OnTextFilterChanged();
  void OnTypeFilterChanged();
  void OnPathFilterChanged();
  void OnFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ListAssets_activated(const QModelIndex& index);
  void on_ListAssets_clicked(const QModelIndex& index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(WInt32 iIconSizePercentage);
  void on_ResetTypeFilter_clicked();
  void OnSearchWidgetTextChanged(const QString& text);
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void on_TypeFilter_currentIndexChanged(int index);
  void OnScrollToItem(WUuid preselectedAsset);
  void OnScrollToFile(QString sPreselectedFile);
  void OnShowSubFolderItemsToggled();
  void OnShowHiddenFolderItemsToggled();
  void OnShowPluginDataDirsToggled();
  void OnResaveAssets();
  void on_ListAssets_customContextMenuRequested(const QPoint& pt);
  void OnListOpenExplorer();
  void OnListOpenAssetDocument();
  void OnListOpenFileWith();
  void OnTransform();
  void OnListToggleSortByRecentlyUsed();
  void OnListCopyAssetGuid();
  void OnFilterToThisPath();
  void OnListFindAllReferences(bool transitive);
  void OnSelectionTimer();
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void OnModelReset();
  void NewAsset();
  void OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset);
  void ImportSelection();
  void OnOpenImportReferenceAsset();
  void RenameCurrent();
  void DeleteSelection(bool bAskUser);
  void DeleteAndReplaceSelection();
  void OnImportAsAboutToShow();
  void OnImportAsClicked();
  void OnExportAssetWithDependencies();


private:
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;

private:
  void AssetCuratorEventHandler(const WAssetCuratorEvent& e);
  void UpdateAssetTypes();
  void ProjectEventHandler(const WToolsProjectEvent& e);
  void AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset);
  void AddImportedViaMenu(QMenu* pMenu);
  void GetSelectedImportableFiles(WDynamicArray<WString>& out_Files) const;
  void UpdateStatusBar();
  void UpdatePluginDataDirNames();
  WAssetBrowserSelection GetCurrentSelectionForActions() const;

  Mode m_Mode = Mode::Browser;
  WQtToolBarActionMapView* m_pToolbar = nullptr;
  WString m_sAllTypesFilter;
  QSharedPointer<WQtAssetBrowserModel> m_Model;
  WQtAssetBrowserFilter* m_pFilter = nullptr;

  /// After creating a new asset and renaming it, we want to open it as well.
  bool m_bOpenAfterRename = false;
};
