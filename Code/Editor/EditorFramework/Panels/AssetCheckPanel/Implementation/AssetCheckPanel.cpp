#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetChecker.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Panels/AssetCheckPanel/AssetCheckPanel.moc.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QPushButton>

W_IMPLEMENT_SINGLETON(WQtAssetCheckPanel);

WQtAssetCheckPanel::WQtAssetCheckPanel(ads::CDockManager* pDockManager)
  : WQtApplicationPanel(pDockManager, "Panel.AssetCheck")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setIcon(WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Checklist.svg"));
  setWindowTitle(WMakeQString(WTranslate("Panel.AssetCheck")));
  setWidget(pDummy);

  MainSplitter->setStretchFactor(0, 1);
  MainSplitter->setStretchFactor(1, 2);

  // The panel has no real geometry yet at construction time (ADS assigns it later), so setSizes() here would be
  // clamped to a 0-size splitter and lost. Apply the ratio on the splitter's first real resize instead.
  MainSplitter->installEventFilter(this);

  connect(RunButton, &QPushButton::clicked, this, &WQtAssetCheckPanel::RunButtonClicked);
  connect(ResultTree, &QTreeWidget::itemDoubleClicked, this, &WQtAssetCheckPanel::ResultTreeItemDoubleClicked);

  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WQtAssetCheckPanel::DocumentManagerEventHandler, this));

  UpdateAssetTypeCombo();
}

WQtAssetCheckPanel::~WQtAssetCheckPanel()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WQtAssetCheckPanel::DocumentManagerEventHandler, this));

  WAssetCheckRule::DestroyRules(m_Rules);
}

bool WQtAssetCheckPanel::eventFilter(QObject* pWatched, QEvent* pEvent)
{
  if (pWatched == MainSplitter && pEvent->type() == QEvent::Resize)
  {
    const int iTotal = MainSplitter->orientation() == Qt::Horizontal ? MainSplitter->width() : MainSplitter->height();
    if (iTotal > 0)
    {
      MainSplitter->setSizes({iTotal / 3, iTotal - (iTotal / 3)});
      MainSplitter->removeEventFilter(this);
    }
  }

  return WQtApplicationPanel::eventFilter(pWatched, pEvent);
}

void WQtAssetCheckPanel::DocumentManagerEventHandler(const WDocumentManager::Event& e)
{
  if (e.m_Type == WDocumentManager::Event::Type::DocumentTypesAdded || e.m_Type == WDocumentManager::Event::Type::DocumentTypesRemoved)
  {
    UpdateAssetTypeCombo();
  }
}

void WQtAssetCheckPanel::UpdateAssetTypeCombo()
{
  // Remember the previous selection (by asset type name) so a plugin (re-)load doesn't reset the user's choice.
  const QString sPreviouslySelected = AssetTypeCombo->currentData().toString();

  AssetTypeCombo->clear();
  AssetTypeCombo->addItem("<All Asset Types>", QString());

  WSet<WString> addedTypes;
  for (auto it : WDocumentManager::GetAllDocumentDescriptors())
  {
    const WDocumentTypeDescriptor* pDesc = it.Value();
    if (WDynamicCast<WAssetDocumentManager*>(pDesc->m_pManager) == nullptr)
      continue;

    if (addedTypes.Contains(pDesc->m_sDocumentTypeName))
      continue;

    addedTypes.Insert(pDesc->m_sDocumentTypeName);

    const QString sTypeName = QString::fromUtf8(pDesc->m_sDocumentTypeName.GetData());
    AssetTypeCombo->addItem(sTypeName, sTypeName);
  }

  if (const int iIndex = AssetTypeCombo->findData(sPreviouslySelected); iIndex >= 0)
    AssetTypeCombo->setCurrentIndex(iIndex);
}

void WQtAssetCheckPanel::FillRuleList()
{
  m_Rules.Clear();
  WAssetCheckRule::CreateRules(m_Rules);

  RuleList->clear();

  for (WUInt32 i = 0; i < m_Rules.GetCount(); ++i)
  {
    WAssetCheckRule* pRule = m_Rules[i];

    const WStringBuilder sText = pRule->GetDisplayName();

    QListWidgetItem* pItem = new QListWidgetItem(QString::fromUtf8(sText.GetData()), RuleList);
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    pItem->setCheckState(Qt::Checked);

    WStringBuilder sTooltip(pRule->GetDescription());

    if (pRule->CanFix())
    {
      pItem->setForeground(WToQtColor(WColorScheme::LightUI(WColorScheme::Green)));
      sTooltip.Append("\n\nThis rule supports auto-fix.");
    }

    pItem->setToolTip(QString::fromUtf8(sTooltip.GetData()));
    pItem->setData(Qt::UserRole, i);
  }
}

void WQtAssetCheckPanel::RunButtonClicked()
{
  WAssetCheckOptions options;

  const QVariant typeData = AssetTypeCombo->currentData();
  options.m_sDocumentTypeName = typeData.toString().toUtf8().data();
  options.m_sNameFilter = NameFilterEdit->text().toUtf8().data();
  options.m_bAutoFix = AutoFixCheck->isChecked();

  for (int i = 0; i < RuleList->count(); ++i)
  {
    QListWidgetItem* pItem = RuleList->item(i);
    if (pItem->checkState() != Qt::Checked)
      continue;

    const WUInt32 uiRuleIndex = pItem->data(Qt::UserRole).toUInt();
    options.m_Rules.PushBack(m_Rules[uiRuleIndex]);
  }

  if (options.m_Rules.IsEmpty())
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("Select at least one rule to run.");
    return;
  }

  WAssetCheckSummary summary;
  {
    // The modal progress dialog appears automatically via the global WProgress.
    WAssetChecker::Run(options, summary);
  }

  // Fill the results tree.
  ResultTree->clear();

  const QIcon errorIcon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/Error.svg");
  const QIcon warningIcon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/Warning.svg");

  for (const WAssetCheckResult& result : summary.m_Results)
  {
    WUInt32 uiErrors = 0, uiWarnings = 0;
    for (const WAssetCheckNote& note : result.m_Notes)
    {
      if (note.m_Severity == WAssetCheckSeverity::Error)
        ++uiErrors;
      else
        ++uiWarnings;
    }

    WStringBuilder sTitle;
    sTitle.SetFormat("{}  ({} errors, {} warnings)", result.m_sAssetPath, uiErrors, uiWarnings);

    // Link target understood by WQtUiServices::GotoLinkTarget: "asset:<assetGuid>[#<objectGuid>]".
    WStringBuilder sAssetGuid;
    WConversionUtils::ToString(result.m_AssetGuid, sAssetGuid);
    WStringBuilder sAssetLink("asset:", sAssetGuid);

    QTreeWidgetItem* pTop = new QTreeWidgetItem(ResultTree);
    pTop->setText(0, QString::fromUtf8(sTitle.GetData()));
    pTop->setIcon(0, uiErrors > 0 ? errorIcon : warningIcon);
    pTop->setData(0, Qt::UserRole, QString::fromUtf8(sAssetLink.GetData()));

    for (const WAssetCheckNote& note : result.m_Notes)
    {
      WStringBuilder sNote = note.m_sMessage;
      if (note.m_bFixed)
        sNote.Append(" (fixed)");

      QTreeWidgetItem* pChild = new QTreeWidgetItem(pTop);
      pChild->setText(0, QString::fromUtf8(sNote.GetData()));
      pChild->setIcon(0, note.m_Severity == WAssetCheckSeverity::Error ? errorIcon : warningIcon);

      if (note.m_bFixed)
        pChild->setForeground(0, WToQtColor(WColorScheme::LightUI(WColorScheme::Green)));

      // Append the object GUID so a double-click selects that object in the opened document.
      WStringBuilder sNoteLink = sAssetLink;
      if (note.m_ObjectGuid.IsValid())
      {
        WStringBuilder sObjGuid;
        WConversionUtils::ToString(note.m_ObjectGuid, sObjGuid);
        sNoteLink.Append("#", sObjGuid);
      }
      pChild->setData(0, Qt::UserRole, QString::fromUtf8(sNoteLink.GetData()));
    }

    if (uiErrors > 0)
      pTop->setExpanded(true);
  }

  WStringBuilder sStatus;
  if (summary.m_Results.IsEmpty())
  {
    sStatus = "No issues found.";
  }
  else
  {
    sStatus.SetFormat("Checked {} assets: {} errors, {} warnings, {} auto-fixed, {} documents saved.",
      summary.m_uiAssetsChecked, summary.m_uiErrors, summary.m_uiWarnings, summary.m_uiFixed, summary.m_uiSaved);
  }

  if (summary.m_bCanceled)
    sStatus.Append(" (Check was canceled; results are partial.)");

  StatusLabel->setText(QString::fromUtf8(sStatus.GetData()));
}

void WQtAssetCheckPanel::ResultTreeItemDoubleClicked(QTreeWidgetItem* pItem, int iColumn)
{
  if (pItem == nullptr)
    return;

  // Opens the asset and, if the link contains an object GUID, selects that object.
  const QString sLinkTarget = pItem->data(0, Qt::UserRole).toString();
  if (sLinkTarget.isEmpty())
    return;

  WQtUiServices::GotoLinkTarget(sLinkTarget.toUtf8().data());
}
