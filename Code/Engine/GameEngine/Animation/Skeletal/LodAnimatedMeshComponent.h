#pragma once

#include <Core/World/World.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>

class W_GAMEENGINE_DLL WLodAnimatedMeshComponentManager : public WComponentManager<class WLodAnimatedMeshComponent, WBlockStorageType::FreeList>
{
public:
  WLodAnimatedMeshComponentManager(WWorld* pWorld);
  ~WLodAnimatedMeshComponentManager();

  virtual void Initialize() override;

  void Update(const WWorldModule::UpdateContext& context);
  void AddToUpdateList(WLodAnimatedMeshComponent* pComponent);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  WDeque<WComponentHandle> m_ComponentsToUpdate;
};

struct WLodAnimatedMeshLod
{
  WMeshResourceHandle m_hMesh;
  float m_fThreshold;
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WLodAnimatedMeshLod);

class W_GAMEENGINE_DLL WLodAnimatedMeshComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WLodAnimatedMeshComponent, WRenderComponent, WLodAnimatedMeshComponentManager);

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
  // WLodAnimatedMeshComponent

public:
  WLodAnimatedMeshComponent();
  ~WLodAnimatedMeshComponent();

  /// An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const WColor& color); // [ property ]
  const WColor& GetColor() const;     // [ property ]

  /// An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const WVec4& vData); // [ property ]
  const WVec4& GetCustomData() const;     // [ property ]

  /// The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;       // [ property ]

  /// Enables text output to show the current coverage value and selected LOD.
  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  /// Disabling the LOD range overlap functionality can make it easier to determine the desired coverage thresholds.
  void SetOverlapRanges(bool bOverlap);                 // [ property ]
  bool GetOverlapRanges() const;                        // [ property ]

  void OnMsgSetColor(WMsgSetColor& ref_msg);           // [ msg handler ]
  void OnMsgSetCustomData(WMsgSetCustomData& ref_msg); // [ msg handler ]

  void RetrievePose(WDynamicArray<WMat4>& out_modelTransforms, WTransform& out_rootTransform, const WSkeleton& skeleton);

protected:
  void UpdateSelectedLod(const WView& view) const;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  WDynamicArray<WLodAnimatedMeshLod> m_Meshes;
  WColor m_Color = WColor::White;
  WVec4 m_vCustomData = WVec4(0, 1, 0, 1);
  float m_fSortingDepthOffset = 0.0f;
  WVec3 m_vBoundsOffset = WVec3::MakeZero();
  float m_fBoundsRadius = 1.0f;

  mutable WInt32 m_iCurLod = 0;
  mutable WInstanceDataOffset m_InstanceDataOffset;

protected:
  void OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(WMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  void MapModelSpacePoseToSkinningSpace(const WHashTable<WHashedString, WMeshResourceDescriptor::BoneData>& bones, const WSkeleton& skeleton, WArrayPtr<const WMat4> modelSpaceTransforms, WBoundingBox* bounds);

  WTransform m_RootTransform = WTransform::MakeIdentity();
  WBoundingBox m_MaxBounds;
  WSkinningState m_SkinningState;
  WSkeletonResourceHandle m_hDefaultSkeleton;
};
