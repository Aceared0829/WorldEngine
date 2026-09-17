#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

class WEventTrack;
struct WMsgGenericEvent;

using WAnimationClipResourceHandle = WTypedResourceHandle<class WAnimationClipResource>;
using WSkeletonResourceHandle = WTypedResourceHandle<class WSkeletonResource>;


/// Component manager for WSimpleAnimationComponent.
///
/// Schedules updates in the async world update phase so that multiple instances can be evaluated in parallel.
class W_GAMEENGINE_DLL WSimpleAnimationComponentManager : public WComponentManager<class WSimpleAnimationComponent, WBlockStorageType::FreeList>
{
public:
  WSimpleAnimationComponentManager(WWorld* pWorld);
  ~WSimpleAnimationComponentManager();

  virtual void Initialize() override;

private:
  void Update(const WWorldModule::UpdateContext& context);
  void ApplyRootMotion(const WWorldModule::UpdateContext& context);
};


/// Plays a single animation clip on an animated mesh.
///
/// \see WAnimatedMeshComponent
class W_GAMEENGINE_DLL WSimpleAnimationComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WSimpleAnimationComponent, WComponent, WSimpleAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WJointAttachmentComponent

public:
  WSimpleAnimationComponent();
  ~WSimpleAnimationComponent();

  WAnimationClipResourceHandle m_hAnimationClip;

  // adds SetAnimationClipFile() and GetAnimationClipFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS(AnimationClip, m_hAnimationClip);

  /// How to play the animation.
  WEnum<WPropertyAnimMode> m_AnimationMode; // [ property ]

  /// How quickly or slowly to play the animation.
  float m_fSpeed = 1.0f; // [ property ]

  /// Sets the current sample position of the animation clip in 0 (start) to 1 (end) range.
  void SetNormalizedPlaybackPosition(float fPosition);

  /// Returns the normalized [0;1] sample position of the animation clip.
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }

  /// How often to update the animation while the animated mesh is invisible.
  WEnum<WAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();
  void ApplyRootMotion();
  bool UpdatePlaybackTime(WTime tDiff, const WEventTrack& eventTrack, WAnimPoseEventTrackSampleMode& out_trackSampling);

  WEnum<WRootMotionMode> m_RootMotionMode;
  float m_fNormalizedPlaybackPosition = 0.0f;
  WTime m_Duration;
  WSkeletonResourceHandle m_hSkeleton;
  WTime m_ElapsedTimeSinceUpdate = WTime::MakeZero();
  bool m_bEnableIK = false;
  WVec3 m_vPendingRootMotion = WVec3::MakeZero();

  ozz::vector<ozz::math::SoaTransform> m_OzzLocalTransforms; // TODO: could be frame allocated
};
