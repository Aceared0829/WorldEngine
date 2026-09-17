#include <EditorFramework/Assets/AssetDocument.h>

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <QMenu>
#include <QTimer>

WQtAssetCuratorFilter::WQtAssetCuratorFilter(QObject* pParent)
  : WQtAssetFilter(pParent)
{
}

void WQtAssetCuratorFilter::SetFilterTransitive(bool bFilterTransitive)
{
  m_bFilterTransitive = bFilterTransitive;
}

bool WQtAssetCuratorFilter::HasIssue(const WSubAsset* pInfo)
{
  if (!pInfo)
    return false;

  if (!pInfo->m_bMainAsset)
    return false;

  const WAssetInfo::TransformState state = pInfo->m_pAssetInfo->m_TransformState;

  return state == WAssetInfo::MissingTransformDependency || state == WAssetInfo::CircularDependency || state == WAssetInfo::MissingThumbnailDependency || state == WAssetInfo::MissingPackageDependency || state == WAssetInfo::TransformError;
}

bool WQtAssetCuratorFilter::IsIndirectIssue(const WSubAsset* pInfo)
{
  // An issue is 'indirect' when every dependency that this asset is missing resolves to an asset
  // that the curator still knows about. Those assets are reported with their own issue, so listing
  // everything downstream of them would only repeat the same root cause.
  auto allDepsResolve = [](const WSet<WString>& deps) -> bool
  {
    for (const WString& ref : deps)
    {
      if (!WAssetCurator::GetSingleton()->FindSubAsset(ref).isValid())
        return false;
    }
    return true;
  };

  switch (pInfo->m_pAssetInfo->m_TransformState)
  {
    case WAssetInfo::MissingThumbnailDependency:
      return allDepsResolve(pInfo->m_pAssetInfo->m_MissingThumbnailDeps);

    case WAssetInfo::MissingPackageDependency:
      return allDepsResolve(pInfo->m_pAssetInfo->m_MissingPackageDeps);

    default:
      return false;
  }
}

WAssetFilterResult WQtAssetCuratorFilter::IsAssetFiltered(WStringView sDataDirParentRelativePath, bool bIsFolder, const WSubAsset* pInfo) const
{
  if (!HasIssue(pInfo))
    return WAssetFilterResult::Filtered;

  if (m_bFilterTransitive && IsIndirectIssue(pInfo))
    return WAssetFilterResult::Filtered;

  return WAssetFilterResult::Visible;
}

W_IMPLEMENT_SINGLETON(WQtAssetCuratorPanel);

WQtAssetCuratorPanel::WQtAssetCuratorPanel(ads::CDockManager* pDockManager)
  : WQtApplicationPanel(pDockManager, "Panel.AssetCurator")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  // using pDummy instead of 'this' breaks auto-connect for slots
  setWidget(pDummy);
  setIcon(WQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/AssetCurator.svg"));
  setWindowTitle(WMakeQString(WTranslate("Panel.AssetCurator")));

  connect(ListAssets, &QTreeView::doubleClicked, this, &WQtAssetCuratorPanel::onListAssetsDoubleClicked);
  connect(CheckIndirect, &QCheckBox::toggled, this, &WQtAssetCuratorPanel::onCheckIndirectToggled);
  connect(ListAssets, &QWidget::customContextMenuRequested, this, &WQtAssetCuratorPanel::onListAssetsContextMenuRequested);

  WAssetProcessor::GetSingleton()->AddLogWriter(WMakeDelegate(&WQtAssetCuratorPanel::LogWriter, this));

  ProcessorProgress->SetGridBarWidget(ProcessorProgressGridBar);
  ProcessorProgress->SetScrollBarWidget(ProcessorScrollBar);

  m_pFilter = new WQtAssetCuratorFilter(this);
  m_Model = QSharedPointer<WQtAssetBrowserModel>(new WQtAssetBrowserModel(this, m_pFilter));
  m_Model->Initialize();
  m_Model->SetIconMode(false);

  TransformLog->ShowControls(false);

  CuratorLog->setVisible(false);

  ListAssets->setModel(m_Model.data());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  W_VERIFY(
    connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr,
    "signal/slot connection failed");
  W_VERIFY(connect(m_Model.data(), &QAbstractItemModel::dataChanged, this,
              [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
              {
                if (m_SelectedIndex.isValid() && topLeft.row() <= m_SelectedIndex.row() && m_SelectedIndex.row() <= bottomRight.row())
                {
                  UpdateIssueInfo();
                }
              }),
    "signal/slot connection failed");

  W_VERIFY(connect(m_Model.data(), &QAbstractItemModel::modelReset, this,
              [this]()
              {
                m_SelectedIndex = QPersistentModelIndex();
                UpdateIssueInfo();
                UpdateIndirectIssueCount();
              }),
    "signal/slot connection failed");

  // An asset can become (or stop being) an indirect issue without ever entering the list, so the
  // model's own signals are not enough to keep the count current - listen to the curator instead.
  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtAssetCuratorPanel::AssetCuratorEventHandler, this));

  UpdateIndirectIssueCount();

  W_VERIFY(connect(ClearHistory, &QToolButton::clicked, ProcessorProgress, &WQtAssetProcessorProgressWidget::ClearHistory), "");
}

WQtAssetCuratorPanel::~WQtAssetCuratorPanel()
{
  WAssetProcessor::GetSingleton()->RemoveLogWriter(WMakeDelegate(&WQtAssetCuratorPanel::LogWriter, this));
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtAssetCuratorPanel::AssetCuratorEventHandler, this));
}

void WQtAssetCuratorPanel::AssetCuratorEventHandler(const WAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case WAssetCuratorEvent::Type::AssetAdded:
    case WAssetCuratorEvent::Type::AssetMoved:
    case WAssetCuratorEvent::Type::AssetRemoved:
    case WAssetCuratorEvent::Type::AssetUpdated:
    case WAssetCuratorEvent::Type::AssetListReset:
      ScheduleIndirectIssueCountUpdate();
      break;

    default:
      break;
  }
}

void WQtAssetCuratorPanel::ScheduleIndirectIssueCountUpdate()
{
  // Curator events arrive in bursts, so coalesce them - recounting walks all known assets.
  if (m_bIndirectCountScheduled)
    return;

  m_bIndirectCountScheduled = true;

  QTimer::singleShot(200, this, [this]()
    {
      m_bIndirectCountScheduled = false;
      UpdateIndirectIssueCount(); //
    });
}

void WQtAssetCuratorPanel::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (selected.isEmpty())
    m_SelectedIndex = QModelIndex();
  else
    m_SelectedIndex = selected.indexes()[0];

  UpdateIssueInfo();
}

void WQtAssetCuratorPanel::onListAssetsDoubleClicked(const QModelIndex& index)
{
  QString sAbsPath = m_Model->data(index, WQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

  WQtEditorApp::GetSingleton()->OpenDocumentQueued(sAbsPath.toUtf8().data());
}

QModelIndex WQtAssetCuratorPanel::GetContextMenuTarget() const
{
  return ListAssets->selectionModel()->currentIndex();
}

void WQtAssetCuratorPanel::onListAssetsContextMenuRequested(const QPoint& pos)
{
  // make the item under the cursor the current one, so that the menu always acts on what was clicked
  const QModelIndex clicked = ListAssets->indexAt(pos);
  if (clicked.isValid())
  {
    ListAssets->selectionModel()->setCurrentIndex(clicked, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }

  const QModelIndex index = GetContextMenuTarget();
  if (!index.isValid())
    return;

  QMenu menu;

  QAction* pOpen = menu.addAction(WMakeQString(WTranslate("AssetCurator.OpenDocument")));
  connect(pOpen, &QAction::triggered, this, [this]()
    {
      const QModelIndex idx = GetContextMenuTarget();
      if (idx.isValid())
        onListAssetsDoubleClicked(idx); //
    });

  QAction* pSelect = menu.addAction(WMakeQString(WTranslate("AssetCurator.SelectInAssetBrowser")));
  connect(pSelect, &QAction::triggered, this, [this]()
    {
      const QModelIndex idx = GetContextMenuTarget();
      if (!idx.isValid())
        return;

      const WUuid assetGuid = m_Model->data(idx, WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();
      if (!assetGuid.IsValid())
        return;

      WQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(assetGuid);
      WQtAssetBrowserPanel::GetSingleton()->EnsureVisible(); //
    });

  // the double click default action is 'open', so make that the bold default entry as well
  menu.setDefaultAction(pOpen);

  menu.exec(ListAssets->viewport()->mapToGlobal(pos));
}

void WQtAssetCuratorPanel::onCheckIndirectToggled(bool checked)
{
  m_pFilter->SetFilterTransitive(!checked);
  m_Model->resetModel();
  UpdateIndirectIssueCount();
}

void WQtAssetCuratorPanel::UpdateIndirectIssueCount()
{
  WUInt32 uiIndirect = 0;

  {
    WAssetCurator::WLockedSubAssetTable allAssetsLocked = WAssetCurator::GetSingleton()->GetKnownSubAssets();

    for (auto it : *allAssetsLocked)
    {
      const WSubAsset* pSubAsset = &it.Value();

      if (WQtAssetCuratorFilter::HasIssue(pSubAsset) && WQtAssetCuratorFilter::IsIndirectIssue(pSubAsset))
      {
        ++uiIndirect;
      }
    }
  }

  if (uiIndirect == 0)
  {
    CheckIndirect->setText("Show Indirect Issues");
  }
  else
  {
    WStringBuilder sText;
    sText.SetFormat(uiIndirect == 1 ? "Show {} Indirect Issue" : "Show {} Indirect Issues", uiIndirect);
    CheckIndirect->setText(WMakeQString(sText));
  }
}

void WQtAssetCuratorPanel::LogWriter(const WLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  WLogEntry msg(e);
  CuratorLog->GetLog()->AddLogMsg(msg);
}

void WQtAssetCuratorPanel::UpdateIssueInfo()
{
  if (!m_SelectedIndex.isValid())
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  WUuid assetGuid = m_Model->data(m_SelectedIndex, WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();
  auto pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (pSubAsset == nullptr)
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  TransformLog->GetLog()->Clear();

  WAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;

  auto getNiceName = [&pSubAsset](const WString& sDep) -> WStringBuilder
  {
    if (WConversionUtils::IsStringUuid(sDep))
    {
      WUuid guid = WConversionUtils::ConvertStringToUuid(sDep);
      auto assetInfoDep = WAssetCurator::GetSingleton()->GetSubAsset(guid);
      if (assetInfoDep)
      {
        return assetInfoDep->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      }

      WUInt64 uiLow;
      WUInt64 uiHigh;
      guid.GetValues(uiLow, uiHigh);

      WString sDocumentPath = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath();

      // Open the document (without requesting a window)
      WDocument* pDocument = WQtEditorApp::GetSingleton()->OpenDocument(sDocumentPath, WDocumentFlags::None);

      constexpr WUInt32 maxResults = 3;
      WTempHybridArray<WAssetDocument::AssetUsage, maxResults> uses;
      if (pDocument != nullptr)
      {
        // cast a document to WAssetDocument to access the FindAssetUsages function.
        if (WAssetDocument* pAssetDoc = WDynamicCast<WAssetDocument*>(pDocument))
        {
          // Find all direct uses of this asset
          pAssetDoc->FindAssetUsages(sDep, uses, maxResults);
        }
      }

      WStringBuilder sTmp;
      if (uses.IsEmpty())
      {
        sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);
      }
      else
      {
        WStringBuilder usesString;
        for (auto& use : uses)
        {
          if (!usesString.IsEmpty())
          {
            usesString.Append(", ");
          }
          usesString.Append("'", use.m_sObjectName, "'");
        }

        sTmp.SetFormat("{}. Used by objects: [[{}|asset:{}#filter:\"ref:{}\"]]", sDep, usesString, pSubAsset->m_pAssetInfo->m_Info->m_DocumentID, sDep);
      }

      return sTmp;
    }

    return sDep;
  };

  WLogEntryDelegate logger(([this](WLogEntry& ref_entry) -> void
    { TransformLog->GetLog()->AddLogMsg(std::move(ref_entry)); }));
  WStringBuilder text;
  if (pAssetInfo->m_TransformState == WAssetInfo::MissingTransformDependency)
  {
    WLog::Error(&logger, "Missing Transform Dependency:");
    for (const WString& dep : pAssetInfo->m_MissingTransformDeps)
    {
      WStringBuilder m_sNiceName = getNiceName(dep);
      WLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == WAssetInfo::CircularDependency)
  {
    WLog::Error(&logger, "Circular Dependency:");
    for (const WString& ref : pAssetInfo->m_CircularDependencies)
    {
      WStringBuilder m_sNiceName = getNiceName(ref);
      WLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == WAssetInfo::MissingThumbnailDependency)
  {
    WLog::Error(&logger, "Missing Thumbnail Dependency:");
    for (const WString& ref : pAssetInfo->m_MissingThumbnailDeps)
    {
      WStringBuilder m_sNiceName = getNiceName(ref);
      WLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == WAssetInfo::MissingPackageDependency)
  {
    WLog::Error(&logger, "Missing Package Dependency:");
    for (const WString& ref : pAssetInfo->m_MissingPackageDeps)
    {
      WStringBuilder m_sNiceName = getNiceName(ref);
      WLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == WAssetInfo::TransformError)
  {
    WLog::Error(&logger, "Transform Error:");
    for (const WLogEntry& logEntry : pAssetInfo->m_LogEntries)
    {
      TransformLog->GetLog()->AddLogMsg(logEntry);
    }
  }
}
