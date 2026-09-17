#pragma once

#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginSubstance/EditorPluginSubstanceDLL.h>

struct WSubstanceUsage
{
  using StorageType = WUInt8;

  enum Enum
  {
    Unknown,

    BaseColor,
    Emissive,
    Height,
    Metallic,
    Mask,
    Normal,
    Occlusion,
    Opacity,
    Roughness,

    Count,

    Default = Unknown
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINSUBSTANCE_DLL, WSubstanceUsage);


struct WSubstanceGraphOutput
{
  bool m_bEnabled = true;
  WEnum<WTexConvCompressionMode> m_CompressionMode = WTexConvCompressionMode::High;
  WEnum<WSubstanceUsage> m_Usage;
  WUInt8 m_uiNumChannels = 1;
  WEnum<WTexConvMipmapMode> m_MipmapMode;
  bool m_bPreserveAlphaCoverage = false;
  WString m_sName;
  WString m_sLabel;
  WUuid m_Uuid;

  bool operator==(const WSubstanceGraphOutput& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_CompressionMode == other.m_CompressionMode &&
           m_Usage == other.m_Usage &&
           m_uiNumChannels == other.m_uiNumChannels &&
           m_MipmapMode == other.m_MipmapMode &&
           m_bPreserveAlphaCoverage == other.m_bPreserveAlphaCoverage &&
           m_sName == other.m_sName &&
           m_sLabel == other.m_sLabel &&
           m_Uuid == other.m_Uuid;
  }
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINSUBSTANCE_DLL, WSubstanceGraphOutput);


struct WSubstanceGraph
{
  bool m_bEnabled = true;

  WString m_sName;

  WUInt8 m_uiOutputWidth = 0;  ///< In base 2, e.g. 8 = 2^8 = 256
  WUInt8 m_uiOutputHeight = 0; ///< In base 2

  WHybridArray<WSubstanceGraphOutput, 8> m_Outputs;

  bool operator==(const WSubstanceGraph& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_sName == other.m_sName &&
           m_uiOutputWidth == other.m_uiOutputWidth &&
           m_uiOutputHeight == other.m_uiOutputHeight &&
           m_Outputs == other.m_Outputs;
  }
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINSUBSTANCE_DLL, WSubstanceGraph);


class WSubstancePackageAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSubstancePackageAssetProperties, WReflectedClass);

public:
  WSubstancePackageAssetProperties() = default;

  WString m_sSubstancePackage;
  WString m_sOutputPattern;

  WHybridArray<WSubstanceGraph, 2> m_Graphs;
};


class WSubstancePackageAssetMetaData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSubstancePackageAssetMetaData, WReflectedClass);

public:
  WDynamicArray<WUuid> m_OutputUuids;
  WDynamicArray<WString> m_OutputNames;
};

class WTextureAssetProfileConfig;

class WSubstancePackageAssetDocument : public WSimpleAssetDocument<WSubstancePackageAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WSubstancePackageAssetDocument, WSimpleAssetDocument<WSubstancePackageAssetProperties>);

public:
  WSubstancePackageAssetDocument(WStringView sDocumentPath);
  ~WSubstancePackageAssetDocument();

  // for previewing purposes
  WUuid m_SelectedOutput;
  WEnum<WTextureChannelMode> m_ChannelMode;
  WInt32 m_iTextureLod = -1; // -1 == regular sampling, >= 0 == sample that level

private:
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override { return WStatus(W_SUCCESS); }
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  void OnPropertyChanged(const WDocumentObjectPropertyEvent& e);

private:
  WResult GetTempDir(WStringBuilder& out_sTempDir) const;
  void GenerateOutputName(const WSubstanceGraph& graph, const WSubstanceGraphOutput& graphOutput, WStringBuilder& out_sOutputName) const;
  WTransformStatus UpdateGraphOutputs(WStringView sAbsolutePath, bool bAllowPropertyModifications);
  WStatus RunTexConv(const char* szInputFile, const char* szTargetFile, const WAssetFileHeader& assetHeader, const WSubstanceGraphOutput& graphOutput, WStringView sThumbnailFile, const WTextureAssetProfileConfig* pAssetConfig);
};
