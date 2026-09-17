#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class WJoltUserData;
class WSkeletonJoint;
class WJoltWorldModule;
class WJoltMaterial;
struct WMsgRetrieveBoneState;
struct WMsgAnimationPoseUpdated;
struct WMsgPhysicsAddImpulse;
struct WSkeletonResourceGeometry;

namespace JPH
{
  class Ragdoll;
  class RagdollSettings;
  class Shape;
  class SkeletonPose;
  class PhysicsSystem;
} // namespace JPH

using WSkeletonResourceHandle = WTypedResourceHandle<class WSkeletonResource>;
using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

class W_JOLTPLUGIN_DLL WJoltRagdollComponentManager : public WComponentManager<class WJoltRagdollComponent, WBlockStorageType::FreeList>
{
public:
  WJoltRagdollComponentManager(WWorld* pWorld);
  ~WJoltRagdollComponentManager();

  virtual void Initialize() override;

  void DriveAnimatedRagdolls(WTime deltaTime);

private:
  friend class WJoltWorldModule;
  friend class WJoltRagdollComponent;

  WMutex m_SkeletonsMutex;
  WDynamicArray<WUniquePtr<JPH::SkeletonPose>> m_FreeSkeletonPoses;

  void Update(const WWorldModule::UpdateContext& context);
};

/// With which pose a ragdoll should start.
struct WJoltRagdollStartMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    WithBindPose,        ///< The ragdoll uses the bind pose (or rest pose) to start with.
    WithNextAnimPose,    ///< The ragdoll waits for a new pose and then starts simulating with that.
    WithCurrentMeshPose, ///< The ragdoll retrieves the current pose from the animated mesh and starts simulating with that.
    Default = WithBindPose
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltRagdollStartMode);

struct WJoltRagdollAnimMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Off,        ///< Not ragdolling yet.
    Limp,       ///< The ragdoll just falls to the ground.
    Powered,    ///< The ragdoll joints are powered by incoming animation poses.
    Controlled, ///< The ragdoll is fully controlled by incoming animation poses (kinematic).

    Default = Limp
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltRagdollAnimMode);

//////////////////////////////////////////////////////////////////////////

/// Creates a physics ragdoll for an animated mesh and creates animation poses from the physics simulation.
///
/// By activating this component on an animated mesh, the component creates the necessary physics shapes to simulate a falling body.
/// The component queries the bone transforms from the physics engine and sends WMsgAnimationPoseUpdated with new poses.
///
/// Once this component is active on an animated mesh, no other pose generating component should be active anymore, otherwise
/// multiple components generate conflicting animation poses.
/// The typical way to use this, is to have the component created and configured, but in an inactive state. Once an NPC dies, the component is activated and all other components that generate animation poses should be deactivated.
///
/// The ragdoll shapes are configured through the WSkeletonResource.
///
/// Ragdolls are also used to create fake "breakable" objects. This is achieved by building a skinned object out of several pieces
/// and giving every piece a bone that has no constraint (joint), so that the object just breaks apart.
class W_JOLTPLUGIN_DLL WJoltRagdollComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltRagdollComponent, WComponent, WJoltRagdollComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltRagdollComponent

public:
  WJoltRagdollComponent();
  ~WJoltRagdollComponent();

  /// Returns the object ID used for all bodies in the ragdoll. This can be used to ignore the entire ragdoll during raycasts and such.
  WUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  /// Adjusts how strongly gravity affects the ragdoll.
  ///
  /// Set this to zero to fully disable gravity.
  void SetGravityFactor(float fFactor);                       // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  /// If true, the ragdoll pieces collide with each other.
  /// This produces more realistic ragdoll behavior, but requires that the ragdoll shapes are well set up.
  /// It may produce jittering and also cost more performance.
  /// Another option is to set up the joint limits such that the ragdoll can't easily intersect itself.
  bool m_bSelfCollision = false; // [ property ]

  /// How easily the joints move. Note that scaling the ragdoll up or down affects the forces and thus stiffness needs to be adjusted as well.
  float m_fStiffnessFactor = 1.0f; // [ property ]

  WUInt8 m_uiWeightCategory = 0;  // [ property ]
  WFloat16 m_fWeightScale = 1.0f; // [ property ]
  WFloat16 m_fWeightMass = 50.0f; // [ property ]

  /// Sets with which pose the ragdoll should start simulating.
  void SetStartMode(WEnum<WJoltRagdollStartMode> mode);                     // [ property ]
  WEnum<WJoltRagdollStartMode> GetStartMode() const { return m_StartMode; } // [ property ]

  void SetAnimMode(WEnum<WJoltRagdollAnimMode> mode);                       // [ property ]
  WEnum<WJoltRagdollAnimMode> GetAnimMode() const { return m_AnimMode; }    // [ property ]

  /// Applies a force to a specific part of the ragdoll.
  void OnMsgPhysicsAddImpulse(WMsgPhysicsAddImpulse& ref_msg); // [ msg handler ]

  /// Call this function BEFORE activating the ragdoll component to specify an impulse that shall be applied to the closest body part when it activates.
  ///
  /// Both position and direction are given in world space.
  ///
  /// This overrides any previously set or accumulated impulses.
  /// If AFTER this call additional impulses are recorded through OnMsgPhysicsAddImpulse(), they are 'added' to the initial impulse.
  ///
  /// Only a single initial impulse is applied after the ragdoll is created.
  /// If multiple impulses are added through OnMsgPhysicsAddImpulse(), their average start position is used to determine the closest body part to apply the impulse on.
  /// Their impulses are accumulated, so the applied impulse can become quite large.
  void SetInitialImpulse(const WVec3& vPosition, const WVec3& vDirectionAndStrength); // [ scriptable ]

  /// Adds to the existing initial impulse. See SetInitialImpulse().
  void AddInitialImpulse(const WVec3& vPosition, const WVec3& vDirectionAndStrength); // [ scriptable ]

  /// How much of the owner object's velocity to transfer to the new ragdoll bodies.
  float m_fOwnerVelocityScale = 1.0f; // [ property ]

  /// If non-zero, when the ragdoll starts, all pieces start out with an outward velocity with this speed.
  ///
  /// This can be used to create breakable objects that should "break apart". Once the ragdoll starts, the bodies will fly
  /// away outward from their center (plus m_vCenterPosition).
  float m_fCenterVelocity = 0.0f; // [ property ]

  /// Similar to m_fCenterVelocity but sets a rotational velocity, so that objects also spin.
  float m_fCenterAngularVelocity = 0.0f; // [ property ]

  /// If center velocity is used, this adds an offset to the object's position to define where the center position should be.
  WVec3 m_vCenterPosition = WVec3::MakeZero(); // [ property ]

  /// Allows to override the type of joint to be used for a bone.
  ///
  /// This has to be called before the component is activated.
  /// Its intended use case is to make certain joints either stiff (by setting the joints to 'fixed'),
  /// or to break pieces off (by setting the type to 'None').
  ///
  /// For example a breakable object could be made up of 10 pieces, but when breaking it, a random number of joints
  /// can be set to 'fixed' or 'none', so that the exact shape of the broken pieces has more variety.
  ///
  /// Similarly, on a animated mesh that is specifically authored to have separable pieces (like an arm on a robot),
  /// one can separate limbs by setting their joint to 'none'.
  void SetJointTypeOverride(WStringView sJointName, WEnum<WSkeletonJointType> overrideType);

  void OnAnimationPoseUpdated(WMsgAnimationPoseUpdated& ref_msg);          // [ msg handler ]
  void OnRetrieveBoneState(WMsgRetrieveBoneState& ref_msg) const;          // [ msg handler ]
  void OnInjectPoseCommands(WMsgInjectPoseCommands& ref_msg);              // [ msg handler ]

  void SetJointMotorStrength(float fStrength);                              // [ scriptable ]
  float GetJointMotorStrength() const;                                      // [ scriptable ]
  void FadeJointMotorStrength(float fTargetStrength, WTime duration);      // [ scriptable ]

protected:
  WEnum<WJoltRagdollStartMode> m_StartMode;                               // [ property ]
  WEnum<WJoltRagdollAnimMode> m_AnimMode;                                 // [ property ]
  float m_fGravityFactor = 1.0f;                                            // [ property ]

  struct Limb
  {
    WUInt16 m_uiPartIndex = WInvalidJointIndex;
  };

  struct LimbConstructionInfo
  {
    WTransform m_GlobalTransform;
    WUInt16 m_uiJoltPartIndex = WInvalidJointIndex;
  };

  void Update(bool bForce);
  void DriveAnimated(WTime deltaTime);
  WResult EnsureSkeletonIsKnown();
  void CreateLimbsFromBindPose();
  void CreateLimbsFromCurrentMeshPose();
  void DestroyAllLimbs();
  void CreateLimbsFromPose(const WMsgAnimationPoseUpdated& pose);
  bool HasCreatedLimbs() const;
  WVec3 RetrieveRagdollPose();
  void SendAnimationPoseMsg();
  void ConfigureRagdollPart(void* pRagdollSettingsPart, const WTransform& globalTransform, WUInt8 uiCollisionLayer, WJoltWorldModule& worldModule);
  void CreateAllLimbs(const WSkeletonResource& skeletonResource, const WMsgAnimationPoseUpdated& pose, WJoltWorldModule& worldModule, float fObjectScale, JPH::RagdollSettings* pRagdollSettings);
  void ComputeLimbModelSpaceTransform(WTransform& transform, const WMsgAnimationPoseUpdated& pose, WUInt32 uiPoseJointIndex);
  void ComputeLimbGlobalTransform(WTransform& transform, const WMsgAnimationPoseUpdated& pose, WUInt32 uiPoseJointIndex);
  void CreateLimb(const WSkeletonResource& skeletonResource, WMap<WUInt16, LimbConstructionInfo>& limbConstructionInfos, WArrayPtr<const WSkeletonResourceGeometry*> geometries, const WMsgAnimationPoseUpdated& pose, WJoltWorldModule& worldModule, float fObjectScale, JPH::RagdollSettings* pRagdollSettings);
  JPH::Shape* CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const WSkeletonResourceGeometry& geo, const WJoltMaterial* pJoltMaterial, const WQuat& qBoneDirAdjustment, const WTransform& skeletonRootTransform, WTransform& out_shapeTransform, float fObjectScale);
  void CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, WArrayPtr<const WSkeletonResourceGeometry*> geometries, const WSkeletonJoint& thisLimbJoint, const WSkeletonResource& skeletonResource, float fObjectScale, JPH::RagdollSettings* pRagdollSettings);
  virtual void ApplyPartInitialVelocity(JPH::RagdollSettings* pRagdollSettings);
  void ApplyBodyMass(JPH::RagdollSettings* pRagdollSettings, float fMass);
  void ApplyInitialImpulse(WJoltWorldModule& worldModule, float fMaxImpulse);

  float GetWeight_Scale() const { return m_fWeightScale; }
  float GetWeight_Mass() const { return m_fWeightMass; }
  void SetWeight_Scale(float fValue) { m_fWeightScale = fValue; }
  void SetWeight_Mass(float fValue) { m_fWeightMass = fValue; }

  void ResetJointMotors();
  void ApplyJointMotorStrength(float fStrength);

  WSkeletonResourceHandle m_hSkeleton;
  WDynamicArray<WMat4> m_CurrentLimbTransforms;
  WMat4 m_mInvSkeletonRootTransform;

  WUInt32 m_uiObjectFilterID = WInvalidIndex;
  WUInt32 m_uiJoltUserDataIndex = WInvalidIndex;
  WJoltUserData* m_pJoltUserData = nullptr;

  WJoltWorldModule* m_pJoltWorldModule = nullptr;
  JPH::Ragdoll* m_pRagdoll = nullptr;
  WDynamicArray<Limb> m_Limbs;
  WTime m_ElapsedTimeSinceUpdate = WTime::MakeZero();

  WVec3 m_vInitialImpulsePosition = WVec3::MakeZero();
  WVec3 m_vInitialImpulseDirection = WVec3::MakeZero();
  WUInt8 m_uiNumInitialImpulses = 0;
  bool m_bIsPowered = false;

  struct JointOverride
  {
    WTempHashedString m_sJointName;
    WEnum<WSkeletonJointType> m_JointType;
  };

  WDynamicArray<JointOverride> m_JointOverrides;

  WUniquePtr<JPH::SkeletonPose> m_pSkeletonPose;

  WTime m_MotorLerpDuration = WTime::MakeZero();
  float m_fMotorStrength = 100.0f;
  float m_fMotorTargetStrength = 100.0f;

  //////////////////////////////////////////////////////////////////////////

  void SetupLimbJoints(const WSkeletonResource* pSkeleton, JPH::RagdollSettings* pRagdollSettings);
  void CreateLimbJoint(const WSkeletonJoint& thisJoint, void* pParentBodyDesc, void* pThisBodyDesc);
};
