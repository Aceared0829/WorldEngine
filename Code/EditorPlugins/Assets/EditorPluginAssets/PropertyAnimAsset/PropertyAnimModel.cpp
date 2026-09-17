#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>

WQtPropertyAnimModel::WQtPropertyAnimModel(WPropertyAnimAssetDocument* pDocument, QObject* pParent)
  : QAbstractItemModel(pParent)
  , m_pAssetDoc(pDocument)
{
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtPropertyAnimModel::DocumentStructureEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtPropertyAnimModel::DocumentPropertyEventHandler, this));

  TriggerBuildMapping();
}

WQtPropertyAnimModel::~WQtPropertyAnimModel()
{
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtPropertyAnimModel::DocumentPropertyEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WQtPropertyAnimModel::DocumentStructureEventHandler, this));
}

QVariant WQtPropertyAnimModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  WQtPropertyAnimModelTreeEntry* pItem = static_cast<WQtPropertyAnimModelTreeEntry*>(index.internalPointer());
  W_ASSERT_DEBUG(pItem != nullptr, "Invalid model index");

  switch (iRole)
  {
    case Qt::DisplayRole:
      return QString(pItem->m_sDisplay.GetData());

    case Qt::DecorationRole:
      return pItem->m_Icon;

    case UserRoles::TrackPtr:
      return QVariant::fromValue((void*)pItem->m_pTrack);

    case UserRoles::TreeItem:
      return QVariant::fromValue((void*)pItem);

    case UserRoles::TrackIdx:
      return pItem->m_iTrackIdx;

    case UserRoles::Path:
      return QString(pItem->m_sPathToItem.GetData());
  }

  return QVariant();
}

Qt::ItemFlags WQtPropertyAnimModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex WQtPropertyAnimModel::index(int iRow, int iColumn, const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (iColumn != 0)
    return QModelIndex();

  WQtPropertyAnimModelTreeEntry* pParentItem = static_cast<WQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  if (pParentItem != nullptr)
  {
    return createIndex(iRow, iColumn, (void*)&m_AllEntries[m_iInUse][pParentItem->m_Children[iRow]]);
  }
  else
  {
    if (iRow >= (int)m_TopLevelEntries[m_iInUse].GetCount())
      return QModelIndex();

    return createIndex(iRow, iColumn, (void*)&m_AllEntries[m_iInUse][m_TopLevelEntries[m_iInUse][iRow]]);
  }
}

QModelIndex WQtPropertyAnimModel::parent(const QModelIndex& index) const
{
  if (!index.isValid() || index.column() != 0)
    return QModelIndex();

  WQtPropertyAnimModelTreeEntry* pItem = static_cast<WQtPropertyAnimModelTreeEntry*>(index.internalPointer());

  if (pItem->m_iParent < 0)
    return QModelIndex();

  return createIndex(m_AllEntries[m_iInUse][pItem->m_iParent].m_uiOwnRowIndex, index.column(), (void*)&m_AllEntries[m_iInUse][pItem->m_iParent]);
}

int WQtPropertyAnimModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (!parent.isValid())
    return m_TopLevelEntries[m_iInUse].GetCount();

  WQtPropertyAnimModelTreeEntry* pItem = static_cast<WQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  return pItem->m_Children.GetCount();
}

int WQtPropertyAnimModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 1;
}

void WQtPropertyAnimModel::DocumentStructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      TriggerBuildMapping();
      break;

    default:
      break;
  }
}

void WQtPropertyAnimModel::DocumentPropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == WDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_sProperty == "ObjectPath")
    {
      TriggerBuildMapping();
      return;
    }
  }
}

void WQtPropertyAnimModel::TriggerBuildMapping()
{
  if (m_bBuildMappingQueued)
    return;

  m_bBuildMappingQueued = true;
  QTimer::singleShot(100, this, SLOT(onBuildMappingTriggered()));
}

void WQtPropertyAnimModel::onBuildMappingTriggered()
{
  BuildMapping();
  m_bBuildMappingQueued = false;
}

void WQtPropertyAnimModel::BuildMapping()
{
  const WInt32 iToUse = (m_iInUse + 1) % 2;
  BuildMapping(iToUse);

  if (m_AllEntries[0] != m_AllEntries[1])
  {
    beginResetModel();
    m_iInUse = iToUse;
    endResetModel();
  }
}

void WQtPropertyAnimModel::BuildMapping(WInt32 iToUse)
{
  m_TopLevelEntries[iToUse].Clear();
  m_AllEntries[iToUse].Clear();

  const WPropertyAnimationTrackGroup& group = *m_pAssetDoc->GetProperties();

  WStringBuilder tmp;

  for (WUInt32 tIdx = 0; tIdx < group.m_Tracks.GetCount(); ++tIdx)
  {
    WPropertyAnimationTrack* pTrack = group.m_Tracks[tIdx];

    tmp = pTrack->m_sObjectSearchSequence;
    if (!pTrack->m_sComponentType.IsEmpty())
    {
      tmp.AppendPath(":");
      tmp.Append(pTrack->m_sComponentType.GetData());
    }
    tmp.AppendPath(pTrack->m_sPropertyPath);

    BuildMapping(iToUse, tIdx, pTrack, m_TopLevelEntries[iToUse], -1, tmp);
  }
}

void WQtPropertyAnimModel::BuildMapping(
  WInt32 iToUse, WInt32 iTrackIdx, WPropertyAnimationTrack* pTrack, WDynamicArray<WInt32>& treeItems, WInt32 iParentEntry, const char* szPath)
{
  const char* szSubPath = WStringUtils::FindSubString(szPath, "/");

  WStringBuilder name, sDisplayString;

  bool bIsComponent = false;
  if (szPath[0] == ':')
  {
    ++szPath;
    bIsComponent = true;
  }

  if (szSubPath != nullptr)
    name.SetSubString_FromTo(szPath, szSubPath);
  else
    name = szPath;

  if (bIsComponent)
    sDisplayString = WTranslate(name);
  else
    sDisplayString = name;

  WInt32 iThisEntry = -1;

  for (WUInt32 i = 0; i < treeItems.GetCount(); ++i)
  {
    if (m_AllEntries[iToUse][treeItems[i]].m_sDisplay.IsEqual_NoCase(sDisplayString))
    {
      iThisEntry = treeItems[i];
      break;
    }
  }

  WQtPropertyAnimModelTreeEntry* pThisEntry = nullptr;

  if (iThisEntry < 0)
  {
    pThisEntry = &m_AllEntries[iToUse].ExpandAndGetRef();
    iThisEntry = m_AllEntries[iToUse].GetCount() - 1;
    treeItems.PushBack(iThisEntry);

    pThisEntry->m_iParent = iParentEntry;
    pThisEntry->m_uiOwnRowIndex = treeItems.GetCount() - 1;
    pThisEntry->m_sDisplay = sDisplayString;

    if (bIsComponent)
    {
      sDisplayString.Set(":/TypeIcons/", name);
      pThisEntry->m_Icon = WQtUiServices::GetSingleton()->GetCachedIconResource(sDisplayString);
    }

    if (iParentEntry >= 0)
    {
      WStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
  else
  {
    pThisEntry = &m_AllEntries[iToUse][iThisEntry];
  }

  if (szSubPath != nullptr)
  {
    szSubPath += 1;
    BuildMapping(iToUse, iTrackIdx, pTrack, pThisEntry->m_Children, iThisEntry, szSubPath);
  }
  else
  {
    pThisEntry->m_iTrackIdx = iTrackIdx;
    pThisEntry->m_pTrack = pTrack;

    switch (pTrack->m_Target)
    {
      case WPropertyAnimTarget::Color:
        pThisEntry->m_Icon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/ColorGradient.svg");
        break;
      case WPropertyAnimTarget::Number:
        pThisEntry->m_Icon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/Curve1D.svg");
        break;
      case WPropertyAnimTarget::VectorX:
      case WPropertyAnimTarget::RotationX:
        pThisEntry->m_Icon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveX.svg");
        name.Append(".x");
        break;
      case WPropertyAnimTarget::VectorY:
      case WPropertyAnimTarget::RotationY:
        pThisEntry->m_Icon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.svg");
        name.Append(".y");
        break;
      case WPropertyAnimTarget::VectorZ:
      case WPropertyAnimTarget::RotationZ:
        pThisEntry->m_Icon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveZ.svg");
        name.Append(".z");
        break;
      case WPropertyAnimTarget::VectorW:
        pThisEntry->m_Icon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveW.svg");
        name.Append(".w");
        break;
    }

    pThisEntry->m_sDisplay = name;

    if (iParentEntry >= 0)
    {
      WStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
}
