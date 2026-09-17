#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSoundEventAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSoundEventAssetProperties, 1, WRTTIDefaultAllocator<WSoundEventAssetProperties>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSoundEventAssetDocument::WSoundEventAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WSoundEventAssetProperties>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

void WSoundEventAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);
}

WTransformStatus WSoundEventAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
  const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  return WStatus(W_SUCCESS);
}
