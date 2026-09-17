#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Final output node that sends the pose to the skinned mesh.
///
/// This node outputs the final animation pose to the renderer. Every animation graph requires at least one
/// PoseResult node. Supports fade in/out control, target weight parameters, and bone weight masking for
/// partial animation application.
class W_RENDERERCORE_DLL WPoseResultAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WPoseResultAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WPoseResultAnimNode

public:
  WPoseResultAnimNode();
  ~WPoseResultAnimNode();

private:
  WTime m_FadeDuration = WTime::MakeFromMilliseconds(200); // [ property ]

  WAnimGraphLocalPoseInputPin m_InPose;                     // [ property ]
  WAnimGraphNumberInputPin m_InTargetWeight;                // [ property ]
  WAnimGraphNumberInputPin m_InFadeDuration;                // [ property ]
  WAnimGraphBoneWeightsInputPin m_InWeights;                // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFadedOut;               // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFadedIn;                // [ property ]
  WAnimGraphNumberOutputPin m_OutCurrentWeight;             // [ property ]

  struct InstanceData
  {
    float m_fStartWeight = 0.0f;
    float m_fEndWeight = 0.0f;
    WTime m_PlayTime = WTime::MakeZero();
    WTime m_EndTime = WTime::MakeZero();
  };
};
