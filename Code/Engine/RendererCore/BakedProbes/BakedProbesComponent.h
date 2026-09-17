#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/BakedProbes/BakingInterface.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgUpdateLocalBounds;
struct WMsgExtractRenderData;
struct WRenderWorldRenderEvent;
class WAbstractObjectNode;

class W_RENDERERCORE_DLL WBakedProbesComponentManager : public WSettingsComponentManager<class WBakedProbesComponent>
{
public:
  WBakedProbesComponentManager(WWorld* pWorld);
  ~WBakedProbesComponentManager();

  virtual void Initialize() override;

  WMeshResourceHandle m_hDebugSphere;
  WMaterialResourceHandle m_hDebugMaterial;

private:
  void RenderDebug(const WWorldModule::UpdateContext& updateContext);
  void CreateDebugResources();
};

class W_RENDERERCORE_DLL WBakedProbesComponent : public WSettingsComponent
{
  W_DECLARE_COMPONENT_TYPE(WBakedProbesComponent, WSettingsComponent, WBakedProbesComponentManager);

public:
  WBakedProbesComponent();
  ~WBakedProbesComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  WBakingSettings m_Settings;                                      // [ property ]

  void SetShowDebugOverlay(bool bShow);                             // [ property ]
  bool GetShowDebugOverlay() const { return m_bShowDebugOverlay; }  // [ property ]

  void SetShowDebugProbes(bool bShow);                              // [ property ]
  bool GetShowDebugProbes() const { return m_bShowDebugProbes; }    // [ property ]

  void SetUseTestPosition(bool bUse);                               // [ property ]
  bool GetUseTestPosition() const { return m_bUseTestPosition; }    // [ property ]

  void SetTestPosition(const WVec3& vPos);                         // [ property ]
  const WVec3& GetTestPosition() const { return m_vTestPosition; } // [ property ]

  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg);
  void OnExtractRenderData(WMsgExtractRenderData& ref_msg) const;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

private:
  void RenderDebugOverlay();
  void OnObjectCreated(const WAbstractObjectNode& node);

  WHashedString m_sProbeTreeResourcePrefix;

  bool m_bShowDebugOverlay = false;
  bool m_bShowDebugProbes = false;
  bool m_bUseTestPosition = false;
  WVec3 m_vTestPosition = WVec3::MakeZero();

  struct RenderDebugViewTask;
  WSharedPtr<RenderDebugViewTask> m_pRenderDebugViewTask;

  WGALTextureHandle m_hDebugViewTexture;

  mutable WInstanceDataOffset m_InstanceDataOffset;
};
