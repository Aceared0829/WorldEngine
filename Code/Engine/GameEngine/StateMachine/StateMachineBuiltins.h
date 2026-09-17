#pragma once

#include <Core/Utils/Blackboard.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

/// A state machine state implementation that represents another state machine nested within this state. This can be used to build hierarchical state machines.
class W_GAMEENGINE_DLL WStateMachineState_NestedStateMachine : public WStateMachineState
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineState_NestedStateMachine, WStateMachineState);

public:
  WStateMachineState_NestedStateMachine(WStringView sName = WStringView());
  ~WStateMachineState_NestedStateMachine();

  virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const override;
  virtual void OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const override;
  virtual void Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) override;

  void SetResource(const WStateMachineResourceHandle& hResource);                // [ property ]
  const WStateMachineResourceHandle& GetResource() const { return m_hResource; } // [ property ]

  /// Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

private:
  WStateMachineResourceHandle m_hResource;
  WHashedString m_sInitialState;

  // Should the inner state machine keep its current state on exit and re-enter or should it exit as well and re-enter the initial state again.
  bool m_bKeepCurrentStateOnExit = false;

  struct InstanceData
  {
    WUniquePtr<WStateMachineInstance> m_pStateMachineInstance;
  };
};

//////////////////////////////////////////////////////////////////////////

/// A state machine state implementation that combines multiple sub states into one.
///
/// Can be used to build states in a more modular way. All calls are simply redirected to all sub states,
/// e.g. when entered it calls OnEnter on all its sub states.
class W_GAMEENGINE_DLL WStateMachineState_Compound : public WStateMachineState
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineState_Compound, WStateMachineState);

public:
  WStateMachineState_Compound(WStringView sName = WStringView());
  ~WStateMachineState_Compound();

  virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const override;
  virtual void OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const override;
  virtual void Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) override;

  WSmallArray<WStateMachineState*, 2> m_SubStates;

private:
  WStateMachineInternal::Compound m_Compound;
};

//////////////////////////////////////////////////////////////////////////

/// An enum that represents the operator of a comparison
struct W_GAMEENGINE_DLL WStateMachineLogicOperator
{
  using StorageType = WUInt8;

  enum Enum
  {
    And,
    Or,

    Default = And
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WStateMachineLogicOperator);

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that checks the instance's blackboard for the given conditions.
class W_GAMEENGINE_DLL WStateMachineTransition_BlackboardConditions : public WStateMachineTransition
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineTransition_BlackboardConditions, WStateMachineTransition);

public:
  WStateMachineTransition_BlackboardConditions();
  ~WStateMachineTransition_BlackboardConditions();

  virtual bool IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WEnum<WStateMachineLogicOperator> m_Operator;
  WHybridArray<WBlackboardCondition, 2> m_Conditions;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that triggers after the given time
class W_GAMEENGINE_DLL WStateMachineTransition_Timeout : public WStateMachineTransition
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineTransition_Timeout, WStateMachineTransition);

public:
  WStateMachineTransition_Timeout();
  ~WStateMachineTransition_Timeout();

  virtual bool IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WTime m_Timeout;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that combines multiple sub transition into one.
///
/// Can be used to build transitions in a more modular way. All calls are simply redirected to all sub transitions
/// and then combined with the given logic operator (AND, OR).
class W_GAMEENGINE_DLL WStateMachineTransition_Compound : public WStateMachineTransition
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineTransition_Compound, WStateMachineTransition);

public:
  WStateMachineTransition_Compound();
  ~WStateMachineTransition_Compound();

  virtual bool IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) override;

  WEnum<WStateMachineLogicOperator> m_Operator;
  WSmallArray<WStateMachineTransition*, 2> m_SubTransitions;

private:
  WStateMachineInternal::Compound m_Compound;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that triggers when a 'transition event' is sent.
class W_GAMEENGINE_DLL WStateMachineTransition_TransitionEvent : public WStateMachineTransition
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineTransition_TransitionEvent, WStateMachineTransition);

public:
  WStateMachineTransition_TransitionEvent();
  ~WStateMachineTransition_TransitionEvent();

  virtual bool IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WHashedString m_sEventName;
};
