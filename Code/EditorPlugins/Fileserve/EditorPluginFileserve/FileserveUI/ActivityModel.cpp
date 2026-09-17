#include <EditorPluginFileserve/EditorPluginFileservePCH.h>

#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>

WQtFileserveActivityModel::WQtFileserveActivityModel(QWidget* pParent)
  : QAbstractListModel(pParent)
{
}

int WQtFileserveActivityModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return m_Items.GetCount() - m_uiAddedItems;
}

int WQtFileserveActivityModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant WQtFileserveActivityModel::data(const QModelIndex& index, int iRole /*= Qt::DisplayRole*/) const
{
  if (!index.isValid())
    return QVariant();

  const auto& item = m_Items[index.row()];

  if (iRole == Qt::ToolTipRole)
  {
    if (item.m_Type == WFileserveActivityType::ReadFile)
    {
      return QString("[TIME] == File was not transferred because the timestamps match on server and client.\n"
                     "[HASH] == File was not transferred because the file hashes matched on server and client.\n"
                     "[N/A] == File does not exist on the server (in the requested data directory).");
    }
  }

  if (index.column() == 0)
  {
    if (iRole == Qt::DisplayRole)
    {
      switch (item.m_Type)
      {
        case WFileserveActivityType::StartServer:
          return "Server Started";
        case WFileserveActivityType::StopServer:
          return "Server Stopped";
        case WFileserveActivityType::ClientConnect:
          return "Client Connected";
        case WFileserveActivityType::ClientReconnected:
          return "Client Re-connected";
        case WFileserveActivityType::ClientDisconnect:
          return "Client Disconnect";
        case WFileserveActivityType::Mount:
          return "Mount";
        case WFileserveActivityType::MountFailed:
          return "Failed Mount";
        case WFileserveActivityType::Unmount:
          return "Unmount";
        case WFileserveActivityType::ReadFile:
          return "Read";
        case WFileserveActivityType::WriteFile:
          return "Write";
        case WFileserveActivityType::DeleteFile:
          return "Delete";

        default:
          return QVariant();
      }
    }

    if (iRole == Qt::ForegroundRole)
    {
      switch (item.m_Type)
      {
        case WFileserveActivityType::StartServer:
          return QColor::fromRgb(0, 200, 0);
        case WFileserveActivityType::StopServer:
          return QColor::fromRgb(200, 200, 0);

        case WFileserveActivityType::ClientConnect:
        case WFileserveActivityType::ClientReconnected:
          return QColor::fromRgb(50, 200, 0);
        case WFileserveActivityType::ClientDisconnect:
          return QColor::fromRgb(250, 100, 0);

        case WFileserveActivityType::Mount:
          return QColor::fromRgb(0, 0, 200);
        case WFileserveActivityType::MountFailed:
          return QColor::fromRgb(255, 0, 0);
        case WFileserveActivityType::Unmount:
          return QColor::fromRgb(150, 0, 200);

        case WFileserveActivityType::ReadFile:
          return QColor::fromRgb(100, 100, 100);
        case WFileserveActivityType::WriteFile:
          return QColor::fromRgb(255, 150, 0);
        case WFileserveActivityType::DeleteFile:
          return QColor::fromRgb(200, 50, 50);

        default:
          return QVariant();
      }
    }
  }

  if (index.column() == 1)
  {
    if (iRole == Qt::DisplayRole)
    {
      return item.m_Text;
    }
  }

  return QVariant();
}


QVariant WQtFileserveActivityModel::headerData(int iSection, Qt::Orientation orientation, int iRole /*= Qt::DisplayRole*/) const
{
  if (iRole == Qt::DisplayRole)
  {
    if (iSection == 0)
    {
      return "Type";
    }

    if (iSection == 1)
    {
      return "Action";
    }
  }

  return QVariant();
}

WQtFileserveActivityItem& WQtFileserveActivityModel::AppendItem()
{
  if (!m_bTimerRunning)
  {
    m_bTimerRunning = true;

    QTimer::singleShot(250, this, &WQtFileserveActivityModel::UpdateViewSlot);
  }

  m_uiAddedItems++;
  return m_Items.ExpandAndGetRef();
}

void WQtFileserveActivityModel::UpdateView()
{
  if (m_uiAddedItems == 0)
    return;

  beginInsertRows(QModelIndex(), m_Items.GetCount() - m_uiAddedItems, m_Items.GetCount() - 1);
  insertRows(m_Items.GetCount(), m_uiAddedItems, QModelIndex());
  m_uiAddedItems = 0;
  endInsertRows();
}

void WQtFileserveActivityModel::Clear()
{
  m_Items.Clear();
  m_uiAddedItems = 0;

  beginResetModel();
  endResetModel();
}

void WQtFileserveActivityModel::UpdateViewSlot()
{
  m_bTimerRunning = false;

  UpdateView();
}
