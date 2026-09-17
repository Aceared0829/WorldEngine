#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QToolTip>

WQtLayerAdapter::WQtLayerAdapter(WScene2Document* pDocument)
  : WQtDocumentTreeModelAdapter(pDocument->GetSceneObjectManager(), WGetStaticRTTI<WSceneLayer>(), nullptr)
{
  m_pSceneDocument = pDocument;
  m_pSceneDocument->m_LayerEvents.AddEventHandler(
    WMakeDelegate(&WQtLayerAdapter::LayerEventHandler, this), m_LayerEventUnsubscriber);

  WDocument::s_EventsAny.AddEventHandler(WMakeDelegate(&WQtLayerAdapter::DocumentEventHander, this), m_DocumentEventUnsubscriber);
}

WQtLayerAdapter::~WQtLayerAdapter()
{
  m_LayerEventUnsubscriber.Unsubscribe();
  m_DocumentEventUnsubscriber.Unsubscribe();
}

QVariant WQtLayerAdapter::data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case UserRoles::LayerGuid:
    {
      WObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      WUuid layerGuid = pAccessor->GetByName<WUuid>(pObject, "Layer");
      return QVariant::fromValue(layerGuid);
    }
    break;
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
      WObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      WUuid layerGuid = pAccessor->GetByName<WUuid>(pObject, "Layer");
      // Use curator to get name in case the layer is unloaded and there is no document to query.
      const WAssetCurator::WLockedSubAsset subAsset = WAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (subAsset.isValid())
      {
        if (iRole == Qt::ToolTipRole)
        {
          return WMakeQString(subAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
        }
        WStringBuilder sName = subAsset->GetName();
        QString sQtName = QString::fromUtf8(sName.GetData());
        if (WSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
        {
          if (pLayer->IsModified())
          {
            sQtName += "*";
          }
        }
        return sQtName;
      }
      else
      {
        return QStringLiteral("Layer guid not found");
      }
    }
    break;

    case Qt::DecorationRole:
    {
      return WQtUiServices::GetCachedIconResource(":/EditorPluginScene/Icons/Layer.svg");
    }
    break;
    case Qt::ForegroundRole:
    {
      WObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      WUuid layerGuid = pAccessor->GetByName<WUuid>(pObject, "Layer");
      if (!m_pSceneDocument->IsLayerLoaded(layerGuid))
      {
        return QVariant();
      }
    }
    break;
    case Qt::FontRole:
    {
      QFont font;
      WObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      WUuid layerGuid = pAccessor->GetByName<WUuid>(pObject, "Layer");
      if (m_pSceneDocument->GetActiveLayer() == layerGuid)
        font.setBold(true);
      return font;
    }
    break;
  }

  return QVariant();
}

bool WQtLayerAdapter::setData(const WDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  return false;
}

void WQtLayerAdapter::LayerEventHandler(const WScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case WScene2LayerEvent::Type::LayerUnloaded:
    case WScene2LayerEvent::Type::LayerLoaded:
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
    }
    break;
    case WScene2LayerEvent::Type::ActiveLayerChanged:
    {
      QVector<int> v;
      v.push_back(Qt::FontRole);
      if (auto pObject = m_pSceneDocument->GetLayerObject(m_CurrentActiveLayer))
      {
        Q_EMIT dataChanged(pObject, v);
      }
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
      m_CurrentActiveLayer = e.m_layerGuid;
    }
    default:
      break;
  }
}

void WQtLayerAdapter::DocumentEventHander(const WDocumentEvent& e)
{
  if (e.m_Type == WDocumentEvent::Type::DocumentSaved || e.m_Type == WDocumentEvent::Type::ModifiedChanged)
  {
    const WDocumentObject* pLayerObj = m_pSceneDocument->GetLayerObject(e.m_pDocument->GetGuid());
    if (pLayerObj)
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(pLayerObj, v);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

WQtLayerDelegate::WQtLayerDelegate(QObject* pParent, WScene2Document* pDocument)
  : WQtItemDelegate(pParent)
  , m_pDocument(pDocument)
{
}

bool WQtLayerDelegate::mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const QRect visibleRect = GetVisibleIconRect(option);
  const QRect loadedRect = GetLoadedIconRect(option);
  if (pEvent->button() == Qt::MouseButton::LeftButton && (visibleRect.contains(pEvent->position().toPoint()) || loadedRect.contains(pEvent->position().toPoint())))
  {
    m_bPressed = true;
    pEvent->accept();
    return true;
  }
  return WQtItemDelegate::mousePressEvent(pEvent, option, index);
}

bool WQtLayerDelegate::mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect = GetLoadedIconRect(option);
    if (visibleRect.contains(pEvent->position().toPoint()))
    {
      const WUuid layerGuid = index.data(WQtLayerAdapter::UserRoles::LayerGuid).value<WUuid>();
      const bool bVisible = !m_pDocument->IsLayerVisible(layerGuid);
      m_pDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
    }
    else if (loadedRect.contains(pEvent->position().toPoint()))
    {
      const WUuid layerGuid = index.data(WQtLayerAdapter::UserRoles::LayerGuid).value<WUuid>();
      if (layerGuid != m_pDocument->GetGuid())
      {
        WLayerAction::ToggleLayerLoaded(m_pDocument, layerGuid);
      }
    }
    m_bPressed = false;
    pEvent->accept();
    return true;
  }
  return WQtItemDelegate::mouseReleaseEvent(pEvent, option, index);
}

bool WQtLayerDelegate::mouseMoveEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    return true;
  }
  return WQtItemDelegate::mouseMoveEvent(pEvent, option, index);
}

void WQtLayerDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  WQtItemDelegate::paint(pPainter, opt, index);

  {
    const WUuid layerGuid = index.data(WQtLayerAdapter::UserRoles::LayerGuid).value<WUuid>();
    if (layerGuid.IsValid())
    {
      {
        const QRect thumbnailRect = GetVisibleIconRect(opt);
        const bool bVisible = m_pDocument->IsLayerVisible(layerGuid);

        if (bVisible)
        {
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ObjectsVisible.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
        else
        {
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ObjectsHidden.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
      }

      if (layerGuid != m_pDocument->GetGuid())
      {
        const QRect thumbnailRect = GetLoadedIconRect(opt);
        const bool bLoaded = m_pDocument->IsLayerLoaded(layerGuid);

        if (bLoaded)
        {
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerLoaded.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
        else
        {
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerUnloaded.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
      }
    }
  }
}

QSize WQtLayerDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  return WQtItemDelegate::sizeHint(opt, index);
}

bool WQtLayerDelegate::helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const WUuid layerGuid = index.data(WQtLayerAdapter::UserRoles::LayerGuid).value<WUuid>();
  if (layerGuid.IsValid())
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect = GetLoadedIconRect(option);
    if (visibleRect.contains(pEvent->pos()))
    {
      const bool bVisible = m_pDocument->IsLayerVisible(layerGuid);
      QToolTip::showText(pEvent->globalPos(), bVisible ? "Hide Layer" : "Show Layer", pView);
      return true;
    }
    else if (loadedRect.contains(pEvent->pos()))
    {
      const bool bLoaded = m_pDocument->IsLayerLoaded(layerGuid);
      QToolTip::showText(pEvent->globalPos(), bLoaded ? "Unload Layer" : "Load Layer", pView);
      return true;
    }
  }
  return WQtItemDelegate::helpEvent(pEvent, pView, option, index);
}

QRect WQtLayerDelegate::GetVisibleIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height(), 0, 0, 0);
}

QRect WQtLayerDelegate::GetLoadedIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height() * 2, 0, -opt.rect.height(), 0);
}

//////////////////////////////////////////////////////////////////////////

WQtLayerModel::WQtLayerModel(WScene2Document* pDocument)
  : WQtDocumentTreeModel(pDocument->GetSceneObjectManager(), pDocument->GetSettingsObject()->GetGuid())
  , m_pDocument(pDocument)
{
  m_sTargetContext = "layertree";
}
