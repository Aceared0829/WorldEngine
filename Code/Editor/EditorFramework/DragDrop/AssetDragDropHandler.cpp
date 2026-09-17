#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DragDrop/AssetDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetDragDropHandler, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

bool WAssetDragDropHandler::IsAssetType(const WDragDropInfo* pInfo) const
{
  return pInfo->m_pMimeData->hasFormat("application/WEditor.AssetGuid");
}

WString WAssetDragDropHandler::GetAssetGuidString(const WDragDropInfo* pInfo) const
{
  QByteArray ba = pInfo->m_pMimeData->data("application/WEditor.AssetGuid");
  QDataStream stream(&ba, QIODevice::ReadOnly);

  WTempHybridArray<QString, 1> guids;
  stream >> guids;

  if (guids.GetCount() > 1)
  {
    WLog::Warning("Dragging more than one asset type is currently not supported");
  }

  return guids[0].toUtf8().data();
}

WString WAssetDragDropHandler::GetAssetsDocumentTypeName(const WUuid& assetTypeGuid) const
{
  return WAssetCurator::GetSingleton()->GetSubAsset(assetTypeGuid)->m_Data.m_sSubAssetsDocumentTypeName.GetData();
}

bool WAssetDragDropHandler::IsSpecificAssetType(const WDragDropInfo* pInfo, const char* szType) const
{
  if (!IsAssetType(pInfo))
    return false;

  const WUuid guid = GetAssetGuid(pInfo);

  return GetAssetsDocumentTypeName(guid) == szType;
}
