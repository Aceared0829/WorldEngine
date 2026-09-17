#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/SharedPtr.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QItemDelegate>
#include <QTreeWidget>
#include <QValidator>

class WQtAssetBrowserFilter;

/// Basic file name validator. Makes sure that under a given parent folder, the new file name is valid and not already in use by a different file.
class WFileNameValidator : public QValidator
{
public:
  /// Constructor. Validator requires the current location and name of the file.
  /// \param sParentFolder Absolute path to the location of the file.
  /// \param sCurrentName Current filename. If set, this name is marked as valid, even though it is already in use.
  WFileNameValidator(QObject* pParent, WStringView sParentFolder, WStringView sCurrentName);
  virtual QValidator::State validate(QString& ref_sInput, int& ref_iPos) const override;

private:
  WString m_sParentFolder;
  WString m_sCurrentName;
};

/// Custom delegate for the eqQtAssetBrowserFolderView to enable renaming folders. Does not do any model modifications. Instead, it fires editingFinished when the delegate editor closes.
class WFolderNameDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  WFolderNameDelegate(QObject* pParent = nullptr);

  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName) const;
};

/// Folder tree of the asset browser to allow filtering by folder.
///
/// This class keeps up to date with the folder structure in WFileSystemModel. Events from WFileSystemModel are cached ans flushed via OnFlushFileSystemEvents.
/// Folder movement, creation and deletion is supported and handled by this class. The context menu is implemented in WQtAssetBrowserWidget as it requires a global context of the asset browser instance.
class eqQtAssetBrowserFolderView : public QTreeWidget
{
  Q_OBJECT
public:
  eqQtAssetBrowserFolderView(QWidget* pParent);
  ~eqQtAssetBrowserFolderView();

  /// Required to be set right after the ctor. This class will call WQtAssetBrowserFilter::SetPathFilter whenever the current selected item changes.
  void SetFilter(WQtAssetBrowserFilter* pFilter);
  /// In dialog mode, any modifications (folder movement, creation and deletion) are disabled.
  void SetDialogMode(bool bDialogMode);

  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;

public Q_SLOTS:
  /// Creates a new folder under the current selected item and enters edit mode to allow the user to rename it.
  void NewFolder();
  /// Opens the current selected item in the windows explorer or OS equivalent.
  void TreeOpenExplorer();
  /// Deletes the currently selected folder after confirmation.
  void DeleteFolder();

private Q_SLOTS:
  void OnFolderEditingFinished(const QString& sAbsPath, const QString& sNewName);
  void OnFlushFileSystemEvents();
  void OnItemSelectionChanged();
  void OnPathFilterChanged();
  void OnPluginDataDirsChanged();

protected:
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void dropEvent(QDropEvent* event) override;
  virtual Qt::DropActions supportedDropActions() const override;
  WStatus canDrop(QDropEvent* e, WDynamicArray<WString>& out_files, WString& out_sTargetFolder);
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  bool SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);
  void UpdateDirectoryTree();
  void ClearDirectoryTree();
  void BuildDirectoryTree(const WDataDirPath& path, WStringView sCurPath, QTreeWidgetItem* pParent, WStringView sCurPathToItem, bool bIsHidden);
  void RemoveDirectoryTreeItem(WStringView sCurPath, QTreeWidgetItem* pParent, WStringView sCurPathToItem);
  QTreeWidgetItem* FindDirectoryTreeItem(WStringView sCurPath, QTreeWidgetItem* pParent, WStringView sCurPathToItem);
  void ProjectEventHandler(const WToolsProjectEvent& e);

private:
  // Shared object as the event handler can be called after this object is destroyed due to the use of WCopyOnBroadcastEvent in WFileSystemModel.
  // This is the only way to prevent race conditions and allow safe add/remove calls to multithreaded event broadcasters.
  class QueuedFolderEvents : public WRefCounted
  {
  public:
    void FileSystemModelFolderEventHandler(const WFolderChangedEvent& e);

    WMutex m_FolderStructureMutex;
    eqQtAssetBrowserFolderView* m_pParent = nullptr;
    WHybridArray<WFolderChangedEvent, 2> m_Events;
  };

  bool m_bDialogMode = false;
  WUInt32 m_uiKnownAssetFolderCount = 0;
  bool m_bTreeSelectionChangeInProgress = false;

  WQtAssetBrowserFilter* m_pFilter = nullptr;
  WSharedPtr<QueuedFolderEvents> m_pFolderEvents;

  WEventSubscriptionID m_FolderChangedSubscription = 0;
};
