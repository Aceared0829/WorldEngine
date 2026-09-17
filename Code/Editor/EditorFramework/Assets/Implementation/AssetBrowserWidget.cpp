#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QFile>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

WQtAssetBrowserWidget::WQtAssetBrowserWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);
  ResetTypeFilter->setEnabled(false);

  m_pFilter = new WQtAssetBrowserFilter(this);
  m_pFilter->SetShowItemsInSubFolders(pPreferences->m_bAssetBrowserShowItemsInSubFolders);

  UpdatePluginDataDirNames();

  TreeFolderFilter->SetFilter(m_pFilter);

  m_Model = QSharedPointer<WQtAssetBrowserModel>(new WQtAssetBrowserModel(this, m_pFilter));
  m_Model->Initialize();
  SearchWidget->setPlaceholderText("Search Assets");

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_Model.data());
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  ListAssets->setDragEnabled(true);
  ListAssets->setAcceptDrops(true);
  ListAssets->setDropIndicatorShown(true);
  on_ButtonIconMode_clicked();

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // Tool Bar
  {
    m_pToolbar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    m_pToolbar->SetActionContext(context);
    m_pToolbar->setObjectName("AssetBrowserToolBar");
    ToolBarLayout->insertWidget(1, m_pToolbar);
  }

  ButtonShowItemsSubFolders->setEnabled(true);
  ButtonShowItemsSubFolders->setChecked(m_pFilter->GetShowItemsInSubFolders());
  W_VERIFY(connect(ButtonShowItemsSubFolders, SIGNAL(toggled(bool)), this, SLOT(OnShowSubFolderItemsToggled())) != nullptr, "signal/slot connection failed");

  ButtonShowItemsHiddenFolders->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
  W_VERIFY(connect(ButtonShowItemsHiddenFolders, SIGNAL(toggled(bool)), this, SLOT(OnShowHiddenFolderItemsToggled())) != nullptr, "signal/slot connection failed");

  AssetStatusBar->setText(QString());

  W_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pFilter, SIGNAL(FilterChanged()), this, SLOT(OnFilterChanged())) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_Model.data(), SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");

  // the status bar counts rows, so it has to follow every row change, not just a full model reset
  connect(m_Model.data(), &QAbstractItemModel::rowsInserted, this, &WQtAssetBrowserWidget::UpdateStatusBar);
  connect(m_Model.data(), &QAbstractItemModel::rowsRemoved, this, &WQtAssetBrowserWidget::UpdateStatusBar);

  // items excluded by a single switch never show up as rows, so their counts need their own notification
  connect(m_Model.data(), &WQtAssetBrowserModel::ExcludedItemCountsChanged, this, &WQtAssetBrowserWidget::UpdateStatusBar);
  W_VERIFY(connect(m_Model.data(), &WQtAssetBrowserModel::editingFinished, this, &WQtAssetBrowserWidget::OnFileEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");

  W_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr, "signal/slot connection failed");
  connect(SearchWidget, &WQtSearchWidget::textChanged, this, &WQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  UpdateAssetTypes();

  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  WToolsProject::s_Events.AddEventHandler(WMakeDelegate(&WQtAssetBrowserWidget::ProjectEventHandler, this));

  setAcceptDrops(true);
}

WQtAssetBrowserWidget::~WQtAssetBrowserWidget()
{
  WToolsProject::s_Events.RemoveEventHandler(WMakeDelegate(&WQtAssetBrowserWidget::ProjectEventHandler, this));
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtAssetBrowserWidget::AssetCuratorEventHandler, this));


  ListAssets->setModel(nullptr);
}

void WQtAssetBrowserWidget::dragEnterEvent(QDragEnterEvent* pEvent)
{
  if (!pEvent->source())
    pEvent->acceptProposedAction();
}

void WQtAssetBrowserWidget::dragMoveEvent(QDragMoveEvent* pEvent)
{
  pEvent->acceptProposedAction();
}

void WQtAssetBrowserWidget::dragLeaveEvent(QDragLeaveEvent* pEvent)
{
  pEvent->accept();
}

static void CleanUpFiles(WArrayPtr<WString> files)
{
  for (const auto& file : files)
  {
    WOSFile::DeleteFile(file).IgnoreResult();
    WFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }
}

void WQtAssetBrowserWidget::dropEvent(QDropEvent* pEvent)
{
  const QMimeData* mime = pEvent->mimeData();
  if (!mime->hasUrls())
  {
    pEvent->ignore();
  }

  pEvent->acceptProposedAction();

  WStringBuilder sTargetDir;

  if (TreeFolderFilter->currentItem() != nullptr)
  {
    sTargetDir = TreeFolderFilter->currentItem()->data(0, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  }

  if (sTargetDir.IsEmpty())
  {
    WQtUiServices::MessageBoxInformation("Please first select a folder in the asset browser as the destination for the file import.");
    return;
  }

  QList<QUrl> urlList = mime->urls();
  WTempHybridArray<WString, 16> assetsToImport;

  // if we leave this function prematurely, delete all these temp files
  W_SCOPE_EXIT(CleanUpFiles(assetsToImport));

  bool overWriteAll = false;
  for (qsizetype i = 0, count = qMin(urlList.size(), qsizetype(32)); i < count; ++i)
  {
    QUrl url = urlList.at(i);
    QFileInfo fileinfo(url.toLocalFile());

    if (!fileinfo.exists())
      continue;

    // build source and destination paths info
    WStringBuilder srcPath = urlList.at(i).path().toUtf8().constData();
    srcPath.TrimWordStart("/"); // remove the "/" at the beginning of the source path

    WStringBuilder dstPath = sTargetDir;
    dstPath.AppendPath(fileinfo.fileName().toUtf8().constData());

    // Move the file/folder
    if (fileinfo.isDir())
    {
      if (!overWriteAll && WOSFile::ExistsDirectory(dstPath))
      {
        const auto res = WQtUiServices::MessageBoxQuestion(WFmt("This folder already exists:\n'{}'\n\nOverwrite existing files inside it?", dstPath), QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel, QMessageBox::Cancel, QMessageBox::YesToAll);

        switch (res)
        {
          case QMessageBox::Yes:
            break;

          case QMessageBox::YesToAll:
            overWriteAll = true;
            break;

          case QMessageBox::Cancel:
          default:
            return;
        }
      }

      if (WOSFile::CopyFolder(srcPath, dstPath, &assetsToImport) != W_SUCCESS)
      {
        WQtUiServices::MessageBoxWarning(WFmt("Failed to copy\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", srcPath, dstPath));
        return;
      }
    }
    else if (fileinfo.isFile())
    {
      if (!overWriteAll && WOSFile::ExistsFile(dstPath))
      {
        const auto res = WQtUiServices::MessageBoxQuestion(WFmt("This file already exists:\n'{}'\n\nOverwrite it?", dstPath), QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel, QMessageBox::Cancel, QMessageBox::YesToAll);

        switch (res)
        {
          case QMessageBox::Yes:
            break;
          case QMessageBox::YesToAll:
            overWriteAll = true;
            break;
          case QMessageBox::Cancel:
          default:
            return;
        }
      }

      if (WOSFile::CopyFile(srcPath, dstPath) != W_SUCCESS)
      {
        WQtUiServices::MessageBoxWarning(WFmt("Failed to copy\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", srcPath, dstPath));
        return;
      }

      assetsToImport.PushBack(dstPath);
    }
  }

  for (const auto& file : assetsToImport)
  {
    WFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }

  QTimer::singleShot(1, this, [=]()
    {
      // return to the OS and import with a slight delay, otherwise the drop operation blocks the OS
      WAssetDocumentGenerator::ImportAssets(assetsToImport);
      //
    });

  // now that we've successfully imported the assets, clear this list so that the files don't get deleted
  assetsToImport.Clear();
}

void WQtAssetBrowserWidget::UpdateAssetTypes()
{
  const auto& assetTypes0 = WAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  WMap<WString, const WDocumentTypeDescriptor*> assetTypes;
  for (auto it : assetTypes0)
  {
    assetTypes[WTranslate(it.Key())] = it.Value();
  }

  {
    WQtScopedBlockSignals block(TypeFilter);

    TypeFilter->clear();

    if (m_Mode == Mode::Browser)
    {
      // '<All Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("<All Files>"));

      // '<Importable Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/EditorFramework/Icons/ImportableFileType.svg")), QLatin1String("<Importable Files>"));
    }

    // '<All Assets>' Filter
    TypeFilter->addItem(QIcon(QLatin1String(":/AssetIcons/Icons/AllAssets.svg")), QLatin1String("<All Assets>"));

    for (const auto& it : assetTypes)
    {
      TypeFilter->addItem(WQtUiServices::GetCachedIconResource(it.Value()->m_sIcon, WColorScheme::GetCategoryColor(it.Value()->m_sAssetCategory, WColorScheme::CategoryColorUsage::AssetMenuIcon)), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      TypeFilter->setItemData(TypeFilter->count() - 1, QString::fromUtf8(it.Value()->m_sDocumentTypeName, it.Value()->m_sDocumentTypeName.GetElementCount()), Qt::UserRole);
    }
  }

  // make sure to apply the previously active type filter settings to the UI
  if (m_Mode == Mode::Browser)
  {
    WSet<WString> importExtensions;
    WAssetDocumentGenerator::GetSupportsFileTypes(importExtensions);
    m_pFilter->UpdateImportExtensions(importExtensions);
  }

  OnTypeFilterChanged();
}

void WQtAssetBrowserWidget::SetMode(Mode mode)
{
  if (m_Mode == mode)
    return;

  m_Mode = mode;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pToolbar->show();
      TreeFolderFilter->SetDialogMode(false);
      ListAssets->SetDialogMode(false);
      break;
    case Mode::FilePicker:
      TypeFilter->setVisible(false);
      ResetTypeFilter->setVisible(false);
      [[fallthrough]];
    case Mode::AssetPicker:
      m_pToolbar->hide();
      TreeFolderFilter->SetDialogMode(true);
      ListAssets->SetDialogMode(true);
      break;
  }

  UpdateAssetTypes();
}

void WQtAssetBrowserWidget::SaveState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    Settings.setValue("SplitterGeometry", splitter->saveGeometry());
    Settings.setValue("SplitterState", splitter->saveState());
    Settings.setValue("IconSize", IconSizeSlider->value());
    Settings.setValue("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode);
  }
  Settings.endGroup();
}

void WQtAssetBrowserWidget::RestoreState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    splitter->restoreGeometry(Settings.value("SplitterGeometry", splitter->saveGeometry()).toByteArray());
    splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
    IconSizeSlider->setValue(Settings.value("IconSize", IconSizeSlider->value()).toInt());

    if (Settings.value("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode).toBool())
      on_ButtonIconMode_clicked();
    else
      on_ButtonListMode_clicked();
  }
  Settings.endGroup();
}

void WQtAssetBrowserWidget::UpdatePluginDataDirNames()
{
  WSet<WString> bundleDirs;
  WQtEditorApp::GetSingleton()->GetActiveBundleDataDirectories(bundleDirs);

  WSet<WString> names;

  WStringBuilder sAbsPath;
  for (const WString& sDir : bundleDirs)
  {
    if (WFileSystem::ResolveSpecialDirectory(sDir, sAbsPath).Failed())
      continue;

    sAbsPath.MakeCleanPath();
    sAbsPath.Trim(nullptr, "/");

    names.Insert(sAbsPath.GetFileNameAndExtension());
  }

  m_pFilter->SetPluginDataDirNames(names);
}

void WQtAssetBrowserWidget::ProjectEventHandler(const WToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectOpened:
    {
      // this is necessary to detect new asset types when a plugin has been loaded (on project load)
      UpdateAssetTypes();
      UpdatePluginDataDirNames();
    }
    break;
    case WToolsProjectEvent::Type::ProjectClosed:
    {
      m_pFilter->Reset();
      m_pFilter->SetPluginDataDirNames(WSet<WString>());
    }
    break;
    default:
      break;
  }
}

void WQtAssetBrowserWidget::AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset)
{
  if (m_Mode != Mode::Browser)
    return;

  const WTempHybridArray<WDocumentManager*, 16>& managers = WDocumentManager::GetAllDocumentManagers();

  WDynamicArray<const WDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = pMenu->addMenu(QIcon(":/GuiFoundation/Icons/DocumentAdd.svg"), "New");

  WStringBuilder sTypeFilter = m_pFilter->GetTypeFilter();

  for (WDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<WAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const WDocumentTypeDescriptor* a, const WDocumentTypeDescriptor* b) -> bool
    { return WTranslate(a->m_sDocumentTypeName).Compare(WTranslate(b->m_sDocumentTypeName)) < 0; });

  QAction* pAction = pSubMenu->addAction(WMakeQString(WTranslate("Folder")));
  pAction->setIcon(WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Folder.svg"));
  connect(pAction, &QAction::triggered, static_cast<eqQtAssetBrowserFolderView*>(TreeFolderFilter), &eqQtAssetBrowserFolderView::NewFolder);

  pSubMenu->addSeparator();

  for (const WDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(WMakeQString(WTranslate(desc->m_sDocumentTypeName)));
    pAction->setIcon(WQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon, WColorScheme::GetCategoryColor(desc->m_sAssetCategory, WColorScheme::CategoryColorUsage::MenuEntryIcon)));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", QVariant::fromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &WQtAssetBrowserWidget::NewAsset);
  }
}


void WQtAssetBrowserWidget::AddImportedViaMenu(QMenu* pMenu)
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  // Find all uses
  WSet<WUuid> importedVia;
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(WQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (!bImportable)
      continue;

    WString sAbsPath = qtToEzString(m_Model->data(id, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
    WAssetCurator::GetSingleton()->FindAllUses(sAbsPath, importedVia);
  }

  // Sort by path
  WTempHybridArray<WUuid, 8> importedViaSorted;
  {
    importedViaSorted.Reserve(importedVia.GetCount());
    WAssetCurator::WLockedSubAssetTable allAssets = WAssetCurator::GetSingleton()->GetKnownSubAssets();
    for (const WUuid& guid : importedVia)
    {
      if (allAssets->Contains(guid))
        importedViaSorted.PushBack(guid);
    }

    importedViaSorted.Sort([&](const WUuid& a, const WUuid& b) -> bool
      { return allAssets->Find(a).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath().Compare(allAssets->Find(b).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath()) < 0; });
  }

  if (importedViaSorted.IsEmpty())
    return;

  // Create actions to open
  QMenu* pSubMenu = pMenu->addMenu("Imported via");
  pSubMenu->setIcon(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")));

  for (const WUuid& guid : importedViaSorted)
  {
    const WAssetCurator::WLockedSubAsset pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(guid);
    QIcon icon = WQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, WColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, WColorScheme::CategoryColorUsage::OverlayIcon));
    QString sRelPath = WMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

    QAction* pAction = pSubMenu->addAction(sRelPath);
    pAction->setIcon(icon);
    pAction->setProperty("AbsPath", WMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath()));
    connect(pAction, &QAction::triggered, this, &WQtAssetBrowserWidget::OnOpenImportReferenceAsset);
  }
}

void WQtAssetBrowserWidget::GetSelectedImportableFiles(WDynamicArray<WString>& out_Files) const
{
  out_Files.Clear();

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(WQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (bImportable)
    {
      out_Files.PushBack(qtToEzString(id.data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString()));
    }
  }
}

WAssetBrowserSelection WQtAssetBrowserWidget::GetCurrentSelectionForActions() const
{
  WAssetBrowserSelection res;

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    res.m_AbsolutePaths.PushBack(qtToEzString(id.data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString()));

    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)id.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (!itemType.IsAnySet(WAssetBrowserItemFlags::Asset | WAssetBrowserItemFlags::SubAsset))
      continue;

    const WUuid subAssetGuid = id.data(WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>();
    if (subAssetGuid.IsValid())
    {
      res.m_SubAssetGuids.PushBack(subAssetGuid);
    }

    const WUuid assetGuid = id.data(WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();
    if (assetGuid.IsValid() && !res.m_AssetGuids.Contains(assetGuid))
    {
      res.m_AssetGuids.PushBack(assetGuid);
    }
  }

  return res;
}

void WQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_Model->data(index, WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void WQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_Model->data(index, WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void WQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  const WUuid guid = m_Model->data(index, WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>();

  if (itemType.IsAnySet(WAssetBrowserItemFlags::Asset | WAssetBrowserItemFlags::SubAsset))
  {
    if (guid.IsValid())
    {
      WAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }
    Q_EMIT ItemChosen(guid, m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsSet(WAssetBrowserItemFlags::File))
  {
    Q_EMIT ItemChosen(WUuid::MakeInvalid(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory))
  {
    m_pFilter->SetPathFilter(m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data());
  }
}

void WQtAssetBrowserWidget::on_ButtonListMode_clicked()
{
  m_Model->SetIconMode(false);
  ListAssets->SetIconMode(false);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(true);
  ButtonIconMode->setChecked(false);
}

void WQtAssetBrowserWidget::on_ButtonIconMode_clicked()
{
  m_Model->SetIconMode(true);
  ListAssets->SetIconMode(true);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(false);
  ButtonIconMode->setChecked(true);
}

void WQtAssetBrowserWidget::on_IconSizeSlider_valueChanged(int iValue)
{
  ListAssets->SetIconScale(iValue);
}

void WQtAssetBrowserWidget::on_ListAssets_ViewZoomed(WInt32 iIconSizePercentage)
{
  WQtScopedBlockSignals block(IconSizeSlider);
  IconSizeSlider->setValue(iIconSizePercentage);
}

void WQtAssetBrowserWidget::on_ResetTypeFilter_clicked()
{
  switch (m_Mode)
  {
    case Mode::Browser:
      TypeFilter->setCurrentIndex(2);
      break;
    case Mode::AssetPicker:
    case Mode::FilePicker:
      TypeFilter->setCurrentIndex(0);
      break;
  }
}

void WQtAssetBrowserWidget::OnTextFilterChanged()
{
  OnFilterChanged();

  const QString sText = WMakeQString(m_pFilter->GetTextFilter());
  if (SearchWidget->text() != sText)
  {
    SearchWidget->setText(sText);
    QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
  }
}

void WQtAssetBrowserWidget::OnFilterChanged()
{
  const QString sText = WMakeQString(m_pFilter->GetTextFilter());
  ButtonShowItemsSubFolders->setEnabled(sText.isEmpty());
  ButtonShowItemsSubFolders->blockSignals(true);
  ButtonShowItemsSubFolders->setChecked(!sText.isEmpty() || m_pFilter->GetShowItemsInSubFolders());
  ButtonShowItemsSubFolders->blockSignals(false);

  ButtonShowItemsHiddenFolders->blockSignals(true);
  ButtonShowItemsHiddenFolders->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
  ButtonShowItemsHiddenFolders->blockSignals(false);
}

void WQtAssetBrowserWidget::OnTypeFilterChanged()
{
  WStringBuilder sTemp;
  const WStringBuilder sFilter(";", m_pFilter->GetTypeFilter(), ";");

  {
    WQtScopedBlockSignals _(TypeFilter);

    WInt32 iCheckedFilter = 0;
    WInt32 iNumChecked = 0;

    for (WInt32 i = 1; i < TypeFilter->count(); ++i)
    {
      sTemp.Set(";", TypeFilter->itemData(i, Qt::UserRole).toString().toUtf8().data(), ";");

      if (sFilter.FindSubString(sTemp) != nullptr)
      {
        ++iNumChecked;
        iCheckedFilter = i;
      }
    }

    if (iNumChecked == ((m_Mode != Mode::Browser) ? 1 : 3))
      TypeFilter->setCurrentIndex(iCheckedFilter);
    else
      TypeFilter->setCurrentIndex((m_Mode != Mode::Browser) ? 0 : 2); // "<All Assets>"

    int index = TypeFilter->currentIndex();

    switch (m_Mode)
    {
      case Mode::Browser:
        m_pFilter->SetShowNonImportableFiles(index == 0);
        m_pFilter->SetShowFiles(index == 0 || index == 1);
        break;
      case Mode::AssetPicker:
        m_pFilter->SetShowNonImportableFiles(false);
        m_pFilter->SetShowFiles(false);
        break;
      case Mode::FilePicker:
        m_pFilter->SetShowNonImportableFiles(true);
        m_pFilter->SetShowFiles(true);
        break;
    }
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void WQtAssetBrowserWidget::OnPathFilterChanged()
{
  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void WQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  m_pFilter->SetTextFilter(text.toUtf8().data());
}

void WQtAssetBrowserWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && m_Mode == Mode::Browser)
  {
    e->accept();

    // For single asset selection, use Delete & Replace to handle references
    QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
    if (selection.count() == 1)
    {
      const WBitflags<WAssetBrowserItemFlags> itemType =
        (WAssetBrowserItemFlags::Enum)selection[0].data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

      if (itemType.IsSet(WAssetBrowserItemFlags::Asset) && !itemType.IsSet(WAssetBrowserItemFlags::SubAsset))
      {
        DeleteAndReplaceSelection();
        return;
      }
    }

    DeleteSelection(true);
    return;
  }

  if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
  {
    e->accept();
    OnListOpenAssetDocument();
    return;
  }
}

void WQtAssetBrowserWidget::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton)
  {
    e->accept();
    WStringBuilder sPath = m_pFilter->GetPathFilter();
    if (sPath.IsEmpty())
      return;
    sPath.PathParentDirectory();
    sPath.Trim("/");

    m_pFilter->SetPathFilter(sPath);
    return;
  }

  QWidget::mousePressEvent(e);
}

void WQtAssetBrowserWidget::RenameCurrent()
{
  m_bOpenAfterRename = false;

  if (ListAssets->currentIndex().isValid())
  {
    ListAssets->edit(ListAssets->currentIndex());
  }
}

void WQtAssetBrowserWidget::DeleteSelection(bool bAskUser)
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)id.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (itemType.IsAnySet(WAssetBrowserItemFlags::SubAsset | WAssetBrowserItemFlags::DataDirectory))
    {
      WQtUiServices::MessageBoxWarning(WFmt("Sub-assets and data directories can't be deleted."));
      return;
    }
  }

  if (bAskUser)
  {
    QMessageBox::StandardButton choice = WQtUiServices::MessageBoxQuestion(WFmt("Delete the selected file?\n\nThis operation cannot be undone."), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Yes);
    if (choice == QMessageBox::StandardButton::Cancel)
      return;
  }

  for (const QModelIndex& id : selection)
  {
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)id.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    QString sQtAbsPath = id.data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    WString sAbsPath = qtToEzString(sQtAbsPath);

    if (itemType.IsSet(WAssetBrowserItemFlags::File))
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        WLog::Error("Failed to delete file '{}'", sAbsPath);
      }
    }
    else
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        WLog::Error("Failed to delete folder '{}'", sAbsPath);
      }
    }
    WFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);
  }
}

void WQtAssetBrowserWidget::DeleteAndReplaceSelection()
{
  // Get the single selected item
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  if (selection.count() != 1)
  {
    WQtUiServices::MessageBoxWarning("Delete & Replace only works with a single asset selection.");
    return;
  }

  const QModelIndex& selectedIndex = selection[0];

  // Verify it's an asset (not sub-asset, folder, etc.)
  const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)selectedIndex.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  if (!itemType.IsSet(WAssetBrowserItemFlags::Asset) || itemType.IsSet(WAssetBrowserItemFlags::SubAsset))
  {
    WQtUiServices::MessageBoxWarning("Delete & Replace only works with main assets, not sub-assets or files.");
    return;
  }

  // Get the asset GUID and info
  WUuid assetGuid = selectedIndex.data(WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();

  const WAssetCurator::WLockedSubAsset pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (!pSubAsset.isValid())
  {
    WQtUiServices::MessageBoxWarning("Could not retrieve asset information.");
    return;
  }

  // Get asset type for filtering the replacement picker
  WString sAssetTypeName = pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName;
  WString sAbsPath = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath();

  // Find all uses of this asset
  WSet<WUuid> uses;
  WAssetCurator::GetSingleton()->FindAllUses(assetGuid, uses, false /* bTransitive */);

  if (uses.IsEmpty())
  {
    // No references - just offer regular delete
    QMessageBox::StandardButton choice = WQtUiServices::MessageBoxQuestion(
      "This asset is not referenced by any other assets.\n\nDo you want to delete it?",
      QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::Cancel,
      QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Yes);

    if (choice == QMessageBox::StandardButton::Yes)
    {
      DeleteSelection(false);
    }
    return;
  }

  // Ask what should happen to the existing references. The three actions are distinct enough that
  // standard Yes/No/Cancel labels would be ambiguous, so custom button texts are used.
  enum class RefAction
  {
    Abort,
    Replace,      ///< point all references at another asset
    Clear,        ///< empty all references, leaving the using assets without one
    LeaveDangling ///< delete and let the references become invalid
  };

  RefAction refAction = RefAction::Abort;

  WStringBuilder sQuestion;
  sQuestion.SetFormat("This asset is referenced by {} other asset(s).\n\n"
                      "What should happen to those references?",
    uses.GetCount());

  if (WQtUiServices::IsUnattended())
  {
    // an automated caller must not silently modify other assets
    WQtUiServices::ReportSuppressedDialog(WStringBuilder("Question, answered automatically: ", sQuestion));
    refAction = RefAction::Abort;
  }
  else
  {
    QMessageBox box(this);
    box.setWindowTitle(WMakeQString(WApplication::GetApplicationInstance()->GetApplicationName()));
    box.setText(WMakeQString(sQuestion));
    box.setIcon(QMessageBox::Icon::Question);

    QPushButton* pReplaceBtn = box.addButton(QLatin1String("Replace With Other Asset..."), QMessageBox::ButtonRole::AcceptRole);
    QPushButton* pClearBtn = box.addButton(QLatin1String("Clear References"), QMessageBox::ButtonRole::DestructiveRole);
    QPushButton* pDeleteBtn = box.addButton(QLatin1String("Delete Anyway"), QMessageBox::ButtonRole::DestructiveRole);
    box.addButton(QMessageBox::StandardButton::Cancel);
    box.setDefaultButton(pReplaceBtn);

    box.exec();

    if (box.clickedButton() == pReplaceBtn)
      refAction = RefAction::Replace;
    else if (box.clickedButton() == pClearBtn)
      refAction = RefAction::Clear;
    else if (box.clickedButton() == pDeleteBtn)
      refAction = RefAction::LeaveDangling;
  }

  if (refAction == RefAction::Abort)
    return;

  if (refAction == RefAction::LeaveDangling)
  {
    // Delete without touching the references
    DeleteSelection(false);
    return;
  }

  // Build reference strings (assets store references as UUID strings)
  WStringBuilder sOldReference;
  WConversionUtils::ToString(assetGuid, sOldReference);

  // An empty string is the 'no asset set' value, so clearing is a replacement with nothing.
  WStringBuilder sNewReference;

  if (refAction == RefAction::Replace)
  {
    // Show asset browser dialog to pick replacement (filtered to same asset type)
    WQtAssetBrowserDlg dlg(this, assetGuid, sAssetTypeName, "Select Replacement Asset");

    if (dlg.exec() != QDialog::Accepted)
      return;

    WUuid replacementGuid = dlg.GetSelectedAssetGuid();

    // Verify replacement is valid and different
    if (!replacementGuid.IsValid())
    {
      WQtUiServices::MessageBoxWarning("No replacement asset was selected.");
      return;
    }

    if (replacementGuid == assetGuid)
    {
      WQtUiServices::MessageBoxWarning("The replacement asset must be different from the asset being deleted.");
      return;
    }

    WConversionUtils::ToString(replacementGuid, sNewReference);
  }

  // Perform replacements in all using documents
  WAssetCurator::ReplaceAssetResult result = WAssetCurator::GetSingleton()->ReplaceAssetReferenceInUses(assetGuid, sOldReference, sNewReference);

  const char* szVerbNoun = (refAction == RefAction::Clear) ? "Clearing references" : "Replacement";
  const char* szVerbPast = (refAction == RefAction::Clear) ? "cleared" : "replaced";

  // Build result message
  WStringBuilder sResultMsg;

  if (result.m_uiDocumentsFailed > 0)
  {
    sResultMsg.SetFormat(
      "{} partially completed:\n\n"
      "- {} document(s) modified successfully\n"
      "- {} document(s) failed\n"
      "- {} property reference(s) {}\n\n"
      "The original asset was NOT deleted due to errors.\n\n"
      "Errors:\n",
      szVerbNoun,
      result.m_uiDocumentsModified,
      result.m_uiDocumentsFailed,
      result.m_uiPropertiesReplaced,
      szVerbPast);

    for (const WString& error : result.m_Errors)
    {
      sResultMsg.Append("- ", error, "\n");
    }

    WQtUiServices::MessageBoxWarning(sResultMsg);
    return;
  }

  // All replacements succeeded - delete the original asset
  QString sQtAbsPath = WMakeQString(sAbsPath);
  if (!QFile::moveToTrash(sQtAbsPath))
  {
    sResultMsg.SetFormat(
      "{} completed successfully:\n"
      "- {} document(s) modified\n"
      "- {} property reference(s) {}\n\n"
      "However, failed to delete the original file:\n{}\n\n"
      "You may need to delete it manually.",
      szVerbNoun,
      result.m_uiDocumentsModified,
      result.m_uiPropertiesReplaced,
      szVerbPast,
      sAbsPath);

    WQtUiServices::MessageBoxWarning(sResultMsg);
    return;
  }

  WFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);

  // Complete success
  sResultMsg.SetFormat(
    "{} completed successfully:\n\n"
    "- {} document(s) modified\n"
    "- {} property reference(s) {}\n"
    "- Original asset deleted",
    szVerbNoun,
    result.m_uiDocumentsModified,
    result.m_uiPropertiesReplaced,
    szVerbPast);

  WQtUiServices::MessageBoxInformation(sResultMsg);
}

void WQtAssetBrowserWidget::OnImportAsAboutToShow()
{
  QMenu* pMenu = qobject_cast<QMenu*>(sender());

  if (!pMenu->actions().isEmpty())
    return;

  WTempHybridArray<WString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  WSet<WString> extensions;
  WStringBuilder sExt;
  for (const auto& file : filesToImport)
  {
    sExt = file.GetFileExtension();
    sExt.ToLower();
    extensions.Insert(sExt);
  }

  WTempHybridArray<WAssetDocumentGenerator*, 16> generators;
  WAssetDocumentGenerator::CreateGenerators(generators);
  WTempHybridArray<WAssetDocumentGenerator::ImportMode, 16> importModes;

  for (WAssetDocumentGenerator* pGen : generators)
  {
    for (WStringView ext : extensions)
    {
      if (pGen->SupportsFileType(ext))
      {
        pGen->GetImportModes({}, importModes);
        break;
      }
    }
  }

  for (const auto& mode : importModes)
  {
    QAction* act = pMenu->addAction(QIcon(WMakeQString(mode.m_sIcon)), WMakeQString(WTranslate(mode.m_sName)));
    act->setData(WMakeQString(mode.m_sName));
    connect(act, &QAction::triggered, this, &WQtAssetBrowserWidget::OnImportAsClicked);
  }

  WAssetDocumentGenerator::DestroyGenerators(generators);
}

void WQtAssetBrowserWidget::OnImportAsClicked()
{
  W_LOG_BLOCK("Importing Assets");

  WTempHybridArray<WString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  QAction* act = qobject_cast<QAction*>(sender());
  WString sMode = qtToEzString(act->data().toString());

  WTempHybridArray<WAssetDocumentGenerator*, 16> generators;
  WAssetDocumentGenerator::CreateGenerators(generators);

  WAssetProcessor::GetSingleton()->m_iPauseProcessing.Increment();
  W_SCOPE_EXIT(WAssetProcessor::GetSingleton()->m_iPauseProcessing.Decrement());

  WProgressRange progress("Importing Assets", filesToImport.GetCount(), true);

  WTempHybridArray<WAssetDocumentGenerator::ImportMode, 16> importModes;
  for (WAssetDocumentGenerator* pGen : generators)
  {
    importModes.Clear();
    pGen->GetImportModes({}, importModes);

    for (const auto& mode : importModes)
    {
      if (mode.m_sName == sMode)
      {
        for (const WString& file : filesToImport)
        {
          if (progress.WasCanceled())
          {
            goto done;
          }

          progress.BeginNextStep(file.GetFileNameAndExtension());


          if (pGen->SupportsFileType(file))
          {
            const WStatus res = pGen->Import(file, sMode, false);
            if (res.Failed())
            {
              res.LogFailure();
              goto done;
            }
          }
        }

        goto done;
      }
    }
  }

done:
  WAssetDocumentGenerator::DestroyGenerators(generators);
}

void WQtAssetBrowserWidget::AssetCuratorEventHandler(const WAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case WAssetCuratorEvent::Type::AssetListReset:
      UpdateAssetTypes();
      break;
    default:
      break;
  }
}

void WQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  const bool bClickedValid = TreeFolderFilter->indexAt(pt).isValid();

  if (bClickedValid)
  {
    const bool bIsRoot = TreeFolderFilter->currentItem() && TreeFolderFilter->currentItem() == TreeFolderFilter->topLevelItem(0);
    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), TreeFolderFilter, SLOT(TreeOpenExplorer()));
    }

    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      // Delete
      const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)TreeFolderFilter->currentItem()->data(0, WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), TreeFolderFilter, &eqQtAssetBrowserFolderView::DeleteFolder);
      if (itemType.IsSet(WAssetBrowserItemFlags::DataDirectory))
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Data directories can't be deleted.");
      }

      // Create
      AddAssetCreatorMenu(&m, false);
    }

    m.addSeparator();
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in hidden folders"), this, SLOT(OnShowHiddenFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
    pAction->setToolTip("Whether to ignore '_data' folders when showing items in sub-folders is enabled.");
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Plugin Directories"), this, SLOT(OnShowPluginDataDirsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowPluginDataDirs());
    pAction->setToolTip("Whether to show the data directories that the active plugins provide.");
  }

  {
    QAction* pAction = m.addAction(QIcon(":/GuiFoundation/Icons/SaveAll.svg"), QLatin1String("Re-save Assets in Folder"), this, SLOT(OnResaveAssets()));
    pAction->setToolTip("Opens every document and saves it. Used to get all documents to the latest version.");
  }

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void WQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  WQtScopedBlockSignals block(TypeFilter);

  WStringBuilder sFilter;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pFilter->SetShowNonImportableFiles(index == 0);
      m_pFilter->SetShowFiles(index == 0 || index == 1);
      ResetTypeFilter->setEnabled(index != 2);
      break;
    case Mode::AssetPicker:
      m_pFilter->SetShowNonImportableFiles(false);
      m_pFilter->SetShowFiles(false);
      ResetTypeFilter->setEnabled(index != 0);
      break;
    case Mode::FilePicker:
      m_pFilter->SetShowNonImportableFiles(true);
      m_pFilter->SetShowFiles(true);
      ResetTypeFilter->setEnabled(false);
      break;
  }


  switch (index)
  {
    case 0:
    case 1:
    case 2:
      // all filters enabled
      // might be different for dialogs
      sFilter = m_sAllTypesFilter;
      break;

    default:
      sFilter.Set(";", TypeFilter->itemData(index, Qt::UserRole).toString().toUtf8().data(), ";");
      break;
  }

  m_pFilter->SetTypeFilter(sFilter);
}

void WQtAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pFilter->SetShowItemsInSubFolders(!m_pFilter->GetShowItemsInSubFolders());

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  pPreferences->m_bAssetBrowserShowItemsInSubFolders = m_pFilter->GetShowItemsInSubFolders();
}

void WQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void WQtAssetBrowserWidget::OnShowPluginDataDirsToggled()
{
  m_pFilter->SetShowPluginDataDirs(!m_pFilter->GetShowPluginDataDirs());
}

void WQtAssetBrowserWidget::UpdateStatusBar()
{
  const WUInt32 uiVisible = (WUInt32)m_Model->rowCount(QModelIndex());

  WStringBuilder sText;
  sText.SetFormat("{} item{}", uiVisible, uiVisible == 1 ? "" : "s");

  auto appendCount = [&](WAssetFilterResult reason, WStringView sSuffix)
  {
    const WUInt32 uiCount = m_Model->GetNumExcludedItems(reason);

    if (uiCount > 0)
    {
      sText.AppendFormat(" | {} {}", uiCount, sSuffix);
    }
  };

  appendCount(WAssetFilterResult::NonAssetFile, "files");
  appendCount(WAssetFilterResult::NonImportableFile, "non-importable files");
  appendCount(WAssetFilterResult::HiddenFolder, "in hidden folders");

  AssetStatusBar->setText(WMakeQString(sText));
}

void WQtAssetBrowserWidget::OnResaveAssets()
{
  if (QTreeWidgetItem* pCurrentItem = TreeFolderFilter->currentItem())
  {
    QModelIndex id = TreeFolderFilter->indexFromItem(pCurrentItem);
    WStringBuilder sAbsPath = qtToEzString(id.data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString());

    WAssetCurator::GetSingleton()->ResaveAllAssets(sAbsPath);
  }
}

void WQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  WAssetBrowserSelection::SetCurrent(GetCurrentSelectionForActions());

  // the actions below read the selection from here, so it has to stay valid until the menu is closed
  W_SCOPE_EXIT(WAssetBrowserSelection::SetCurrent(WAssetBrowserSelection()));

  if (ListAssets->selectionModel()->hasSelection())
  {
    bool bShowDocumentActions = false;

    if (m_Mode == Mode::Browser)
    {
      QString sTitle = "Open Selection";
      QIcon icon = QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg"));

      bool bShowOpenWith = false;

      if (ListAssets->selectionModel()->selectedIndexes().count() == 1)
      {
        const QModelIndex firstItem = ListAssets->selectionModel()->selectedIndexes()[0];
        const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)firstItem.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
        if (itemType.IsAnySet(WAssetBrowserItemFlags::Asset | WAssetBrowserItemFlags::SubAsset))
        {
          sTitle = "Open Document";
          bShowDocumentActions = true;
        }
        else if (itemType.IsSet(WAssetBrowserItemFlags::File))
        {
          sTitle = "Open File";
          bShowOpenWith = true;
        }
        else if (itemType.IsAnySet(WAssetBrowserItemFlags::DataDirectory | WAssetBrowserItemFlags::Folder))
        {
          sTitle = "Enter Folder";
          icon = QIcon(QLatin1String(":/EditorFramework/Icons/Folder.svg"));
        }
      }
      m.setDefaultAction(m.addAction(icon, sTitle, this, SLOT(OnListOpenAssetDocument())));

      if (bShowOpenWith)
      {
        m.addAction(icon, "Open With...", this, SLOT(OnListOpenFileWith()));
      }
    }
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut.svg")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));

    if (bShowDocumentActions)
    {
      m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/TransformAsset.svg")), QLatin1String("Transform"), this, SLOT(OnTransform()));
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));

      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct references to this asset"), this, [&]()
        { OnListFindAllReferences(false); });
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]()
        { OnListFindAllReferences(true); });
    }
  }

  // actions that plugins contributed for specific asset types.
  // Holds the references to the proxies, so it has to outlive m.exec() below.
  WHashTable<WUuid, QSharedPointer<WQtProxy>> contextMenuProxies;
  {
    WActionContext context;
    context.m_sMapping = "AssetBrowserContextMenu";
    context.m_pWindow = this;

    if (WActionMap* pActionMap = WActionMapManager::GetActionMap(context.m_sMapping))
    {
      const int iActionsBefore = m.actions().count();
      WQtMenuActionMapView::AddDocumentObjectToMenu(contextMenuProxies, context, pActionMap, &m, pActionMap->BuildActionTree());

      // the proxies are cached and reused, so they still reflect the selection they were built with
      for (auto it : contextMenuProxies)
      {
        if (WAction* pAction = it.Value()->GetAction())
        {
          pAction->RefreshState();
        }

        it.Value()->Update();
      }

      // a sub-menu has no state of its own, so it stays visible even when every entry in it hid itself
      for (QAction* pAction : m.actions())
      {
        QMenu* pSubMenu = pAction->menu();
        if (pSubMenu == nullptr)
          continue;

        bool bAnyEntryVisible = false;
        for (QAction* pEntry : pSubMenu->actions())
        {
          if (pEntry->isVisible() && !pEntry->isSeparator())
          {
            bAnyEntryVisible = true;
            break;
          }
        }

        pAction->setVisible(bAnyEntryVisible);
      }

      // Only separate when something is actually shown: the action map adds its own separators around
      // each category, and an action that hid itself still counts towards actions().
      bool bAnyVisible = false;
      for (int i = iActionsBefore; i < m.actions().count(); ++i)
      {
        if (m.actions()[i]->isVisible() && !m.actions()[i]->isSeparator())
        {
          bAnyVisible = true;
          break;
        }
      }

      if (bAnyVisible)
      {
        m.addSeparator();
      }
    }
  }

  if (m_Mode == Mode::Browser && ListAssets->selectionModel()->hasSelection())
  {
    QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
    bool bImportable = false;
    bool bAllFiles = true;
    bool bHasExportableItems = false;

    for (const QModelIndex& id : selection)
    {
      bImportable |= id.data(WQtAssetBrowserModel::UserRoles::Importable).toBool();

      const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)id.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(WAssetBrowserItemFlags::SubAsset | WAssetBrowserItemFlags::DataDirectory))
      {
        bAllFiles = false;
      }

      if (itemType.IsAnySet(WAssetBrowserItemFlags::Asset | WAssetBrowserItemFlags::SubAsset | WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory))
      {
        bHasExportableItems = true;
      }
    }

    // Rename
    {
      bool bCanRename = true;

      QModelIndex id = ListAssets->currentIndex();

      const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)id.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(WAssetBrowserItemFlags::SubAsset | WAssetBrowserItemFlags::DataDirectory))
      {
        bCanRename = false;
      }

      QAction* pRename = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Rename.svg")), QLatin1String("Rename"), this, SLOT(RenameCurrent()));
      pRename->setShortcut(QKeySequence("F2"));
      if (!bCanRename)
      {
        pRename->setEnabled(false);
        pRename->setToolTip("Sub-assets and data directories can't be renamed.");
      }
    }

    // Delete & Replace (single main asset only)
    if (selection.count() == 1)
    {
      const QModelIndex& firstItem = selection[0];
      const WBitflags<WAssetBrowserItemFlags> firstItemType =
        (WAssetBrowserItemFlags::Enum)firstItem.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

      if (firstItemType.IsSet(WAssetBrowserItemFlags::Asset) &&
          !firstItemType.IsSet(WAssetBrowserItemFlags::SubAsset))
      {
        m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")),
          QLatin1String("Delete && Replace..."),
          this, SLOT(DeleteAndReplaceSelection()));
      }
      else if (bAllFiles)
      {
        // Single non-asset file or folder - show regular Delete
        QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"));
        pDelete->setShortcut(QKeySequence("Del"));
        connect(pDelete, &QAction::triggered, this, [this]()
          { DeleteSelection(true); });
      }
    }
    else // Delete (multiple selections)
    {
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"));
      pDelete->setShortcut(QKeySequence("Del"));
      connect(pDelete, &QAction::triggered, this, [this]()
        { DeleteSelection(true); });
      if (!bAllFiles)
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Sub-assets and data directories can't be deleted.");
      }
    }

    // Export
    if (bHasExportableItems)
    {
      m.addSeparator();
      m.addAction(QLatin1String("Export with Dependencies..."), this, SLOT(OnExportAssetWithDependencies()));
    }

    // Import assets
    if (bImportable)
    {
      m.addSeparator();
      // Import dialog is superseeded by better alternatives
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), QLatin1String("Import..."), this, SLOT(ImportSelection()));
      QMenu* imp = m.addMenu(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), "Import As");
      connect(imp, &QMenu::aboutToShow, this, &WQtAssetBrowserWidget::OnImportAsAboutToShow);
      AddImportedViaMenu(&m);
    }
  }

  m.addSeparator();

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());

  m.addSeparator();
  AddAssetCreatorMenu(&m, true);

  m.exec(ListAssets->viewport()->mapToGlobal(pt));
}

void WQtAssetBrowserWidget::OnListOpenAssetDocument()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  for (auto& index : selection)
  {
    // Only enter folders on a single selection. Otherwise the results are undefined.
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (selection.count() > 1 && itemType.IsAnySet(WAssetBrowserItemFlags::DataDirectory | WAssetBrowserItemFlags::Folder))
      continue;
    on_ListAssets_doubleClicked(index);
  }
}

void WQtAssetBrowserWidget::OnListOpenFileWith()
{
  if (!ListAssets->currentIndex().isValid())
    return;

  WString sPath = m_Model->data(ListAssets->currentIndex(), WQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  WQtUiServices::OpenWith(sPath);
}


void WQtAssetBrowserWidget::OnTransform()
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  WProgressRange range("Transforming Assets", 1 + selection.length(), true);

  for (auto& index : selection)
  {
    if (range.WasCanceled())
      break;

    WUuid guid = m_Model->data(index, WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();
    QString sPath = m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString();
    range.BeginNextStep(sPath.toUtf8());
    WTransformStatus res = WAssetCurator::GetSingleton()->TransformAsset(guid, WTransformFlags::TriggeredManually);
    if (res.Failed())
    {
      WLog::Error("{0} ({1})", res.m_sMessage, sPath.toUtf8().data());
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  WAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
}

void WQtAssetBrowserWidget::OnListOpenExplorer()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  WString sPath = m_Model->data(ListAssets->currentIndex(), WQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  WQtUiServices::OpenInExplorer(sPath, true);
}

void WQtAssetBrowserWidget::OnListCopyAssetGuid()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  WStringBuilder tmp;
  WUuid guid = m_Model->data(ListAssets->currentIndex(), WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>();

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* mimeData = new QMimeData();
  mimeData->setText(WConversionUtils::ToString(guid, tmp).GetData());
  clipboard->setMimeData(mimeData);

  WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(WFmt("Copied asset GUID: {}", tmp), WTime::MakeFromSeconds(5));
}

void WQtAssetBrowserWidget::OnFilterToThisPath()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  WStringBuilder sPath = m_Model->data(ListAssets->currentIndex(), WQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  sPath.PathParentDirectory();
  sPath.Trim("/");

  m_pFilter->SetPathFilter(sPath);
}

void WQtAssetBrowserWidget::OnListFindAllReferences(bool transitive)
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  WUuid guid = m_Model->data(ListAssets->currentIndex(), WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>();
  WStringBuilder sAssetGuid;
  WConversionUtils::ToString(guid, sAssetGuid);

  WStringBuilder sFilter;
  sFilter.SetFormat("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
  m_pFilter->SetTextFilter(sFilter);
  m_pFilter->SetPathFilter("");
}

void WQtAssetBrowserWidget::OnSelectionTimer()
{
  if (m_Model->rowCount() == 1)
  {
    auto index = m_Model->index(0, 0);

    ListAssets->selectionModel()->select(index, QItemSelectionModel::SelectionFlag::ClearAndSelect);
  }
}


void WQtAssetBrowserWidget::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    WUuid guid = m_Model->data(index, WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>();
    Q_EMIT ItemSelected(guid, m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
}

void WQtAssetBrowserWidget::OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    WUuid guid = m_Model->data(index, WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>();
    Q_EMIT ItemSelected(guid, m_Model->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
}


void WQtAssetBrowserWidget::OnModelReset()
{
  UpdateStatusBar();

  Q_EMIT ItemCleared();
}

void WQtAssetBrowserWidget::OnExportAssetWithDependencies()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  WAssetCurator* pCurator = WAssetCurator::GetSingleton();
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  WDynamicArray<WString> sources;
  WStringBuilder sTemp;

  for (const QModelIndex& index : selection)
  {
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    if (itemType.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory))
    {
      QString sAbsPath = m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
      WString sFolderPath = sAbsPath.toUtf8().data();

      pCurator->GetAllAssetsInFolder(sFolderPath, sources);
    }
    else if (itemType.IsAnySet(WAssetBrowserItemFlags::Asset | WAssetBrowserItemFlags::SubAsset))
    {
      WUuid guid = m_Model->data(index, WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>();

      if (guid.IsValid())
      {
        WConversionUtils::ToString(guid, sTemp);
        sources.PushBack(sTemp);
      }
    }
  }

  if (sources.IsEmpty())
  {
    WQtUiServices::MessageBoxWarning("No valid assets or folders selected for export.");
    return;
  }

  QString sDestination = QFileDialog::getExistingDirectory(
    this,
    QLatin1String("Select Export Destination"),
    QString(),
    QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

  if (sDestination.isEmpty())
    return;

  WStringBuilder sDestPath = sDestination.toUtf8().data();
  WAssetCurator::ExportResult result = pCurator->ExportAssets(sources, sDestPath, WDependencyFlags::Transform | WDependencyFlags::Thumbnail | WDependencyFlags::Package);

  WStringBuilder sMessage;
  sMessage.SetFormat("Export completed.\n\nCopied {} file(s).", result.m_uiCopiedFiles);

  if (result.m_uiFailedFiles > 0)
  {
    sMessage.AppendFormat("\n\nFailed to copy {} file(s).", result.m_uiFailedFiles);
    WQtUiServices::MessageBoxWarning(sMessage);
  }
  else
  {
    WQtUiServices::MessageBoxInformation(sMessage);
  }
}


void WQtAssetBrowserWidget::NewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  WAssetDocumentManager* pManager = (WAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  WString sAssetType = pSender->property("AssetType").toString().toUtf8().data();
  WString sStartFileName = WTranslate(sAssetType);
  WString sExtension = pSender->property("Extension").toString().toUtf8().data();
  bool useSelection = pSender->property("UseSelection").toBool();

  QString sStartDir;

  // find path
  {
    if (TreeFolderFilter->selectionModel()->hasSelection())
    {
      auto idx = TreeFolderFilter->selectionModel()->selection().indexes()[0];
      sStartDir = TreeFolderFilter->itemFromIndex(idx)->data(0, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
    }

    // this will take precedence
    if (useSelection && ListAssets->selectionModel()->hasSelection())
    {
      WString sPath = m_Model->data(ListAssets->currentIndex(), WQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty())
      {
        WStringBuilder temp = sPath;
        sPath = temp.GetFileDirectory();

        sStartDir = sPath.GetData();

        sStartFileName = temp.GetFileName();
      }
    }
  }

  if (sStartDir.isEmpty())
  {
    // this happens when the root node is selected
    sStartDir = WToolsProject::GetSingleton()->GetProjectDirectory().GetData();
  }

  WStringBuilder sNewAsset = qtToEzString(sStartDir);
  WStringBuilder sBaseFileName;
  WPathUtils::MakeValidFilename(sStartFileName, ' ', sBaseFileName);
  sNewAsset.AppendFormat("/{}.{}", sBaseFileName, sExtension);

  for (WUInt32 i = 2; WOSFile::ExistsFile(sNewAsset); i++)
  {
    sNewAsset = qtToEzString(sStartDir);
    sNewAsset.AppendFormat("/{}{}.{}", sBaseFileName, i, sExtension);
  }

  sNewAsset.MakeCleanPath();

  WDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sNewAsset, pDoc, WDocumentFlags::Default).Failed())
  {
    WLog::Error("Failed to create document: {}", sNewAsset);
    return;
  }

  {
    WStringBuilder sRelativePath = sNewAsset;
    if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelativePath))
    {
      m_pFilter->SetTemporaryPinnedItem(sRelativePath);
    }
    WFileSystemModel::GetSingleton()->NotifyOfChange(sNewAsset);
    m_Model->OnFileSystemUpdate();
  }

  WInt32 iNewIndex = m_Model->FindIndex(sNewAsset);
  if (iNewIndex != -1)
  {
    m_bOpenAfterRename = true;
    QModelIndex idx = m_Model->index(iNewIndex, 0);
    ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    ListAssets->edit(idx);
  }
}


void WQtAssetBrowserWidget::OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset)
{
  WStringBuilder sOldPath = qtToEzString(sAbsPath);
  WStringBuilder sNewPath = sOldPath;
  sNewPath.ChangeFileName(qtToEzString(sNewName));

  if (sOldPath != sNewPath)
  {
    if (WOSFile::MoveFileOrDirectory(sOldPath, sNewPath).Failed())
    {
      WLog::Error("Failed to rename '{}' to '{}'", sOldPath, sNewPath);
      return;
    }

    WFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    WFileSystemModel::GetSingleton()->NotifyOfChange(sOldPath);

    // If we have a temporary item, make sure that any renames ensure that the item is still set as the new temporary
    // A common case is: a type filter is active that excludes a newly created asset. Thus, on creation the new asset is set as the pinned item. Editing of the item is started and the user gives it a new name and we end up here. We want the item to remain pinned.
    if (!m_pFilter->GetTemporaryPinnedItem().IsEmpty())
    {
      WStringBuilder sOldRelativePath = sOldPath;
      WStringBuilder sNewRelativePath = sNewPath;
      if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sOldRelativePath) && WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewRelativePath))
      {
        if (sOldRelativePath == m_pFilter->GetTemporaryPinnedItem())
        {
          m_pFilter->SetTemporaryPinnedItem(sNewRelativePath);
        }
      }
    }
    m_Model->OnFileSystemUpdate();

    // it is necessary to flush the events queued on the main thread, otherwise opening the asset may not work as intended
    WAssetCurator::GetSingleton()->MainThreadTick(true);

    WInt32 iNewIndex = m_Model->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_Model->index(iNewIndex, 0);
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    }
  }

  if (m_bOpenAfterRename)
  {
    m_bOpenAfterRename = false;

    WInt32 iNewIndex = m_Model->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_Model->index(iNewIndex, 0);
      on_ListAssets_doubleClicked(idx);
    }
  }
}

void WQtAssetBrowserWidget::ImportSelection()
{
  WTempHybridArray<WString, 4> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  WAssetDocumentGenerator::ImportAssets(filesToImport);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    Q_EMIT m_Model->dataChanged(id, id);
  }
}

void WQtAssetBrowserWidget::OnOpenImportReferenceAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());
  WString sAbsPath = qtToEzString(pSender->property("AbsPath").toString());

  WQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, WDocumentFlags::RequestWindow | WDocumentFlags::AddToRecentFilesList);
}

void WQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
}

void WQtAssetBrowserWidget::SetSelectedAsset(WUuid preselectedAsset)
{
  if (!preselectedAsset.IsValid())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToItem", Qt::ConnectionType::QueuedConnection, Q_ARG(WUuid, preselectedAsset));
}

void WQtAssetBrowserWidget::SetSelectedFile(WStringView sAbsPath)
{
  if (sAbsPath.IsEmpty())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToFile", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, WMakeQString(sAbsPath)));
}

void WQtAssetBrowserWidget::OnScrollToItem(WUuid preselectedAsset)
{
  const WAssetCurator::WLockedSubAsset pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(preselectedAsset);
  if (pSubAsset.isValid())
  {
    WStringBuilder sPath = WMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath()).toUtf8().data();
    sPath.PathParentDirectory();
    sPath.Trim("/");
    m_pFilter->SetPathFilter(sPath);
    m_pFilter->SetTextFilter("");
  }

  for (WInt32 i = 0; i < m_Model->rowCount(); ++i)
  {
    QModelIndex idx = m_Model->index(i, 0);
    if (m_Model->data(idx, WQtAssetBrowserModel::UserRoles::SubAssetGuid).value<WUuid>() == preselectedAsset)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void WQtAssetBrowserWidget::OnScrollToFile(QString sPreselectedFile)
{
  WStringBuilder sPath = sPreselectedFile.toUtf8().data();
  if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
  {
    sPath.PathParentDirectory();
    sPath.Trim("/");
    m_pFilter->SetPathFilter(sPath);
  }

  for (WInt32 i = 0; i < m_Model->rowCount(); ++i)
  {
    QModelIndex idx = m_Model->index(i, 0);

    if (m_Model->data(idx, WQtAssetBrowserModel::UserRoles::AbsolutePath).value<QString>() == sPreselectedFile)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void WQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(WStringView sFilters)
{
  m_sAllTypesFilter.Clear();

  if (!sFilters.IsEmpty())
  {
    WStringBuilder sFilter;
    const WStringBuilder sAllFilters(";", sFilters, ";");

    m_sAllTypesFilter = sAllFilters;

    {
      WQtScopedBlockSignals block(TypeFilter);

      for (WInt32 i = TypeFilter->count(); i > 1; --i)
      {
        const WInt32 idx = i - 1;

        sFilter.Set(";", TypeFilter->itemData(idx, Qt::UserRole).toString().toUtf8().data(), ";");

        if (sAllFilters.FindSubString(sFilter) == nullptr)
        {
          TypeFilter->removeItem(idx);
        }
      }
    }
  }

  m_pFilter->SetAllTypesFilter(m_sAllTypesFilter);
  m_pFilter->SetTypeFilter(m_sAllTypesFilter);
}

void WQtAssetBrowserWidget::UseFileExtensionFilters(WStringView sFileExtensions)
{
  m_pFilter->SetFileExtensionFilters(sFileExtensions);
}

void WQtAssetBrowserWidget::SetRequiredTag(WStringView sRequiredTag)
{
  m_pFilter->SetRequiredTag(sRequiredTag);
}
