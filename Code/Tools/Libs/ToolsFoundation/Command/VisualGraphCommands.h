#pragma once

#include <ToolsFoundation/Command/Command.h>

class WDocumentObject;
class WCommandHistory;
class WVisualGraphPin;

/// Command to remove a node from a visual graph.
///
/// Removes the specified node and all its connections. The operation is undoable.
class W_TOOLSFOUNDATION_DLL WRemoveNodeCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WRemoveNodeCommand, WCommand);

public:
  WRemoveNodeCommand();

public: // Properties
  WUuid m_Object;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  WDocumentObject* m_pObject = nullptr;
};

/// Command to move a node to a new position in the visual graph.
///
/// Changes the node's position in the graph editor. The operation is undoable.
class W_TOOLSFOUNDATION_DLL WMoveNodeCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WMoveNodeCommand, WCommand);

public:
  WMoveNodeCommand();

public: // Properties
  WUuid m_Object;
  WVec2 m_NewPos = WVec2::MakeZero();

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pObject = nullptr;
  WVec2 m_vOldPos = WVec2::MakeZero();
};

/// Command to connect two pins in a visual graph.
///
/// Creates a connection between an output pin and an input pin. The operation is undoable.
class W_TOOLSFOUNDATION_DLL WConnectNodePinsCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WConnectNodePinsCommand, WCommand);

public:
  WConnectNodePinsCommand();

public: // Properties
  WUuid m_ConnectionObject;
  WUuid m_ObjectSource;
  WUuid m_ObjectTarget;
  WString m_sSourcePin;
  WString m_sTargetPin;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pConnectionObject = nullptr;
  WDocumentObject* m_pObjectSource = nullptr;
  WDocumentObject* m_pObjectTarget = nullptr;
};

/// Command to disconnect two pins in a visual graph.
///
/// Removes an existing connection between pins. The operation is undoable.
class W_TOOLSFOUNDATION_DLL WDisconnectNodePinsCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WDisconnectNodePinsCommand, WCommand);

public:
  WDisconnectNodePinsCommand();

public: // Properties
  WUuid m_ConnectionObject;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pConnectionObject = nullptr;
  const WDocumentObject* m_pObjectSource = nullptr;
  const WDocumentObject* m_pObjectTarget = nullptr;
  WString m_sSourcePin;
  WString m_sTargetPin;
};

/// Helper class for executing common visual graph command operations.
class W_TOOLSFOUNDATION_DLL WNodeCommands
{
public:
  /// Creates a connection between two pins by adding a connection object and connecting it.
  static WStatus AddAndConnectCommand(WCommandHistory* pHistory, const WRTTI* pConnectionType, const WVisualGraphPin& sourcePin, const WVisualGraphPin& targetPin);

  /// Disconnects pins and removes the connection object.
  static WStatus DisconnectAndRemoveCommand(WCommandHistory* pHistory, const WUuid& connectionObject);
};
