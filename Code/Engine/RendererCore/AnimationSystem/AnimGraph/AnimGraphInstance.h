#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WGameObject;
class WAnimGraph;
class WAnimController;

/// Runtime state for a single animation graph, owned by WAnimController.
///
/// While WAnimGraph defines the structure (nodes, connections, logic), WAnimGraphInstance holds
/// the per-character runtime state needed to execute that graph. This allows one graph definition
/// to be shared by many characters, each with their own instance storing playback positions,
/// blend weights, transition states, and pin values.
///
/// ## Node Instance Data Access
///
/// Nodes that need per-instance state (playback time, transition progress, etc.) access it via
/// GetAnimNodeInstanceData<T>(). The instance data offset is pre-computed during graph preparation
/// and stored in the node.
///
/// **Example from a node's Step() method:**
/// ```cpp
/// InstanceData* pState = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);
/// pState->m_PlaybackTime += tDiff;
/// ```
class W_RENDERERCORE_DLL WAnimGraphInstance
{
  W_DISALLOW_COPY_AND_ASSIGN(WAnimGraphInstance);

public:
  WAnimGraphInstance();
  ~WAnimGraphInstance();

  /// Allocates instance data and initializes pin state pointers for the given graph.
  ///
  /// Must be called once after construction. The graph must have been prepared via PrepareForUse().
  void Configure(const WAnimGraph& animGraph);

  /// Executes all nodes in the graph to generate animation output.
  void Update(WAnimController& ref_controller, WTime diff, WGameObject* pTarget, const WSkeletonResource* pSekeltonResource);

  /// Retrieves the instance data for a specific node.
  ///
  /// Nodes use this to access their per-instance state (playback time, blend weights, etc.).
  /// The type T should match the InstanceData struct defined in the node class.
  template <typename T>
  T* GetAnimNodeInstanceData(const WAnimGraphNode& node)
  {
    return reinterpret_cast<T*>(WInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), node.m_uiInstanceDataOffset));
  }


private:
  const WAnimGraph* m_pAnimGraph = nullptr;

  WBlob m_InstanceData;

  // EXTEND THIS if a new type is introduced
  WInt8* m_pTriggerInputPinStates = nullptr;
  double* m_pNumberInputPinStates = nullptr;
  bool* m_pBoolInputPinStates = nullptr;
  WUInt16* m_pBoneWeightInputPinStates = nullptr;
  WDynamicArray<WHybridArray<WUInt16, 1>> m_LocalPoseInputPinStates;
  WUInt16* m_pModelPoseInputPinStates = nullptr;

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
};
