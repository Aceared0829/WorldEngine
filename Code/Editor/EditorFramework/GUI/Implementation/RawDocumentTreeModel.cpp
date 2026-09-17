#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>

WQtDocumentTreeModelAdapter::WQtDocumentTreeModelAdapter(const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty)
  : m_pTree(pTree)
  , m_pType(pType)
  , m_sChildProperty(szChildProperty)
{
  if (!m_sChildProperty.IsEmpty())
  {
    auto pProp = pType->FindPropertyByName(m_sChildProperty);
    W_ASSERT_DEV(pProp != nullptr && (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set),
      "The visualized object property tree must either be a set or array!");
    W_ASSERT_DEV(!pProp->GetFlags().IsSet(WPropertyFlags::Pointer) || pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner),
      "The visualized object must have ownership of the property objects!");
  }
}

const WRTTI* WQtDocumentTreeModelAdapter::GetType() const
{
  return m_pType;
}


const WString& WQtDocumentTreeModelAdapter::GetChildProperty() const
{
  return m_sChildProperty;
}

bool WQtDocumentTreeModelAdapter::setData(const WDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  return false;
}

Qt::ItemFlags WQtDocumentTreeModelAdapter::flags(const WDocumentObject* pObject, int iRow, int iColumn) const
{
  if (iColumn == 0)
  {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}


WQtDummyAdapter::WQtDummyAdapter(const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty)
  : WQtDocumentTreeModelAdapter(pTree, pType, szChildProperty)
{
}

QVariant WQtDummyAdapter::data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  if (iColumn == 0)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        WStringBuilder tmp;
        return QString::fromUtf8(pObject->GetTypeAccessor().GetType()->GetTypeName().GetData(tmp));
      }
      break;
    }
  }
  return QVariant();
}

WQtNamedAdapter::WQtNamedAdapter(const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty, const char* szNameProperty)
  : WQtDocumentTreeModelAdapter(pTree, pType, szChildProperty)
  , m_sNameProperty(szNameProperty)
{
  auto pProp = pType->FindPropertyByName(m_sNameProperty);
  W_ASSERT_DEV(pProp != nullptr && pProp->GetCategory() == WPropertyCategory::Member && pProp->GetSpecificType()->GetVariantType() == WVariantType::String, "The name property must be a string member property.");

  m_pTree->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtNamedAdapter::TreePropertyEventHandler, this));
}

WQtNamedAdapter::~WQtNamedAdapter()
{
  m_pTree->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtNamedAdapter::TreePropertyEventHandler, this));
}

QVariant WQtNamedAdapter::data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  if (iColumn == 0)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        return QString::fromUtf8(pObject->GetTypeAccessor().GetValue(m_sNameProperty).ConvertTo<WString>().GetData());
      }
      break;
    }
  }
  return QVariant();
}

void WQtNamedAdapter::TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == m_sNameProperty)
  {
    QVector<int> v;
    v.push_back(Qt::DisplayRole);
    v.push_back(Qt::EditRole);
    Q_EMIT dataChanged(e.m_pObject, v);
  }
}

WQtNameableAdapter::WQtNameableAdapter(
  const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty, const char* szNameProperty)
  : WQtNamedAdapter(pTree, pType, szChildProperty, szNameProperty)
{
}

WQtNameableAdapter::~WQtNameableAdapter() = default;

bool WQtNameableAdapter::setData(const WDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  if (iColumn == 0 && iRole == Qt::EditRole)
  {
    auto pHistory = m_pTree->GetDocument()->GetCommandHistory();

    pHistory->StartTransaction(WFmt("Rename to '{0}'", value.toString().toUtf8().data()));

    WSetObjectPropertyCommand cmd;
    cmd.m_NewValue = value.toString().toUtf8().data();
    cmd.m_Object = pObject->GetGuid();
    cmd.m_sProperty = m_sNameProperty;

    pHistory->AddCommand(cmd).AssertSuccess();

    pHistory->FinishTransaction();

    return true;
  }
  return false;
}

Qt::ItemFlags WQtNameableAdapter::flags(const WDocumentObject* pObject, int iRow, int iColumn) const
{
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

  if (iColumn == 0)
  {
    return flags | Qt::ItemIsEditable;
  }

  return flags;
}

//////////////////////////////////////////////////////////////////////////

WQtDocumentTreeModel::WQtDocumentTreeModel(const WDocumentObjectManager* pTree, const WUuid& root)
  : QAbstractItemModel(nullptr)
  , m_pDocumentTree(pTree)
  , m_Root(root)
{
  m_pDocumentTree->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtDocumentTreeModel::TreeEventHandler, this));
}

WQtDocumentTreeModel::~WQtDocumentTreeModel()
{
  m_pDocumentTree->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WQtDocumentTreeModel::TreeEventHandler, this));
}

void WQtDocumentTreeModel::AddAdapter(WQtDocumentTreeModelAdapter* pAdapter)
{
  W_ASSERT_DEV(!m_Adapters.Contains(pAdapter->GetType()), "An adapter for the given type was already registered.");

  pAdapter->setParent(this);
  connect(pAdapter, &WQtDocumentTreeModelAdapter::dataChanged, this, [this](const WDocumentObject* pObject, QVector<int> roles)
    {
    if (!pObject)
      return;
    auto index = ComputeModelIndex(pObject);
    if (!index.isValid())
      return;

    QModelIndex idx2 = index.siblingAtColumn(columnCount() - 1); // mark the entire row as modified
    Q_EMIT dataChanged(index, idx2, roles); });
  m_Adapters.Insert(pAdapter->GetType(), pAdapter);
  beginResetModel();
  endResetModel();
}

const WQtDocumentTreeModelAdapter* WQtDocumentTreeModel::GetAdapter(const WRTTI* pType) const
{
  while (pType != nullptr)
  {
    if (const WQtDocumentTreeModelAdapter* const* adapter = m_Adapters.GetValue(pType))
    {
      return *adapter;
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}

void WQtDocumentTreeModel::TreeEventHandler(const WDocumentObjectStructureEvent& e)
{
  const WDocumentObject* pParent = nullptr;
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::BeforeReset:
      beginResetModel();
      return;
    case WDocumentObjectStructureEvent::Type::AfterReset:
      endResetModel();
      return;
    case WDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      pParent = e.m_pPreviousParent;
      break;
    case WDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      pParent = e.m_pNewParent;
      break;
  }
  W_ASSERT_DEV(pParent != nullptr, "Each structure event should have a parent set.");
  if (!IsUnderRoot(pParent))
    return;
  auto pType = pParent->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return;

  if (pAdapter->GetChildProperty() != e.m_sParentProperty)
    return;

  // TODO: BLA root object could have other objects instead of m_pBaseClass, in which case indices are broken on root.

  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      WInt32 iIndex = (WInt32)e.m_NewPropertyIndex.ConvertTo<WInt32>();
      if (e.m_pNewParent == GetRoot())
        beginInsertRows(QModelIndex(), iIndex, iIndex);
      else
        beginInsertRows(ComputeModelIndex(e.m_pNewParent), iIndex, iIndex);
    }
    break;
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      endInsertRows();
    }
    break;
    case WDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      WInt32 iIndex = ComputeIndex(e.m_pObject);

      beginRemoveRows(ComputeParent(e.m_pObject), iIndex, iIndex);
    }
    break;
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      endRemoveRows();
    }
    break;
    case WDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      WInt32 iNewIndex = (WInt32)e.m_NewPropertyIndex.ConvertTo<WInt32>();
      WInt32 iIndex = ComputeIndex(e.m_pObject);
      beginMoveRows(ComputeModelIndex(e.m_pPreviousParent), iIndex, iIndex, ComputeModelIndex(e.m_pNewParent), iNewIndex);
    }
    break;
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      endMoveRows();
    }
    break;
    default:
      break;
  }
}

QModelIndex WQtDocumentTreeModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  const WDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const WDocumentObject*)parent.internalPointer();
  }

  auto pType = pObject->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();
  if (iRow >= pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty()))
    return QModelIndex();

  WVariant value = pObject->GetTypeAccessor().GetValue(pAdapter->GetChildProperty(), iRow);
  W_ASSERT_DEV(value.IsValid() && value.IsA<WUuid>(), "Tree corruption!");
  const WDocumentObject* pChild = m_pDocumentTree->GetObject(value.Get<WUuid>());
  return createIndex(iRow, iColumn, const_cast<WDocumentObject*>(pChild));
}

WInt32 WQtDocumentTreeModel::ComputeIndex(const WDocumentObject* pObject) const
{
  WInt32 iIndex = pObject->GetPropertyIndex().ConvertTo<WInt32>();
  return iIndex;
}

const WDocumentObject* WQtDocumentTreeModel::GetRoot() const
{
  if (m_Root.IsValid())
  {
    return m_pDocumentTree->GetObject(m_Root);
  }
  return m_pDocumentTree->GetRootObject();
}

bool WQtDocumentTreeModel::IsUnderRoot(const WDocumentObject* pObject) const
{
  const WDocumentObject* pRoot = GetRoot();
  while (pObject)
  {
    if (pRoot == pObject)
      return true;

    pObject = pObject->GetParent();
  }
  return false;
}

QModelIndex WQtDocumentTreeModel::ComputeModelIndex(const WDocumentObject* pObject) const
{
  // Filter out objects that are not under the child property of the
  // parents adapter.
  if (pObject == GetRoot())
    return QModelIndex();

  auto pType = pObject->GetParent()->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();

  if (pAdapter->GetChildProperty() != pObject->GetParentProperty())
    return QModelIndex();

  return index(ComputeIndex(pObject), 0, ComputeParent(pObject));
}


void WQtDocumentTreeModel::SetAllowDragDrop(bool bAllow)
{
  m_bAllowDragDrop = bAllow;
}

QModelIndex WQtDocumentTreeModel::ComputeParent(const WDocumentObject* pObject) const
{
  const WDocumentObject* pParent = pObject->GetParent();

  if (pParent == GetRoot())
    return QModelIndex();

  WInt32 iIndex = ComputeIndex(pParent);

  return createIndex(iIndex, 0, const_cast<WDocumentObject*>(pParent));
}

QModelIndex WQtDocumentTreeModel::parent(const QModelIndex& child) const
{
  const WDocumentObject* pObject = (const WDocumentObject*)child.internalPointer();

  return ComputeParent(pObject);
}

int WQtDocumentTreeModel::rowCount(const QModelIndex& parent) const
{
  int iCount = 0;
  const WDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const WDocumentObject*)parent.internalPointer();
  }

  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    if (!pAdapter->GetChildProperty().IsEmpty())
    {
      iCount = pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty());
    }
  }

  return iCount;
}

int WQtDocumentTreeModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant WQtDocumentTreeModel::data(const QModelIndex& index, int iRole) const
{
  // if (index.isValid())
  {
    const WDocumentObject* pObject = (const WDocumentObject*)index.internalPointer();
    auto pType = pObject->GetTypeAccessor().GetType();
    if (auto pAdapter = GetAdapter(pType))
    {
      return pAdapter->data(pObject, index.row(), index.column(), iRole);
    }
  }

  return QVariant();
}

Qt::DropActions WQtDocumentTreeModel::supportedDropActions() const
{
  if (m_bAllowDragDrop)
    return Qt::MoveAction | Qt::CopyAction;

  return Qt::IgnoreAction;
}

Qt::ItemFlags WQtDocumentTreeModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  const WDocumentObject* pObject = (const WDocumentObject*)index.internalPointer();
  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->flags(pObject, index.row(), index.column());
  }

  return Qt::ItemFlag::NoItemFlags;
}


bool WQtDocumentTreeModel::canDropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent) const
{
  const WDocumentObject* pNewParent = (const WDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  WDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = iRow;
  info.m_pMimeData = pData;
  info.m_sTargetContext = m_sTargetContext;
  info.m_TargetDocument = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject = pNewParent->GetGuid();
  info.m_bCtrlKeyDown = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter = GetAdapter(pNewParent->GetType());

  if (WDragDropHandler::CanDropOnly(&info))
    return true;

  {
    // Test 'CanMove' of the target object manager.
    QByteArray encodedData = pData->data("application/WEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    WTempHybridArray<WDocumentObject*, 32> Dragged;
    stream >> Dragged;

    auto pType = pNewParent->GetTypeAccessor().GetType();
    auto pAdapter = GetAdapter(pType);
    const WString& sProperty = pAdapter->GetChildProperty();
    for (const WDocumentObject* pItem : Dragged)
    {
      // If the item's and the target tree's document don't match we can't operate via this code.
      if (pItem->GetDocumentObjectManager()->GetDocument() != m_pDocumentTree->GetDocument())
        return false;
      if (m_pDocumentTree->CanMove(pItem, pNewParent, sProperty, info.m_iTargetObjectInsertChildIndex).Failed())
        return false;
    }
    return QAbstractItemModel::canDropMimeData(pData, action, iRow, iColumn, parent);
  }
  return false;
}

bool WQtDocumentTreeModel::dropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent)
{
  if (!m_bAllowDragDrop)
    return false;

  if (iColumn > 0)
    return false;

  const WDocumentObject* pNewParent = (const WDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  WDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = iRow;
  info.m_pMimeData = pData;
  info.m_sTargetContext = m_sTargetContext;
  info.m_TargetDocument = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject = pNewParent->GetGuid();
  info.m_bCtrlKeyDown = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter = GetAdapter(pNewParent->GetType());
  if (WDragDropHandler::DropOnly(&info))
    return true;

  return WQtDocumentTreeModel::MoveObjects(info);
}


bool WQtDocumentTreeModel::MoveObjects(const WDragDropInfo& info)
{
  if (info.m_pMimeData->hasFormat("application/WEditor.ObjectSelection"))
  {
    auto pDoc = WDocumentManager::GetDocumentByGuid(info.m_TargetDocument);
    const WDocumentObject* pTarget = pDoc->GetObjectManager()->GetObject(info.m_TargetObject);
    W_ASSERT_DEBUG(pTarget != nullptr, "object from info should always be valid");

    QByteArray encodedData = info.m_pMimeData->data("application/WEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    WTempHybridArray<WDocumentObject*, 32> Dragged;
    stream >> Dragged;

    for (const WDocumentObject* pDocObject : Dragged)
    {
      // if (action != Qt::DropAction::MoveAction)
      {
        bool bCanMove = true;
        const WDocumentObject* pCurParent = pTarget;

        while (pCurParent)
        {
          if (pCurParent == pDocObject)
          {
            bCanMove = false;
            break;
          }

          pCurParent = pCurParent->GetParent();
        }

        if (!bCanMove)
        {
          WQtUiServices::MessageBoxInformation("Cannot move an object to one of its own children");
          return false;
        }
      }
    }

    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Reparent Object");

    WStatus res(W_SUCCESS);
    for (WUInt32 i = 0; i < Dragged.GetCount(); ++i)
    {
      WMoveObjectCommand cmd;
      cmd.m_Object = Dragged[i]->GetGuid();
      cmd.m_Index = info.m_iTargetObjectInsertChildIndex;
      cmd.m_sParentProperty = info.m_pAdapter->GetChildProperty();
      cmd.m_NewParent = pTarget->GetGuid();

      res = pHistory->AddCommand(cmd);
      if (res.Failed())
        break;
    }

    if (res.Failed())
      pHistory->CancelTransaction();
    else
      pHistory->FinishTransaction();

    WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node move failed.");
    return true;
  }

  return false;
}

const WDocumentObject* WQtDocumentTreeModel::GetObject(const QModelIndex index) const
{
  return (const WDocumentObject*)index.internalPointer();
}

QStringList WQtDocumentTreeModel::mimeTypes() const
{
  QStringList types;
  if (m_bAllowDragDrop)
  {
    types << "application/WEditor.ObjectSelection";
  }

  return types;
}

QMimeData* WQtDocumentTreeModel::mimeData(const QModelIndexList& indexes) const
{
  if (!m_bAllowDragDrop)
    return nullptr;

  WTempHybridArray<void*, 1> ptrs;
  for (const QModelIndex& index : indexes)
  {
    if (index.isValid())
    {
      void* pObject = index.internalPointer();
      ptrs.PushBack(pObject);
    }
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << ptrs;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/WEditor.ObjectSelection", encodedData);
  return mimeData;
}

bool WQtDocumentTreeModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  const WDocumentObject* pObject = (const WDocumentObject*)index.internalPointer();
  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->setData(pObject, index.row(), index.column(), value, iRole);
  }

  return false;
}
