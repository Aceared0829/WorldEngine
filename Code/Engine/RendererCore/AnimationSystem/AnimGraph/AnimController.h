#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

class WGameObject;
class WAnimGraph;

using WAnimGraphResourceHandle = WTypedResourceHandle<class WAnimGraphResource>;
using WSkeletonResourceHandle = WTypedResourceHandle<class WSkeletonResource>;

W_DEFINE_AS_POD_TYPE(ozz::math::SimdFloat4);

/// Runtime data for bone weight pins, controlling which bones are affected by animations.
///
/// Used to mask out specific bones from animation influence, allowing selective animation blending.
struct WAnimGraphPinDataBoneWeights
{
  WUInt16 m_uiOwnIndex = 0xFFFF;
  float m_fOverallWeight = 1.0f;
  const WAnimGraphSharedBoneWeights* m_pSharedBoneWeights = nullptr;
};

/// A single named float value sampled from a custom curve in an animation clip.
struct WAnimGraphCustomCurveData
{
  WHashedString m_sName;
  float m_fValue = 0.0f;
};

/// Runtime data for local pose pins, containing bone transforms in local space (relative to parent).
///
/// Local poses are the output of pose sampling and blending nodes before forward kinematics is applied.
struct WAnimGraphPinDataLocalTransforms
{
  WUInt16 m_uiOwnIndex = 0xFFFF;
  WAnimPoseGeneratorCommandID m_CommandID;
  const WAnimGraphPinDataBoneWeights* m_pWeights = nullptr;
  float m_fOverallWeight = 1.0f;
  WVec3 m_vRootMotion = WVec3::MakeZero();
  bool m_bUseRootMotion = false;
  WSmallArray<WAnimGraphCustomCurveData, 2> m_CustomCurveValues;
};

/// Runtime data for model pose pins, containing bone transforms in model space (relative to skeleton root).
///
/// Model poses are the output after forward kinematics has been applied, ready for final rendering.
struct WAnimGraphPinDataModelTransforms
{
  WUInt16 m_uiOwnIndex = 0xFFFF;
  WAnimPoseGeneratorCommandID m_CommandID;
  WVec3 m_vRootMotion = WVec3::MakeZero();
  WAngle m_RootRotationX;
  WAngle m_RootRotationY;
  WAngle m_RootRotationZ;
  bool m_bUseRootMotion = false;
};

/// Manages and updates animation graph instances to generate animation poses.
///
/// This is the runtime bridge between animation graphs (WAnimGraph) and the pose generation system
/// (WAnimPoseGenerator). It owns graph instances, manages their execution, handles data flow between
/// graph nodes, and forwards the final pose to the renderer.
///
/// ## Responsibilities
///
/// - Creates and updates an WAnimGraphInstance object for each loaded graph
/// - Allocates and manages pin data storage (bone weights, local poses, model poses)
/// - Accumulates root motion from animations for character movement
/// - Forwards the provided blackboard for data sharing between graph nodes and gameplay code
/// - Maps animation clip names to actual animation clip resources
/// - Manages shared bone weight masks across all characters
///
/// ## Usage Pattern
///
/// Typically owned by WAnimationControllerComponent. The component calls Initialize() once,
/// then Update() every frame to generate new animation poses.
///
/// **Basic workflow:**
/// ```cpp
/// WAnimController controller;
/// controller.Initialize(skeletonHandle, poseGenerator, blackboard);
/// controller.AddAnimGraph(animGraphHandle);
/// controller.SetAnimationClipInfo("Walk", walkClipInfo);
///
/// // Each frame:
/// controller.Update(deltaTime, pGameObject, bEnableIK);
/// controller.GetRootMotion(translation, rotX, rotY, rotZ);
/// ```
///
/// ## Blackboard Integration
///
/// The blackboard is used for communication between gameplay code and animation graphs.
/// Nodes can read blackboard values (movement speed, aim direction) to drive blending,
/// and write values (footstep events, animation state) back to gameplay.
///
/// ## Multiple Graphs
///
/// Multiple animation graphs can be added to create layered animations (e.g., base locomotion
/// + upper body aim).
class W_RENDERERCORE_DLL WAnimController
{
  W_DISALLOW_COPY_AND_ASSIGN(WAnimController);

public:
  WAnimController();
  ~WAnimController();

  /// Initializes the controller with a skeleton and pose generator.
  ///
  /// Must be called before adding graphs or updating. The optional blackboard is used for
  /// data sharing between animation graphs and gameplay code.
  void Initialize(const WSkeletonResourceHandle& hSkeleton, WAnimPoseGenerator& ref_poseGenerator, const WSharedPtr<WBlackboard>& pBlackboard = nullptr);

  /// Updates all animation graph instances and generates the final pose.
  ///
  /// Steps all nodes in all graphs, accumulates results, forwards the final pose to the pose generator,
  /// and sends WMsgAnimationPoseUpdated to the target object and its children.
  ///
  /// Returns false if the target requested to stop animating via msg.m_bContinueAnimating.
  bool Update(WTime diff, WGameObject* pTarget, bool bEnableIK);

  /// Retrieves accumulated root motion from the last Update().
  ///
  /// Root motion is the translation and rotation extracted from animations, used to move characters
  /// based on their animation. This could be applied to the character controller or entity.
  void GetRootMotion(WVec3& ref_vTranslation, WAngle& ref_rotationX, WAngle& ref_rotationY, WAngle& ref_rotationZ) const;

  const WSharedPtr<WBlackboard>& GetBlackboard() { return m_pBlackboard; }

  WAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  /// Creates or retrieves a shared bone weight mask.
  ///
  /// Bone weights are shared globally across all characters to save memory. The fill delegate is called
  /// only once when the weights are first created. Subsequent calls with the same name return the cached weights.
  static WSharedPtr<WAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const WSkeletonResource& skeleton, WDelegate<void(WAnimGraphSharedBoneWeights&)> fill);

  void SetOutputModelTransform(WAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const WVec3& vTranslation, WAngle rotationX, WAngle rotationY, WAngle rotationZ);

  void AddOutputLocalTransforms(WAnimGraphPinDataLocalTransforms* pLocalTransforms);

  WAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  WAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  WAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  /// Loads an animation graph and creates a runtime instance for it.
  ///
  /// Multiple graphs can be added to layer animations. They are evaluated in the order added.
  void AddAnimGraph(const WAnimGraphResourceHandle& hGraph);

  struct AnimClipInfo
  {
    WAnimationClipResourceHandle m_hClip;
  };

  const AnimClipInfo& GetAnimationClipInfo(WTempHashedString sClipName) const;

  /// Sets which animation clip is used for the named animation.
  ///
  /// Should only be called right at the start or when it is absolutely certain that an animation clip isn't in use right now,
  /// otherwise the running animation playback may produce weird results.
  void SetAnimationClipInfo(const WHashedString& sClipName, const AnimClipInfo& info);

private:
  void GenerateLocalResultProcessors(const WSkeletonResource* pSkeleton);

  WSkeletonResourceHandle m_hSkeleton;
  WAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  WVec3 m_vRootMotion = WVec3::MakeZero();
  WAngle m_RootRotationX;
  WAngle m_RootRotationY;
  WAngle m_RootRotationZ;

  WDynamicArray<ozz::math::SimdFloat4, WAlignedAllocatorWrapper> m_BlendMask;

  WAnimPoseGenerator* m_pPoseGenerator = nullptr;
  WSharedPtr<WBlackboard> m_pBlackboard = nullptr;

  WSmallArray<WUInt32, 8> m_CurrentLocalTransformOutputs;

  static WMutex s_SharedDataMutex;
  static WHashTable<WString, WSharedPtr<WAnimGraphSharedBoneWeights>> s_SharedBoneWeights;

  struct GraphInstance
  {
    WAnimGraphResourceHandle m_hAnimGraph;
    WUniquePtr<WAnimGraphInstance> m_pInstance;
  };

  WSmallArray<GraphInstance, 2> m_Instances;

  AnimClipInfo m_InvalidClipInfo;
  WHashTable<WHashedString, AnimClipInfo> m_AnimationClipMapping;

private:
  friend class WAnimGraphTriggerOutputPin;
  friend class WAnimGraphTriggerInputPin;
  friend class WAnimGraphBoneWeightsInputPin;
  friend class WAnimGraphBoneWeightsOutputPin;
  friend class WAnimGraphLocalPoseInputPin;
  friend class WAnimGraphLocalPoseOutputPin;
  friend class WAnimGraphNumberInputPin;
  friend class WAnimGraphNumberOutputPin;
  friend class WAnimGraphBoolInputPin;
  friend class WAnimGraphBoolOutputPin;

  WSmallArray<WAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  WSmallArray<WAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  WSmallArray<WAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;

  /// Accumulates weighted custom curve samples across all active pins for a single named curve.
  struct FinalCurveValue
  {
    WHashedString m_sName;
    float m_fWeightedSum = 0.0f; ///< Sum of (value * pinWeight) across all contributing pins.
    float m_fTotalWeight = 0.0f; ///< Sum of pin weights. Divide m_fWeightedSum by this to get the weighted average.
    float m_fMin = 0.0f;
    float m_fMax = 0.0f;
  };
  WSmallArray<FinalCurveValue, 4> m_FinalCurveValues;
};
