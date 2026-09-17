#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <QColor>
#include <QThread>


WQtLogModel::WQtLogModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
  m_bIsValid = true;
  m_LogLevel = WLogMsgType::InfoMsg;
}

void WQtLogModel::Invalidate()
{
  if (!m_bIsValid)
    return;

  beginResetModel();
  m_bIsValid = false;
  endResetModel();
}

void WQtLogModel::Clear()
{
  if (m_AllMessages.IsEmpty())
    return;

  {
    W_LOCK(m_NewMessagesMutex);

    m_uiNumErrors = 0;
    m_uiNumSeriousWarnings = 0;
    m_uiNumWarnings = 0;

    m_AllMessages.Clear();
    m_VisibleMessages.Clear();
    m_BlockQueue.Clear();
    Invalidate();
    m_bIsValid = true;
  }

  Q_EMIT NewErrorsOrWarnings(nullptr, false);
}

void WQtLogModel::SetLogLevel(WLogMsgType::Enum logLevel)
{
  if (m_LogLevel == logLevel)
    return;

  m_LogLevel = logLevel;
  Invalidate();
}

void WQtLogModel::SetSearchText(const char* szText)
{
  if (m_sSearchText == szText)
    return;

  m_sSearchText = szText;
  Invalidate();
}

void WQtLogModel::AddLogMsg(const WLogEntry& msg)
{
  bool bNeedInvoke = false;
  {
    W_LOCK(m_NewMessagesMutex);

    if (m_NewMessages.GetCount() > 1000)
    {
      // at the end of a play session, there can be a lot of log message queued up
      // this will take ages to process and is mostly pointless, so just print the last ones
      m_NewMessages.PopFront();
    }

    m_NewMessages.PushBack(msg);

    // Only queue ProcessNewMessages if not already pending
    if (!m_bProcessingPending)
    {
      m_bProcessingPending = true;
      bNeedInvoke = true;
    }
  }

  // always queue the message processing, otherwise it can happen that an error during this
  // triggers recursive logging, which is forbidden
  if (bNeedInvoke)
  {
    QMetaObject::invokeMethod(this, "ProcessNewMessages", Qt::ConnectionType::QueuedConnection);
  }
}

bool WQtLogModel::IsFiltered(const WLogEntry& lm) const
{
  if (lm.m_Type < WLogMsgType::None)
    return false;

  if (lm.m_Type > m_LogLevel)
    return true;

  if (m_sSearchText.IsEmpty())
    return false;

  if (lm.m_sMsg.FindSubString_NoCase(m_sSearchText.GetData()))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////
// WQtLogModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant WQtLogModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  UpdateVisibleEntries();

  const WInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (WInt32)m_VisibleMessages.GetCount())
    return QVariant();

  const ModelEntry& entry = *m_VisibleMessages[iRow];
  const WLogEntry& msg = entry.m_Log;

  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      if (msg.m_sMsg.FindSubString("\n") != nullptr)
      {
        WStringBuilder sTemp = msg.m_sMsg;
        sTemp.ReplaceAll("\n", " ");
        return WMakeQString(sTemp);
      }
      return WMakeQString(msg.m_sMsg);
    }
    case Qt::ToolTipRole:
    {
      return WMakeQString(msg.m_sMsg);
    }
    case Qt::ForegroundRole:
    {
      switch (msg.m_Type)
      {
        case WLogMsgType::BeginGroup:
          return WToQtColor(WColorScheme::LightUI(WColorScheme::Gray));
        case WLogMsgType::EndGroup:
          return WToQtColor(WColorScheme::DarkUI(WColorScheme::Gray));
        case WLogMsgType::ErrorMsg:
          return WToQtColor(WColorScheme::LightUI(WColorScheme::Red));
        case WLogMsgType::SeriousWarningMsg:
          return WToQtColor(WColorScheme::LightUI(WColorScheme::Orange));
        case WLogMsgType::WarningMsg:
          return WToQtColor(WColorScheme::LightUI(WColorScheme::Yellow));
        case WLogMsgType::SuccessMsg:
          return WToQtColor(WColorScheme::LightUI(WColorScheme::Green));
        case WLogMsgType::DevMsg:
          return WToQtColor(WColorScheme::LightUI(WColorScheme::Blue));
        case WLogMsgType::DebugMsg:
          return WToQtColor(WColorScheme::LightUI(WColorScheme::Cyan));
        default:
          return QVariant();
      }
    }

    case UserRoles::Link:
      if (entry.m_sLink.IsEmpty())
        return QVariant();

      return WMakeQString(entry.m_sLink);

    case UserRoles::LinkText:
      if (entry.m_sLinkText.IsEmpty())
        return QVariant();

      return WMakeQString(entry.m_sLinkText);

    case UserRoles::LinkTarget:
      if (entry.m_sLinkTarget.IsEmpty())
        return QVariant();

      return WMakeQString(entry.m_sLinkTarget);

    default:
      return QVariant();
  }

  return QVariant();
}

Qt::ItemFlags WQtLogModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant WQtLogModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  return QVariant();
}

QModelIndex WQtLogModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn, iRow);
}

QModelIndex WQtLogModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int WQtLogModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  UpdateVisibleEntries();

  return (int)m_VisibleMessages.GetCount();
}

int WQtLogModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


void WQtLogModel::ProcessNewMessages()
{
  bool bNewErrors = false;
  WStringBuilder sLatestWarning;
  WStringBuilder sLatestError;

  // Collect messages to add to visible list
  WTempHybridArray<const ModelEntry*, 64> messagesToAdd;

  {
    W_LOCK(m_NewMessagesMutex);

    // Reset processing flag so new messages can trigger another ProcessNewMessages
    m_bProcessingPending = false;

    if (m_NewMessages.IsEmpty())
      return;

    WStringBuilder s;
    for (const auto& msg : m_NewMessages)
    {
      auto& entry = m_AllMessages.ExpandAndGetRef();
      entry.m_Log = msg;

      if (msg.m_Type == WLogMsgType::BeginGroup || msg.m_Type == WLogMsgType::EndGroup)
      {
        s.SetPrintf("%*s<<< %s", msg.m_uiIndentation, "", msg.m_sMsg.GetData());

        if (msg.m_Type == WLogMsgType::EndGroup)
        {
          s.AppendFormat(" ({0} sec) >>>", WArgF(msg.m_fSeconds, 3));
        }
        else if (!msg.m_sTag.IsEmpty())
        {
          s.Append(" (", msg.m_sTag, ") >>>");
        }
        else
        {
          s.Append(" >>>");
        }

        entry.m_Log.m_sMsg = s;
      }
      else
      {
        s.SetPrintf("%*s%s", 4 * msg.m_uiIndentation, "", msg.m_sMsg.GetData());
        entry.m_Log.m_sMsg = s;

        if (msg.m_Type == WLogMsgType::ErrorMsg)
        {
          sLatestError = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumErrors;
        }
        else if (msg.m_Type == WLogMsgType::SeriousWarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumSeriousWarnings;
        }
        else if (msg.m_Type == WLogMsgType::WarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumWarnings;
        }
      }

      FindLink(entry);

      // if the message would not be shown anyway, don't trigger an update
      if (IsFiltered(msg))
        continue;

      if (msg.m_Type == WLogMsgType::BeginGroup)
      {
        m_BlockQueue.PushBack(&m_AllMessages.PeekBack());
        continue;
      }
      else if (msg.m_Type == WLogMsgType::EndGroup)
      {
        if (!m_BlockQueue.IsEmpty())
        {
          m_BlockQueue.PopBack();
          continue;
        }
      }

      // Add any queued block messages
      for (auto pMsg : m_BlockQueue)
      {
        messagesToAdd.PushBack(pMsg);
      }
      m_BlockQueue.Clear();

      // Add the current message
      messagesToAdd.PushBack(&m_AllMessages.PeekBack());
    }

    m_NewMessages.Clear();
  }

  // Batch insert all visible messages at once
  if (!messagesToAdd.IsEmpty())
  {
    const int iFirstRow = static_cast<int>(m_VisibleMessages.GetCount());
    const int iLastRow = iFirstRow + static_cast<int>(messagesToAdd.GetCount()) - 1;

    beginInsertRows(QModelIndex(), iFirstRow, iLastRow);
    for (auto pMsg : messagesToAdd)
    {
      m_VisibleMessages.PushBack(pMsg);
    }
    endInsertRows();
  }

  if (bNewErrors)
  {
    if (!sLatestError.IsEmpty())
    {
      Q_EMIT NewErrorsOrWarnings(sLatestError, true);
    }
    else
    {
      Q_EMIT NewErrorsOrWarnings(sLatestWarning, false);
    }
  }
}

void WQtLogModel::UpdateVisibleEntries() const
{
  if (m_bIsValid)
    return;


  m_bIsValid = true;
  m_VisibleMessages.Clear();
  for (const auto& msg : m_AllMessages)
  {
    if (IsFiltered(msg.m_Log))
      continue;

    if (msg.m_Log.m_Type == WLogMsgType::EndGroup)
    {
      if (!m_VisibleMessages.IsEmpty())
      {
        if (m_VisibleMessages.PeekBack()->m_Log.m_Type == WLogMsgType::BeginGroup)
          m_VisibleMessages.PopBack();
        else
          m_VisibleMessages.PushBack(&msg);
      }
    }
    else
    {
      m_VisibleMessages.PushBack(&msg);
    }
  }
}

void WQtLogModel::FindLink(ModelEntry& ref_entry) const
{
  // Link format:
  //   [[Display Text|scheme:target]]
  //
  // For instance:
  //   Object [[Bernd|asset:{ Doc-UUID }#{ Obj-UUID }]] not found.
  //
  // Link handler can parse the link target further, e.g. detect the scheme to support different actions.

  const char* szStart = ref_entry.m_Log.m_sMsg.FindSubString("[[");
  while (szStart != nullptr)
  {
    const char* szSeparator = ref_entry.m_Log.m_sMsg.FindSubString("|", szStart);
    const char* szEnd = ref_entry.m_Log.m_sMsg.FindSubString("]]", szStart);

    if (szSeparator && szEnd && szSeparator < szEnd)
    {
      ref_entry.m_sLink = WStringView(szStart, szEnd + 2);

      ref_entry.m_sLinkText = WStringView(szStart + 2, szSeparator);
      ref_entry.m_sLinkTarget = WStringView(szSeparator + 1, szEnd);

      ref_entry.m_sLinkText.Trim();
      ref_entry.m_sLinkTarget.Trim();
      return;
    }

    // Find next candidate
    szStart = ref_entry.m_Log.m_sMsg.FindSubString("[[", szStart + 1);
  }
}
