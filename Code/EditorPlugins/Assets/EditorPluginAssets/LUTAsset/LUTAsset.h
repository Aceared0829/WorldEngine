#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class WTextureAssetProfileConfig;

class WLUTAssetDocument : public WSimpleAssetDocument<WLUTAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WLUTAssetDocument, WSimpleAssetDocument<WLUTAssetProperties>);

public:
  WLUTAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override
  {
    return WStatus(W_SUCCESS);
  }
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class WLUTAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WLUTAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WLUTAssetDocumentGenerator();
  ~WLUTAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WLUTAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "LUTs"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
