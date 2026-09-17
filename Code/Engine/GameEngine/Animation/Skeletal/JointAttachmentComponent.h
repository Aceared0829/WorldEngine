#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using WJointAttachmentComponentManager = WComponentManager<class WJointAttachmentComponent, WBlockStorageType::FreeList>;

/// Used to expose an animated mesh's bone as a game object, such that objects can be attached to it to move along.
///
/// The animation system deals with bone animations internally.
/// Sometimes it is desirable to move certain objects along with a bone,
/// for example when a character should hold something in their hand.
///
/// This component references a bone by name, and takes care to position the owner object at the same location as the bone
/// whenever the animation pose changes.
/// Thus it is possible to attach other objects as child objects to this one, so that they move along as well.
class W_GAMEENGINE_DLL WJointAttachmentComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WJointAttachmentComponent, WComponent, WJointAttachmentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WJointAttachmentComponent

public:
  WJointAttachmentComponent();
  ~WJointAttachmentComponent();

  /// Sets the bone name whose transform should be copied into this game object.
  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  /// An additional local offset to be added to the transform.
  WVec3 m_vLocalPositionOffset = WVec3::MakeZero(); // [ property ]

  /// An additional local offset to be added to the transform.
  WQuat m_vLocalRotationOffset = WQuat::MakeIdentity();      // [ property ]

protected:
  void OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& msg); // [ msg handler ]

  WHashedString m_sJointToAttachTo;
  WUInt16 m_uiJointIndex = WInvalidJointIndex;
};
