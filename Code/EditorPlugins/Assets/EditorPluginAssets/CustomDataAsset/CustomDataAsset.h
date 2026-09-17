#pragma once

#include <Core/Utils/CustomData.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class WCustomDataAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WCustomDataAssetProperties, WReflectedClass);

public:
  WCustomData* m_pType = nullptr;
};


class WCustomDataAssetDocument : public WSimpleAssetDocument<WCustomDataAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WCustomDataAssetDocument, WSimpleAssetDocument<WCustomDataAssetProperties>);

public:
  WCustomDataAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};
