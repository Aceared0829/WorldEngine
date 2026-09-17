#pragma once

#include <Core/Messages/EventMessageSender.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

/// Message that is sent by WStateMachineState_SendMsg once the state is entered.
struct W_GAMEENGINE_DLL WMsgStateMachineStateChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgStateMachineStateChanged, WMessage);

  WHashedString m_sOldStateName;
  WHashedString m_sNewStateName;

private:
  const char* GetOldStateName() const { return m_sOldStateName; }
  void SetOldStateName(const char* szName) { m_sOldStateName.Assign(szName); }

  const char* GetNewStateName() const { return m_sNewStateName; }
  void SetNewStateName(const char* szName) { m_sNewStateName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

/// A state machine state that sends a WMsgStateMachineStateChanged on state enter or exit to the owner of the
/// state machine instance. Currently only works for WStateMachineComponent.
///
/// Optionally it can also log a message on state enter or exit.
class WStateMachineState_SendMsg : public WStateMachineState
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineState_SendMsg, WStateMachineState);

public:
  WStateMachineState_SendMsg(WStringView sName = WStringView());
  ~WStateMachineState_SendMsg();

  virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const override;
  virtual void OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WTime m_MessageDelay;

  bool m_bSendMessageOnEnter = true;
  bool m_bSendMessageOnExit = false;
  bool m_bLogOnEnter = false;
  bool m_bLogOnExit = false;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine state that sets the enabled flag on a game object and disables all other objects in the same group.
///
/// This state allows to easily switch the representation of a game object.
/// For instance you may have two objects states: normal and burning
/// You can basically just build two objects, one in the normal state, and one with all the effects needed for the fire.
/// Then you group both objects under a shared parent (e.g. with name 'visuals'), give both of them a name ('normal', 'burning') and disable one of them.
///
/// When the state machine transitions from the normal state to the burning state, you can then use this type of state
/// to say that from the 'visuals' group you want to activate the 'burning' object and deactivate all other objects in the same group.
///
/// Because the state activates one object and deactivates all others, you can have many different visuals and switch between them.
/// You can also only activate an object and keep the rest in the group as they are (e.g. to enable more and more effects).
/// If you only give a group path, but no object name, you can also use it to just disable all objects in a group.
/// If multiple objects in the same group have the same name, they will all get activated simultaneously.
///
/// Make sure that essential other objects (like the physics representation or other scripts) are located on other objects, that don't get deactivated.
class WStateMachineState_SwitchObject : public WStateMachineState
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineState_SwitchObject, WStateMachineState);

public:
  WStateMachineState_SwitchObject(WStringView sName = WStringView());
  ~WStateMachineState_SwitchObject();

  virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WString m_sGroupPath;
  WString m_sObjectToEnable;
  bool m_bDeactivateOthers = true;
};

//////////////////////////////////////////////////////////////////////////

class W_GAMEENGINE_DLL WStateMachineComponentManager : public WComponentManager<class WStateMachineComponent, WBlockStorageType::Compact>
{
public:
  WStateMachineComponentManager(WWorld* pWorld);
  ~WStateMachineComponentManager();

  virtual void Initialize() override;

  void Update(const WWorldModule::UpdateContext& context);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  WHashSet<WComponentHandle> m_ComponentsToReload;
};

//////////////////////////////////////////////////////////////////////////

/// A component that holds an WStateMachineInstance using the WStateMachineDescription from the resource assigned to this component.
class W_GAMEENGINE_DLL WStateMachineComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WStateMachineComponent, WComponent, WStateMachineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WStateMachineComponent

public:
  WStateMachineComponent();
  WStateMachineComponent(WStateMachineComponent&& other);
  ~WStateMachineComponent();

  WStateMachineComponent& operator=(WStateMachineComponent&& other);

  /// Returns the WStateMachineInstance owned by this component
  WStateMachineInstance* GetStateMachineInstance() { return m_pStateMachineInstance.Borrow(); }
  const WStateMachineInstance* GetStateMachineInstance() const { return m_pStateMachineInstance.Borrow(); }

  void SetResource(const WStateMachineResourceHandle& hResource);                // [ property ]
  const WStateMachineResourceHandle& GetResource() const { return m_hResource; } // [ property ]

  /// Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

  /// Sets the current state with the given name.
  bool SetState(WStringView sName); // [ scriptable ]

  /// Returns the name of the currently active state.
  WStringView GetCurrentState() const; // [ scriptable ]

  /// Sends a named event that state transitions can react to.
  void FireTransitionEvent(WStringView sEvent);

  void SetBlackboardName(const char* szName);                         // [ property ]
  const char* GetBlackboardName() const { return m_sBlackboardName; } // [ property ]

private:
  friend class WStateMachineState_SendMsg;
  void SendStateChangedMsg(WMsgStateMachineStateChanged& msg, WTime delay);
  void InstantiateStateMachine();
  void Update();

  WStateMachineResourceHandle m_hResource;
  WHashedString m_sInitialState;
  WHashedString m_sBlackboardName;

  WUniquePtr<WStateMachineInstance> m_pStateMachineInstance;

  WEventMessageSender<WMsgStateMachineStateChanged> m_StateChangedSender; // [ event ]
};
