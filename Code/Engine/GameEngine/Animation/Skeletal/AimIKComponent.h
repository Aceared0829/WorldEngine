#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using WAimIKComponentManager = WComponentManager<class WAimIKComponent, WBlockStorageType::FreeList>;

struct WIkJointEntry
{
  WHashedString m_sJointName;
  float m_fWeight = 1.0f;
  mutable WUInt16 m_uiJointIdx = 0;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WIkJointEntry);

/// Adds inverse kinematics for a single joint of an animated mesh to point towards a target.
///
/// This can be used to make a creature look at something or to aim at a target.
/// The component has to be attached to a child object of an animated mesh.
/// The animated mesh needs to be driven by another component that generates an animation pose,
/// such as a WAnimationControllerComponent or a WSimpleAnimationComponent.
/// On those "EnableIK" must be set, then they will forward the pose to all their child objects and give them the
/// opportunity to override the pose using IK.
class W_GAMEENGINE_DLL WAimIKComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WAimIKComponent, WComponent, WAimIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WAimIKComponent

public:
  WAimIKComponent();
  ~WAimIKComponent();

  void SetPoleVectorReference(const char* szReference);         // [ property ]

  WGameObjectHandle m_hPoleVector;                             ///< [ property ] An optional other object used as the pole vector for the joint to align with.
  WEnum<WBasisAxis> m_ForwardVector = WBasisAxis::PositiveX; ///< [ property ] The local forward direction of the joint to orient towards the position of this object.
  WEnum<WBasisAxis> m_UpVector = WBasisAxis::PositiveZ;      ///< [ property ] The local up direction of the joint to orient towards the pole vector.
  float m_fWeight = 1.0f;                                       ///< [ property ] Factor between 0 and 1 for how much to apply the IK.
  WHybridArray<WIkJointEntry, 2> m_Joints;                    ///< [ property ] A list of joints to apply the aim IK to. If multiple joints along a chain are used, set a weight of less than 1 for the first joints and a factor of 1 for the last joint, to distribute gradual aiming along the chain.

  bool m_bInversePoleVector = false;                            ///< [ property ] If true, the pole-vector will point away from the given position, not towards it.

  WUInt16 m_uiOrder = 0;                                       ///< [ property ] At which point in the IK calculation to execute this.

  void SetDebugVisScale(float fScale);                          // [ property ] Scale for debug visualizations. 0 to disable.
  float GetDebugVisScale() const;

protected:
  void OnInjectPoseCommands(WMsgInjectPoseCommands& msg) const; // [ msg handler ]

  WUInt8 m_uiDebugVisScale = 0;

  const char* DummyGetter() const { return nullptr; }
};
