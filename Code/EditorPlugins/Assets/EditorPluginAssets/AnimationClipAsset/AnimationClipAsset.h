#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

class WAnimationClipAssetDocument;
struct WPropertyMetaStateEvent;

//////////////////////////////////////////////////////////////////////////

struct WRootMotionSource
{
  using StorageType = WUInt8;

  enum Enum
  {
    None,
    Constant,

    Default = None
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WRootMotionSource);

//////////////////////////////////////////////////////////////////////////

struct WAdditiveAnimationReference
{
  using StorageType = WUInt8;

  enum Enum
  {
    FirstKeyFrame,
    LastKeyFrame,

    Default = FirstKeyFrame
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WAdditiveAnimationReference);

//////////////////////////////////////////////////////////////////////////

/// Stores a single named float curve for use in an animation clip.
///
/// The color used for display in the editor is derived automatically from the name.
class W_NO_LINKAGE WAnimationClipCurveData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipCurveData, WReflectedClass);

public:
  WString m_sName;          ///< Identifies this curve across clips. Used to match values from multiple clips for blending.
  WSingleCurveData m_Curve; ///< The curve data. Color is overridden at edit time based on m_sName.
};

//////////////////////////////////////////////////////////////////////////

class WAnimationClipAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipAssetProperties, WReflectedClass);

public:
  WAnimationClipAssetProperties();
  ~WAnimationClipAssetProperties();

  WString m_sSourceFile;
  WString m_sAnimationClipToExtract;
  bool m_bAdditive = false;
  WUInt32 m_uiFirstFrame = 0;
  WUInt32 m_uiNumFrames = 0;
  WString m_sPreviewMesh;
  WString m_sPreviewAnim;
  WEnum<WRootMotionSource> m_RootMotionMode;
  WEnum<WAdditiveAnimationReference> m_AdditiveReference;
  WVec3 m_vConstantRootMotion;
  float m_fConstantRootMotionLength = 0.0f;
  float m_fAnimationPositionScale = 1.0f;

  WEventTrackData m_EventTrack;
  WDynamicArray<WAnimationClipCurveData> m_Curves;

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
};

//////////////////////////////////////////////////////////////////////////

class WAnimationClipAssetDocument : public WSimpleAssetDocument<WAnimationClipAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipAssetDocument, WSimpleAssetDocument<WAnimationClipAssetProperties>);

public:
  WAnimationClipAssetDocument(WStringView sDocumentPath);

  virtual void SetCommonAssetUiState(WCommonAssetUiState::Enum state, double value) override;
  virtual double GetCommonAssetUiState(WCommonAssetUiState::Enum state) const override;

  WUuid InsertEventTrackCpAt(WInt64 iTickX, const char* szValue);

  /// Fills the 'AnimationClipsInSourceFile' enum with the clips that the last transform found.
  ///
  /// Subscribed to WDynamicStringEnum::s_RefreshValuesEvent, so that the values match the document being shown.
  static void OnRefreshDynamicStringEnum(WDynamicStringEnum::RefreshValuesEvent& e);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  float m_fSimulationSpeed = 1.0f;
};

//////////////////////////////////////////////////////////////////////////

class WAnimationClipAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WAnimationClipAssetDocumentGenerator();
  ~WAnimationClipAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WAnimationClipAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual bool NeedsImport(WStringView sInputFileAbs, WStringView sMode) const override;
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
