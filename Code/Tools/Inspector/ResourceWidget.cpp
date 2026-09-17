#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <QComboBox>
#include <qgraphicsitem.h>

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDesktopServices>
#include <QFileDialog>

/// \todo Refcount ? (Max?)
/// \todo Select Resource -> send to App for preview

void FormatSize(WStringBuilder& s, WStringView sPrefix, WUInt64 uiSize);

WQtResourceWidget* WQtResourceWidget::s_pWidget = nullptr;

WQtResourceWidget::WQtResourceWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Resource Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(ResourceWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/Resources.svg"));

  m_bShowDeleted = true;

  ResetStats();
}

void WQtResourceWidget::ResetStats()
{
  m_Resources.Clear();

  m_bUpdateTable = true;
  m_bUpdateTypeBox = true;
  m_LastTableUpdate = WTime::MakeFromSeconds(0);

  Table->clear();
  Table->setRowCount(0);

  {
    QStringList Headers;
    Headers.append(" Resource Type ");
    Headers.append(" Priority ");
    Headers.append(" State ");
    Headers.append(" QL Disc. ");
    Headers.append(" QL Load. ");
    Headers.append(" CPU Mem. ");
    Headers.append(" GPU Mem. ");
    Headers.append(" Resource ID ");
    Headers.append(" Description ");

    Table->setColumnCount(static_cast<int>(Headers.size()));
    Table->setHorizontalHeaderLabels(Headers);
    Table->horizontalHeader()->show();
  }

  Table->resizeColumnsToContents();
  Table->sortByColumn(0, Qt::DescendingOrder);
  CheckShowDeleted->setChecked(m_bShowDeleted);
}


void WQtResourceWidget::UpdateStats()
{
  if (!m_bUpdateTable)
    return;

  UpdateTable();
}

class ByteSizeItem : public QTableWidgetItem
{
public:
  ByteSizeItem(WUInt32 uiBytes, const char* szString)
    : QTableWidgetItem(szString)
  {
    m_uiBytes = uiBytes;
  }

  bool operator<(const QTableWidgetItem& other) const { return m_uiBytes < ((ByteSizeItem&)other).m_uiBytes; }

  WUInt32 m_uiBytes;
};

void WQtResourceWidget::UpdateTable()
{
  if (!m_bUpdateTable)
    return;

  if (WTime::Now() - m_LastTableUpdate < WTime::MakeFromSeconds(0.25))
    return;

  bool bResizeFirstColumn = false;

  if (m_bUpdateTypeBox)
  {
    WQtScopedUpdatesDisabled _1(ComboResourceTypes);

    m_bUpdateTypeBox = false;

    if (ComboResourceTypes->currentIndex() == 0)
    {
      m_sTypeFilter.Clear();
    }
    else
    {
      m_sTypeFilter = ComboResourceTypes->currentText().toUtf8().data();
    }

    ComboResourceTypes->clear();
    ComboResourceTypes->addItem("All Resource Types");

    WUInt32 uiSelected = 0;
    for (auto it = m_ResourceTypes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Key() == m_sTypeFilter)
        uiSelected = ComboResourceTypes->count();

      ComboResourceTypes->addItem(QLatin1String(it.Key().GetData()));
    }

    ComboResourceTypes->setCurrentIndex(uiSelected);

    bResizeFirstColumn = true;
  }

  m_LastTableUpdate = WTime::Now();
  m_bUpdateTable = false;

  WQtScopedUpdatesDisabled _2(Table);

  Table->setSortingEnabled(false);

  WStringBuilder sTemp;

  for (auto it = m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    auto& res = it.Value();

    if (res.m_bUpdate)
    {
      res.m_bUpdate = false;

      bool bShowItem = true;

      if (!m_bShowDeleted && res.m_LoadingState.m_State == WResourceState::Invalid)
      {
        bShowItem = false;
      }
      else if (!m_sTypeFilter.IsEmpty() && res.m_sResourceType != m_sTypeFilter)
      {
        bShowItem = false;
      }
      else if (!m_sNameFilter.IsEmpty() && res.m_sResourceID.FindSubString_NoCase(m_sNameFilter) == nullptr &&
               res.m_sResourceDescription.FindSubString_NoCase(m_sNameFilter) == nullptr)
      {
        bShowItem = false;
      }

      if (!bShowItem)
      {
        if (res.m_pMainItem != nullptr)
        {
          Table->removeRow(Table->row(res.m_pMainItem));
          res.m_pMainItem = nullptr;
        }

        continue;
      }


      QTableWidgetItem* pItem;

      WInt32 iTableRow = -1;

      if (res.m_pMainItem == nullptr)
      {
        iTableRow = Table->rowCount();
        Table->insertRow(iTableRow);

        res.m_pMainItem = new QTableWidgetItem();
        Table->setItem(iTableRow, 0, res.m_pMainItem);
        Table->setItem(iTableRow, 1, new QTableWidgetItem());
        Table->setItem(iTableRow, 2, new QTableWidgetItem());
        Table->setItem(iTableRow, 3, new QTableWidgetItem());
        Table->setItem(iTableRow, 4, new QTableWidgetItem());
        Table->setItem(iTableRow, 5, new ByteSizeItem(0, ""));
        Table->setItem(iTableRow, 6, new ByteSizeItem(0, ""));
        Table->setItem(iTableRow, 7, new QTableWidgetItem());
        Table->setItem(iTableRow, 8, new QTableWidgetItem());
      }
      else
      {
        iTableRow = Table->row(res.m_pMainItem);
      }

      pItem = Table->item(iTableRow, 8);
      pItem->setText(res.m_sResourceDescription.GetData());

      pItem = Table->item(iTableRow, 7);
      pItem->setText(res.m_sResourceID.GetData());

      if (res.m_LoadingState.m_State == WResourceState::LoadedResourceMissing)
      {
        pItem->setIcon(QIcon(":/Icons/Icons/ResourceMissing.svg"));
        pItem->setToolTip("The resource could not be loaded.");
      }
      else if (!res.m_Flags.IsAnySet(WResourceFlags::IsReloadable))
      {
        pItem->setIcon(QIcon(":/Icons/Icons/ResourceCreated.svg"));
        pItem->setToolTip("Resource is not reloadable.");
      }
      else if (res.m_Flags.IsAnySet(WResourceFlags::ResourceHasFallback))
      {
        pItem->setIcon(QIcon(":/Icons/Icons/ResourceFallback.svg"));
        pItem->setToolTip("A fallback resource is specified.");
      }
      else
      {
        pItem->setIcon(QIcon(":/Icons/Icons/Resource.svg"));
        pItem->setToolTip("Resource is reloadable but no fallback is available.");
      }

      pItem = Table->item(iTableRow, 0);
      pItem->setText(res.m_sResourceType.GetData());

      pItem = Table->item(iTableRow, 1);
      pItem->setTextAlignment(Qt::AlignHCenter);

      switch (res.m_Priority)
      {
        case WResourcePriority::Critical:
          pItem->setText("Critical");
          pItem->setForeground(QColor::fromRgb(255, 0, 0));
          break;
        case WResourcePriority::VeryHigh:
          pItem->setText("Highest");
          pItem->setForeground(QColor::fromRgb(255, 106, 0));
          break;
        case WResourcePriority::High:
          pItem->setText("High");
          pItem->setForeground(QColor::fromRgb(255, 216, 0));
          break;
        case WResourcePriority::Medium:
          pItem->setText("Normal");
          pItem->setForeground(QColor::fromRgb(0, 148, 255));
          break;
        case WResourcePriority::Low:
          pItem->setText("Low");
          pItem->setForeground(QColor::fromRgb(127, 146, 255));
          break;
        case WResourcePriority::VeryLow:
          pItem->setText("Lowest");
          pItem->setForeground(QColor::fromRgb(127, 201, 255));
          break;
      }

      // if (res.m_Flags.IsAnySet(WResourceFlags::IsPreloading))
      //{
      //  pItem->setText("Preloading");
      //  pItem->setForeground(QColor::fromRgb(86, 255, 25));
      //}

      pItem = Table->item(iTableRow, 2);
      pItem->setTextAlignment(Qt::AlignHCenter);
      switch (res.m_LoadingState.m_State)
      {
        case WResourceState::Invalid:
          pItem->setText("Deleted");
          pItem->setForeground(QColor::fromRgb(128, 128, 128));
          break;
        case WResourceState::Unloaded:
          pItem->setText("Unloaded");
          pItem->setForeground(QColor::fromRgb(255, 216, 0));
          break;
        case WResourceState::Loaded:
          pItem->setText("Loaded");
          pItem->setForeground(QColor::fromRgb(182, 255, 0));
          break;
        case WResourceState::LoadedResourceMissing:
          pItem->setText("Missing");
          pItem->setForeground(QColor::fromRgb(255, 0, 0));
          break;
      }

      pItem = Table->item(iTableRow, 3);
      sTemp.SetFormat("{0}", res.m_LoadingState.m_uiQualityLevelsDiscardable);
      pItem->setText(sTemp.GetData());
      pItem->setToolTip("The number of quality levels that could be discarded to free up memory.");

      pItem = Table->item(iTableRow, 4);
      sTemp.SetFormat("{0}", res.m_LoadingState.m_uiQualityLevelsLoadable);
      pItem->setText(sTemp.GetData());
      pItem->setToolTip("The number of quality levels that could be additionally loaded for higher quality.");

      ByteSizeItem* pByteItem;

      pByteItem = (ByteSizeItem*)Table->item(iTableRow, 5);
      sTemp.SetFormat("{0} Bytes", res.m_Memory.m_uiMemoryCPU);
      pByteItem->setToolTip(sTemp.GetData());
      FormatSize(sTemp, "", res.m_Memory.m_uiMemoryCPU);
      pByteItem->setText(sTemp.GetData());
      pByteItem->m_uiBytes = static_cast<WUInt32>(res.m_Memory.m_uiMemoryCPU);

      pByteItem = (ByteSizeItem*)Table->item(iTableRow, 6);
      sTemp.SetFormat("{0} Bytes", res.m_Memory.m_uiMemoryGPU);
      pByteItem->setToolTip(sTemp.GetData());
      FormatSize(sTemp, "", res.m_Memory.m_uiMemoryGPU);
      pByteItem->setText(sTemp.GetData());
      pByteItem->m_uiBytes = static_cast<WUInt32>(res.m_Memory.m_uiMemoryGPU);

      if (res.m_LoadingState.m_State == WResourceState::Invalid)
      {
        Table->item(iTableRow, 7)->setIcon(QIcon(":/Icons/Icons/ResourceDeleted.svg"));
        // Table->item(iTableRow, 1)->setText(""); // Priority
        // Table->item(iTableRow, 3)->setText(""); // QL D
        // Table->item(iTableRow, 4)->setText(""); // QL L

        for (int i = 0; i < 8; ++i)
          Table->item(iTableRow, i)->setForeground(QColor::fromRgb(128, 128, 128));
      }
    }
  }

  if (bResizeFirstColumn)
  {
    Table->resizeColumnToContents(0);
  }

  Table->setSortingEnabled(true);
}

void WQtResourceWidget::UpdateAll()
{
  m_bUpdateTable = true;

  for (auto it = m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bUpdate = true;
  }
}

void WQtResourceWidget::on_LineFilterByName_textChanged()
{
  m_sNameFilter = LineFilterByName->text().toUtf8().data();

  UpdateAll();
}

void WQtResourceWidget::on_ComboResourceTypes_currentIndexChanged(int state)
{
  if (state == 0)
    m_sTypeFilter.Clear();
  else
    m_sTypeFilter = ComboResourceTypes->currentText().toUtf8().data();

  UpdateAll();
}

void WQtResourceWidget::on_CheckShowDeleted_toggled(bool checked)
{
  m_bShowDeleted = checked;
  UpdateAll();
}

static WStringView StateToString(WResourceState state)
{
  switch (state)
  {
    case WResourceState::Invalid:
      return "Deleted";
    case WResourceState::Unloaded:
      return "Unloaded";
    case WResourceState::Loaded:
      return "Loaded";
    case WResourceState::LoadedResourceMissing:
      return "Missing";
  }

  return "unknown";
}

static WStringView PriorityToString(WResourcePriority priority)
{
  switch (priority)
  {
    case WResourcePriority::Critical:
      return "Critical";
    case WResourcePriority::VeryHigh:
      return "Very High";
    case WResourcePriority::High:
      return "High";
    case WResourcePriority::Medium:
      return "Normal";
    case WResourcePriority::Low:
      return "Low";
    case WResourcePriority::VeryLow:
      return "Lowest";
  }

  return "unknown";
}

void WQtResourceWidget::on_ButtonSave_clicked()
{
  static QString sLastDir;

  QString sFile = QFileDialog::getSaveFileName(this, "Save Resource Table", sLastDir, "CSV (*.csv)\nAll Files (*.*)", nullptr);

  if (sFile.isEmpty())
    return;

  sLastDir = sFile;


  QFile file(sFile);
  if (!file.open(QIODevice::WriteOnly))
    return;

  WStringBuilder sLine;

  sLine = "sep=,\n";
  file.write(sLine);

  sLine = "Resource Type, Priority, State, CPU, GPU, Resource ID, Description\n";
  file.write(sLine);

  for (auto it = m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    auto& res = it.Value();

    if (!m_bShowDeleted && res.m_LoadingState.m_State == WResourceState::Invalid)
      continue;

    if (!m_sTypeFilter.IsEmpty() && res.m_sResourceType != m_sTypeFilter)
      continue;

    if (!m_sNameFilter.IsEmpty() && res.m_sResourceID.FindSubString_NoCase(m_sNameFilter) == nullptr &&
        res.m_sResourceDescription.FindSubString_NoCase(m_sNameFilter) == nullptr)
      continue;

    sLine.SetFormat("{}, {}, {}, {}, {}, {}, {}\n", res.m_sResourceType, PriorityToString(res.m_Priority), StateToString(res.m_LoadingState.m_State),
      res.m_Memory.m_uiMemoryCPU, res.m_Memory.m_uiMemoryGPU, res.m_sResourceID, res.m_sResourceDescription);
    file.write(sLine);
  }

  QDesktopServices::openUrl(QUrl::fromLocalFile(sFile));
}

void WQtResourceWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage Msg;

  while (WTelemetry::RetrieveMessage('RESM', Msg) == W_SUCCESS)
  {
    s_pWidget->m_bUpdateTable = true;

    WUInt64 uiResourceNameHash = 0;
    Msg.GetReader() >> uiResourceNameHash;

    ResourceData& rd = s_pWidget->m_Resources[uiResourceNameHash];
    rd.m_bUpdate = true;

    if (Msg.GetMessageID() == ' SET')
    {
      Msg.GetReader() >> rd.m_sResourceID;

      Msg.GetReader() >> rd.m_sResourceType;

      if (!s_pWidget->m_ResourceTypes.Contains(rd.m_sResourceType))
      {
        s_pWidget->m_bUpdateTypeBox = true;
        s_pWidget->m_ResourceTypes.Insert(rd.m_sResourceType);
      }

      WUInt8 uiPriority = 0;
      Msg.GetReader() >> uiPriority;
      rd.m_Priority = (WResourcePriority)uiPriority;

      WUInt8 uiFlags = 0;
      Msg.GetReader() >> uiFlags;
      rd.m_Flags.Clear();
      rd.m_Flags.Add((WResourceFlags::Enum)uiFlags);

      WUInt8 uiLoadingState = 0;
      Msg.GetReader() >> uiLoadingState;

      rd.m_LoadingState.m_State = (WResourceState)uiLoadingState;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsDiscardable;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsLoadable;

      Msg.GetReader() >> rd.m_Memory.m_uiMemoryCPU;
      Msg.GetReader() >> rd.m_Memory.m_uiMemoryGPU;
      Msg.GetReader() >> rd.m_sResourceDescription;
    }

    if (Msg.GetMessageID() == 'UPDT')
    {
      WUInt8 uiPriority = 0;
      Msg.GetReader() >> uiPriority;
      rd.m_Priority = (WResourcePriority)uiPriority;

      WUInt8 uiFlags = 0;
      Msg.GetReader() >> uiFlags;
      rd.m_Flags.Clear();
      rd.m_Flags.Add((WResourceFlags::Enum)uiFlags);

      WUInt8 uiLoadingState = 0;
      Msg.GetReader() >> uiLoadingState;

      rd.m_LoadingState.m_State = (WResourceState)uiLoadingState;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsDiscardable;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsLoadable;

      Msg.GetReader() >> rd.m_Memory.m_uiMemoryCPU;
      Msg.GetReader() >> rd.m_Memory.m_uiMemoryGPU;
    }

    if (Msg.GetMessageID() == ' DEL')
    {
      rd.m_Flags.Remove(WResourceFlags::IsQueuedForLoading);
      rd.m_LoadingState.m_State = WResourceState::Invalid;
    }
  }
}
