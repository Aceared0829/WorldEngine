#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using WJointOverrideComponentManager = WComponentManager<class WJointOverrideComponent, WBlockStorageType::FreeList>;

/// Overrides the local transform of a bone in a skeletal animation.
///
/// Every time a new animation pose is prepared, this component replaces the transform of the chosen bone
/// to be the same as the local transform of the owner game object.
///
/// That allows you to do a simple kind of forward kinematics. For example it can be used to modify a targeting bone,
/// so that an animated object points into the right direction.
///
/// The global transform of the game object is irrelevant, but the local transform is used to copy over.
class W_GAMEENGINE_DLL WJointOverrideComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WJointOverrideComponent, WComponent, WJointOverrideComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WJointOverrideComponent

public:
  WJointOverrideComponent();
  ~WJointOverrideComponent();

  /// The name of the bone whose transform should be replaced with the transform of this game object.
  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  /// If true, the position of the bone will be overridden.
  bool m_bOverridePosition = false; // [ property ]

  /// If true, the rotation of the bone will be overridden.
  bool m_bOverrideRotation = true; // [ property ]

  /// If true, the scale of the bone will be overridden.
  bool m_bOverrideScale = false; // [ property ]

  /// Set a weight between 0 and 1 to fade the override in our out.
  float m_fWeight = 1.0f;                                                // [ property ]

protected:
  void OnAnimationPosePreparing(WMsgAnimationPosePreparing& msg) const; // [ msg handler ]

  WHashedString m_sJointToOverride;
  mutable WUInt16 m_uiJointIndex = WInvalidJointIndex;
};
