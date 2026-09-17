#include <TestFramework/TestFrameworkPCH.h>

#ifdef W_USE_QT

#  include <QStringBuilder>
#  include <TestFramework/Framework/Qt/qtLogMessageDock.h>
#  include <TestFramework/Framework/TestFramework.h>

////////////////////////////////////////////////////////////////////////
// WQtLogMessageDock public functions
////////////////////////////////////////////////////////////////////////

WQtLogMessageDock::WQtLogMessageDock(QObject* pParent, const WTestFrameworkResult* pResult)
{
  setupUi(this);
  m_pModel = new WQtLogMessageModel(this, pResult);
  ListView->setModel(m_pModel);
}

WQtLogMessageDock::~WQtLogMessageDock()
{
  ListView->setModel(nullptr);
  delete m_pModel;
  m_pModel = nullptr;
}

void WQtLogMessageDock::resetModel()
{
  m_pModel->resetModel();
}

void WQtLogMessageDock::currentTestResultChanged(const WTestResultData* pTestResult)
{
  m_pModel->currentTestResultChanged(pTestResult);
  ListView->scrollToBottom();
}

void WQtLogMessageDock::currentTestSelectionChanged(const WTestResultData* pTestResult)
{
  m_pModel->currentTestSelectionChanged(pTestResult);
  ListView->scrollTo(m_pModel->GetLastIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
  ListView->scrollTo(m_pModel->GetFirstIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
}

////////////////////////////////////////////////////////////////////////
// WQtLogMessageModel public functions
////////////////////////////////////////////////////////////////////////

WQtLogMessageModel::WQtLogMessageModel(QObject* pParent, const WTestFrameworkResult* pResult)
  : QAbstractItemModel(pParent)
  , m_pTestResult(pResult)
{
}

WQtLogMessageModel::~WQtLogMessageModel() = default;

void WQtLogMessageModel::resetModel()
{
  beginResetModel();
  currentTestResultChanged(nullptr);
  endResetModel();
}

QModelIndex WQtLogMessageModel::GetFirstIndexOfTestSelection()
{
  if (m_pCurrentTestSelection == nullptr || m_pCurrentTestSelection->m_iFirstOutput == -1)
    return QModelIndex();

  WInt32 iEntries = (WInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((WInt32)m_VisibleEntries[i] >= m_pCurrentTestSelection->m_iFirstOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

QModelIndex WQtLogMessageModel::GetLastIndexOfTestSelection()
{
  if (m_pCurrentTestSelection == nullptr || m_pCurrentTestSelection->m_iLastOutput == -1)
    return QModelIndex();

  WInt32 iEntries = (WInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((WInt32)m_VisibleEntries[i] >= m_pCurrentTestSelection->m_iLastOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

void WQtLogMessageModel::currentTestResultChanged(const WTestResultData* pTestResult)
{
  UpdateVisibleEntries();
  currentTestSelectionChanged(pTestResult);
}

void WQtLogMessageModel::currentTestSelectionChanged(const WTestResultData* pTestResult)
{
  m_pCurrentTestSelection = pTestResult;
  if (m_pCurrentTestSelection != nullptr)
  {
    dataChanged(index(m_pCurrentTestSelection->m_iFirstOutput, 0), index(m_pCurrentTestSelection->m_iLastOutput, 0));
  }
}


////////////////////////////////////////////////////////////////////////
// WQtLogMessageModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant WQtLogMessageModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || m_pTestResult == nullptr || index.column() != 0)
    return QVariant();

  const WInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (WInt32)m_VisibleEntries.size())
    return QVariant();

  const WUInt32 uiLogIdx = m_VisibleEntries[iRow];
  const WUInt8 uiIndention = m_VisibleEntriesIndention[iRow];
  const WTestOutputMessage& Message = *m_pTestResult->GetOutputMessage(uiLogIdx);
  const WTestErrorMessage* pError = (Message.m_iErrorIndex != -1) ? m_pTestResult->GetErrorMessage(Message.m_iErrorIndex) : nullptr;
  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      if (pError != nullptr)
      {
        QString sBlockStart = QLatin1String("\n") % QString((uiIndention + 1) * 3, ' ');
        QString sBlockName =
          pError->m_sBlock.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Block: ") + QLatin1String(pError->m_sBlock.c_str()));
        QString sMessage =
          pError->m_sMessage.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Message: ") + QLatin1String(pError->m_sMessage.c_str()));
        QString sErrorMessage = QString(uiIndention * 3, ' ') % QString(Message.m_sMessage.c_str()) % sBlockName % sBlockStart %
                                QLatin1String("File: ") % QLatin1String(pError->m_sFile.c_str()) % sBlockStart % QLatin1String("Line: ") %
                                QString::number(pError->m_iLine) % sBlockStart % QLatin1String("Function: ") %
                                QLatin1String(pError->m_sFunction.c_str()) % sMessage;

        return sErrorMessage;
      }
      return QString(uiIndention * 3, ' ') + QString(Message.m_sMessage.c_str());
    }
    case Qt::ForegroundRole:
    {
      switch (Message.m_Type)
      {
        case WTestOutput::BeginBlock:
        case WTestOutput::Message:
          return QColor(Qt::yellow);
        case WTestOutput::Error:
          return QColor(Qt::red);
        case WTestOutput::Success:
          return QColor(Qt::green);
        case WTestOutput::Warning:
          return QColor(qRgb(255, 100, 0));
        case WTestOutput::StartOutput:
        case WTestOutput::EndBlock:
        case WTestOutput::ImportantInfo:
        case WTestOutput::Details:
        case WTestOutput::Duration:
        case WTestOutput::FinalResult:
          return QVariant();
        default:
          return QVariant();
      }
    }
    case Qt::BackgroundRole:
    {
      QPalette palette = QApplication::palette();
      if (m_pCurrentTestSelection != nullptr && m_pCurrentTestSelection->m_iFirstOutput != -1)
      {
        if (m_pCurrentTestSelection->m_iFirstOutput <= (WInt32)uiLogIdx && (WInt32)uiLogIdx <= m_pCurrentTestSelection->m_iLastOutput)
        {
          return palette.midlight().color();
        }
      }
      return palette.base().color();
    }

    default:
      return QVariant();
  }
}

Qt::ItemFlags WQtLogMessageModel::flags(const QModelIndex& index) const
{
  if (!index.isValid() || m_pTestResult == nullptr)
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant WQtLogMessageModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Log Entry");
    }
  }
  return QVariant();
}

QModelIndex WQtLogMessageModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn, iRow);
}

QModelIndex WQtLogMessageModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int WQtLogMessageModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr)
    return 0;

  return (int)m_VisibleEntries.size();
}

int WQtLogMessageModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


////////////////////////////////////////////////////////////////////////
// WQtLogMessageModel private functions
////////////////////////////////////////////////////////////////////////

void WQtLogMessageModel::UpdateVisibleEntries()
{
  m_VisibleEntries.clear();
  m_VisibleEntriesIndention.clear();
  if (m_pTestResult == nullptr)
    return;

  WUInt8 uiIndention = 0;
  WUInt32 uiEntries = m_pTestResult->GetOutputMessageCount();
  /// \todo filter out uninteresting messages
  for (WUInt32 i = 0; i < uiEntries; ++i)
  {
    WTestOutput::Enum Type = m_pTestResult->GetOutputMessage(i)->m_Type;
    if (Type == WTestOutput::BeginBlock)
      uiIndention++;
    if (Type == WTestOutput::EndBlock)
      uiIndention--;

    m_VisibleEntries.push_back(i);
    m_VisibleEntriesIndention.push_back(uiIndention);
  }
  beginResetModel();
  endResetModel();
}

#endif
