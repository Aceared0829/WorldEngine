#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class WSoundEventAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSoundEventAssetProperties, WReflectedClass);

public:
  WSoundEventAssetProperties() = default;
};


class WSoundEventAssetDocument : public WSimpleAssetDocument<WSoundEventAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WSoundEventAssetDocument, WSimpleAssetDocument<WSoundEventAssetProperties>);

public:
  WSoundEventAssetDocument(WStringView sDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
};
