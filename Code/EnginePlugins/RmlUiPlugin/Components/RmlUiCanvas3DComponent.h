#pragma once

#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>

using WCpuMeshResourceHandle = WTypedResourceHandle<class WCpuMeshResource>;

using WRmlUiCanvas3DComponentManager = WComponentManagerSimple<class WRmlUiCanvas3DComponent, WComponentUpdateType::Always, WBlockStorageType::Compact, WWorldUpdatePhase::PostTransform>;

class W_RMLUIPLUGIN_DLL WRmlUiCanvas3DComponent : public WRmlUiCanvasComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WRmlUiCanvas3DComponent, WRmlUiCanvasComponentBase, WRmlUiCanvas3DComponentManager);

public:
  WRmlUiCanvas3DComponent();
  ~WRmlUiCanvas3DComponent();

  WRmlUiCanvas3DComponent& operator=(WRmlUiCanvas3DComponent&& rhs);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update() final override;

  bool ReceiveInput(const WVec2& vMousePosInsideCanvas, WRmlUiInputSnapshot input) override;
  bool RaycastInput(const WVec3& vRayOrigin, const WVec3& vRayDir, WRmlUiInputSnapshot input);

  /// Changes which mesh will be used for hit testing.
  void SetProxyMesh(const WMeshResourceHandle& hMesh) { m_hProxyMesh = hMesh; } // [ property ]
  const WMeshResourceHandle& GetProxyMesh() const { return m_hProxyMesh; }      // [ property ]

  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(ProxyMesh, m_hProxyMesh, SetProxyMesh);

  void SetBaseMaterial(const WMaterialResourceHandle& hMaterial);                    // [ property ]
  const WMaterialResourceHandle& GetBaseMaterial() const { return m_hBaseMaterial; } // [ property ]

  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(BaseMaterial, m_hBaseMaterial, SetBaseMaterial);

  void SetMaterialIndex(WUInt32 uiMaterialIndex);                       // [ property ]
  WUInt32 GetMaterialIndex() const { return m_uiMaterialIndex; }        // [ property ]

  void SetTextureSlotName(WStringView sName);                           // [ property ]
  WStringView GetTextureSlotName() const { return m_sTextureSlotName; } // [ property ]

  void SetTextureSize(const WVec2U32& vSize);                           // [ property ]
  const WVec2U32& GetTextureSize() const { return m_vSize; }            // [ property ]

  void SetDpiScale(float fDpiScale);                                     // [ property ]
  float GetDpiScale() const { return m_fDpiScale; }                      // [ property ]

  void SetClearStaleInput(bool bClearStaleInput);                        // [ property ]
  bool GetClearStaleInput() const { return m_bClearStaleInput; }         // [ property ]

  void SetInteractive(bool bIsInteractive);                              // [ property ]
  bool IsInteractive() const { return m_bIsInteractive; }                // [ property ]

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const override; // [ msg handler ]
  virtual void OnMsgReload(WMsgRmlUiReload& msg) override;                        // [ msg handler ]

  bool UpdateTextureAndMaterial();

  static bool RaycastMeshTexCoords(const class WCpuMeshResource* pMesh, WUInt32 uiSubMeshIndex, const WVec3& vRayOrigin, const WVec3& vRayDir, WVec2& out_vTexCoords, float fEpsilon = 0.00001f);

  // properties
  WMeshResourceHandle m_hProxyMesh;
  WMaterialResourceHandle m_hBaseMaterial;
  WUInt32 m_uiMaterialIndex = 0;
  WHashedString m_sTextureSlotName;
  float m_fDpiScale = 1.0f;
  bool m_bClearStaleInput = true;
  bool m_bIsInteractive = true;

  // runtime data
  WInt8 m_iInputAge = -1;
  WCpuMeshResourceHandle m_hCachedCpuMesh;
  WMaterialResourceHandle m_hMaterial;
  WTexture2DResourceHandle m_hTexture;
};
