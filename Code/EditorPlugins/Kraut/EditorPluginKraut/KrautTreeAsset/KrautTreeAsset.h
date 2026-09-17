#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

struct WKrautTreeResourceDescriptor;
struct WKrautGeneratorResourceDescriptor;

namespace WModelImporter2
{
  enum class TextureSemantic : WInt8;
}

struct WKrautTreeAssetEvent
{
  enum class Type
  {
    WindStrengthChanged,
    FrondsLeavesVisibilityChanged,
  };

  Type m_Type;
};

class WKrautTreeAssetDocument : public WSimpleAssetDocument<WKrautTreeAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WKrautTreeAssetDocument, WSimpleAssetDocument<WKrautTreeAssetProperties>);

public:
  WKrautTreeAssetDocument(WStringView sDocumentPath);

  WStatus WriteKrautAsset(WStreamWriter& ref_stream) const;

  WKrautWindStrength::Enum GetWindStrength() const { return m_WindStrength; }
  void SetWindStrength(WKrautWindStrength::Enum strength);

  bool GetShowFrondsLeaves() const { return m_bShowFrondsLeaves; }
  void SetShowFrondsLeaves(bool bShow);

  WEvent<const WKrautTreeAssetEvent&> m_Events;

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  void SyncBackAssetProperties(WKrautTreeAssetProperties*& pProp, const WKrautGeneratorResourceDescriptor& desc);

  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  WKrautWindStrength::Enum m_WindStrength = WKrautWindStrength::Light;
  bool m_bShowFrondsLeaves = true;
};

//////////////////////////////////////////////////////////////////////////


class WKrautTreeAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WKrautTreeAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WKrautTreeAssetDocumentGenerator();
  ~WKrautTreeAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WKrautTreeAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "KrautTrees"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
