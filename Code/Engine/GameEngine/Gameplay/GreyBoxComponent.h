#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>

class WMeshRenderData;
class WGeometry;
struct WMsgExtractRenderData;
struct WMsgBuildStaticMesh;
struct WMsgExtractGeometry;
struct WMsgExtractOccluderData;
struct WMsgSetMeshMaterial;
struct WMsgSetColor;
class WMeshResourceDescriptor;
struct WMsgSetCustomData;
using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;
using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;

using WGreyBoxComponentManager = WComponentManager<class WGreyBoxComponent, WBlockStorageType::Compact>;

struct W_GAMEENGINE_DLL WGreyBoxShape
{
  using StorageType = WUInt8;

  enum Enum
  {
    Box,
    RampPosX,
    RampNegX,
    RampPosY,
    RampNegY,
    Column,
    StairsPosX,
    StairsNegX,
    StairsPosY,
    StairsNegY,
    ArchX,
    ArchY,
    SpiralStairs,

    Default = Box
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WGreyBoxShape)

/// Creates basic geometry for prototyping levels.
///
/// It automatically creates physics collision geometry and also sets up rendering occluders to improve performance.
class W_GAMEENGINE_DLL WGreyBoxComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WGreyBoxComponent, WRenderComponent, WGreyBoxComponentManager);

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
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // WGreyBoxComponent

public:
  WGreyBoxComponent();
  ~WGreyBoxComponent();

  /// The geometry type to build.
  void SetShape(WEnum<WGreyBoxShape> shape);                // [ property ]
  WEnum<WGreyBoxShape> GetShape() const { return m_Shape; } // [ property ]

  /// An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const WColor& color); // [ property ]
  const WColor& GetColor() const;     // [ property ]

  /// An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const WVec4& vData); // [ property ]
  const WVec4& GetCustomData() const;     // [ property ]

  /// Sets the extent along the negative X axis of the bounding box.
  void SetSizeNegX(float f);                        // [ property ]
  float GetSizeNegX() const { return m_fSizeNegX; } // [ property ]

  /// Sets the extent along the positive X axis of the bounding box.
  void SetSizePosX(float f);                        // [ property ]
  float GetSizePosX() const { return m_fSizePosX; } // [ property ]

  /// Sets the extent along the negative Y axis of the bounding box.
  void SetSizeNegY(float f);                        // [ property ]
  float GetSizeNegY() const { return m_fSizeNegY; } // [ property ]

  /// Sets the extent along the positive Y axis of the bounding box.
  void SetSizePosY(float f);                        // [ property ]
  float GetSizePosY() const { return m_fSizePosY; } // [ property ]

  /// Sets the extent along the negative Z axis of the bounding box.
  void SetSizeNegZ(float f);                        // [ property ]
  float GetSizeNegZ() const { return m_fSizeNegZ; } // [ property ]

  /// Sets the extent along the positive Z axis of the bounding box.
  void SetSizePosZ(float f);                        // [ property ]
  float GetSizePosZ() const { return m_fSizePosZ; } // [ property ]

  /// Sets the detail of the geometry. The meaning is geometry type specific, e.g. for cylinders this is the number of polygons around the perimeter.
  void SetDetail(WUInt32 uiDetail);                // [ property ]
  WUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  /// Geometry type specific: Sets an angle, used to curve stairs, etc.
  void SetCurvature(WAngle curvature);                // [ property ]
  WAngle GetCurvature() const { return m_Curvature; } // [ property ]

  /// For curved stairs to make the top smooth.
  void SetSlopedTop(bool b);                         // [ property ]
  bool GetSlopedTop() const { return m_bSlopedTop; } // [ property ]

  /// For curved stairs to make the bottom smooth.
  void SetSlopedBottom(bool b);                            // [ property ]
  bool GetSlopedBottom() const { return m_bSlopedBottom; } // [ property ]

  /// Geometry type specific: Sets a thickness, e.g. for curved stairs.
  void SetThickness(float f);                         // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  /// Whether the mesh should be used as a collider.
  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  /// Sets the WMaterialResource to use for rendering.
  void SetMaterial(const WMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  WMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

protected:
  void OnBuildStaticMesh(WMsgBuildStaticMesh& msg) const;
  void OnMsgExtractGeometry(WMsgExtractGeometry& msg) const;
  void OnMsgExtractOccluderData(WMsgExtractOccluderData& msg) const;

  void OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(WMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetCustomData(WMsgSetCustomData& ref_msg);     // [ msg handler ]

  WEnum<WGreyBoxShape> m_Shape;
  WMaterialResourceHandle m_hMaterial;
  WColor m_Color = WColor::White;
  WVec4 m_vCustomData = WVec4(0, 1, 0, 1);
  float m_fSizeNegX = 0;
  float m_fSizePosX = 0;
  float m_fSizeNegY = 0;
  float m_fSizePosY = 0;
  float m_fSizeNegZ = 0;
  float m_fSizePosZ = 0;
  WUInt32 m_uiDetail = 16;
  WAngle m_Curvature;
  float m_fThickness = 0.5f;
  bool m_bSlopedTop = false;
  bool m_bSlopedBottom = false;
  bool m_bGenerateCollision = true;
  bool m_bUseAsOccluder = true;

  void InvalidateMesh();
  void BuildGeometry(WGeometry& geom, WEnum<WGreyBoxShape> shape, bool bOnlyRoughDetails) const;

  template <typename ResourceType>
  WTypedResourceHandle<ResourceType> GenerateMesh() const;

  void GenerateMeshName(WStringBuilder& out_sName) const;
  void GenerateMeshResourceDescriptor(WMeshResourceDescriptor& desc) const;

  WMeshResourceHandle m_hMesh;

  mutable WSharedPtr<const WRasterizerObject> m_pOccluderObject;
  mutable WInstanceDataOffset m_InstanceDataOffset;
};
