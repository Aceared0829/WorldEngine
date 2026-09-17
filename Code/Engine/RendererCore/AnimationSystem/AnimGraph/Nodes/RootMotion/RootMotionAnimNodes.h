#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Applies rotation adjustments to extracted root motion.
///
/// This node modifies the root rotation values (pitch, yaw, roll) extracted from animations.
/// Useful for scaling or filtering specific rotation components of character movement.
class W_RENDERERCORE_DLL WRootRotationAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WRootRotationAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WRootRotationAnimNode

public:
  WRootRotationAnimNode();
  ~WRootRotationAnimNode();

private:
  WAnimGraphNumberInputPin m_InRotateX; // [ property ]
  WAnimGraphNumberInputPin m_InRotateY; // [ property ]
  WAnimGraphNumberInputPin m_InRotateZ; // [ property ]
};
