#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Sends a named event message to the target game object when triggered.
///
/// This node forwards animation events to gameplay code. Use it to trigger sound effects at footstep times,
/// spawn particles during attack animations, or send any gameplay event at specific animation frames.
/// The event is sent when the trigger input is activated.
class W_RENDERERCORE_DLL WSendEventAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSendEventAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSendEventAnimNode

public:
  void SetEventName(const char* szSz) { m_sEventName.Assign(szSz); }
  const char* GetEventName() const { return m_sEventName.GetString(); }

private:
  WHashedString m_sEventName;             // [ property ]
  WAnimGraphTriggerInputPin m_InActivate; // [ property ]
};
