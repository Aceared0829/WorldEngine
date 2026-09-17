#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Switches between multiple poses with automatic fade transitions.
///
/// This node selects one pose from multiple inputs by index and smoothly fades to it over a configurable duration.
/// Useful for state-based animations where you need instant transitions with blending (weapon switching, combat stances).
class W_RENDERERCORE_DLL WSwitchPoseAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSwitchPoseAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSelectPoseAnimNode

private:
  WTime m_TransitionDuration = WTime::MakeFromMilliseconds(200); // [ property ]
  WUInt8 m_uiPosesCount = 0;                                      // [ property ]
  WHybridArray<WAnimGraphLocalPoseInputPin, 4> m_InPoses;        // [ property ]
  WAnimGraphNumberInputPin m_InIndex;                             // [ property ]
  WAnimGraphLocalPoseOutputPin m_OutPose;                         // [ property ]

  struct InstanceData
  {
    WTime m_TransitionTime;
    WInt8 m_iTransitionFromIndex = -1;
    WInt8 m_iTransitionToIndex = -1;
  };
};
