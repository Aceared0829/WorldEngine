#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class WSceneDocument;


/// Custom delegate for game objects, used in WQtGameObjectWidget.
///
/// Renders additional icons to display stats.
class W_EDITORFRAMEWORK_DLL WQtGameObjectDelegate : public WQtItemDelegate
{
  Q_OBJECT
public:
  WQtGameObjectDelegate(QObject* pParent, WGameObjectDocument* pDocument);
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  static QRect GetHiddenIconRect(const QStyleOptionViewItem& opt);
  static QRect GetActiveParentIconRect(const QStyleOptionViewItem& opt);

  WGameObjectDocument* m_pDocument = nullptr;
};

class W_EDITORFRAMEWORK_DLL WQtGameObjectAdapter : public WQtNameableAdapter
{
  Q_OBJECT;

public:
  enum UserRoles
  {
    HiddenRole = Qt::UserRole + 0,
    ActiveParentRole = Qt::UserRole + 1,
  };

  WQtGameObjectAdapter(WDocumentObjectManager* pObjectManager, WObjectMetaData<WUuid, WDocumentObjectMetaData>* pObjectMetaData = nullptr, WObjectMetaData<WUuid, WGameObjectMetaData>* pGameObjectMetaData = nullptr);
  ~WQtGameObjectAdapter();
  virtual QVariant data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool setData(const WDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

public:
  void DocumentObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WDocumentObjectMetaData>::EventData& e);
  void GameObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WGameObjectMetaData>::EventData& e);

protected:
  WDocumentObjectManager* m_pObjectManager = nullptr;
  WGameObjectDocument* m_pGameObjectDocument = nullptr;
  WObjectMetaData<WUuid, WDocumentObjectMetaData>* m_pObjectMetaData = nullptr;
  WObjectMetaData<WUuid, WGameObjectMetaData>* m_pGameObjectMetaData = nullptr;
  WEventSubscriptionID m_GameObjectMetaDataSubscription;
  WEventSubscriptionID m_DocumentObjectMetaDataSubscription;
};

class W_EDITORFRAMEWORK_DLL WQtGameObjectModel : public WQtDocumentTreeModel
{
  Q_OBJECT

public:
  WQtGameObjectModel(const WDocumentObjectManager* pObjectManager, const WUuid& root = WUuid());
  ~WQtGameObjectModel();
};
