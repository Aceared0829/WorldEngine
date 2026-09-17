#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

struct WMsgExtractGeometry;

using WLodMeshComponentManager = WComponentManager<class WLodMeshComponent, WBlockStorageType::Compact>;

struct WLodMeshLod
{
  WMeshResourceHandle m_hMesh; // [ property ]
  float m_fThreshold;           // [ property ]
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WLodMeshLod);

/// Renders one of several level-of-detail meshes depending on the distance to the camera.
///
/// This component is very similar to the WLodComponent, please read it's description for details.
/// The difference is, that this component doesn't switch child object on and off, but rather only selects between different render-meshes.
/// As such there is less performance impact for switching between meshes and also the memory overhead for storing LOD information is smaller.
/// If it is only desired to switch between meshes, it is also more convenient to work with just a single component.
///
/// The component does not allow to place the LOD meshes differently, they all need to have the same origin.
/// Compared with the regular WMeshComponent there is also no way to override the used materials, since each LOD mesh may use different materials.
class W_RENDERERCORE_DLL WLodMeshComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WLodMeshComponent, WRenderComponent, WLodMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WLodMeshComponent

public:
  WLodMeshComponent();
  ~WLodMeshComponent();

  /// An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const WColor& color); // [ property ]
  const WColor& GetColor() const;     // [ property ]

  /// An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const WVec4& vData); // [ property ]
  const WVec4& GetCustomData() const;     // [ property ]

  /// The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;       // [ property ]

  /// Enables text output to show the current coverage value and selected LOD.
  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  /// Disabling the LOD range overlap functionality can make it easier to determine the desired coverage thresholds.
  void SetOverlapRanges(bool bOverlap);                 // [ property ]
  bool GetOverlapRanges() const;                        // [ property ]

  void OnMsgSetColor(WMsgSetColor& ref_msg);           // [ msg handler ]
  void OnMsgSetCustomData(WMsgSetCustomData& ref_msg); // [ msg handler ]

  /// Provides the coarsest LOD that actually has a mesh, since the geometry is wanted for things like
  /// exporting the scene, where the close-up detail is not useful. Only answers a request for render
  /// geometry; collision geometry is expected to come from a dedicated collider component.
  void OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const; // [ msg handler ]

protected:
  void UpdateSelectedLod(const WView& view) const;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  WDynamicArray<WLodMeshLod> m_Meshes;
  WColor m_Color = WColor::White;
  WVec4 m_vCustomData = WVec4(0, 1, 0, 1);
  float m_fSortingDepthOffset = 0.0f;
  WVec3 m_vBoundsOffset = WVec3::MakeZero();
  float m_fBoundsRadius = 1.0f;

  mutable WInt32 m_iCurLod = 0;
  mutable WInstanceDataOffset m_InstanceDataOffset;
};
