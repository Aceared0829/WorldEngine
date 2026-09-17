#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class WAssetFileHeader;
struct WPropertyMetaStateEvent;

struct WDecalMode
{
  using StorageType = WInt8;

  enum Enum
  {
    BaseColor,
    BaseColorNormal,
    BaseColorORM,
    BaseColorNormalORM,
    BaseColorEmissive,

    Default = BaseColor
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WDecalMode);

class WDecalAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDecalAssetProperties, WReflectedClass);

public:
  WDecalAssetProperties();

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  WEnum<WDecalMode> m_Mode;
  bool m_bBlendModeColorize = false;

  WString m_sAlphaMask;
  WString m_sBaseColor;
  WString m_sNormal;
  WString m_sORM;
  WString m_sEmissive;

  /// Into how many columns and rows the input textures are subdivided.
  ///
  /// Every cell must contain an independent variation of the decal. All input textures (base color, normal, ...)
  /// must use the same subdivision. The texture is packed into the decal atlas as a whole, an WDecalComponent
  /// then displays only a single cell.
  WUInt8 m_uiNumVariationsX = 1;
  WUInt8 m_uiNumVariationsY = 1;

  /// If no base color texture is given, an opaque white one is generated, so that the alpha mask alone defines the decal.
  bool NeedsBaseColor() const { return !m_sBaseColor.IsEmpty(); }
  bool NeedsNormal() const { return m_Mode == WDecalMode::BaseColorNormal || m_Mode == WDecalMode::BaseColorNormalORM; }
  bool NeedsORM() const { return m_Mode == WDecalMode::BaseColorORM || m_Mode == WDecalMode::BaseColorNormalORM; }
  bool NeedsEmissive() const { return m_Mode == WDecalMode::BaseColorEmissive; }
};


class WDecalAssetDocument : public WSimpleAssetDocument<WDecalAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WDecalAssetDocument, WSimpleAssetDocument<WDecalAssetProperties>);

public:
  WDecalAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class WDecalAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WDecalAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WDecalAssetDocumentGenerator();
  ~WDecalAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WDecalAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Images"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
