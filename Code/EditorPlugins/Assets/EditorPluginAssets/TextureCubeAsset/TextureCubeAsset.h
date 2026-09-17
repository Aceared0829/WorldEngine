#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>

class WTextureCubeAssetDocument : public WSimpleAssetDocument<WTextureCubeAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WTextureCubeAssetDocument, WSimpleAssetDocument<WTextureCubeAssetProperties>);

public:
  WTextureCubeAssetDocument(WStringView sDocumentPath);

  // for previewing purposes
  WEnum<WTextureChannelMode> m_ChannelMode;
  WInt32 m_iTextureLod = -1; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override
  {
    return WStatus(W_SUCCESS);
  }
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  WStatus RunTexConv(const char* szTargetFile, const WAssetFileHeader& AssetHeader, bool bUpdateThumbnail);

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class WTextureCubeAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WTextureCubeAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WTextureCubeAssetDocumentGenerator();
  ~WTextureCubeAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WTextureCubeAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Images"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
