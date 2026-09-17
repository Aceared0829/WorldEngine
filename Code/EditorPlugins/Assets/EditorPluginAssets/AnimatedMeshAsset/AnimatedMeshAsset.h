#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

class WMeshResourceDescriptor;
class WMaterialAssetDocument;

class WAnimatedMeshAssetDocument : public WSimpleAssetDocument<WAnimatedMeshAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WAnimatedMeshAssetDocument, WSimpleAssetDocument<WAnimatedMeshAssetProperties>);

public:
  WAnimatedMeshAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  WStatus CreateMeshFromFile(WAnimatedMeshAssetProperties* pProp, WMeshResourceDescriptor& desc);

  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};
