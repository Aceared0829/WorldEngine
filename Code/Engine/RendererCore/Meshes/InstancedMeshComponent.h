#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

struct WPerInstanceData;
struct WRenderWorldRenderEvent;
class WInstancedMeshComponent;
struct WMsgExtractGeometry;
class WStreamWriter;
class WStreamReader;

struct W_RENDERERCORE_DLL WMeshInstanceData
{
  void SetLocalPosition(WVec3 vPosition);
  WVec3 GetLocalPosition() const;

  void SetLocalRotation(WQuat qRotation);
  WQuat GetLocalRotation() const;

  void SetLocalScaling(WVec3 vScaling);
  WVec3 GetLocalScaling() const;

  WResult Serialize(WStreamWriter& ref_writer) const;
  WResult Deserialize(WStreamReader& ref_reader);

  WTransform m_transform;

  WColor m_color;
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WMeshInstanceData);

//////////////////////////////////////////////////////////////////////////

using WInstancedMeshComponentManager = WComponentManager<class WInstancedMeshComponent, WBlockStorageType::Compact>;

/// Renders multiple instances of the same mesh.
///
/// This is used as an optimization to render many instances of the same (usually small mesh).
/// For example, if you need to render 1000 pieces of grass in a small area,
/// instead of creating 1000 game objects each with a mesh component,
/// it is more efficient to create one game object with an instanced mesh component and give it the locations of the 1000 pieces.
/// Due to the small area, there is no benefit in culling the instances separately.
///
/// However, editing instanced mesh components isn't very convenient, so usually this component would be created and configured
/// in code, rather than by hand in the editor. For example a procedural plant placement system could use this.
class W_RENDERERCORE_DLL WInstancedMeshComponent : public WMeshComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WInstancedMeshComponent, WMeshComponentBase, WInstancedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WInstancedMeshComponent

public:
  WInstancedMeshComponent();
  ~WInstancedMeshComponent();

  /// Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg);            // [ msg handler ]

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;      // [ msg handler ]

  WUInt32 Instances_GetCount() const;                                 // [ property ]
  WMeshInstanceData Instances_GetValue(WUInt32 uiIndex) const;       // [ property ]
  void Instances_SetValue(WUInt32 uiIndex, WMeshInstanceData value); // [ property ]
  void Instances_Insert(WUInt32 uiIndex, WMeshInstanceData value);   // [ property ]
  void Instances_Remove(WUInt32 uiIndex);                             // [ property ]

  // Unpacked, reflected instance data for editing and ease of access
  WDynamicArray<WMeshInstanceData> m_RawInstancedData;

  float m_fBoundingSphereRadius = 1.0f;
};
