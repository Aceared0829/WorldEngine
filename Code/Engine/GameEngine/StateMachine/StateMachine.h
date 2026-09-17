#pragma once

#include <GameEngine/StateMachine/Implementation/StateMachineInstanceData.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class WComponent;
class WWorld;
class WBlackboard;
class WStateMachineInstance;

/// Base class for a state in a state machine.
///
/// Note that states are shared between multiple instances and thus
/// shouldn't modify any data on their own but always operate on the passed instance and instance data.
/// \see WStateMachineInstanceDataDesc
class W_GAMEENGINE_DLL WStateMachineState : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineState, WReflectedClass);

public:
  WStateMachineState(WStringView sName = WStringView());

  void SetName(WStringView sName);
  WStringView GetName() const { return m_sName; }
  const WHashedString& GetNameHashed() const { return m_sName; }

  virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const = 0;
  virtual void OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const;
  virtual void Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const;

  virtual WResult Serialize(WStreamWriter& inout_stream) const;
  virtual WResult Deserialize(WStreamReader& inout_stream);

  /// Returns whether this state needs additional instance data and if so fills the out_desc.
  ///
  /// \see WStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc);

private:
  // These are dummy functions for the scripting reflection
  void Reflection_OnEnter(WStateMachineInstance* pStateMachineInstance, const WStateMachineState* pFromState);
  void Reflection_OnExit(WStateMachineInstance* pStateMachineInstance, const WStateMachineState* pToState);
  void Reflection_Update(WStateMachineInstance* pStateMachineInstance, WTime deltaTime);

  WHashedString m_sName;
};

class W_GAMEENGINE_DLL WStateMachineState_Empty final : public WStateMachineState
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineState_Empty, WStateMachineState);

public:
  WStateMachineState_Empty(WStringView sName = WStringView());
  ~WStateMachineState_Empty() = default;

  virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const override {}
};

struct WStateMachineState_ScriptBaseClassFunctions
{
  enum Enum
  {
    OnEnter,
    OnExit,
    Update,

    Count
  };
};

/// Base class for a transition in a state machine. The target state of a transition is automatically set
/// once its condition has been met.
///
/// Same as with states, transitions are also shared between multiple instances and thus
/// should decide their condition based on the passed instance and instance data.
/// \see WStateMachineInstanceDataDesc
class W_GAMEENGINE_DLL WStateMachineTransition : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineTransition, WReflectedClass);

  virtual bool IsConditionMet(WStateMachineInstance& ref_instance, void* pInstanceData) const = 0;

  virtual WResult Serialize(WStreamWriter& inout_stream) const;
  virtual WResult Deserialize(WStreamReader& inout_stream);

  /// Returns whether this transition needs additional instance data and if so fills the out_desc.
  ///
  /// \see WStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc);
};

/// The state machine description defines the structure of a state machine like e.g.
/// what states it has and how to transition between them.
/// Once an instance is created from a description it is not allowed to change the description afterwards.
class W_GAMEENGINE_DLL WStateMachineDescription : public WRefCounted
{
  W_DISALLOW_COPY_AND_ASSIGN(WStateMachineDescription);

public:
  WStateMachineDescription();
  ~WStateMachineDescription();

  /// Adds the given state to the description and returns the state index.
  WUInt32 AddState(WUniquePtr<WStateMachineState>&& pState);

  /// Adds the given transition between the two given states. A uiFromStateIndex of WInvalidIndex generates a transition that can be done from any other possible state.
  void AddTransition(WUInt32 uiFromStateIndex, WUInt32 uiToStateIndex, WUniquePtr<WStateMachineTransition>&& pTransistion);

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

private:
  friend class WStateMachineInstance;

  struct TransitionContext
  {
    WUniquePtr<WStateMachineTransition> m_pTransition;
    WUInt32 m_uiToStateIndex = 0;
    WUInt32 m_uiInstanceDataOffset = WInvalidIndex;
  };

  using TransitionArray = WSmallArray<TransitionContext, 2>;
  TransitionArray m_FromAnyTransitions;

  struct StateContext
  {
    WUniquePtr<WStateMachineState> m_pState;
    TransitionArray m_Transitions;
    WUInt32 m_uiInstanceDataOffset = WInvalidIndex;
  };

  WDynamicArray<StateContext> m_States;
  WHashTable<WHashedString, WUInt32> m_StateNameToIndexTable;

  WInstanceDataAllocator m_InstanceDataAllocator;
};

/// The state machine instance represents the actual state machine.
/// Typically it is created from a description but for small use cases it can also be used without a description.
class W_GAMEENGINE_DLL WStateMachineInstance
{
  W_DISALLOW_COPY_AND_ASSIGN(WStateMachineInstance);

public:
  WStateMachineInstance(WReflectedClass& ref_owner, const WSharedPtr<const WStateMachineDescription>& pDescription = nullptr);
  ~WStateMachineInstance();

  WResult SetState(WStateMachineState* pState);
  WResult SetState(WUInt32 uiStateIndex);
  WResult SetState(const WHashedString& sStateName);
  WResult SetStateOrFallback(const WHashedString& sStateName, WUInt32 uiFallbackStateIndex = 0);
  WStateMachineState* GetCurrentState() { return m_pCurrentState; }

  void Update(WTime deltaTime);

  WReflectedClass& GetOwner() { return m_Owner; }
  WWorld* GetOwnerWorld();

  void SetBlackboard(const WSharedPtr<WBlackboard>& pBlackboard);
  const WSharedPtr<WBlackboard>& GetBlackboard() const { return m_pBlackboard; }

  /// Returns how long the state machine is in its current state
  WTime GetTimeInCurrentState() const { return m_TimeInCurrentState; }

  /// Sends a named event that state transitions can react to.
  void FireTransitionEvent(WStringView sEvent);

  WStringView GetCurrentTransitionEvent() const { return m_sCurrentTransitionEvent; }

private:
  W_ALLOW_PRIVATE_PROPERTIES(WStateMachineInstance);

  bool Reflection_SetState(const WHashedString& sStateName);
  WComponent* Reflection_GetOwnerComponent() const;
  WBlackboard* Reflection_GetBlackboard() const { return m_pBlackboard.Borrow(); }

  void SetStateInternal(WUInt32 uiStateIndex);
  void EnterCurrentState(const WStateMachineState* pFromState);
  void ExitCurrentState(const WStateMachineState* pToState);
  WUInt32 FindNewStateToTransitionTo();

  W_ALWAYS_INLINE void* GetInstanceData(WUInt32 uiOffset)
  {
    return WInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), uiOffset);
  }

  W_ALWAYS_INLINE void* GetCurrentStateInstanceData()
  {
    if (m_pDescription != nullptr && m_uiCurrentStateIndex < m_pDescription->m_States.GetCount())
    {
      return GetInstanceData(m_pDescription->m_States[m_uiCurrentStateIndex].m_uiInstanceDataOffset);
    }
    return nullptr;
  }

  WReflectedClass& m_Owner;
  WSharedPtr<const WStateMachineDescription> m_pDescription;
  WSharedPtr<WBlackboard> m_pBlackboard;

  WStateMachineState* m_pCurrentState = nullptr;
  WUInt32 m_uiCurrentStateIndex = WInvalidIndex;
  WTime m_TimeInCurrentState;
  WStringView m_sCurrentTransitionEvent;

  const WStateMachineDescription::TransitionArray* m_pCurrentTransitions = nullptr;

  WBlob m_InstanceData;
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WStateMachineInstance);
