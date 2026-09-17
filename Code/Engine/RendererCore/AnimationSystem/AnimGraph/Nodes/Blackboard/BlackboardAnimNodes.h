#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Writes a number value to the animation graph's blackboard.
///
/// This node stores numerical values in the blackboard for sharing between nodes or with gameplay code.
/// Use it to store locomotion parameters, state values, or any data that needs to be accessed by multiple nodes.
class W_RENDERERCORE_DLL WSetBlackboardNumberAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSetBlackboardNumberAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSetBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  double m_fNumber = 0.0f;                      // [ property ]

private:
  WAnimGraphTriggerInputPin m_InActivate;      // [ property ]
  WAnimGraphNumberInputPin m_InNumber;         // [ property ]
  WHashedString m_sBlackboardEntry;            // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Reads a number value from the animation graph's blackboard.
///
/// This node retrieves numerical values stored in the blackboard. Use it to read locomotion parameters
/// from gameplay code or values written by other nodes.
class W_RENDERERCORE_DLL WGetBlackboardNumberAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WGetBlackboardNumberAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WGetBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  WHashedString m_sBlackboardEntry;            // [ property ]
  WAnimGraphNumberOutputPin m_OutNumber;       // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Compares a blackboard number against a reference value and outputs boolean results.
///
/// This node reads a number from the blackboard, performs a comparison, and outputs the result as booleans.
/// Useful for state transitions based on blackboard values.
class W_RENDERERCORE_DLL WCompareBlackboardNumberAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WCompareBlackboardNumberAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WCompareBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  double m_fReferenceValue = 0.0;               // [ property ]
  WEnum<WComparisonOperator> m_Comparison;    // [ property ]

private:
  WHashedString m_sBlackboardEntry;            // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnTrue;      // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFalse;     // [ property ]
  WAnimGraphBoolOutputPin m_OutIsTrue;         // [ property ]
  WAnimGraphBoolOutputPin m_OutIsFalse;        // [ property ]

  struct InstanceData
  {
    WInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_RENDERERCORE_DLL WCheckBlackboardBoolAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WCheckBlackboardBoolAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WCheckBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  WHashedString m_sBlackboardEntry;            // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnTrue;      // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnFalse;     // [ property ]
  WAnimGraphBoolOutputPin m_OutBool;           // [ property ]

  struct InstanceData
  {
    WInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_RENDERERCORE_DLL WSetBlackboardBoolAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WSetBlackboardBoolAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WSetBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  bool m_bBool = false;                         // [ property ]

private:
  WHashedString m_sBlackboardEntry;            // [ property ]
  WAnimGraphTriggerInputPin m_InActivate;      // [ property ]
  WAnimGraphBoolInputPin m_InBool;             // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_RENDERERCORE_DLL WGetBlackboardBoolAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WGetBlackboardBoolAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WGetBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  WHashedString m_sBlackboardEntry;            // [ property ]
  WAnimGraphBoolOutputPin m_OutBool;           // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_RENDERERCORE_DLL WOnBlackboardValueChangedAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WOnBlackboardValueChangedAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // WOnBlackboardValuechangedAnimNode

public:
  void SetBlackboardEntry(const char* szEntry);    // [ property ]
  const char* GetBlackboardEntry() const;          // [ property ]

private:
  WHashedString m_sBlackboardEntry;               // [ property ]
  WAnimGraphTriggerOutputPin m_OutOnValueChanged; // [ property ]

  struct InstanceData
  {
    WUInt32 m_uiChangeCounter = WInvalidIndex;
  };
};
