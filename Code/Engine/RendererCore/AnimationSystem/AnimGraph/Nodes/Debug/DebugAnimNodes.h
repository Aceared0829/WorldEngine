#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Base class for logging nodes that output debugging information.
///
/// This node logs text and number values to the console when triggered. Use derived classes
/// (WLogInfoAnimNode, WLogErrorAnimNode) for different log levels. Useful for debugging animation
/// state, tracking blend weights, or monitoring node execution.
class W_RENDERERCORE_DLL WLogAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WLogAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WLogAnimNode

protected:
  WString m_sText;                                        // [ property ]
  WAnimGraphTriggerInputPin m_InActivate;                 // [ property ]
  WUInt8 m_uiNumberCount = 1;                             // [ property ]
  WHybridArray<WAnimGraphNumberInputPin, 2> m_InNumbers; // [ property ]
};

/// Logs informational messages to the console.
class W_RENDERERCORE_DLL WLogInfoAnimNode : public WLogAnimNode
{
  W_ADD_DYNAMIC_REFLECTION(WLogInfoAnimNode, WLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // WLogAnimNode

protected:
  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
};

/// Logs error messages to the console.
class W_RENDERERCORE_DLL WLogErrorAnimNode : public WLogAnimNode
{
  W_ADD_DYNAMIC_REFLECTION(WLogErrorAnimNode, WLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // WLogAnimNode

protected:
  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
};
