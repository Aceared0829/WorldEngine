#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>

WQtJointAdapter::WQtJointAdapter(const WSkeletonAssetDocument* pDocument)
  : WQtNamedAdapter(pDocument->GetObjectManager(), WGetStaticRTTI<WEditableSkeletonJoint>(), "Children", "Name")
  , m_pDocument(pDocument)
{
}

WQtJointAdapter::~WQtJointAdapter() = default;

QVariant WQtJointAdapter::data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case Qt::DecorationRole:
    {
      QIcon icon = WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.svg"); // Giv ICon Plez!
      return icon;
    }
    break;
  }
  return WQtNamedAdapter::data(pObject, iRow, iColumn, iRole);
}
