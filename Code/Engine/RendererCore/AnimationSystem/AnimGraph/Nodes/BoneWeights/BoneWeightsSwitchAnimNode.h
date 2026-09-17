#pragma once

#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

struct WAnimGraphSharedBoneWeights;

/// Switches between different bone weight masks with smooth transitions.
///
/// This node selects one bone weight mask from multiple inputs by index and smoothly fades to it over a
/// configurable duration. Useful for dynamic body part masking (weapon-specific bone masks, injury states).
class W_RENDERERCORE_DLL WSwitchBoneWeightsAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSwitchBoneWeightsAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSwitchBoneWeightsAnimNode

private:
  WTime m_TransitionDuration = WTime::MakeFromMilliseconds(200); // [ property ]
  WAnimGraphNumberInputPin m_InIndex;                             // [ property ]
  WUInt8 m_uiWeightsCount = 0;                                    // [ property ]
  WHybridArray<WAnimGraphBoneWeightsInputPin, 2> m_InWeights;    // [ property ]
  WAnimGraphBoneWeightsOutputPin m_OutWeights;                    // [ property ]

  struct InstanceData
  {
    WTime m_TransitionTime;
    WInt8 m_iTransitionFromIndex = -1;
    WInt8 m_iTransitionToIndex = -1;
    WSharedPtr<WAnimGraphSharedBoneWeights> m_pBlendedBoneWeights;
  };
};
