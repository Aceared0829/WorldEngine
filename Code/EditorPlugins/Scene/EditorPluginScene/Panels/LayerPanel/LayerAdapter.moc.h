#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>

class WScene2Document;
struct WScene2LayerEvent;
struct WDocumentEvent;

/// Custom adapter for layers, used in WQtLayerPanel.
class W_EDITORPLUGINSCENE_DLL WQtLayerAdapter : public WQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  WQtLayerAdapter(WScene2Document* pDocument);
  ~WQtLayerAdapter();
  virtual QVariant data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool setData(const WDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

  enum UserRoles
  {
    LayerGuid = Qt::UserRole + 0,
  };

private:
  void LayerEventHandler(const WScene2LayerEvent& e);
  void DocumentEventHander(const WDocumentEvent& e);

private:
  WScene2Document* m_pSceneDocument;
  WEvent<const WScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  WEvent<const WDocumentEvent&>::Unsubscriber m_DocumentEventUnsubscriber;
  WUuid m_CurrentActiveLayer;
};

/// Custom delegate for layers, used in WQtLayerPanel.
/// Provides buttons to toggle the layer visible / loaded states.
/// Relies on WQtLayerAdapter to trigger updates and provide the LayerGuid.
class WQtLayerDelegate : public WQtItemDelegate
{
  Q_OBJECT
public:
  WQtLayerDelegate(QObject* pParent, WScene2Document* pDocument);

  virtual bool mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseMoveEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  static QRect GetVisibleIconRect(const QStyleOptionViewItem& opt);
  static QRect GetLoadedIconRect(const QStyleOptionViewItem& opt);

  bool m_bPressed = false;
  WScene2Document* m_pDocument = nullptr;
};

/// Custom model for layers, used in WQtLayerPanel.
class WQtLayerModel : public WQtDocumentTreeModel
{
  Q_OBJECT

public:
  WQtLayerModel(WScene2Document* pDocument);
  ~WQtLayerModel() = default;

private:
  WScene2Document* m_pDocument = nullptr;
};
