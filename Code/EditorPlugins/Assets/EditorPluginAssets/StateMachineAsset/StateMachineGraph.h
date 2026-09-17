#pragma once

#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

/// Visual graph pin for state machine nodes.
///
/// Represents connection points between states. Output pins trigger transitions, input pins receive them.
class WStateMachinePin : public WVisualGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachinePin, WVisualGraphPin);

public:
  WStateMachinePin(Type type, const WDocumentObject* pObject);
};

/// Object manager for state machine graphs.
///
/// Manages states and transitions in a state machine. Ensures that exactly one initial state exists
/// and handles special states like the "Any State" node which can transition to any other state.
class WStateMachineNodeManager : public WVisualGraphObjectManager
{
public:
  WStateMachineNodeManager();
  ~WStateMachineNodeManager();

  bool IsInitialState(const WDocumentObject* pObject) const;
  const WDocumentObject* GetInitialState() const;

  bool IsAnyState(const WDocumentObject* pObject) const;

private:
  virtual bool InternalIsNode(const WDocumentObject* pObject) const override;
  virtual WStatus InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const WDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual const WRTTI* GetConnectionType() const override;

  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
};

/// Command to designate which state is the initial state in a state machine.
///
/// Sets or changes the initial state of a state machine graph. The operation is undoable.
class WStateMachine_SetInitialStateCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachine_SetInitialStateCommand, WCommand);

public:
  WStateMachine_SetInitialStateCommand();

public: // Properties
  WUuid m_NewInitialStateObject;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pOldInitialStateObject = nullptr;
  WDocumentObject* m_pNewInitialStateObject = nullptr;
};
