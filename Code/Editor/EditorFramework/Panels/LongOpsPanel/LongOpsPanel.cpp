#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>


W_IMPLEMENT_SINGLETON(WQtLongOpsPanel);

constexpr int COL_DOCUMENT = 0;
constexpr int COL_OPERATION = 1;
constexpr int COL_PROGRESS = 2;
constexpr int COL_DURATION = 3;
constexpr int COL_BUTTON = 4;

WQtLongOpsPanel ::WQtLongOpsPanel(ads::CDockManager* pDockManager)
  : WQtApplicationPanel(pDockManager, "Panel.LongOps")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setWidget(pDummy);
  setIcon(WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Background.svg"));
  setWindowTitle(WMakeQString(WTranslate("Panel.LongOps")));

  // setup table
  {
    QStringList header;
    header.push_back("Document");
    header.push_back("Operation");
    header.push_back("Progress");
    header.push_back("Duration");
    header.push_back(""); // Start / Cancel button

    OperationsTable->setColumnCount(header.size());
    OperationsTable->setHorizontalHeaderLabels(header);

    OperationsTable->horizontalHeader()->setStretchLastSection(false);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_DOCUMENT, QHeaderView::ResizeMode::ResizeToContents);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_OPERATION, QHeaderView::ResizeMode::ResizeToContents);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_PROGRESS, QHeaderView::ResizeMode::Stretch);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_DURATION, QHeaderView::ResizeMode::Fixed);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_BUTTON, QHeaderView::ResizeMode::Fixed);

    connect(OperationsTable, &QTableWidget::cellDoubleClicked, this, &WQtLongOpsPanel::OnCellDoubleClicked);
  }

  WLongOpControllerManager::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtLongOpsPanel::LongOpsEventHandler, this));

  RebuildTable();
}

WQtLongOpsPanel::~WQtLongOpsPanel()
{
  WLongOpControllerManager::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtLongOpsPanel::LongOpsEventHandler, this));
}

void WQtLongOpsPanel::LongOpsEventHandler(const WLongOpControllerEvent& e)
{
  if (e.m_Type == WLongOpControllerEvent::Type::OpProgress)
  {
    m_bUpdateTable = true;
  }
  else
  {
    m_bRebuildTable = true;
  }

  QMetaObject::invokeMethod(this, "StartUpdateTimer", Qt::ConnectionType::QueuedConnection);
}

void WQtLongOpsPanel::RebuildTable()
{
  auto* opMan = WLongOpControllerManager::GetSingleton();
  W_LOCK(opMan->m_Mutex);

  m_bRebuildTable = false;
  m_bUpdateTable = false;

  WQtScopedBlockSignals _1(OperationsTable);

  OperationsTable->setRowCount(0);
  m_LongOpGuidToRow.Clear();

  const auto& opsList = opMan->GetOperations();
  for (WUInt32 idx = 0; idx < opsList.GetCount(); ++idx)
  {
    const auto& opInfo = *opsList[idx];
    const int rowIdx = OperationsTable->rowCount();
    OperationsTable->setRowCount(rowIdx + 1);

    // document name
    {
      WStringBuilder docName = WPathUtils::GetFileName(WDocumentManager::GetDocumentByGuid(opInfo.m_DocumentGuid)->GetDocumentPath());

      OperationsTable->setItem(rowIdx, COL_DOCUMENT, new QTableWidgetItem(docName.GetData()));
    }

    // operation name
    {
      OperationsTable->setItem(rowIdx, COL_OPERATION, new QTableWidgetItem(opInfo.m_pProxyOp->GetDisplayName()));
    }

    // progress bar
    {
      QProgressBar* pProgress = new QProgressBar();
      pProgress->setValue((int)(opInfo.m_fCompletion * 100.0f));
      OperationsTable->setCellWidget(rowIdx, COL_PROGRESS, pProgress);
    }

    // duration
    {
      WTime duration = opInfo.m_StartOrDuration;

      if (opInfo.m_bIsRunning)
        duration = WTime::Now() - opInfo.m_StartOrDuration;

      OperationsTable->setItem(rowIdx, COL_DURATION, new QTableWidgetItem(QString("%1 sec").arg(duration.GetSeconds())));
    }

    // button
    {
      QPushButton* pButton = new QPushButton(opInfo.m_bIsRunning ? "Cancel" : "Start");
      pButton->setProperty("opGuid", QVariant::fromValue(opInfo.m_OperationGuid));

      OperationsTable->setCellWidget(rowIdx, COL_BUTTON, pButton);
      connect(pButton, &QPushButton::clicked, this, &WQtLongOpsPanel::OnClickButton);
    }

    m_LongOpGuidToRow[opInfo.m_OperationGuid] = rowIdx;
  }
}

void WQtLongOpsPanel::UpdateTable()
{
  auto* opMan = WLongOpControllerManager::GetSingleton();
  W_LOCK(opMan->m_Mutex);

  m_bUpdateTable = false;

  const auto& opsList = opMan->GetOperations();
  for (WUInt32 idx = 0; idx < opsList.GetCount(); ++idx)
  {
    const auto& pOpInfo = opsList[idx];

    WUInt32 rowIdx;
    if (!m_LongOpGuidToRow.TryGetValue(pOpInfo->m_OperationGuid, rowIdx))
      continue;

    // progress
    {
      QProgressBar* pProgress = qobject_cast<QProgressBar*>(OperationsTable->cellWidget(rowIdx, COL_PROGRESS));
      pProgress->setValue((int)(pOpInfo->m_fCompletion * 100.0f));
    }

    // button
    {
      QPushButton* pButton = qobject_cast<QPushButton*>(OperationsTable->cellWidget(rowIdx, COL_BUTTON));
      pButton->setText(pOpInfo->m_bIsRunning ? "Cancel" : "Start");
    }

    // duration
    {
      WTime duration = pOpInfo->m_StartOrDuration;

      if (pOpInfo->m_bIsRunning)
        duration = WTime::Now() - pOpInfo->m_StartOrDuration;

      OperationsTable->setItem(rowIdx, COL_DURATION, new QTableWidgetItem(QString("%1 sec").arg(duration.GetSeconds())));
    }
  }
}

void WQtLongOpsPanel::StartUpdateTimer()
{
  if (m_bUpdateTimerRunning)
    return;

  m_bUpdateTimerRunning = true;
  QTimer::singleShot(50, this, SLOT(UpdateUI()));
}

void WQtLongOpsPanel::UpdateUI()
{
  m_bUpdateTimerRunning = false;

  if (m_bRebuildTable)
  {
    RebuildTable();
  }

  if (m_bUpdateTable)
  {
    UpdateTable();
  }
}

void WQtLongOpsPanel::OnClickButton(bool)
{
  auto* opMan = WLongOpControllerManager::GetSingleton();
  W_LOCK(opMan->m_Mutex);

  QPushButton* pButton = qobject_cast<QPushButton*>(sender());
  const WUuid opGuid = pButton->property("opGuid").value<WUuid>();

  if (pButton->text() == "Cancel")
    opMan->CancelOperation(opGuid);
  else
    opMan->StartOperation(opGuid);
}

void WQtLongOpsPanel::OnCellDoubleClicked(int row, int column)
{
  QPushButton* pButton = qobject_cast<QPushButton*>(OperationsTable->cellWidget(row, COL_BUTTON));
  const WUuid opGuid = pButton->property("opGuid").value<WUuid>();

  auto* opMan = WLongOpControllerManager::GetSingleton();
  W_LOCK(opMan->m_Mutex);

  auto opInfoPtr = opMan->GetOperation(opGuid);
  if (opInfoPtr == nullptr)
    return;

  WDocument* pDoc = WDocumentManager::GetDocumentByGuid(opInfoPtr->m_DocumentGuid);
  pDoc->EnsureVisible();

  WDocumentObject* pObj = pDoc->GetObjectManager()->GetObject(opInfoPtr->m_ComponentGuid);

  pDoc->GetSelectionManager()->SetSelection(pObj->GetParent());
}
