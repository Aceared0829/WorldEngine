#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgSetColor;
struct WMsgSetCustomData;

class W_RENDERERCORE_DLL WMeshRenderData : public WInstanceableRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WMeshRenderData, WInstanceableRenderData);

public:
  W_FORCE_INLINE void Fill(WInstanceDataOffset instanceDataOffset, WGALDynamicBufferHandle hInstanceDataBuffer, WMaterialResourceHandle hMaterial, WMeshResourceHandle hMesh, WUInt32 uiMaterialSlotIndex = 0, WUInt32 uiSubMeshIndex = 0, WUInt32 uiNumInstances = 1)
  {
    m_uiNumInstances = uiNumInstances;
    m_DataOffsets.m_uiInstance = instanceDataOffset.m_uiOffset;
    m_DataOffsets.m_uiMaterial = uiMaterialSlotIndex << 24; // Encode the material slot index into the upper byte for picking purposes.
    m_hInstanceDataBuffer = hInstanceDataBuffer;

    m_hMaterial = hMaterial;
    m_hMesh = hMesh;
    m_uiSubMeshIndex = uiSubMeshIndex;

    FillSortingKey();
  }

  W_ALWAYS_INLINE void SetFallbackGlobalBounds(const WBoundingBoxSphere& globalBounds)
  {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (globalBounds.IsValid())
    {
      m_FallbackGlobalBBox = globalBounds.GetBox();
    }
#endif
  }

  void FillSortingKey();
  virtual bool CanBatch(const WRenderData& other) const override;

  WMaterialResourceHandle m_hMaterial;
  WMeshResourceHandle m_hMesh;
  WUInt32 m_uiSubMeshIndex = 0;

  WGALDynamicBufferHandle m_hCustomInstanceDataBuffer;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WBoundingBox m_FallbackGlobalBBox = WBoundingBox::MakeInvalid();
#endif
};

/// This message is used to replace the material on a mesh.
struct W_RENDERERCORE_DLL WMsgSetMeshMaterial : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSetMeshMaterial, WMessage);

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS(Material, m_hMaterial);

  /// The material to be used.
  WMaterialResourceHandle m_hMaterial; // [ property ]

  /// The slot on the mesh component where the material should be set.
  WUInt32 m_uiMaterialSlot = 0; // [ property ]

  virtual void Serialize(WStreamWriter& inout_stream) const override;
  virtual void Deserialize(WStreamReader& inout_stream, WUInt8 uiTypeVersion) override;
};

/// Base class for components that render static or animated meshes.
class W_RENDERERCORE_DLL WMeshComponentBase : public WRenderComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WMeshComponentBase, WRenderComponent);

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
  // WRenderMeshComponent

public:
  WMeshComponentBase();
  ~WMeshComponentBase();

  /// Changes which mesh to render.
  void SetMesh(const WMeshResourceHandle& hMesh);                                 // [ property ]
  W_ALWAYS_INLINE const WMeshResourceHandle& GetMesh() const { return m_hMesh; } // [ property ]

  // adds SetMeshFile() and GetMeshFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Mesh, m_hMesh, SetMesh);

  /// Sets the material that should be used for the sub-mesh with the given index.
  void SetMaterial(WUInt32 uiIndex, const WMaterialResourceHandle& hMaterial); // [ property ]
  WMaterialResourceHandle GetMaterial(WUInt32 uiIndex) const;                  // [ property ]

  /// An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const WColor& color);                // [ property ]
  const WColor& GetColor() const { return m_Color; } // [ property ]

  /// An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const WVec4& vData);                      // [ property ]
  const WVec4& GetCustomData() const { return m_vCustomData; } // [ property ]

  /// The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset);                            // [ property ]
  float GetSortingDepthOffset() const { return m_fSortingDepthOffset; } // [ property ]

  void OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg);             // [ msg handler ]
  void OnMsgSetColor(WMsgSetColor& ref_msg);                           // [ msg handler ]
  void OnMsgSetCustomData(WMsgSetCustomData& ref_msg);                 // [ msg handler ]

  /// Set custom instance data for this mesh component which can be used by custom material shaders when
  /// the simple custom data vector is not sufficient.
  ///
  /// Typically another component besides this mesh component would create and manage the buffer that holds the custom instance data.
  /// See WRenderDataManager how to create and fill such a buffer.
  /// The renderer will bind the buffer to the 'perInstanceDataCustom' shader resource slot, so add something like this to your shader code:
  /// StructuredBuffer<MyCustomDataStruct> perInstanceDataCustom BIND_GROUP(BG_DRAW_CALL);
  /// and access the data with the corresponding data offset:
  /// perInstanceDataCustom[G.Input.DataOffsets.y]
  void SetCustomInstanceData(WCustomInstanceDataOffset offset, WGALDynamicBufferHandle hBuffer);
  WCustomInstanceDataOffset GetCustomInstanceDataOffset() const { return m_CustomInstanceDataOffset; }
  WGALDynamicBufferHandle GetCustomInstanceDataBuffer() const { return m_hCustomInstanceDataBuffer; }

protected:
  virtual WTransform GetFinalGlobalTransform() const;
  virtual WMeshRenderData* CreateRenderData(const WRenderDataManager* pRenderDataManager) const;

  // TODO: Using WStringView for the array accessors doesn't work (currently)

  WUInt32 Materials_GetCount() const;                        // [ property ]
  WString Materials_GetValue(WUInt32 uiIndex) const;        // [ property ]
  void Materials_SetValue(WUInt32 uiIndex, WString sValue); // [ property ]
  void Materials_Insert(WUInt32 uiIndex, WString sValue);   // [ property ]
  void Materials_Remove(WUInt32 uiIndex);                    // [ property ]

  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void DeleteInstanceData();

  WMeshResourceHandle m_hMesh;
  WSmallArray<WMaterialResourceHandle, 2> m_Materials;
  WColor m_Color = WColor::White;
  WVec4 m_vCustomData = WVec4(0, 1, 0, 1);
  float m_fSortingDepthOffset = 0.0f;

  mutable WInstanceDataOffset m_InstanceDataOffset;

  WCustomInstanceDataOffset m_CustomInstanceDataOffset;
  WGALDynamicBufferHandle m_hCustomInstanceDataBuffer;
};
