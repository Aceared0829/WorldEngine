#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Meshes/SkinnedMeshRenderData.h>

using WSkeletonResourceHandle = WTypedResourceHandle<class WSkeletonResource>;

class W_GAMEENGINE_DLL WAnimatedMeshComponentManager : public WComponentManager<class WAnimatedMeshComponent, WBlockStorageType::FreeList>
{
public:
  WAnimatedMeshComponentManager(WWorld* pWorld);
  ~WAnimatedMeshComponentManager();

  virtual void Initialize() override;

  void Update(const WWorldModule::UpdateContext& context);
  void AddToUpdateList(WAnimatedMeshComponent* pComponent);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  WDeque<WComponentHandle> m_ComponentsToUpdate;
};

/// Instantiates a mesh that can be animated through skeletal animation.
///
/// The referenced mesh has to contain skinning information.
///
/// This component only creates an animated mesh for rendering. It does not animate the mesh in any way.
/// The component handles messages of type WMsgAnimationPoseUpdated. Using this message other systems can set a new pose
/// for the animated mesh.
///
/// For example the WSkeletonPoseComponent, WSimpleAnimationComponent and WAnimationControllerComponent do this
/// to change the pose of the animated mesh.
class W_GAMEENGINE_DLL WAnimatedMeshComponent : public WMeshComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WAnimatedMeshComponent, WMeshComponentBase, WAnimatedMeshComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WMeshComponentBase

protected:
  virtual WTransform GetFinalGlobalTransform() const override;
  virtual WMeshRenderData* CreateRenderData(const WRenderDataManager* pRenderDataManager) const override;
  virtual WResult GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WAnimatedMeshComponent

public:
  WAnimatedMeshComponent();
  ~WAnimatedMeshComponent();

  void RetrievePose(WDynamicArray<WMat4>& out_modelTransforms, WTransform& out_rootTransform, const WSkeleton& skeleton);

protected:
  void OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg);                          // [ msg handler ]
  void OnQueryAnimationSkeleton(WMsgQueryAnimationSkeleton& msg);                      // [ msg handler ]
  void OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& msg); // [ msg handler ]

  void InitializeAnimationPose();

  void MapModelSpacePoseToSkinningSpace(const WHashTable<WHashedString, WMeshResourceDescriptor::BoneData>& bones, const WSkeleton& skeleton, WArrayPtr<const WMat4> modelSpaceTransforms, WBoundingBox* bounds);

  WTransform m_RootTransform = WTransform::MakeIdentity();
  WBoundingBox m_MaxBounds;
  WSkinningState m_SkinningState;
  WSkeletonResourceHandle m_hDefaultSkeleton;
};


struct WRootMotionMode
{
  using StorageType = WInt8;

  enum Enum
  {
    Ignore,
    ApplyToOwner,
    SendMoveCharacterMsg,

    Default = Ignore
  };

  W_GAMEENGINE_DLL static void Apply(WRootMotionMode::Enum mode, WGameObject* pObject, const WVec3& vTranslation, WAngle rotationX, WAngle rotationY, WAngle rotationZ);
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WRootMotionMode);
