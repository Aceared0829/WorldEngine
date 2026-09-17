#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Evaluates a custom math expression with up to 4 input values.
///
/// This node allows defining custom mathematical operations using expressions like "a * 2 + b".
/// Supports standard math operations and functions. Useful for computing blend weights, animation speeds,
/// or other derived values without creating dedicated nodes.
class W_RENDERERCORE_DLL WMathExpressionAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WMathExpressionAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WLogicAndAnimNode

public:
  WMathExpressionAnimNode();
  ~WMathExpressionAnimNode();

  void SetExpression(WString sExpr);
  WString GetExpression() const;

private:
  WAnimGraphNumberInputPin m_ValueAPin;  // [ property ]
  WAnimGraphNumberInputPin m_ValueBPin;  // [ property ]
  WAnimGraphNumberInputPin m_ValueCPin;  // [ property ]
  WAnimGraphNumberInputPin m_ValueDPin;  // [ property ]
  WAnimGraphNumberOutputPin m_ResultPin; // [ property ]

  WString m_sExpression;

  struct InstanceData
  {
    WMathExpression m_mExpression;
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Compares a number against a reference value and outputs boolean results.
///
/// This node performs comparisons (less than, greater than, equal) and outputs the result as booleans.
/// Useful for condition checks in state machines or for controlling animation blending based on thresholds.
class W_RENDERERCORE_DLL WCompareNumberAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WCompareNumberAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WCompareNumberAnimNode

public:
  double m_fReferenceValue = 0.0f;           // [ property ]
  WEnum<WComparisonOperator> m_Comparison; // [ property ]

private:
  WAnimGraphNumberInputPin m_InNumber;      // [ property ]
  WAnimGraphNumberInputPin m_InReference;   // [ property ]
  WAnimGraphBoolOutputPin m_OutIsTrue;      // [ property ]
  WAnimGraphBoolOutputPin m_OutIsFalse;     // [ property ]
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_RENDERERCORE_DLL WBoolToNumberAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WBoolToNumberAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WBoolToNumberAnimNode

public:
  WBoolToNumberAnimNode();
  ~WBoolToNumberAnimNode();

  double m_fFalseValue = 0.0f;
  double m_fTrueValue = 1.0f;

private:
  WAnimGraphBoolInputPin m_InValue;      // [ property ]
  WAnimGraphNumberOutputPin m_OutNumber; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_RENDERERCORE_DLL WBoolToTriggerAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WBoolToTriggerAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WBoolToNumberAnimNode

public:
  WBoolToTriggerAnimNode();
  ~WBoolToTriggerAnimNode();

private:
  WAnimGraphBoolInputPin m_InValue;        // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnTrue;  // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFalse; // [ property ]

  struct InstanceData
  {
    WInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};
