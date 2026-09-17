#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

/// Render data used to feed the WMeshRenderer.
class W_RENDERERCORE_DLL WCustomMeshRenderData : public WInstanceableRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WCustomMeshRenderData, WInstanceableRenderData);

public:
  void FillSortingKey();
  virtual bool CanBatch(const WRenderData& other) const override;

  WMaterialResourceHandle m_hMaterial;
  WDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;

  WUInt32 m_uiFirstPrimitive = 0;
  WUInt32 m_uiNumPrimitives = 0xFFFFFFFF;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WBoundingBox m_FallbackGlobalBBox = WBoundingBox::MakeInvalid();
#endif
};

using WDynamicMeshBufferResourceHandle = WTypedResourceHandle<class WDynamicMeshBufferResource>;
using WCustomMeshComponentManager = WComponentManager<class WCustomMeshComponent, WBlockStorageType::Compact>;

/// This component is used to render custom geometry.
///
/// Sometimes game code needs to build geometry on the fly to visualize dynamic things.
/// The WDynamicMeshBufferResource is an easy to use resource to build geometry and change it frequently.
/// This component takes such a resource and takes care of rendering it.
/// The same resource can be set on multiple components to instantiate it in different locations.
class W_RENDERERCORE_DLL WCustomMeshComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WCustomMeshComponent, WRenderComponent, WCustomMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WCustomMeshComponent

public:
  WCustomMeshComponent();
  ~WCustomMeshComponent();

  /// Creates a new dynamic mesh buffer.
  ///
  /// The new buffer can hold the given number of vertices and indices (either 16 bit or 32 bit).
  WDynamicMeshBufferResourceHandle CreateMeshResource(WGALPrimitiveTopology::Enum topology, WUInt32 uiMaxVertices, WUInt32 uiMaxPrimitives, WGALIndexType::Enum indexType);

  /// Returns the currently set mesh resource.
  WDynamicMeshBufferResourceHandle GetMeshResource() const { return m_hDynamicMesh; }

  /// Sets which mesh buffer to use.
  ///
  /// This can be used to have multiple WCustomMeshComponent's reference the same mesh buffer,
  /// such that the object gets instanced in different locations.
  void SetMeshResource(const WDynamicMeshBufferResourceHandle& hMesh);

  /// Configures the component to render only a subset of the primitives in the mesh buffer.
  void SetUsePrimitiveRange(WUInt32 uiFirstPrimitive = 0, WUInt32 uiNumPrimitives = WMath::MaxValue<WUInt32>());

  /// Sets the bounds that are used for culling.
  ///
  /// Note: It is very important that this is called whenever the mesh buffer is modified and the size of
  /// the mesh has changed, otherwise the object might not appear or be culled incorrectly.
  void SetBounds(const WBoundingBoxSphere& bounds);

  /// Sets the material for rendering.
  void SetMaterial(const WMaterialResourceHandle& hMaterial);

  /// Returns the material that is used for rendering.
  WMaterialResourceHandle GetMaterial() const;

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS(Material, m_hMaterial);

  /// Sets the mesh instance color.
  void SetColor(const WColor& color); // [ property ]

  /// Returns the mesh instance color.
  const WColor& GetColor() const; // [ property ]

  /// An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const WVec4& vData); // [ property ]
  const WVec4& GetCustomData() const;     // [ property ]

  /// Sets the sorting depth offset value.
  ///
  /// The effect of sorting depth depends on the RenderDataCategory that the mesh is rendered in.
  /// E.g. if the sorting function of the render data category is WRenderSortingFunctions::ByDepthOffsetOnly
  /// the offset is the *only* thing that decides the order, which makes it a way to give overlapping transparent meshes a fixed draw order.
  /// With the typical distance based sorting functions this has little impact.
  void SetSortingDepthOffset(float fOffset);                            // [ property ]
  float GetSortingDepthOffset() const { return m_fSortingDepthOffset; } // [ property ]

  void OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg);             // [ msg handler ]
  void OnMsgSetColor(WMsgSetColor& ref_msg);                           // [ msg handler ]
  void OnMsgSetCustomData(WMsgSetCustomData& ref_msg);                 // [ msg handler ]

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  WMaterialResourceHandle m_hMaterial; // [ property ]
  WColor m_Color = WColor::White;
  WVec4 m_vCustomData = WVec4(0, 1, 0, 1);
  float m_fSortingDepthOffset = 0.0f;
  WBoundingBoxSphere m_Bounds = WBoundingBoxSphere::MakeInvalid();

  mutable WInstanceDataOffset m_InstanceDataOffset;

  WUInt32 m_uiFirstPrimitive = 0;
  WUInt32 m_uiNumPrimitives = 0xFFFFFFFF;
  WDynamicMeshBufferResourceHandle m_hDynamicMesh;
};
