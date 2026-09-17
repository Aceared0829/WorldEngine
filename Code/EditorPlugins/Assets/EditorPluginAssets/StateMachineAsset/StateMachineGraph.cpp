#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, StateMachine)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WQtVisualGraphScene::GetPinFactory().RegisterCreator(WGetStaticRTTI<WStateMachinePin>(), [](const WRTTI* pRtti)->WQtVisualGraphPin* { return new WQtStateMachinePin(); });
    WQtVisualGraphScene::GetConnectionFactory().RegisterCreator(WGetStaticRTTI<WStateMachineConnection>(), [](const WRTTI* pRtti)->WQtVisualGraphConnection* { return new WQtStateMachineConnection(); });
    WQtVisualGraphScene::GetNodeFactory().RegisterCreator(WGetStaticRTTI<WStateMachineNodeBase>(), [](const WRTTI* pRtti)->WQtVisualGraphNode* { return new WQtStateMachineNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WQtVisualGraphScene::GetPinFactory().UnregisterCreator(WGetStaticRTTI<WStateMachinePin>());
    WQtVisualGraphScene::GetConnectionFactory().UnregisterCreator(WGetStaticRTTI<WStateMachineConnection>());
    WQtVisualGraphScene::GetNodeFactory().UnregisterCreator(WGetStaticRTTI<WStateMachineNodeBase>());
  }

W_END_SUBSYSTEM_DECLARATION;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachinePin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachinePin::WStateMachinePin(Type type, const WDocumentObject* pObject)
  : WVisualGraphPin(type, type == Type::Input ? "Enter" : "Exit", WColor::Grey, pObject)
{
}

//////////////////////////////////////////////////////////////////////////

constexpr const char* s_szIsInitialState = "IsInitialState";

WStateMachineNodeManager::WStateMachineNodeManager()
{
  m_StructureEvents.AddEventHandler(WMakeDelegate(&WStateMachineNodeManager::StructureEventHandler, this));
}

WStateMachineNodeManager::~WStateMachineNodeManager()
{
  m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WStateMachineNodeManager::StructureEventHandler, this));
}

bool WStateMachineNodeManager::IsInitialState(const WDocumentObject* pObject) const
{
  WVariant val = pObject->GetTypeAccessor().GetValue(s_szIsInitialState);
  if (val.IsValid())
  {
    return val.Get<bool>() == true;
  }

  return false;
}

const WDocumentObject* WStateMachineNodeManager::GetInitialState() const
{
  for (auto pObject : GetRootObject()->GetChildren())
  {
    if (IsNode(pObject) && IsInitialState(pObject))
    {
      return pObject;
    }
  }

  return nullptr;
}

bool WStateMachineNodeManager::IsAnyState(const WDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<WStateMachineNodeAny>();
  }
  return false;
}

bool WStateMachineNodeManager::InternalIsNode(const WDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<WStateMachineNodeBase>();
  }
  return false;
}

WStatus WStateMachineNodeManager::InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return WStatus(W_SUCCESS);
}

void WStateMachineNodeManager::InternalCreatePins(const WDocumentObject* pObject, NodeInternal& node)
{
  if (IsNode(pObject) == false)
    return;

  if (IsAnyState(pObject) == false)
  {
    auto pPin = W_DEFAULT_NEW(WStateMachinePin, WVisualGraphPin::Type::Input, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  {
    auto pPin = W_DEFAULT_NEW(WStateMachinePin, WVisualGraphPin::Type::Output, pObject);
    node.m_Outputs.PushBack(pPin);
  }
}

void WStateMachineNodeManager::GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WStateMachineNode>());
  out_types.PushBack(WGetStaticRTTI<WStateMachineNodeAny>());
}

const WRTTI* WStateMachineNodeManager::GetConnectionType() const
{
  return WGetStaticRTTI<WStateMachineConnection>();
}

void WStateMachineNodeManager::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (IsNode(e.m_pObject) == false || IsAnyState(e.m_pObject))
    return;

  auto pCommandHistory = GetDocument()->GetCommandHistory();
  if (pCommandHistory == nullptr || pCommandHistory->IsInTransaction() == false)
    return;

  if (e.m_EventType == WDocumentObjectStructureEvent::Type::AfterObjectAdded &&
      e.m_pObject->GetTypeAccessor().GetValue(s_szIsInitialState) == false &&
      GetInitialState() == nullptr)
  {
    WSetObjectPropertyCommand propCmd;
    propCmd.m_Object = e.m_pObject->GetGuid();
    propCmd.m_sProperty = s_szIsInitialState;
    propCmd.m_NewValue = WVariant(true);

    W_VERIFY(pCommandHistory->AddCommand(propCmd).Succeeded(), "");
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachine_SetInitialStateCommand, 1, WRTTIDefaultAllocator<WStateMachine_SetInitialStateCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("NewInitialStateObject", m_NewInitialStateObject),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachine_SetInitialStateCommand::WStateMachine_SetInitialStateCommand() = default;

WStatus WStateMachine_SetInitialStateCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();
  auto pManager = static_cast<WStateMachineNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    if (m_NewInitialStateObject.IsValid())
      m_pNewInitialStateObject = pManager->GetObject(m_NewInitialStateObject);

    if (auto pOldInitialStateObject = pManager->GetInitialState())
      m_pOldInitialStateObject = pManager->GetObject(pOldInitialStateObject->GetGuid());
  }

  if (m_pNewInitialStateObject)
    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pNewInitialStateObject, s_szIsInitialState, WVariant(true)));

  if (m_pOldInitialStateObject)
    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pOldInitialStateObject, s_szIsInitialState, WVariant(false)));

  return WStatus(W_SUCCESS);
}

WStatus WStateMachine_SetInitialStateCommand::UndoInternal(bool bFireEvents)
{
  WDocument* pDocument = GetDocument();
  auto pManager = static_cast<WStateMachineNodeManager*>(pDocument->GetObjectManager());

  if (m_pNewInitialStateObject)
    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pNewInitialStateObject, s_szIsInitialState, WVariant(false)));

  if (m_pOldInitialStateObject)
    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pOldInitialStateObject, s_szIsInitialState, WVariant(true)));

  return WStatus(W_SUCCESS);
}
