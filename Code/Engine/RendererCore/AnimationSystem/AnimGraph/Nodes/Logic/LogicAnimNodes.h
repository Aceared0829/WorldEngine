#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Boolean AND logic node that outputs true only when all inputs are true.
///
/// This node combines multiple boolean inputs using AND logic. Useful for combining multiple
/// conditions before triggering animations or state transitions.
class W_RENDERERCORE_DLL WLogicAndAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WLogicAndAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WLogicAndAnimNode

public:
  WLogicAndAnimNode();
  ~WLogicAndAnimNode();

private:
  WUInt8 m_uiBoolCount = 2;                          // [ property ]
  WHybridArray<WAnimGraphBoolInputPin, 2> m_InBool; // [ property ]
  WAnimGraphBoolOutputPin m_OutIsTrue;               // [ property ]
  WAnimGraphBoolOutputPin m_OutIsFalse;              // [ property ]
};

/// Forwards a trigger event only when a boolean condition is true.
///
/// This node gates trigger events based on a boolean input. The trigger is only forwarded
/// when the boolean condition is satisfied. Useful for conditional event routing.
class W_RENDERERCORE_DLL WLogicEventAndAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WLogicEventAndAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WLogicEventAndAnimNode

public:
  WLogicEventAndAnimNode();
  ~WLogicEventAndAnimNode();

private:
  WAnimGraphTriggerInputPin m_InActivate;      // [ property ]
  WAnimGraphBoolInputPin m_InBool;             // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnActivated; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Boolean OR logic node that outputs true when any input is true.
///
/// This node combines multiple boolean inputs using OR logic. Useful for triggering animations
/// when any of several conditions are met.
class W_RENDERERCORE_DLL WLogicOrAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WLogicOrAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WLogicOrAnimNode

public:
  WLogicOrAnimNode();
  ~WLogicOrAnimNode();

private:
  WUInt8 m_uiBoolCount = 2;                          // [ property ]
  WHybridArray<WAnimGraphBoolInputPin, 2> m_InBool; // [ property ]
  WAnimGraphBoolOutputPin m_OutIsTrue;               // [ property ]
  WAnimGraphBoolOutputPin m_OutIsFalse;              // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_RENDERERCORE_DLL WLogicNotAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WLogicNotAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WLogicNotAnimNode

public:
  WLogicNotAnimNode();
  ~WLogicNotAnimNode();

private:
  WAnimGraphBoolInputPin m_InBool;   // [ property ]
  WAnimGraphBoolOutputPin m_OutBool; // [ property ]
};
