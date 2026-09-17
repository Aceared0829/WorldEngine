#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>

using WSkeletonResourceHandle = WTypedResourceHandle<class WSkeletonResource>;
using WAnimGraphResourceHandle = WTypedResourceHandle<class WAnimGraphResource>;

class WAnimationControllerComponentManager : public WComponentManager<class WAnimationControllerComponent, WBlockStorageType::FreeList>
{
public:
  WAnimationControllerComponentManager(WWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  void Update(const WWorldModule::UpdateContext& context);
  void ApplyRootMotion(const WWorldModule::UpdateContext& context);
  void ResourceEvent(const WResourceEvent& e);

  WDeque<WComponentHandle> m_ComponentsToReset;
};

/// Evaluates an WAnimGraphResource and provides the result through the WMsgAnimationPoseUpdated.
///
/// WAnimGraph's contain logic to generate an animation pose. This component decides when it is necessary
/// to reevaluate the state, which mostly means it tracks when the object is visible.
///
/// The result is sent as a recursive message, which is usually consumed by an WAnimatedMeshComponent.
/// The mesh component may be on the same game object or a child object.
class W_GAMEENGINE_DLL WAnimationControllerComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WAnimationControllerComponent, WComponent, WAnimationControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WAnimationControllerComponent

public:
  WAnimationControllerComponent();
  ~WAnimationControllerComponent();

  /// How often to update the animation while the animated mesh is invisible.
  WEnum<WAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

  /// If enabled, child game objects can add IK computation commands to influence the final pose.
  bool m_bEnableIK = false; // [ property ]

  /// A list of animation clips to use instead of the default ones that are set up in the animation graph.
  WDynamicArray<WAnimationClipMapping> m_AnimationClipOverrides; // [ property ]

  /// Overrides which animation clip resource to use for the given animation.
  ///
  /// Should only be called right at the start or when it is absolutely certain that an animation clip isn't in use right now,
  /// otherwise the running animation playback may produce weird results.
  void SetAnimationClipOverride(WStringView sAnimationName, WStringView sAnimationClipResource); // [ scriptable ]

protected:
  void Update();
  void ApplyRootMotion();

  WEnum<WRootMotionMode> m_RootMotionMode;

  WAnimGraphResourceHandle m_hAnimGraph;
  WAnimController m_AnimController;
  WAnimPoseGenerator m_PoseGenerator;

  WTime m_ElapsedTimeSinceUpdate = WTime::MakeZero();

  WVec3 m_vPendingTranslation = WVec3::MakeZero();
  WAngle m_PendingRotationX, m_PendingRotationY, m_PendingRotationZ;
};
