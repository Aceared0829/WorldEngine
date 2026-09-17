#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

struct WImageDataAssetEvent
{
  enum class Type
  {
    Transformed,
  };

  Type m_Type = Type::Transformed;
};

class WImageDataAssetDocument : public WSimpleAssetDocument<WImageDataAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WImageDataAssetDocument, WSimpleAssetDocument<WImageDataAssetProperties>);

public:
  WImageDataAssetDocument(WStringView sDocumentPath);

  const WEvent<const WImageDataAssetEvent&>& Events() const { return m_Events; }

protected:
  WEvent<const WImageDataAssetEvent&> m_Events;

  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override { return WStatus(W_SUCCESS); }
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  WStatus RunTexConv(const char* szTargetFile, const WAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};
