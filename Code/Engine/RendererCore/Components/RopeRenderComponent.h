#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/SkinnedMeshRenderData.h>

struct WMsgExtractRenderData;
struct WMsgSetColor;
struct WMsgSetMeshMaterial;
struct WMsgRopePoseUpdated;
class WShaderTransform;

using WRopeRenderComponentManager = WComponentManager<class WRopeRenderComponent, WBlockStorageType::Compact>;

/// Used to render a rope or cable.
///
/// This is needed to visualize the WFakeRopeComponent or WJoltRopeComponent.
/// The component handles the message WMsgRopePoseUpdated to generate an animated mesh and apply the pose.
/// The component has to be attached to the same object as the rope simulation component.
class W_RENDERERCORE_DLL WRopeRenderComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WRopeRenderComponent, WRenderComponent, WRopeRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

protected:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const; // [ msg handler ]

  //////////////////////////////////////////////////////////////////////////
  // WRopeRenderComponent

public:
  WRopeRenderComponent();
  ~WRopeRenderComponent();

  WColor m_Color = WColor::White;                                                        // [ property ]


  void SetMaterial(const WMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; } // [ property ]
  const WMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }              // [ property ]

  /// Changes how thick the rope visualization is. This is independent of the simulated rope thickness.
  void SetThickness(float fThickness);                // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  /// Sets how round the rope shall be.
  void SetDetail(WUInt32 uiDetail);                // [ property ]
  WUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  /// If enabled, the rendered mesh will be slightly more detailed along the rope.
  void SetSubdivide(bool bSubdivide);                // [ property ]
  bool GetSubdivide() const { return m_bSubdivide; } // [ property ]

  /// How often to repeat the U texture coordinate along the rope's length.
  void SetUScale(float fUScale);                                                        // [ property ]
  float GetUScale() const { return m_fUScale; }                                         // [ property ]

  void OnMsgSetColor(WMsgSetColor& ref_msg);                                           // [ msg handler ]
  void OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg);                             // [ msg handler ]

private:
  void OnRopePoseUpdated(WMsgRopePoseUpdated& msg);                                    // [ msg handler ]
  void OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& msg); // [ msg handler ]

  void GenerateRenderMesh(WUInt32 uiNumRopePieces);
  void UpdateSkinningTransformBuffer(WArrayPtr<const WTransform> skinningTransforms);

  WBoundingBoxSphere m_LocalBounds;

  WSkinningState m_SkinningState;

  WMeshResourceHandle m_hMesh;
  WMaterialResourceHandle m_hMaterial;

  float m_fThickness = 0.05f;
  WUInt32 m_uiDetail = 6;
  bool m_bSubdivide = false;

  float m_fUScale = 1.0f;

  mutable WInstanceDataOffset m_InstanceDataOffset;
};
