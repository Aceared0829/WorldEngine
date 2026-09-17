#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>

class WTextureAssetProfileConfig;

struct WTextureChannelMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    RGBA,
    RGB,
    Red,
    Green,
    Blue,
    Alpha,
    CoverageRed,
    CoverageAlpha,

    Default = RGBA
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINASSETS_DLL, WTextureChannelMode);

class WTextureAssetDocument : public WSimpleAssetDocument<WTextureAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WTextureAssetDocument, WSimpleAssetDocument<WTextureAssetProperties>);

public:
  WTextureAssetDocument(WStringView sDocumentPath);

  // for previewing purposes
  WEnum<WTextureChannelMode> m_ChannelMode;
  WInt32 m_iTextureLod = -1; // -1 == regular sampling, >= 0 == sample that level
  bool m_bIsRenderTarget = false;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override { return WStatus(W_SUCCESS); }
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  WStatus RunTexConv(const char* szTargetFile, const WAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const WTextureAssetProfileConfig* pAssetConfig);

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class WTextureAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WTextureAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WTextureAssetDocumentGenerator();
  ~WTextureAssetDocumentGenerator();

  enum class TextureType
  {
    Diffuse,
    NormalDX,
    NormalGL,
    Occlusion,
    Roughness,
    Metalness,
    ORM,
    Height,
    HDR,
    Linear,
  };

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WTextureAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Images"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;

  static TextureType DetermineTextureType(WStringView sFile);
};
