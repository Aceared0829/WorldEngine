#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSurfaceAssetDocument, 3, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WSurfaceAssetDocument::WSurfaceAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WSurfaceResourceDescriptor>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

WTransformStatus WSurfaceAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
  const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const WSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return WStatus(W_SUCCESS);
}
