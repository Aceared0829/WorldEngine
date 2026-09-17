#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

//////////////////////////////////////////////////////////////////////////

struct WPropertyMetaStateEvent;
class WSkeletonAssetDocument;

struct WSkeletonAssetEvent
{
  enum Type
  {
    RenderStateChanged,
    Transformed,
  };

  WSkeletonAssetDocument* m_pDocument = nullptr;
  Type m_Type;
};

class WSkeletonAssetDocument : public WSimpleAssetDocument<WEditableSkeleton>
{
  W_ADD_DYNAMIC_REFLECTION(WSkeletonAssetDocument, WSimpleAssetDocument<WEditableSkeleton>);

public:
  WSkeletonAssetDocument(WStringView sDocumentPath);
  ~WSkeletonAssetDocument();

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  WStatus WriteResource(WStreamWriter& inout_stream, const WEditableSkeleton& skeleton, WUInt16* out_pNumBones = nullptr) const;

  bool m_bIsTransforming = false;

  virtual WManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return WManipulatorSearchStrategy::SelectedObject;
  }

  const WEvent<const WSkeletonAssetEvent&>& Events() const { return m_Events; }

  void SetRenderBones(bool bEnable);
  bool GetRenderBones() const { return m_bRenderBones; }

  void SetRenderColliders(bool bEnable);
  bool GetRenderColliders() const { return m_bRenderColliders; }

  void SetRenderJoints(bool bEnable);
  bool GetRenderJoints() const { return m_bRenderJoints; }

  void SetRenderSwingLimits(bool bEnable);
  bool GetRenderSwingLimits() const { return m_bRenderSwingLimits; }

  void SetRenderTwistLimits(bool bEnable);
  bool GetRenderTwistLimits() const { return m_bRenderTwistLimits; }

  void SetRenderPreviewMesh(bool bEnable);
  bool GetRenderPreviewMesh() const { return m_bRenderPreviewMesh; }

protected:
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  const WEditableSkeleton* MergeWithNewSkeleton(WEditableSkeleton& newSkeleton);

  /// Whether merging in newSkeleton would change the joint hierarchy stored in this document.
  bool WouldSkeletonHierarchyChange(const WEditableSkeleton& newSkeleton) const;

  WEvent<const WSkeletonAssetEvent&> m_Events;
  bool m_bRenderBones = true;
  bool m_bRenderColliders = true;
  bool m_bRenderJoints = false; // currently not exposed
  bool m_bRenderSwingLimits = true;
  bool m_bRenderTwistLimits = true;
  bool m_bRenderPreviewMesh = true;
};

//////////////////////////////////////////////////////////////////////////

class WSkeletonAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WSkeletonAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WSkeletonAssetDocumentGenerator();
  ~WSkeletonAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WSkeletonAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
