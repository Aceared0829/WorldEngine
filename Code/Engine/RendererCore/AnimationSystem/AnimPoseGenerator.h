#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_float.h>
#include <ozz/base/maths/soa_transform.h>

W_DEFINE_AS_POD_TYPE(ozz::math::SoaTransform);

class WSkeletonResource;
class WAnimPoseGenerator;
class WGameObject;

using WAnimationClipResourceHandle = WTypedResourceHandle<class WAnimationClipResource>;

using WAnimPoseGeneratorLocalPoseID = WUInt32;
using WAnimPoseGeneratorModelPoseID = WUInt32;
using WAnimPoseGeneratorCommandID = WUInt32;

/// The type of WAnimPoseGeneratorCommand
enum class WAnimPoseGeneratorCommandType
{
  Invalid,
  SampleTrack,
  RestPose,
  CombinePoses,
  LocalToModelPose,
  SampleEventTrack,
  AimIK,
  TwoBoneIK,
};

enum class WAnimPoseEventTrackSampleMode : WUInt8
{
  None,         ///< Don't sample the event track at all
  OnlyBetween,  ///< Sample the event track only between PrevSamplePos and SamplePos
  LoopAtEnd,    ///< Sample the event track between PrevSamplePos and End, then Start and SamplePos
  LoopAtStart,  ///< Sample the event track between PrevSamplePos and Start, then End and SamplePos
  BounceAtEnd,  ///< Sample the event track between PrevSamplePos and End, then End and SamplePos
  BounceAtStart ///< Sample the event track between PrevSamplePos and Start, then Start and SamplePos
};

/// Base class for all pose generator commands
///
/// All commands have a unique command ID with which they are referenced.
/// All commands can have zero or N other commands set as *inputs*.
/// Every type of command only accepts certain types and amount of inputs.
///
/// The pose generation graph is built by allocating commands on the graph and then setting up
/// which command is an input to which other node.
/// A command can be an input to multiple other commands. It will be evaluated only once.
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommand
{
  WHybridArray<WAnimPoseGeneratorCommandID, 4> m_Inputs;

  WAnimPoseGeneratorCommandID GetCommandID() const { return m_CommandID; }
  WAnimPoseGeneratorCommandType GetType() const { return m_Type; }

private:
  friend class WAnimPoseGenerator;

  bool m_bExecuted = false;
  WAnimPoseGeneratorCommandID m_CommandID = WInvalidIndex;
  WAnimPoseGeneratorCommandType m_Type = WAnimPoseGeneratorCommandType::Invalid;
};

/// Returns the rest pose (also often called 'bind pose').
///
/// The command has to be added as an input to one of
/// * WAnimPoseGeneratorCommandCombinePoses
/// * WAnimPoseGeneratorCommandLocalToModelPose
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandRestPose final : public WAnimPoseGeneratorCommand
{
private:
  friend class WAnimPoseGenerator;

  WAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = WInvalidIndex;
};

/// Samples an animation clip at a given time and optionally also its event track.
///
/// The command has to be added as an input to one of
/// * WAnimPoseGeneratorCommandCombinePoses
/// * WAnimPoseGeneratorCommandLocalToModelPose
///
/// If the event track shall be sampled as well, event messages are sent to the WGameObject for which the pose is generated.
///
/// This command can optionally have input commands of type WAnimPoseGeneratorCommandSampleEventTrack.
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandSampleTrack final : public WAnimPoseGeneratorCommand
{
  WAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos = 0.0f;
  float m_fPreviousNormalizedSamplePos = 0.0f;

  WAnimPoseEventTrackSampleMode m_EventSampling = WAnimPoseEventTrackSampleMode::None;

private:
  friend class WAnimPoseGenerator;

  bool m_bAdditive = false;
  WUInt32 m_uiUniqueID = 0;
  WAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = WInvalidIndex;
};

/// Combines all the local space poses that are given as input into one local pose.
///
/// The input commands must be of type
/// * WAnimPoseGeneratorCommandSampleTrack
/// * WAnimPoseGeneratorCommandCombinePoses
/// * WAnimPoseGeneratorCommandRestPose
///
/// Every input pose gets both an overall weight, as well as optionally a per-bone weight mask.
/// If a per-bone mask is used, the respective input pose will only affect those bones.
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandCombinePoses final : public WAnimPoseGeneratorCommand
{
  WHybridArray<float, 4> m_InputWeights;
  WHybridArray<WArrayPtr<const ozz::math::SimdFloat4>, 4> m_InputBoneWeights;

private:
  friend class WAnimPoseGenerator;

  WAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = WInvalidIndex;
};

/// Samples the event track of an animation clip but doesn't generate an animation pose.
///
/// Commands of this type can be added as inputs to commands of type
/// * WAnimPoseGeneratorCommandSampleTrack
/// * WAnimPoseGeneratorCommandSampleEventTrack
///
/// They are used to sample event tracks only.
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandSampleEventTrack final : public WAnimPoseGeneratorCommand
{
  WAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;
  float m_fPreviousNormalizedSamplePos;

  WAnimPoseEventTrackSampleMode m_EventSampling = WAnimPoseEventTrackSampleMode::None;

private:
  friend class WAnimPoseGenerator;

  WUInt32 m_uiUniqueID = 0;
};

/// Base class for commands that produce or update a model pose.
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandModelPose : public WAnimPoseGeneratorCommand
{
protected:
  friend class WAnimPoseGenerator;

  WAnimPoseGeneratorModelPoseID m_ModelPoseOutput = WInvalidIndex;
  WAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = WInvalidIndex;
};

/// Accepts a single input in local space and converts it to model space.
///
/// The input command must be of type
/// * WAnimPoseGeneratorCommandSampleTrack
/// * WAnimPoseGeneratorCommandCombinePoses
/// * WAnimPoseGeneratorCommandRestPose
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandLocalToModelPose final : public WAnimPoseGeneratorCommandModelPose
{
  WGameObject* m_pSendLocalPoseMsgTo = nullptr;
};

/// Accepts a single input in model space and applies aim IK (inverse kinematics) on it. Updates the model pose in place.
///
/// The input command must be of type
/// * WAnimPoseGeneratorCommandLocalToModelPose
/// * WAnimPoseGeneratorCommandAimIK
/// * WAnimPoseGeneratorCommandTwoBoneIK
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandAimIK final : public WAnimPoseGeneratorCommandModelPose
{
  float m_fDebugVisScale = 0.0f;                                ///< If >0, debug visualization will be rendered.
  WVec3 m_vTargetPosition;                                     ///< The position for the bone to point at. Must be in model space of the skeleton, ie even the m_RootTransform must have been removed.
  WUInt16 m_uiJointIdx;                                        ///< The index of the joint to aim.
  WUInt16 m_uiRecalcModelPoseToJointIdx = WInvalidJointIndex; ///< Optimization hint to prevent unnecessary recalculation of model poses for joints that get updated later again.
  float m_fWeight = 1.0f;                                       ///< Factor between 0 and 1 for how much to apply the IK.
  WVec3 m_vForwardVector = WVec3::MakeAxisX();                ///< The local joint direction that should aim at the target. Typically there is a convention to use +X, +Y or +Z.
  WVec3 m_vUpVector = WVec3::MakeAxisZ();                     ///< The local joint direction that should point towards the pole vector. Must be orthogonal to the forward vector.
  WVec3 m_vPoleVectorPosition = WVec3::MakeAxisY();           ///< In the same space as the target position, a position that the up vector of the joint should (roughly) point towards. Used to have bones point into the right direction, for example to make an elbow point properly sideways.
  bool m_bInversePoleVector = false;                            ///< If true, the pole vector direction will point away from the pole vector position, rather than towards it. Useful in an aim IK chain to have all bones point away from a central point.
};

/// Accepts a single input in model space and applies two-bone IK (inverse kinematics) on it. Updates the model pose in place.
///
/// The input command must be of type
/// * WAnimPoseGeneratorCommandLocalToModelPose
/// * WAnimPoseGeneratorCommandAimIK
/// * WAnimPoseGeneratorCommandTwoBoneIK
struct W_RENDERERCORE_DLL WAnimPoseGeneratorCommandTwoBoneIK final : public WAnimPoseGeneratorCommandModelPose
{
  float m_fDebugVisScale = 0.0f;                                ///< If >0, debug visualization will be rendered.
  WVec3 m_vTargetPosition;                                     ///< The position for the 'end' joint to try to reach. Must be in model space of the skeleton, ie even the m_RootTransform must have been removed.
  WUInt16 m_uiJointIdxStart;                                   ///< Index of the top joint in a chain of three joints. The IK result may freely rotate around this joint into any (unnatural direction).
  WUInt16 m_uiJointIdxMiddle;                                  ///< Index of the middle joint in a chain of three joints. The IK result will bend at this joint around the joint local mid-axis.
  WUInt16 m_uiJointIdxEnd;                                     ///< Index of the end joint that is supposed to reach the target.
  WUInt16 m_uiRecalcModelPoseToJointIdx = WInvalidJointIndex; ///< Optimization hint to prevent unnecessary recalculation of model poses for joints that get updated later again.
  WVec3 m_vMidAxis = WVec3::MakeAxisZ();                      ///< The local joint direction around which to bend the middle joint. Typically there is a convention to use +X, +Y or +Z to bend around.
  WVec3 m_vPoleVectorPosition = WVec3::MakeAxisY();           ///< In the same space as the target position, a position that the middle joint should (roughly) point towards. Used to have bones point into the right direction, for example to make a knee point properly forwards.
  float m_fWeight = 1.0f;                                       ///< Factor between 0 and 1 for how much to apply the IK.
  float m_fSoften = 1.0f;                                       ///< Factor between 0 and 1. See OZZ for details.
  WAngle m_TwistAngle;                                         ///< After IK how much to rotate the chain. Seems to be redundant with the pole vector. See OZZ for details.
};

/// Low-level infrastructure to generate animation poses using a command-based DAG (directed acyclic graph).
///
/// ## Architecture
///
/// The pose generator uses a command pattern to build a dependency graph for pose generation.
/// Commands are allocated and connected to form a DAG where each command can have zero or more input commands.
/// This architecture provides two key benefits:
/// 1. Efficient evaluation: Only commands reachable from the "final command" are executed
/// 2. Avoid redundant work: Each command executes exactly once, even if multiple commands depend on it
///
/// ## Command Types
///
/// These command types are currently available:
/// - **SampleTrack**: Sample an animation clip at a specific time position
/// - **RestPose**: Output the skeleton's rest/bind pose
/// - **CombinePoses**: Blend multiple local-space poses with weights and optional per-bone masks
/// - **LocalToModelPose**: Convert local-space transforms to model-space (concatenate hierarchy)
/// - **SampleEventTrack**: Sample animation events without generating pose data
/// - **AimIK**: Point a bone at a target position (inverse kinematics)
/// - **TwoBoneIK**: Solve a two-bone chain to reach a target (arm/leg IK)
///
/// ## Data Flow
///
/// Commands operate on two types of pose data:
/// - **Local poses**: Joint transforms relative to parent (ozz::math::SoaTransform format for SIMD efficiency)
/// - **Model poses**: Joint transforms relative to skeleton root (WMat4 format)
///
/// Typical flow:
/// 1. Sample/RestPose commands generate local poses
/// 2. CombinePoses blends local poses together
/// 3. LocalToModelPose converts to model space
/// 4. IK commands modify model-space transforms
/// 5. Final model pose is retrieved via GetCurrentPose()
///
/// ## Usage Pattern
///
/// The generator is reused across frames for efficiency (caches sampling contexts), but commands
/// are recreated each frame to build a new pose.
class W_RENDERERCORE_DLL WAnimPoseGenerator final
{
public:
  WAnimPoseGenerator();
  ~WAnimPoseGenerator();

  void Reset(const WSkeletonResource* pSkeleton, WGameObject* pTarget);

  const WSkeletonResource* GetSkeleton() const { return m_pSkeleton; }
  const WGameObject* GetTargetObject() const { return m_pTargetGameObject; }

  WAnimPoseGeneratorCommandSampleTrack& AllocCommandSampleTrack(WUInt32 uiDeterministicID);
  WAnimPoseGeneratorCommandRestPose& AllocCommandRestPose();
  WAnimPoseGeneratorCommandCombinePoses& AllocCommandCombinePoses();
  WAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  WAnimPoseGeneratorCommandSampleEventTrack& AllocCommandSampleEventTrack();
  WAnimPoseGeneratorCommandAimIK& AllocCommandAimIK();
  WAnimPoseGeneratorCommandTwoBoneIK& AllocCommandTwoBoneIK();

  const WAnimPoseGeneratorCommand& GetCommand(WAnimPoseGeneratorCommandID id) const;
  WAnimPoseGeneratorCommand& GetCommand(WAnimPoseGeneratorCommandID id);

  /// Calculates the pose, using the final command as reference where to start.
  ///
  /// If bRequestExternalPoseGeneration is true, inverse-kinematics (IK) and powered ragdolls are also used.
  void UpdatePose(bool bRequestExternalPoseGeneration);

  WArrayPtr<WMat4> GetCurrentPose() const { return m_OutputPose; }

  /// Sets the (currently) final command in the pose generation.
  ///
  /// This will be used to determine which other commands are necessary to calculate.
  void SetFinalCommand(WAnimPoseGeneratorCommandID cmdId) { m_FinalCommand = cmdId; }

  /// Returns the (currently) final command.
  ///
  /// Can be used to inject further commands after it. Call SetFinalCommand() afterwards.
  WAnimPoseGeneratorCommandID GetFinalCommand() const { return m_FinalCommand; }

  /// Whether the caller should send WMsgAnimationPoseUpdated to its children.
  ///
  /// Typically this should be done, to forward the calculated pose to the animated mesh components.
  /// However, when some other component takes over control (usually a ragdoll),
  /// the pose from this generator isn't the final result and thus should not be forwarded.
  /// The other component will take care of this already.
  bool ShouldSendPoseResultMsg() const { return m_bSendResultMsg; }

private:
  void Validate() const;

  void Execute(WAnimPoseGeneratorCommand& cmd);
  void ExecuteCmd(WAnimPoseGeneratorCommandSampleTrack& cmd);
  void ExecuteCmd(WAnimPoseGeneratorCommandRestPose& cmd);
  void ExecuteCmd(WAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(WAnimPoseGeneratorCommandLocalToModelPose& cmd);
  void ExecuteCmd(WAnimPoseGeneratorCommandSampleEventTrack& cmd);
  void ExecuteCmd(WAnimPoseGeneratorCommandAimIK& cmd);
  void ExecuteCmd(WAnimPoseGeneratorCommandTwoBoneIK& cmd);
  void SampleEventTrack(const WAnimationClipResource* pResource, WAnimPoseEventTrackSampleMode mode, float fPrevPos, float fCurPos);

  WArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(WAnimPoseGeneratorLocalPoseID id);
  WArrayPtr<WMat4> AcquireModelPoseTransforms(WAnimPoseGeneratorModelPoseID id);

  WGameObject* m_pTargetGameObject = nullptr;
  const WSkeletonResource* m_pSkeleton = nullptr;

  WArrayPtr<WMat4> m_OutputPose;

  WAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  WAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  bool m_bSendResultMsg = true;
  WAnimPoseGeneratorCommandID m_FinalCommand = 0;

  WHybridArray<WArrayPtr<ozz::math::SoaTransform>, 8> m_UsedLocalTransforms;
  WHybridArray<WDynamicArray<WMat4, WAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  WHybridArray<WAnimPoseGeneratorCommandSampleTrack, 4> m_CommandsSampleTrack;
  WHybridArray<WAnimPoseGeneratorCommandRestPose, 1> m_CommandsRestPose;
  WHybridArray<WAnimPoseGeneratorCommandCombinePoses, 1> m_CommandsCombinePoses;
  WHybridArray<WAnimPoseGeneratorCommandLocalToModelPose, 1> m_CommandsLocalToModelPose;
  WHybridArray<WAnimPoseGeneratorCommandSampleEventTrack, 2> m_CommandsSampleEventTrack;
  WHybridArray<WAnimPoseGeneratorCommandAimIK, 2> m_CommandsAimIK;
  WHybridArray<WAnimPoseGeneratorCommandTwoBoneIK, 2> m_CommandsTwoBoneIK;

  WArrayMap<WUInt32, ozz::animation::SamplingJob::Context*> m_SamplingCaches;
};
