#include <EditorPluginFileserve/EditorPluginFileservePCH.h>

#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>
#include <EditorPluginFileserve/FileserveUI/AllFilesModel.moc.h>
#include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#include <Foundation/Utilities/CommandLineUtils.h>

WQtFileserveWidget::WQtFileserveWidget(QWidget* pParent /*= nullptr*/)
{
  setupUi(this);
  Progress->reset();
  m_pActivityModel = new WQtFileserveActivityModel(this);
  m_pAllFilesModel = new WQtFileserveAllFilesModel(this);

  ActivityList->setModel(m_pActivityModel);
  AllFilesList->setModel(m_pAllFilesModel);

  ActivityList->horizontalHeader()->setVisible(true);
  AllFilesList->horizontalHeader()->setVisible(true);

  {
    ClientsList->setColumnCount(3);

    QStringList header;
    header.append("");
    header.append("");
    header.append("");
    ClientsList->setHeaderLabels(header);
    ClientsList->setHeaderHidden(false);
  }

  {
    QHeaderView* verticalHeader = ActivityList->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(24);
  }

  {
    QHeaderView* verticalHeader = AllFilesList->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(24);
  }

  connect(m_pActivityModel, SIGNAL(rowsInserted(QModelIndex, int, int)), ActivityList, SLOT(scrollToBottom()));

  if (WFileserver::GetSingleton())
  {
    WFileserver::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtFileserveWidget::FileserverEventHandler, this));
    const WUInt16 uiPort = WFileserver::GetSingleton()->GetPort();

    PortLineEdit->setText(QString::number(uiPort));
  }
  else
  {
    setEnabled(false);
  }

  {
    WStringBuilder sDisplayString;
    FindOwnIP(sDisplayString);

    IpLabel->setText(sDisplayString.GetData());
  }

  ReloadResourcesButton->setEnabled(false);
  SpecialDirAddButton->setVisible(false);
  SpecialDirBrowseButton->setVisible(false);
  SpecialDirRemoveButton->setVisible(false);

  SpecialDirList->setToolTip("Special directories allow to redirect mount requests from the client to a specific folder on the server.\n\n"
                             "Some special directories are built in (e.g. 'sdk', 'user' and 'appdir') but you can add custom ones, if your app needs one.\n"
                             "To add special directories, run Fileserve with the command line argument '-specialdirs' followed by the name and the path to a "
                             "directory.\n\n"
                             "For instance:\n"
                             "-specialdirs project \"C:\\path\\to\\project\" secondDir \"d:\\another\\path\"");

  ConfigureSpecialDirectories();

  UpdateSpecialDirectoryUI();

  if (!WCommandLineUtils::GetGlobalInstance()->GetBoolOption("-fs_nostart"))
  {
    QTimer::singleShot(100, this, &WQtFileserveWidget::on_StartServerButton_clicked);
  }
}

void WQtFileserveWidget::FindOwnIP(WStringBuilder& out_sDisplay, WHybridArray<WStringBuilder, 4>* out_pAllIPs)
{
  WStringBuilder hardwarename;
  out_sDisplay.Clear();

  for (const QNetworkInterface& neti : QNetworkInterface::allInterfaces())
  {
    hardwarename = neti.humanReadableName().toUtf8().data();

    if (!neti.isValid())
      continue;
    if (neti.flags().testFlag(QNetworkInterface::IsLoopBack))
      continue;

    if (!neti.flags().testFlag(QNetworkInterface::IsUp))
      continue;
    if (!neti.flags().testFlag(QNetworkInterface::IsRunning))
      continue;
    if (!neti.flags().testFlag(QNetworkInterface::CanBroadcast))
      continue;

    for (const QNetworkAddressEntry& entry : neti.addressEntries())
    {
      if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
        continue;
      if (entry.ip().isLoopback())
        continue;
      if (entry.ip().isMulticast())
        continue;
      if (entry.ip().isNull())
        continue;

      // if we DO find multiple adapters, display them all
      if (!out_sDisplay.IsEmpty())
        out_sDisplay.Append("\n");

      out_sDisplay.AppendFormat("Adapter: '{0}' = {1}", hardwarename, entry.ip().toString().toUtf8().data());

      if (out_pAllIPs != nullptr)
      {
        out_pAllIPs->PushBack(entry.ip().toString().toUtf8().data());
      }
    }
  }
}

WQtFileserveWidget::~WQtFileserveWidget()
{
  if (WFileserver::GetSingleton())
  {
    WFileserver::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtFileserveWidget::FileserverEventHandler, this));
  }
}

void WQtFileserveWidget::on_StartServerButton_clicked()
{
  if (WFileserver::GetSingleton())
  {
    if (WFileserver::GetSingleton()->IsServerRunning())
    {
      if (QMessageBox::question(this, "Stop Server?", "Stop Server?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        return;

      WFileserver::GetSingleton()->StopServer();
    }
    else
    {
      QString sPort = PortLineEdit->text();
      bool bOk = false;
      WUInt32 uiPort = sPort.toUInt(&bOk);

      if (bOk && uiPort <= 0xFFFF)
      {
        WFileserver::GetSingleton()->SetPort((WUInt16)uiPort);
        WFileserver::GetSingleton()->StartServer();
      }
      else
        QMessageBox::information(this, "Invalid Port", "The port must be a number between 0 and 65535", QMessageBox::Ok, QMessageBox::Ok);
    }
  }
}

void WQtFileserveWidget::on_ClearActivityButton_clicked()
{
  m_pActivityModel->Clear();
}

void WQtFileserveWidget::on_ClearAllFilesButton_clicked()
{
  m_pAllFilesModel->Clear();
}


void WQtFileserveWidget::on_ReloadResourcesButton_clicked()
{
  if (WFileserver::GetSingleton())
  {
    WFileserver::GetSingleton()->BroadcastReloadResourcesCommand();
  }
}

void WQtFileserveWidget::on_ConnectClient_clicked()
{
  QString sIP;

  {
    QSettings Settings;
    Settings.beginGroup(QLatin1String("Fileserve"));
    sIP = Settings.value("ConnectClientIP", "").toString();
    Settings.endGroup();
  }

  bool ok = false;
  sIP = QInputDialog::getText(this, "Connect to Device", "Device IP:", QLineEdit::Normal, sIP, &ok);
  if (!ok)
    return;

  {
    QSettings Settings;
    Settings.beginGroup(QLatin1String("Fileserve"));
    Settings.setValue("ConnectClientIP", sIP);
    Settings.endGroup();
  }

  WStringBuilder sDisplayString;
  WTempHybridArray<WStringBuilder, 4> AllIPs;
  FindOwnIP(sDisplayString, &AllIPs);

  if (WFileserver::SendConnectionInfo(sIP.toUtf8().data(), PortLineEdit->text().toInt(), AllIPs).Succeeded())
  {
    LogActivity(WFmt("Successfully sent server info to client at '{0}'", sIP.toUtf8().data()), WFileserveActivityType::Other);
  }
  else
  {
    LogActivity(WFmt("Failed to connect with client at '{0}'", sIP.toUtf8().data()), WFileserveActivityType::Other);
  }
}

void WQtFileserveWidget::FileserverEventHandler(const WFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case WFileserverEvent::Type::None:
      W_ASSERT_DEV(false, "None event should never be fired");
      break;

    case WFileserverEvent::Type::ServerStarted:
    {
      LogActivity("", WFileserveActivityType::StartServer);
      PortLineEdit->setEnabled(false);
      ReloadResourcesButton->setEnabled(true);
      StartServerButton->setText("Stop Server");

      WStringBuilder sDisplayString;
      FindOwnIP(sDisplayString);

      Q_EMIT ServerStarted(sDisplayString.GetData(), WFileserver::GetSingleton()->GetPort());
    }
    break;

    case WFileserverEvent::Type::ServerStopped:
    {
      LogActivity("", WFileserveActivityType::StopServer);
      PortLineEdit->setEnabled(true);
      ReloadResourcesButton->setEnabled(false);
      StartServerButton->setText("Start Server");

      Q_EMIT ServerStopped();
    }
    break;

    case WFileserverEvent::Type::ClientConnected:
    {
      LogActivity("", WFileserveActivityType::ClientConnect);
      m_Clients[e.m_uiClientID].m_bConnected = true;

      UpdateClientList();
    }
    break;

    case WFileserverEvent::Type::ClientReconnected:
    {
      LogActivity("", WFileserveActivityType::ClientReconnected);
      m_Clients[e.m_uiClientID].m_bConnected = true;

      UpdateClientList();
    }
    break;

    case WFileserverEvent::Type::ClientDisconnected:
    {
      LogActivity("", WFileserveActivityType::ClientDisconnect);
      m_Clients[e.m_uiClientID].m_bConnected = false;

      UpdateClientList();
    }
    break;

    case WFileserverEvent::Type::MountDataDir:
    {
      LogActivity(e.m_szPath, WFileserveActivityType::Mount);

      DataDirInfo& dd = m_Clients[e.m_uiClientID].m_DataDirs.ExpandAndGetRef();
      dd.m_sName = e.m_szName;
      dd.m_sPath = e.m_szPath;
      dd.m_sRedirectedPath = e.m_szRedirectedPath;

      UpdateClientList();
    }
    break;

    case WFileserverEvent::Type::MountDataDirFailed:
    {
      LogActivity(e.m_szPath, WFileserveActivityType::MountFailed);

      DataDirInfo& dd = m_Clients[e.m_uiClientID].m_DataDirs.ExpandAndGetRef();
      dd.m_sName = e.m_szName;
      dd.m_sPath = e.m_szPath;
      dd.m_sRedirectedPath = "Failed to mount this directory. Special directory name unknown.";

      UpdateClientList();
    }
    break;

    case WFileserverEvent::Type::UnmountDataDir:
    {
      LogActivity(e.m_szName, WFileserveActivityType::Unmount);

      auto& dds = m_Clients[e.m_uiClientID].m_DataDirs;
      for (WUInt32 i = 0; i < dds.GetCount(); ++i)
      {
        if (dds[i].m_sName == e.m_szName)
        {
          dds.RemoveAtAndCopy(i);
          break;
        }
      }

      UpdateClientList();
    }
    break;

    case WFileserverEvent::Type::FileDownloadRequest:
    {
      m_pAllFilesModel->AddAccessedFile(e.m_szPath);
      TransferLabel->setText(QString("Downloading: %1").arg(e.m_szPath));
      m_LastProgressUpdate = WTime::Now();

      if (e.m_FileState == WFileserveFileState::NonExistant)
        LogActivity(WFmt("[N/A] {0}", e.m_szPath), WFileserveActivityType::ReadFile);

      if (e.m_FileState == WFileserveFileState::SameHash)
        LogActivity(WFmt("[HASH] {0}", e.m_szPath), WFileserveActivityType::ReadFile);

      if (e.m_FileState == WFileserveFileState::SameTimestamp)
        LogActivity(WFmt("[TIME] {0}", e.m_szPath), WFileserveActivityType::ReadFile);

      if (e.m_FileState == WFileserveFileState::NonExistantEither)
        LogActivity(WFmt("[N/A] {0}", e.m_szPath), WFileserveActivityType::ReadFile);

      if (e.m_FileState == WFileserveFileState::Different)
        LogActivity(WFmt("({1} KB) {0}", e.m_szPath, WArgF(e.m_uiSizeTotal / 1024.0f, 1)), WFileserveActivityType::ReadFile);
    }
    break;

    case WFileserverEvent::Type::FileDownloading:
    {
      if (WTime::Now() - m_LastProgressUpdate > WTime::MakeFromMilliseconds(100))
      {
        m_LastProgressUpdate = WTime::Now();
        Progress->setValue((int)(100.0 * e.m_uiSentTotal / e.m_uiSizeTotal));
      }
    }
    break;

    case WFileserverEvent::Type::FileDownloadFinished:
    {
      TransferLabel->setText(QString());
      Progress->reset();
    }
    break;

    case WFileserverEvent::Type::FileDeleteRequest:
      LogActivity(e.m_szPath, WFileserveActivityType::DeleteFile);
      break;

    case WFileserverEvent::Type::FileUploadRequest:
    {
      LogActivity(WFmt("({1} KB) {0}", e.m_szPath, WArgF(e.m_uiSizeTotal / 1024.0f, 1)), WFileserveActivityType::WriteFile);
      TransferLabel->setText(QString("Uploading: %1").arg(e.m_szPath));
      m_LastProgressUpdate = WTime::Now();
    }
    break;

    case WFileserverEvent::Type::FileUploading:
    {
      if (WTime::Now() - m_LastProgressUpdate > WTime::MakeFromMilliseconds(100))
      {
        m_LastProgressUpdate = WTime::Now();
        Progress->setValue((int)(100.0 * e.m_uiSentTotal / e.m_uiSizeTotal));
      }
    }
    break;

    case WFileserverEvent::Type::FileUploadFinished:
    {
      TransferLabel->setText(QString());
      Progress->reset();
    }
    break;

    case WFileserverEvent::Type::AreYouThereRequest:
    {
      LogActivity("Client searching for Server", WFileserveActivityType::Other);
    }
    break;

    case WFileserverEvent::Type::LogCustomActivity:
    {
      LogActivity(e.m_szName, WFileserveActivityType::Other);
    }
    break;
  }
}

void WQtFileserveWidget::LogActivity(const WFormatString& text, WFileserveActivityType type)
{
  auto& item = m_pActivityModel->AppendItem();

  WStringBuilder tmp;
  item.m_Text = text.GetTextCStr(tmp);
  item.m_Type = type;
}

void WQtFileserveWidget::UpdateSpecialDirectoryUI()
{
  QTableWidget* pTable = SpecialDirList;
  WQtScopedBlockSignals bs(SpecialDirList);

  QStringList header;
  header.append("Special Directory");
  header.append("Path");

  SpecialDirList->clear();
  pTable->setColumnCount(2);
  pTable->setRowCount(m_SpecialDirectories.GetCount() + 3);
  pTable->horizontalHeader()->setStretchLastSection(true);
  pTable->verticalHeader()->setDefaultSectionSize(24);
  pTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  pTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  pTable->verticalHeader()->setHidden(true);
  pTable->setHorizontalHeaderLabels(header);

  WStringBuilder sResolved;
  QTableWidgetItem* pItem;

  WUInt32 row = 0;

  for (WUInt32 i = 0; i < m_SpecialDirectories.GetCount(); ++i, ++row)
  {
    pItem = new QTableWidgetItem();
    pItem->setText(m_SpecialDirectories[i].m_sName.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(m_SpecialDirectories[i].m_sPath.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);
  }

  {
    WFileSystem::ResolveSpecialDirectory(">sdk", sResolved).IgnoreResult();

    pItem = new QTableWidgetItem();
    pItem->setText("sdk");
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(sResolved.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);

    ++row;
  }

  {
    WFileSystem::ResolveSpecialDirectory(">user", sResolved).IgnoreResult();

    pItem = new QTableWidgetItem();
    pItem->setText("user");
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(sResolved.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);

    ++row;
  }

  {
    WFileSystem::ResolveSpecialDirectory(">appdir", sResolved).IgnoreResult();

    pItem = new QTableWidgetItem();
    pItem->setText("appdir");
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(sResolved.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);

    ++row;
  }
}

void WQtFileserveWidget::UpdateClientList()
{
  WQtScopedBlockSignals bs(ClientsList);

  ClientsList->clear();

  WStringBuilder sName;

  for (auto it = m_Clients.GetIterator(); it.IsValid(); ++it)
  {
    QTreeWidgetItem* pClient = new QTreeWidgetItem();

    sName.SetFormat("Client: {0}", it.Key());
    pClient->setText(0, sName.GetData());
    pClient->setText(1, it.Value().m_bConnected ? "connected" : "disconnected");

    ClientsList->addTopLevelItem(pClient);

    for (const DataDirInfo& dd : it.Value().m_DataDirs)
    {
      QTreeWidgetItem* pDir = new QTreeWidgetItem(pClient);

      sName = dd.m_sName.GetData();
      if (!sName.IsEmpty())
        sName.Prepend(":");

      pDir->setText(0, sName.GetData());
      pDir->setText(1, dd.m_sPath.GetData());
      pDir->setText(2, dd.m_sRedirectedPath.GetData());

      if (dd.m_sRedirectedPath.StartsWith("Failed"))
        pDir->setForeground(2, QColor::fromRgb(255, 0, 0));
    }

    pClient->setExpanded(it.Value().m_bConnected);
  }

  ClientsList->resizeColumnToContents(2);
  ClientsList->resizeColumnToContents(1);
  ClientsList->resizeColumnToContents(0);
}

void WQtFileserveWidget::ConfigureSpecialDirectories()
{
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();
  const WUInt32 uiArgs = pCmd->GetStringOptionArguments("-specialdirs");

  WStringBuilder sDir, sPath;

  for (WUInt32 i = 0; i < uiArgs; i += 2)
  {
    sDir = pCmd->GetStringOption("-specialdirs", i, "");
    sPath = pCmd->GetStringOption("-specialdirs", i + 1, "");

    if (sDir.IsEmpty() || sPath.IsEmpty())
      continue;

    WFileSystem::SetSpecialDirectory(sDir, sPath);
    sPath.MakeCleanPath();

    auto& sd = m_SpecialDirectories.ExpandAndGetRef();
    sd.m_sName = sDir;
    sd.m_sPath = sPath;
  }
}
