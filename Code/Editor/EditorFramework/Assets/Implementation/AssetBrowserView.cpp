#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserView.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>


WQtAssetBrowserView::WQtAssetBrowserView(QWidget* pParent)
  : WQtItemView<QListView>(pParent)
{
  m_iIconSizePercentage = 100;
  m_pDelegate = new WQtIconViewDelegate(this);

  SetDialogMode(false);

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setViewMode(QListView::ViewMode::IconMode);
  setUniformItemSizes(true);
  setResizeMode(QListView::ResizeMode::Adjust);

  setItemDelegate(m_pDelegate);
  SetIconScale(m_iIconSizePercentage);
}

void WQtAssetBrowserView::startDrag(Qt::DropActions supportedActions)
{
  // overridden so that we can get rid of the preview image

  QModelIndexList indexes = selectedIndexes();
  if (indexes.count() > 0)
  {
    QMimeData* data = model()->mimeData(indexes);
    if (!data)
    {
      return;
    }

    QDrag* drag = new QDrag(this);
    drag->setMimeData(data);

    drag->exec(supportedActions, Qt::MoveAction);
  }
}

void WQtAssetBrowserView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pDelegate->SetDrawTransformState(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  }
  else
  {
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_pDelegate->SetDrawTransformState(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  }
}

void WQtAssetBrowserView::SetIconMode(bool bIconMode)
{
  if (bIconMode)
  {
    setViewMode(QListView::ViewMode::IconMode);
    SetIconScale(m_iIconSizePercentage);
  }
  else
  {
    setViewMode(QListView::ViewMode::ListMode);
    setGridSize(QSize());
  }
}

void WQtAssetBrowserView::SetIconScale(WInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = WMath::Clamp(iIconSizePercentage, 10, 100);
  m_pDelegate->SetIconScale(m_iIconSizePercentage);

  if (viewMode() != QListView::ViewMode::IconMode)
    return;

  setGridSize(m_pDelegate->sizeHint(QStyleOptionViewItem(), QModelIndex()));
}

WInt32 WQtAssetBrowserView::GetIconScale() const
{
  return m_iIconSizePercentage;
}

void WQtAssetBrowserView::dragEnterEvent(QDragEnterEvent* pEvent)
{
  if (pEvent->source())
    pEvent->acceptProposedAction();
}

void WQtAssetBrowserView::dragMoveEvent(QDragMoveEvent* pEvent)
{
  pEvent->acceptProposedAction();
}

void WQtAssetBrowserView::dragLeaveEvent(QDragLeaveEvent* pEvent)
{
  pEvent->accept();
}

static void NotifyFileChanges(WArrayPtr<WString> files)
{
  for (const auto& file : files)
  {
    WFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }
}

void WQtAssetBrowserView::dropEvent(QDropEvent* pEvent)
{
  if (!pEvent->mimeData()->hasUrls())
    return;

  QList<QUrl> paths = pEvent->mimeData()->urls();
  const WString targetDirectory = indexAt(pEvent->position().toPoint()).data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  if (targetDirectory.IsEmpty())
  {
    return;
  }

  WTempHybridArray<WString, 32> touchedFiles;
  // make sure to notify the filesystem of files and folders that were touched
  W_SCOPE_EXIT(NotifyFileChanges(touchedFiles));

  for (auto it = paths.begin(); it != paths.end(); it++)
  {
    WStringBuilder src = it->path().toUtf8().constData();
    src.TrimWordStart("/"); // remove '/' at start
    src.MakeCleanPath();

    WStringBuilder dst = targetDirectory;
    dst.MakeCleanPath();

    // prevent moving stuff into itself
    if (src == dst)
      continue;

    // don't allow dropping anything onto an existing file
    if (WOSFile::ExistsFile(dst))
      continue;

    dst.AppendPath(qtToEzString(it->fileName()));

    if (WOSFile::ExistsDirectory(src))
    {
      if (WOSFile::ExistsDirectory(dst)) // ask to overwrite if target already exists
      {
        const int res = WQtUiServices::MessageBoxQuestion(WFmt("Directory already exists:\n\n'{}'\n\nOverwrite files inside directory?", dst), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel, QMessageBox::Yes);

        if (res == QMessageBox::Cancel)
          return;

        if (res == QMessageBox::No)
          continue;
      }

      if (WOSFile::CopyFolder(src, dst, &touchedFiles).Failed())
      {
        WQtUiServices::MessageBoxWarning(WFmt("Failed to copy folder:\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", src, dst));
        return;
      }

      touchedFiles.PushBack(dst);

      if (WOSFile::DeleteFolder(src).Failed())
      {
        WQtUiServices::MessageBoxWarning(WFmt("Failed to remove folder:\n\n'{}'\n\nAborting operation.", src));
        return;
      }
    }
    else if (WOSFile::ExistsFile(src))
    {
      if (WOSFile::ExistsFile(dst)) // ask to overwrite if target already exists
      {
        const int res = WQtUiServices::MessageBoxQuestion(WFmt("The file already exists:\n\n'{}'\n\nOverwrite file?", dst), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel, QMessageBox::Yes);

        if (res == QMessageBox::Cancel)
          return;

        if (res == QMessageBox::No)
          continue;

        WOSFile::DeleteFile(dst).IgnoreResult();
      }

      touchedFiles.PushBack(src);
      touchedFiles.PushBack(dst);

      if (WOSFile::MoveFileOrDirectory(src, dst).Failed())
      {
        WQtUiServices::MessageBoxWarning(WFmt("Failed to move file:\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", src, dst));
        return;
      }
    }
  }
}

void WQtAssetBrowserView::wheelEvent(QWheelEvent* pEvent)
{
  if (pEvent->modifiers() == Qt::CTRL)
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    WInt32 iDelta = pEvent->angleDelta().y() > 0 ? 5 : -5;
#else
    WInt32 iDelta = pEvent->delta() > 0 ? 5 : -5;
#endif
    SetIconScale(m_iIconSizePercentage + iDelta);
    Q_EMIT ViewZoomed(m_iIconSizePercentage);
    return;
  }

  QListView::wheelEvent(pEvent);
}

void WQtAssetBrowserView::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::MouseButton::BackButton)
  {
    pEvent->ignore();
    return;
  }

  QListView::mouseDoubleClickEvent(pEvent);
}

void WQtAssetBrowserView::mousePressEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::MouseButton::BackButton)
  {
    pEvent->ignore();
    return;
  }

  QListView::mousePressEvent(pEvent);
}

void WQtAssetBrowserView::mouseMoveEvent(QMouseEvent* pEvent)
{
  // only allow dragging with left mouse button
  if (state() == DraggingState && !pEvent->buttons().testFlag(Qt::MouseButton::LeftButton))
  {
    return;
  }

  QListView::mouseMoveEvent(pEvent);
}

WQtIconViewDelegate::WQtIconViewDelegate(WQtAssetBrowserView* pParent)
  : WQtItemDelegate(pParent)
{
  m_bDrawTransformState = true;
  m_iIconSizePercentage = 100;
  m_pView = pParent;
}

void WQtIconViewDelegate::SetIconScale(WInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = iIconSizePercentage;
}

bool WQtIconViewDelegate::mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  if (!itemType.IsSet(WAssetBrowserItemFlags::Asset))
    return false;

  const WUInt32 uiThumbnailSize = ThumbnailSize();
  QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(pEvent->position().toPoint()))
  {
    pEvent->accept();
    return true;
  }
  return false;
}

bool WQtIconViewDelegate::mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  if (!itemType.IsSet(WAssetBrowserItemFlags::Asset))
    return false;

  const WUInt32 uiThumbnailSize = ThumbnailSize();
  QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(pEvent->position().toPoint()))
  {
    WUuid guid = index.data(WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();

    WTransformStatus ret = WAssetCurator::GetSingleton()->TransformAsset(guid, WTransformFlags::TriggeredManually);

    if (ret.Failed())
    {
      QString path = index.data(WQtAssetBrowserModel::UserRoles::RelativePath).toString();
      WLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, path.toUtf8().data());
    }
    else
    {
      WAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }

    pEvent->accept();
    return true;
  }
  return false;
}

QWidget* WQtIconViewDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  WStringBuilder sAbsPath = index.data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().constData();

  QLineEdit* editor = new QLineEdit(pParent);
  editor->setValidator(new WFileNameValidator(editor, sAbsPath.GetFileDirectory(), sAbsPath.GetFileNameAndExtension()));
  return editor;
}

void WQtIconViewDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
  QString sOldName = index.data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
  QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor);
  pModel->setData(index, pLineEdit->text());
}

void WQtIconViewDelegate::updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (!pEditor)
    return;

  const WUInt32 uiThumbnailSize = ThumbnailSize();
  const QRect textRect = option.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing, -ItemSideMargin, -ItemSideMargin - TextSpacing);
  pEditor->setGeometry(textRect);
}

void WQtIconViewDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (!IsInIconMode())
  {
    WQtItemDelegate::paint(pPainter, opt, index);
    return;
  }

  const WUInt32 uiThumbnailSize = ThumbnailSize();
  const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  // Prepare painter.
  {
    pPainter->save();
    if (hasClipping())
      pPainter->setClipRect(opt.rect);

    pPainter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  }

  // Draw assets with a background to distinguish them easily from normal files / folders.
  if (itemType.IsAnySet(WAssetBrowserItemFlags::Asset | WAssetBrowserItemFlags::SubAsset))
  {
    QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
      cg = QPalette::Inactive;

    WInt32 border = ItemSideMargin - HighlightBorderWidth;
    QRect assetRect = opt.rect.adjusted(border, border, -border, -border);
    pPainter->fillRect(assetRect, opt.palette.brush(cg, QPalette::AlternateBase));
  }

  // Draw highlight background (copy of QItemDelegate::drawBackground)
  {
    QRect highlightRect = opt.rect.adjusted(ItemSideMargin - HighlightBorderWidth, ItemSideMargin - HighlightBorderWidth, 0, 0);
    highlightRect.setHeight(uiThumbnailSize + 2 * HighlightBorderWidth);
    highlightRect.setWidth(uiThumbnailSize + 2 * HighlightBorderWidth);

    if ((opt.state & QStyle::State_Selected))
    {
      QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

      pPainter->fillRect(highlightRect, opt.palette.brush(cg, QPalette::Highlight));
    }
    else
    {
      QVariant value = index.data(Qt::BackgroundRole);
      if (value.canConvert<QBrush>())
      {
        QPointF oldBO = pPainter->brushOrigin();
        pPainter->setBrushOrigin(highlightRect.topLeft());
        pPainter->fillRect(highlightRect, qvariant_cast<QBrush>(value));
        pPainter->setBrushOrigin(oldBO);
      }
    }
  }

  if (itemType.IsAnySet(WAssetBrowserItemFlags::File) && !itemType.IsAnySet(WAssetBrowserItemFlags::Asset))
  {
    // Draw thumbnail.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
      icon.paint(pPainter, thumbnailRect);
    }

    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));
      QIcon icon = qvariant_cast<QIcon>(index.data(WQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }
  }
  else if (itemType.IsAnySet(WAssetBrowserItemFlags::Folder | WAssetBrowserItemFlags::DataDirectory))
  {
    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QIcon icon = qvariant_cast<QIcon>(index.data(WQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }
  }
  else // asset
  {
    // Draw thumbnail.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
      pPainter->drawPixmap(thumbnailRect, pixmap);
    }

    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));
      QIcon icon = qvariant_cast<QIcon>(index.data(WQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }

    // Draw Transform State Icon
    if (m_bDrawTransformState)
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));

      WAssetInfo::TransformState state = (WAssetInfo::TransformState)index.data(WQtAssetBrowserModel::UserRoles::TransformState).toInt();

      switch (state)
      {
        case WAssetInfo::TransformState::Unknown:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetUnknown.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::NeedsThumbnail:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsThumbnail.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::NeedsTransform:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::UpToDate:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetOk.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::MissingTransformDependency:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingDependency.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::MissingPackageDependency:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingDependency.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::MissingThumbnailDependency:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingReference.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::CircularDependency:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::TransformError:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::NeedsImport:
          WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsImport.svg").paint(pPainter, thumbnailRect);
          break;
        case WAssetInfo::TransformState::COUNT:
          break;
      }
    }
  }

  // Draw caption.
  {
    pPainter->setFont(GetFont());
    QRect textRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing, -ItemSideMargin, -ItemSideMargin - TextSpacing);

    QString caption = qvariant_cast<QString>(index.data(Qt::DisplayRole));
    pPainter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, caption);
  }


  pPainter->restore();
}

QSize WQtIconViewDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (IsInIconMode())
  {
    return ItemSize();
  }
  else
  {
    return WQtItemDelegate::sizeHint(opt, index);
  }
}

QSize WQtIconViewDelegate::ItemSize() const
{
  QFont font = GetFont();
  QFontMetrics fm(font);

  WUInt32 iThumbnail = ThumbnailSize();
  const WUInt32 iItemWidth = iThumbnail + 2 * ItemSideMargin;
  const WUInt32 iItemHeight = iThumbnail + 2 * (ItemSideMargin + fm.height() + TextSpacing);

  return QSize(iItemWidth, iItemHeight);
}

QFont WQtIconViewDelegate::GetFont() const
{
  QFont font = QApplication::font();

  float fScaleFactor = WMath::Clamp((1.0f + (m_iIconSizePercentage / 100.0f)) * 0.75f, 0.75f, 1.25f);

  font.setPointSizeF(font.pointSizeF() * fScaleFactor);
  return font;
}

WUInt32 WQtIconViewDelegate::ThumbnailSize() const
{
  return static_cast<WUInt32>((float)MaxSize * (float)m_iIconSizePercentage / 100.0f);
}

bool WQtIconViewDelegate::IsInIconMode() const
{
  return m_pView->viewMode() == QListView::ViewMode::IconMode;
}
