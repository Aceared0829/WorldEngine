#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class WSoundBankAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSoundBankAssetProperties, WReflectedClass);

public:
  WSoundBankAssetProperties() = default;

  WString m_sSoundBank;
};

class WSoundBankAssetDocument : public WSimpleAssetDocument<WSoundBankAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WSoundBankAssetDocument, WSimpleAssetDocument<WSoundBankAssetProperties>);

public:
  WSoundBankAssetDocument(WStringView sDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
};
