#pragma once

#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <QItemDelegate>
#include <QListView>

class WQtIconViewDelegate;

class WQtAssetBrowserView : public WQtItemView<QListView>
{
  Q_OBJECT

public:
  WQtAssetBrowserView(QWidget* pParent);
  void SetDialogMode(bool bDialogMode);

  void SetIconMode(bool bIconMode);
  void SetIconScale(WInt32 iIconSizePercentage);
  WInt32 GetIconScale() const;

  void dragEnterEvent(QDragEnterEvent* pEvent) override;
  void dragMoveEvent(QDragMoveEvent* pEvent) override;
  void dragLeaveEvent(QDragLeaveEvent* pEvent) override;
  void dropEvent(QDropEvent* pEvent) override;
  void startDrag(Qt::DropActions supportedActions) override;

Q_SIGNALS:
  void ViewZoomed(WInt32 iIconSizePercentage);

protected:
  virtual void wheelEvent(QWheelEvent* pEvent) override;
  virtual void mousePressEvent(QMouseEvent* pEvent) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
  virtual void mouseMoveEvent(QMouseEvent* pEvent) override;

private:
  bool m_bDialogMode;
  WQtIconViewDelegate* m_pDelegate;
  WInt32 m_iIconSizePercentage;
};


class WQtIconViewDelegate : public WQtItemDelegate
{
  Q_OBJECT

public:
  WQtIconViewDelegate(WQtAssetBrowserView* pParent = nullptr);

  void SetDrawTransformState(bool b) { m_bDrawTransformState = b; }

  void SetIconScale(WInt32 iIconSizePercentage);

  virtual bool mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;


public:
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;
  virtual void updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
  QSize ItemSize() const;
  QFont GetFont() const;
  WUInt32 ThumbnailSize() const;
  bool IsInIconMode() const;

private:
  enum
  {
    MaxSize = WThumbnailSize,
    HighlightBorderWidth = 3,
    ItemSideMargin = 5,
    TextSpacing = 5
  };

  bool m_bDrawTransformState;
  WInt32 m_iIconSizePercentage;
  WQtAssetBrowserView* m_pView;
};
