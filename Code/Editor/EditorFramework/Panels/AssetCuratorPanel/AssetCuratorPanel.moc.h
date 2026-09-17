#pragma once

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetCuratorPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

struct WLoggingEventData;

class W_EDITORFRAMEWORK_DLL WQtAssetCuratorFilter : public WQtAssetFilter
{
  Q_OBJECT
public:
  explicit WQtAssetCuratorFilter(QObject* pParent);

  void SetFilterTransitive(bool bFilterTransitive);

public:
  virtual WAssetFilterResult IsAssetFiltered(WStringView sDataDirParentRelativePath, bool bIsFolder, const WSubAsset* pInfo) const override;

  /// Whether this asset is in a state that the curator panel reports at all.
  static bool HasIssue(const WSubAsset* pInfo);

  /// Whether the asset's issue is only a consequence of another asset's issue, which is reported
  /// separately. Only meaningful for assets that HasIssue() accepts.
  static bool IsIndirectIssue(const WSubAsset* pInfo);

  bool m_bFilterTransitive = true;
};

class W_EDITORFRAMEWORK_DLL WQtAssetCuratorPanel : public WQtApplicationPanel, public Ui_AssetCuratorPanel
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtAssetCuratorPanel);

public:
  WQtAssetCuratorPanel(ads::CDockManager* pDockManager);
  ~WQtAssetCuratorPanel();

public Q_SLOTS:
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private Q_SLOTS:
  // note, because of the way we set up the widget, auto-connect doesn't work
  void onListAssetsDoubleClicked(const QModelIndex& index);
  void onCheckIndirectToggled(bool checked);
  void onListAssetsContextMenuRequested(const QPoint& pos);

private:
  void LogWriter(const WLoggingEventData& e);
  void UpdateIssueInfo();
  void AssetCuratorEventHandler(const WAssetCuratorEvent& e);

  /// Recounts the issues that are currently hidden as indirect and shows the number on the
  /// 'show indirect issues' checkbox, so they are discoverable without ticking it.
  void UpdateIndirectIssueCount();
  void ScheduleIndirectIssueCountUpdate();

  /// Returns the index of the item the context menu should operate on, which is the current
  /// selection. Returns an invalid index if nothing is selected.
  QModelIndex GetContextMenuTarget() const;

  QSharedPointer<WQtAssetBrowserModel> m_Model;
  WQtAssetCuratorFilter* m_pFilter;
  QPersistentModelIndex m_SelectedIndex;
  bool m_bIndirectCountScheduled = false;
};
