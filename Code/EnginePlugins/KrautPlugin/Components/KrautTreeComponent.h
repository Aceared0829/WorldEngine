#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

struct WMsgExtractGeometry;
struct WMsgBuildStaticMesh;
struct WResourceEvent;
class WKrautRenderData;
class WAbstractObjectNode;

using WKrautTreeResourceHandle = WTypedResourceHandle<class WKrautTreeResource>;
using WKrautGeneratorResourceHandle = WTypedResourceHandle<class WKrautGeneratorResource>;

/// Component manager for WKrautTreeComponent.
///
/// Drives per-frame LOD updates and wind simulation for all active tree components.
class W_KRAUTPLUGIN_DLL WKrautTreeComponentManager : public WComponentManager<class WKrautTreeComponent, WBlockStorageType::Compact>
{
public:
  using SUPER = WComponentManager<WKrautTreeComponent, WBlockStorageType::Compact>;

  WKrautTreeComponentManager(WWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const WWorldModule::UpdateContext& context);
  void UpdateWind(const WWorldModule::UpdateContext& context);
  void EnqueueUpdate(WComponentHandle hComponent);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  mutable WMutex m_Mutex;
  WDeque<WComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

/// Instantiates a Kraut tree model.
///
/// References an WKrautGeneratorResource and selects a random seed to determine the tree's
/// visual variation. The component requests LOD meshes on demand via the generator resource
/// and renders the tree using the appropriate LOD for the current camera distance.
///
/// Seed selection priority (highest to lowest):
///   1. CustomRandomSeed — if set, always uses this exact seed.
///   2. VariationIndex — selects from the generator's curated "good seeds" list.
///   3. Owner object's stable random seed — used when neither override is set.
///
/// The local bounds are scaled by s_iLocalBoundsScale to give the renderer early visibility
/// even when only a rough bounding box is available before full mesh generation.
class W_KRAUTPLUGIN_DLL WKrautTreeComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WKrautTreeComponent, WRenderComponent, WKrautTreeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

protected:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // WKrautTreeComponent

public:
  WKrautTreeComponent();
  ~WKrautTreeComponent();

  // see WKrautTreeComponent::GetLocalBounds for details
  static const int s_iLocalBoundsScale = 3;

  /// Currently this adds a cylinder mesh as a rough approximation for the tree collision shape.
  void OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const;
  /// Currently this adds a cylinder mesh as a rough approximation for the tree collision shape.
  void OnBuildStaticMesh(WMsgBuildStaticMesh& ref_msg) const;

  /// Selects a variation from the generator resource's curated "good seeds" list.
  ///
  /// In the tree editor, designers can add "good seeds", i.e. seed values that produce nice results.
  /// Using the variation index you can select one of those good seeds.
  ///
  /// VariationIndex and CustomRandomSeed are mutually exclusive.
  /// If neither is set, a random variation is used, using the owner object's stable random seed.
  /// This is the preferred method to place trees and get a good random set, but requires that a tree model has defined "good seeds".
  void SetVariationIndex(WUInt16 uiIndex); // [ property ]
  WUInt16 GetVariationIndex() const;       // [ property ]

  /// Overrides the seed used for tree generation with a fixed value.
  ///
  /// Trees with the same random seed look identical; different seeds produce different trees.
  /// Mutually exclusive with SetVariationIndex().
  void SetCustomRandomSeed(WUInt16 uiSeed); // [ property ]
  WUInt16 GetCustomRandomSeed() const;      // [ property ]

  /// Sets the Kraut resource that is used to generate the tree mesh.
  void SetKrautGeneratorResource(const WKrautGeneratorResourceHandle& hTree);                          // [ property ]
  const WKrautGeneratorResourceHandle& GetKrautGeneratorResource() const { return m_hKrautGenerator; } // [ property ]

  // Development options for the Kraut asset preview
  WInt8 m_iLodOverride = -1;             ///< When >= 0, forces a specific LOD index regardless of camera distance. -1 = automatic.
  bool m_bHideFrondsAndLeafs = false;     ///< When true, frond and leaf sub-meshes are skipped during rendering.
  bool m_bForceGenerateImmediate = false; ///< When true, LOD generation runs synchronously instead of via background tasks.

  const WKrautTreeResourceHandle& GetKrautTreeResource() const { return m_hKrautTree; }

private:
  /// Currently this adds a cylinder mesh as a rough approximation of the tree trunk for collision.
  WResult CreateGeometry(WGeometry& geo, WWorldGeoExtractionUtil::ExtractionMode mode) const;
  void EnsureTreeIsGenerated();

  WUInt16 m_uiVariationIndex = 0xFFFF;
  WUInt16 m_uiCustomRandomSeed = 0xFFFF;
  WUInt32 m_uiCurrentSeed = 0;

  /// The LOD index rendered in the most recent frame, or -1 if nothing has been rendered yet.
  /// Used by EnsureTreeIsGenerated() to delay switching to a regenerated tree until that LOD is ready,
  /// so the old tree continues to render without flickering.
  mutable WInt8 m_iLastRenderedLod = -1;

  WKrautTreeResourceHandle m_hKrautTree;
  WKrautGeneratorResourceHandle m_hKrautGenerator;

  void ComputeWind();

  WVec3 m_vWindSpringPos;
  WVec3 m_vWindSpringVel;

  mutable WInstanceDataOffset m_InstanceDataOffset;
};
