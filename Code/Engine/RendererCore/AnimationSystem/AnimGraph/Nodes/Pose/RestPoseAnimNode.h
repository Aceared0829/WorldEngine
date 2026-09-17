#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// Outputs the skeleton's rest/bind pose.
///
/// This node provides the default T-pose or A-pose from the skeleton definition. Commonly used as a fallback
/// when no animation is active, as a base for additive blending, or when animations fail to load.
class W_RENDERERCORE_DLL WRestPoseAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WRestPoseAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WRestPoseAnimNode

private:
  WAnimGraphLocalPoseOutputPin m_OutPose; // [ property ]
};
