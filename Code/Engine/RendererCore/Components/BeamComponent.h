#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

using WBeamComponentManager = WComponentManagerSimple<class WBeamComponent, WComponentUpdateType::Always>;

struct WMsgExtractRenderData;
class WGeometry;
class WMeshResourceDescriptor;

/// Renders a thick line from its own location to the position of another game object.
///
/// This is meant for simple effects, like laser beams. The geometry is very low resolution and won't look good close up.
/// When possible, use a highly emissive material without any pattern, where the bloom will hide the simple geometry.
///
/// For doing dynamic laser beams, you can combine it with the WRaycastComponent, which will move the target component.
class W_RENDERERCORE_DLL WBeamComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WBeamComponent, WRenderComponent, WBeamComponentManager);

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

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // WBeamComponent

public:
  WBeamComponent();
  ~WBeamComponent();

  /// Sets the GUID of the target object to which to draw the beam.
  void SetTargetObject(const char* szReference); // [ property ]

  /// How wide to make the beam geometry
  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  /// How many world units the texture coordinates should take up, for using a repeatable texture for the beam.
  void SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit); // [ property ]
  float GetUVUnitsPerWorldUnit() const;                    // [ property ]

  WMaterialResourceHandle GetMaterial() const;

  /// The object to which to draw the beam.
  WGameObjectHandle m_hTargetObject; // [ property ]

  /// Optional color to tint the beam.
  WColor m_Color = WColor::White; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  float m_fWidth = 0.1f;               // [ property ]
  float m_fUVUnitsPerWorldUnit = 1.0f; // [ property ]

  /// Which material asset to use for rendering the beam geometry.
  WMaterialResourceHandle m_hMaterial; // [ property ]

  const float m_fDistanceUpdateEpsilon = 0.02f;

  WMeshResourceHandle m_hMesh;

  WVec3 m_vLastOwnerPosition = WVec3::MakeZero();
  WVec3 m_vLastTargetPosition = WVec3::MakeZero();

  void CreateMeshes();
  void BuildMeshResourceFromGeometry(WGeometry& Geometry, WMeshResourceDescriptor& MeshDesc) const;
  void ReinitMeshes();
  void Cleanup();

  const char* DummyGetter() const { return nullptr; }

  mutable WInstanceDataOffset m_InstanceDataOffset;
};
