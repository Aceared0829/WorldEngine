#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QUrl>
#include <qdesktopservices.h>

WQtDataWidget* WQtDataWidget::s_pWidget = nullptr;

WQtDataWidget::WQtDataWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Data Transfer Widget", pParent)
{
  /// \todo Improve Data Transfer UI

  s_pWidget = this;

  setupUi(this);
  setWidget(DataTransferWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/database_refresh.svg"));

  ResetStats();
}

void WQtDataWidget::ResetStats()
{
  m_Transfers.Clear();
  ComboTransfers->clear();
  ComboItems->clear();
}

void WQtDataWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage msg;

  while (WTelemetry::RetrieveMessage('TRAN', msg) == W_SUCCESS)
  {
    if (msg.GetMessageID() == ' CLR')
    {
      s_pWidget->ResetStats();
    }

    if (msg.GetMessageID() == 'ENBL')
    {
      WString sName;
      msg.GetReader() >> sName;

      // this will create the item, do not remove!
      TransferData& td = s_pWidget->m_Transfers[sName];
      W_IGNORE_UNUSED(td);

      s_pWidget->ComboTransfers->addItem(sName.GetData());
    }

    if (msg.GetMessageID() == 'DSBL')
    {
      WString sName;
      msg.GetReader() >> sName;

      auto it = s_pWidget->m_Transfers.Find(sName);

      if (it.IsValid())
      {
        WInt32 iIndex = s_pWidget->ComboTransfers->findText(sName.GetData());

        if (iIndex >= 0)
          s_pWidget->ComboTransfers->removeItem(iIndex);
      }
    }

    if (msg.GetMessageID() == 'DATA')
    {
      WString sBelongsTo, sName, sMimeType, sExtension;

      msg.GetReader() >> sBelongsTo;
      msg.GetReader() >> sName;
      msg.GetReader() >> sMimeType;
      msg.GetReader() >> sExtension;

      auto Transfer = s_pWidget->m_Transfers.Find(sBelongsTo);

      if (Transfer.IsValid())
      {
        TransferDataObject& tdo = Transfer.Value().m_Items[sName];
        tdo.m_sMimeType = sMimeType;
        tdo.m_sExtension = sExtension;

        WMemoryStreamWriter Writer(&tdo.m_Storage);

        // copy the entire memory stream over and store it for later
        while (true)
        {
          WUInt8 uiTemp[1024];
          const WUInt64 uiRead = msg.GetReader().ReadBytes(uiTemp, 1024);

          if (uiRead == 0)
            break;

          Writer.WriteBytes(uiTemp, uiRead).IgnoreResult();
        }

        s_pWidget->on_ComboTransfers_currentIndexChanged(s_pWidget->ComboTransfers->currentIndex());
      }
    }
  }
}

void WQtDataWidget::on_ButtonRefresh_clicked()
{
  if (ComboTransfers->currentIndex() < 0)
    return;

  const WStringBuilder sName = ComboTransfers->currentText().toUtf8().data();

  auto it = m_Transfers.Find(sName);

  if (!it.IsValid())
    return;

  LabelImage->setPixmap(QPixmap());
  LabelImage->setText("Waiting for data transfer...");

  WTelemetryMessage msg;
  msg.SetMessageID('DTRA', ' REQ');
  msg.GetWriter() << it.Key();
  WTelemetry::SendToServer(msg);
}

void WQtDataWidget::on_ComboTransfers_currentIndexChanged(int index)
{
  ComboItems->clear();

  if (index < 0)
    return;

  WStringBuilder sName = ComboTransfers->currentText().toUtf8().data();

  auto itTransfer = m_Transfers.Find(sName);

  if (!itTransfer.IsValid())
    return;

  {
    WQtScopedUpdatesDisabled _1(ComboItems);

    for (auto itItem = itTransfer.Value().m_Items.GetIterator(); itItem.IsValid(); ++itItem)
    {
      ComboItems->addItem(itItem.Key().GetData());
    }

    ComboItems->setCurrentIndex(0);
  }

  on_ComboItems_currentIndexChanged(ComboItems->currentIndex());
}

WQtDataWidget::TransferDataObject* WQtDataWidget::GetCurrentItem()
{
  auto Transfer = GetCurrentTransfer();

  if (Transfer == nullptr)
    return nullptr;

  WString sItem = ComboItems->currentText().toUtf8().data();

  auto itItem = Transfer->m_Items.Find(sItem);
  if (!itItem.IsValid())
    return nullptr;

  return &itItem.Value();
}

WQtDataWidget::TransferData* WQtDataWidget::GetCurrentTransfer()
{
  WString sTransfer = ComboTransfers->currentText().toUtf8().data();

  auto itTransfer = m_Transfers.Find(sTransfer);
  if (!itTransfer.IsValid())
    return nullptr;

  return &itTransfer.Value();
}

void WQtDataWidget::on_ComboItems_currentIndexChanged(int index)
{
  if (index < 0)
    return;

  auto pItem = GetCurrentItem();

  if (!pItem)
    return;

  const WString sMime = pItem->m_sMimeType;
  auto& Stream = pItem->m_Storage;

  WMemoryStreamReader Reader(&Stream);

  if (sMime == "image/rgba8")
  {
    WUInt32 uiWidth, uiHeight;
    Reader >> uiWidth;
    Reader >> uiHeight;

    WDynamicArray<WUInt8> Image;
    Image.SetCountUninitialized(uiWidth * uiHeight * 4);

    Reader.ReadBytes(&Image[0], Image.GetCount());

    QImage i(&Image[0], uiWidth, uiHeight, QImage::Format_ARGB32);

    LabelImage->setPixmap(QPixmap::fromImage(i));
  }
  else if (sMime == "text/xml" || sMime == "application/json" || sMime == "text/plain")
  {
    const WUInt32 uiMaxBytes = WMath::Min<WUInt32>(1024 * 16, Reader.GetByteCount32());

    WTempHybridArray<WUInt8, 1024> Temp;
    Temp.SetCountUninitialized(uiMaxBytes + 1);

    Reader.ReadBytes(Temp.GetData(), uiMaxBytes);
    Temp[uiMaxBytes] = '\0';

    LabelImage->setText((const char*)Temp.GetData());
  }
  else
  {
    WStringBuilder sText;
    sText.SetFormat("Cannot display data of Mime-Type '{0}'", sMime);

    LabelImage->setText(sText.GetData());
  }
}

bool WQtDataWidget::SaveToFile(TransferDataObject& item, WStringView sFile)
{
  auto& Stream = item.m_Storage;
  WMemoryStreamReader Reader(&Stream);

  WStringBuilder tmp;
  QFile FileOut(sFile.GetData(tmp));
  if (!FileOut.open(QIODevice::WriteOnly))
  {
    QMessageBox::warning(this, QLatin1String("Error writing to file"), QLatin1String("Could not open the specified file for writing."), QMessageBox::Ok, QMessageBox::Ok);
    return false;
  }

  WTempHybridArray<WUInt8, 1024> Temp;
  Temp.SetCountUninitialized(Reader.GetByteCount32());

  Reader.ReadBytes(&Temp[0], Reader.GetByteCount32());

  if (!Temp.IsEmpty())
    FileOut.write((const char*)&Temp[0], Temp.GetCount());

  FileOut.close();
  return true;
}

void WQtDataWidget::on_ButtonSave_clicked()
{
  auto pItem = GetCurrentItem();

  if (!pItem)
  {
    QMessageBox::information(this, QLatin1String("WInspector"), QLatin1String("No valid item selected."), QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  QString sFilter;

  if (!pItem->m_sExtension.IsEmpty())
  {
    sFilter = "Default (*.";
    sFilter.append(pItem->m_sExtension.GetData());
    sFilter.append(");;");
  }

  sFilter.append("All Files (*.*)");

  QString sResult = QFileDialog::getSaveFileName(this, QLatin1String("Save Data"), pItem->m_sFileName.GetData(), sFilter);

  if (sResult.isEmpty())
    return;

  pItem->m_sFileName = sResult.toUtf8().data();

  SaveToFile(*pItem, pItem->m_sFileName.GetData());
}

void WQtDataWidget::on_ButtonOpen_clicked()
{
  auto pItem = GetCurrentItem();

  if (!pItem)
  {
    QMessageBox::information(this, QLatin1String("WInspector"), QLatin1String("No valid item selected."), QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  if (pItem->m_sFileName.IsEmpty())
    on_ButtonSave_clicked();

  if (pItem->m_sFileName.IsEmpty())
    return;

  SaveToFile(*pItem, pItem->m_sFileName.GetData());

  if (!QDesktopServices::openUrl(QUrl(pItem->m_sFileName.GetData())))
    QMessageBox::information(this, QLatin1String("WInspector"), QLatin1String("Could not open the file. There is probably no application registered to handle this file type."), QMessageBox::Ok, QMessageBox::Ok);
}
