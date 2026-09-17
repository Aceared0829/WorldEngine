#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Defines the structure and logic of an animation graph as a directed acyclic graph (DAG) of nodes.
///
/// An animation graph consists of interconnected nodes that process animation data. Nodes can sample animation clips,
/// blend poses, handle logic and math operations, react to events, and output final poses. Data flows through
/// typed pins that connect nodes together.
///
/// ## Architecture
///
/// **Graph Definition vs Instance:**
/// - WAnimGraph is the shared definition (nodes, connections, structure)
/// - WAnimGraphInstance is per-character runtime state (playback time, blend weights, pin values)
/// - One graph can be shared by hundreds of characters, each with their own instance
///
/// **Node Types:**
/// - Pose sampling: Sample animation clips, blend spaces, sequences
/// - Blending: Lerp, switch between poses, blend spaces (1D/2D)
/// - Logic: Boolean operations, comparisons, state machines
/// - Math: Add, multiply, clamp numbers for blend weights
/// - Events: Trigger gameplay events at specific animation times
/// - Bone weights: Control which bones are affected by animations
/// - Root motion: Extract character movement from animations
/// - Output: Final pose output to renderer
///
/// **Pin Types:**
/// - Trigger: One-shot events (animation finished, state entered)
/// - Number: Blend weights, playback speeds, parameters
/// - Bool: Conditions, flags
/// - BoneWeights: Masks controlling which bones are affected
/// - LocalPose: Bone transforms in local space
/// - ModelPose: Bone transforms in model space
///
/// ## Usage Pattern
///
/// Animation graphs are typically created by the editor and stored in WAnimGraphResource.
/// At runtime, WAnimController is used to load a graph resource, create an WAnimGraphInstance
/// and updates it each frame.
class W_RENDERERCORE_DLL WAnimGraph
{
  W_DISALLOW_COPY_AND_ASSIGN(WAnimGraph);

public:
  WAnimGraph();
  ~WAnimGraph();

  void Clear();

  WAnimGraphNode* AddNode(WUniquePtr<WAnimGraphNode>&& pNode);
  void AddConnection(const WAnimGraphNode* pSrcNode, WStringView sSrcPinName, WAnimGraphNode* pDstNode, WStringView sDstPinName);

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

  const WInstanceDataAllocator& GetInstanceDataAlloator() const { return m_InstanceDataAllocator; }
  WArrayPtr<const WUniquePtr<WAnimGraphNode>> GetNodes() const { return m_Nodes; }

  /// Prepares the graph for use by sorting nodes, assigning pin indices, and allocating instance data descriptors.
  ///
  /// Must be called after all nodes and connections have been added and before creating instances.
  /// This is automatically called when deserializing a graph or when creating instances.
  void PrepareForUse();

private:
  friend class WAnimGraphInstance;

  struct ConnectionTo
  {
    WString m_sSrcPinName;
    const WAnimGraphNode* m_pDstNode = nullptr;
    WString m_sDstPinName;
    WAnimGraphPin* m_pSrcPin = nullptr;
    WAnimGraphPin* m_pDstPin = nullptr;
  };

  struct ConnectionsTo
  {
    WHybridArray<ConnectionTo, 2> m_To;
  };

  void SortNodesByPriority();
  void PreparePinMapping();
  void AssignInputPinIndices();
  void AssignOutputPinIndices();
  WUInt16 ComputeNodePriority(const WAnimGraphNode* pNode, WMap<const WAnimGraphNode*, WUInt16>& inout_Prios, WUInt16& inout_uiOutputPrio) const;

  bool m_bPreparedForUse = true;
  WUInt32 m_uiInputPinCounts[WAnimGraphPin::Type::ENUM_COUNT];
  WUInt32 m_uiPinInstanceDataOffset[WAnimGraphPin::Type::ENUM_COUNT];
  WMap<const WAnimGraphNode*, ConnectionsTo> m_From;

  WDynamicArray<WUniquePtr<WAnimGraphNode>> m_Nodes;
  WDynamicArray<WHybridArray<WUInt16, 1>> m_OutputPinToInputPinMapping[WAnimGraphPin::ENUM_COUNT];
  WInstanceDataAllocator m_InstanceDataAllocator;

  friend class WAnimGraphTriggerOutputPin;
  friend class WAnimGraphNumberOutputPin;
  friend class WAnimGraphBoolOutputPin;
  friend class WAnimGraphBoneWeightsOutputPin;
  friend class WAnimGraphLocalPoseOutputPin;
};
