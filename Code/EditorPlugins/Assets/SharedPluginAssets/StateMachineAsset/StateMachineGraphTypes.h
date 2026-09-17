#pragma once

#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class WStateMachineState;
class WStateMachineTransition;

/// Connection representing a state machine transition.
///
/// Since the editor doesn't support choosing different connection types, users can switch the transition type
/// in the properties panel instead.
class W_SHAREDPLUGINASSETS_DLL WStateMachineConnection : public WDocumentObject_ConnectionBase
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineConnection, WDocumentObject_ConnectionBase);

public:
  WStateMachineTransition* m_pType = nullptr;
};

/// Base class for nodes in a state machine graph.
class W_SHAREDPLUGINASSETS_DLL WStateMachineNodeBase : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineNodeBase, WReflectedClass);
};

/// Node representing a state machine state.
///
/// Does not use WStateMachineState directly to allow users to switch the state type in the properties panel,
/// similar to transition handling.
class W_SHAREDPLUGINASSETS_DLL WStateMachineNode : public WStateMachineNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineNode, WStateMachineNodeBase);

public:
  WString m_sName;
  WStateMachineState* m_pType = nullptr;
  bool m_bIsInitialState = false;
};

/// Node representing the "any state" in a state machine.
///
/// Used when a transition with the same conditions is possible from any other state.
/// Instead of creating many identical connections, an "any state" node simplifies the graph and maintenance.
/// At runtime, there is no "any state" object - only the transitions are stored.
class W_SHAREDPLUGINASSETS_DLL WStateMachineNodeAny : public WStateMachineNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineNodeAny, WStateMachineNodeBase);
};
