#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Math/Vec2.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

class WGeometry;
struct WMsgExtractRenderData;
struct WMsgExtractGeometry;
class WHeightfieldComponent;
class WMeshResourceDescriptor;

using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;
using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WImageDataResourceHandle = WTypedResourceHandle<class WImageDataResource>;

class W_GAMECOMPONENTS_DLL WHeightfieldComponentManager : public WComponentManager<WHeightfieldComponent, WBlockStorageType::Compact>
{
public:
  WHeightfieldComponentManager(WWorld* pWorld);
  ~WHeightfieldComponentManager();

  virtual void Initialize() override;

  void Update(const WWorldModule::UpdateContext& context);
  void AddToUpdateList(WHeightfieldComponent* pComponent);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  WDeque<WComponentHandle> m_ComponentsToUpdate;
};

/// This component utilizes a greyscale image to generate an elevation mesh, which is typically used for simple terrain
///
/// The component always creates a mesh for rendering, which uses a single material.
/// For different layers of grass, dirt, etc. the material can combine multiple textures and a mask.
///
/// If the "GenerateCollision" property is set, the component also generates a static collision mesh during scene export.
class W_GAMECOMPONENTS_DLL WHeightfieldComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WHeightfieldComponent, WRenderComponent, WHeightfieldComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent
protected:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // WHeightfieldComponent

public:
  WHeightfieldComponent();
  ~WHeightfieldComponent();

  WVec2 GetHalfExtents() const { return m_vHalfExtents; }       // [ property ]
  void SetHalfExtents(WVec2 value);                             // [ property ]

  float GetHeight() const { return m_fHeight; }                  // [ property ]
  void SetHeight(float value);                                   // [ property ]

  WVec2 GetTexCoordOffset() const { return m_vTexCoordOffset; } // [ property ]
  void SetTexCoordOffset(WVec2 value);                          // [ property ]

  WVec2 GetTexCoordScale() const { return m_vTexCoordScale; }   // [ property ]
  void SetTexCoordScale(WVec2 value);                           // [ property ]

  void SetMaterial(const WMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  WMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  void SetHeightfield(const WImageDataResourceHandle& hResource);                   // [ property ]
  const WImageDataResourceHandle& GetHeightfield() const { return m_hHeightfield; } // [ property ]

  WVec2U32 GetTesselation() const { return m_vTesselation; }                        // [ property ]
  void SetTesselation(WVec2U32 value);                                              // [ property ]

  void SetGenerateCollision(bool b);                                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; }                 // [ property ]

  WVec2U32 GetColMeshTesselation() const { return m_vColMeshTesselation; }          // [ property ]
  void SetColMeshTesselation(WVec2U32 value);                                       // [ property ]

protected:
  void OnMsgExtractGeometry(WMsgExtractGeometry& msg) const;                        // [ msg handler ]

  void InvalidateMesh();
  void PushHeightfieldCollider();
  void BuildGeometry(WGeometry& geom) const;
  WResult BuildMeshDescriptor(WMeshResourceDescriptor& desc) const;

  template <typename ResourceType>
  WTypedResourceHandle<ResourceType> GenerateMesh() const;

  WUInt32 m_uiHeightfieldChangeCounter = 0;
  WImageDataResourceHandle m_hHeightfield;
  WMaterialResourceHandle m_hMaterial;

  WVec2 m_vHalfExtents = WVec2(100.0f);
  float m_fHeight = 50.0f;

  WVec2 m_vTexCoordOffset = WVec2::MakeZero();
  WVec2 m_vTexCoordScale = WVec2(1);

  WVec2U32 m_vTesselation = WVec2U32(128);
  WVec2U32 m_vColMeshTesselation = WVec2U32(0); ///< 0 means "use the same resolution as the render mesh"

  bool m_bGenerateCollision = true;

  WMeshResourceHandle m_hMesh;

  mutable WInstanceDataOffset m_InstanceDataOffset;
};
