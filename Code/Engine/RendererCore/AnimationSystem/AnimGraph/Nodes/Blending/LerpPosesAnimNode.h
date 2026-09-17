#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Linearly interpolates (blends) between multiple poses.
///
/// This node blends 2 or more poses using a lerp parameter (0-1 or 0-N for multiple poses).
/// Weights are automatically normalized. Common use cases include blending walk and run animations,
/// or smoothly transitioning between any set of poses.
class W_RENDERERCORE_DLL WLerpPosesAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WLerpPosesAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WLerpPosesAnimNode

public:
  WLerpPosesAnimNode();
  ~WLerpPosesAnimNode();

  float m_fLerp = 0.5f;                                     // [ property ]

private:
  WUInt8 m_uiPosesCount = 0;                               // [ property ]
  WHybridArray<WAnimGraphLocalPoseInputPin, 2> m_InPoses; // [ property ]
  WAnimGraphNumberInputPin m_InLerp;                       // [ property ]
  WAnimGraphLocalPoseOutputPin m_OutPose;                  // [ property ]
};
