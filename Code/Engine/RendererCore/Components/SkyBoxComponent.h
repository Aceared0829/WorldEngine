#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

using WSkyBoxComponentManager = WComponentManager<class WSkyBoxComponent, WBlockStorageType::Compact>;
using WTextureCubeResourceHandle = WTypedResourceHandle<class WTextureCubeResource>;

/// Adds a static image of a sky to the scene.
///
/// This is used to fill the scene background with a picture of a sky.
/// The sky image comes from a cubemap texture.
///
/// Position and scale of the game object are irrelevant, the sky always appears behind all other objects.
/// The rotation, however, is used to rotate the sky image.
class W_RENDERERCORE_DLL WSkyBoxComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WSkyBoxComponent, WRenderComponent, WSkyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WSkyBoxComponent

public:
  WSkyBoxComponent();
  ~WSkyBoxComponent();

  /// Changes the brightness of the sky image. Mainly useful when an HDR skybox is used.
  void SetExposureBias(float fExposureBias);                // [ property ]
  float GetExposureBias() const { return m_fExposureBias; } // [ property ]

  /// For HDR skyboxes this should stay off. For LDR skyboxes, enabling this will improve brightness and contrast.
  void SetInverseTonemap(bool bInverseTonemap);                // [ property ]
  bool GetInverseTonemap() const { return m_bInverseTonemap; } // [ property ]

  /// Enables that fog is applied to the sky. See SetVirtualDistance().
  void SetUseFog(bool bUseFog);                // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  /// If fog is enabled, the virtual distance is used to determine how foggy the sky should be.
  void SetVirtualDistance(float fVirtualDistance);                // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

  // adds SetCubeMapFile() and GetCubeMapFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(CubeMap, m_hCubeMap, SetCubeMap);

  void SetCubeMap(const WTextureCubeResourceHandle& hCubeMap); // [ property ]
  const WTextureCubeResourceHandle& GetCubeMap() const;        // [ property ]

private:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void UpdateMaterials();

  float m_fExposureBias = 0.0f;
  float m_fVirtualDistance = 1000.0f;
  bool m_bInverseTonemap = false;
  bool m_bUseFog = true;

  WTextureCubeResourceHandle m_hCubeMap;

  WMeshResourceHandle m_hMesh;
  WMaterialResourceHandle m_hCubeMapMaterial;

  mutable WInstanceDataOffset m_InstanceDataOffset;
};
