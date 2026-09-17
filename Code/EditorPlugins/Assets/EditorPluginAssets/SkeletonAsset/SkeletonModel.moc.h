#pragma once

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WSkeletonAssetDocument;

class WQtJointAdapter : public WQtNamedAdapter
{
  Q_OBJECT;

public:
  WQtJointAdapter(const WSkeletonAssetDocument* pDocument);
  ~WQtJointAdapter();
  virtual QVariant data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;

private:
  const WSkeletonAssetDocument* m_pDocument;
};
