#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WDragDropInfo;

/// Adapter that defines data for specific type in the WQtDocumentTreeModel.
///
/// Adapters are defined for a given type and define the property for child elements (needs to be array or set).
/// Furthermore they implement various model functions that will be redirected to it by the model for
/// objects of the given type.
class W_EDITORFRAMEWORK_DLL WQtDocumentTreeModelAdapter : public QObject
{
  Q_OBJECT;

public:
  /// Constructor. If m_sChildProperty is empty, this type does not have children.
  WQtDocumentTreeModelAdapter(const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty);
  virtual const WRTTI* GetType() const;
  virtual const WString& GetChildProperty() const;

  virtual QVariant data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole = Qt::DisplayRole) const = 0;
  virtual bool setData(const WDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const;
  virtual Qt::ItemFlags flags(const WDocumentObject* pObject, int iRow, int iColumn) const;

Q_SIGNALS:
  void dataChanged(const WDocumentObject* pObject, QVector<int> roles);

protected:
  const WDocumentObjectManager* m_pTree = nullptr;
  const WRTTI* m_pType = nullptr;
  WString m_sChildProperty;
};

/// Convenience class that returns the typename as Qt::DisplayRole.
/// Use this for testing or for the document root that can't be seen and is just for defining the hierarchy.
///
/// Example:
/// WQtDummyAdapter(pDocument->GetObjectManager(), WGetStaticRTTI<WDocumentRoot>(), "Children");
class W_EDITORFRAMEWORK_DLL WQtDummyAdapter : public WQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  WQtDummyAdapter(const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty);

  virtual QVariant data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
};

/// Convenience class that implements getting the name via a property on the object.
class W_EDITORFRAMEWORK_DLL WQtNamedAdapter : public WQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  WQtNamedAdapter(const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty, const char* szNameProperty);
  ~WQtNamedAdapter();
  virtual QVariant data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;

protected:
  virtual void TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e);

protected:
  WString m_sNameProperty;
};

/// Convenience class that implements setting the name via a property on the object.
class W_EDITORFRAMEWORK_DLL WQtNameableAdapter : public WQtNamedAdapter
{
  Q_OBJECT;

public:
  WQtNameableAdapter(const WDocumentObjectManager* pTree, const WRTTI* pType, const char* szChildProperty, const char* szNameProperty);
  ~WQtNameableAdapter();
  virtual bool setData(const WDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;
  virtual Qt::ItemFlags flags(const WDocumentObject* pObject, int iRow, int iColumn) const override;
};

/// Model that maps a document to a qt tree model.
///
/// Hierarchy is defined by WQtDocumentTreeModelAdapter that have to be added via AddAdapter.
class W_EDITORFRAMEWORK_DLL WQtDocumentTreeModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  WQtDocumentTreeModel(const WDocumentObjectManager* pTree, const WUuid& root = WUuid());
  ~WQtDocumentTreeModel();

  const WDocumentObjectManager* GetDocumentTree() const { return m_pDocumentTree; }
  /// Adds an adapter. There can only be one adapter for any object type.
  /// Added adapters are taken ownership of by the model.
  void AddAdapter(WQtDocumentTreeModelAdapter* pAdapter);
  /// Returns the QModelIndex for the given object.
  /// Returned value is invalid if object is not mapped in model.
  QModelIndex ComputeModelIndex(const WDocumentObject* pObject) const;

  /// Enable drag&drop support, disabled by default.
  void SetAllowDragDrop(bool bAllow);

  static bool MoveObjects(const WDragDropInfo& info);

  /// Returns the WDocumentObject that the index points to.
  const WDocumentObject* GetObject(const QModelIndex index) const;

public: // QAbstractItemModel
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& child) const override;

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

  virtual QVariant data(const QModelIndex& index, int iRole = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole) override;

  virtual Qt::DropActions supportedDropActions() const override;

  virtual bool canDropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent) const override;
  virtual bool dropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

protected:
  virtual void TreeEventHandler(const WDocumentObjectStructureEvent& e);

private:
  QModelIndex ComputeParent(const WDocumentObject* pObject) const;
  WInt32 ComputeIndex(const WDocumentObject* pObject) const;
  const WDocumentObject* GetRoot() const;
  bool IsUnderRoot(const WDocumentObject* pObject) const;

  const WQtDocumentTreeModelAdapter* GetAdapter(const WRTTI* pType) const;

protected:
  const WDocumentObjectManager* m_pDocumentTree = nullptr;
  const WUuid m_Root;
  WHashTable<const WRTTI*, WQtDocumentTreeModelAdapter*> m_Adapters;
  bool m_bAllowDragDrop = false;
  WString m_sTargetContext = "scenetree";
};
