#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRemoveNodeCommand, 1, WRTTIDefaultAllocator<WRemoveNodeCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMoveNodeCommand, 1, WRTTIDefaultAllocator<WMoveNodeCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
    W_MEMBER_PROPERTY("NewPos", m_NewPos),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConnectNodePinsCommand, 1, WRTTIDefaultAllocator<WConnectNodePinsCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
    W_MEMBER_PROPERTY("SourceGuid", m_ObjectSource),
    W_MEMBER_PROPERTY("TargetGuid", m_ObjectTarget),
    W_MEMBER_PROPERTY("SourcePin", m_sSourcePin),
    W_MEMBER_PROPERTY("TargetPin", m_sTargetPin),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDisconnectNodePinsCommand, 1, WRTTIDefaultAllocator<WDisconnectNodePinsCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WRemoveNodeCommand
////////////////////////////////////////////////////////////////////////

WRemoveNodeCommand::WRemoveNodeCommand() = default;

WStatus WRemoveNodeCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(pDocument->GetObjectManager());

  auto RemoveConnections = [&](const WVisualGraphPin& pin)
  {
    while (true)
    {
      auto connections = pManager->GetConnections(pin);

      if (connections.IsEmpty())
        break;

      WDisconnectNodePinsCommand cmd;
      cmd.m_ConnectionObject = connections[0]->GetParent()->GetGuid();
      WStatus res = AddSubCommand(cmd);
      if (res.Succeeded())
      {
        WRemoveObjectCommand remove;
        remove.m_Object = cmd.m_ConnectionObject;
        res = AddSubCommand(remove);
      }

      W_SUCCEED_OR_RETURN(res);
    }
    return WStatus(W_SUCCESS);
  };

  if (!bRedo)
  {
    m_pObject = pManager->GetObject(m_Object);
    if (m_pObject == nullptr)
      return WStatus("Remove Node: The given object does not exist!");

    auto inputs = pManager->GetInputPins(m_pObject);
    for (auto& pPinTarget : inputs)
    {
      W_SUCCEED_OR_RETURN(RemoveConnections(*pPinTarget));
    }

    auto outputs = pManager->GetOutputPins(m_pObject);
    for (auto& pPinSource : outputs)
    {
      W_SUCCEED_OR_RETURN(RemoveConnections(*pPinSource));
    }

    WRemoveObjectCommand cmd;
    cmd.m_Object = m_Object;
    auto res = AddSubCommand(cmd);
    if (res.Failed())
    {
      return res;
    }
  }
  return WStatus(W_SUCCESS);
}

WStatus WRemoveNodeCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  return WStatus(W_SUCCESS);
}

void WRemoveNodeCommand::CleanupInternal(CommandState state) {}


////////////////////////////////////////////////////////////////////////
// WMoveObjectCommand
////////////////////////////////////////////////////////////////////////

WMoveNodeCommand::WMoveNodeCommand() = default;

WStatus WMoveNodeCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return WStatus("Move Node: The given object does not exist!");

    m_vOldPos = pManager->GetNodePos(m_pObject);
    W_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_NewPos));
  }

  pManager->MoveNode(m_pObject, m_NewPos);
  return WStatus(W_SUCCESS);
}

WStatus WMoveNodeCommand::UndoInternal(bool bFireEvents)
{
  WDocument* pDocument = GetDocument();
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(pDocument->GetObjectManager());
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  W_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_vOldPos));

  pManager->MoveNode(m_pObject, m_vOldPos);

  return WStatus(W_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// WConnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

WConnectNodePinsCommand::WConnectNodePinsCommand() = default;

WStatus WConnectNodePinsCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return WStatus("Connect Node Pins: The given connection object is not valid connection!");

    m_pObjectSource = pManager->GetObject(m_ObjectSource);
    if (m_pObjectSource == nullptr)
      return WStatus("Connect Node Pins: The given node does not exist!");
    m_pObjectTarget = pManager->GetObject(m_ObjectTarget);
    if (m_pObjectTarget == nullptr)
      return WStatus("Connect Node Pins: The given node does not exist!");
  }

  const WVisualGraphPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return WStatus("Connect Node Pins: The given pin does not exist!");

  const WVisualGraphPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return WStatus("Connect Node Pins: The given pin does not exist!");

  WVisualGraphObjectManager::CanConnectResult res;
  W_SUCCEED_OR_RETURN(pManager->CanConnect(m_pConnectionObject->GetType(), *pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return WStatus(W_SUCCESS);
}

WStatus WConnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  WDocument* pDocument = GetDocument();
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(pDocument->GetObjectManager());

  W_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);
  return WStatus(W_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// WDisconnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

WDisconnectNodePinsCommand::WDisconnectNodePinsCommand() = default;

WStatus WDisconnectNodePinsCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return WStatus("Disconnect Node Pins: The given connection object is not valid connection!");

    W_SUCCEED_OR_RETURN(pManager->CanRemove(m_pConnectionObject));

    const WVisualGraphConnection& connection = pManager->GetConnection(m_pConnectionObject);
    const WVisualGraphPin& pinSource = connection.GetSourcePin();
    const WVisualGraphPin& pinTarget = connection.GetTargetPin();

    m_pObjectSource = pinSource.GetParent();
    m_pObjectTarget = pinTarget.GetParent();
    m_sSourcePin = pinSource.GetName();
    m_sTargetPin = pinTarget.GetName();
  }

  W_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);

  return WStatus(W_SUCCESS);
}

WStatus WDisconnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  WDocument* pDocument = GetDocument();
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(pDocument->GetObjectManager());

  const WVisualGraphPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return WStatus("Connect Node: The given pin does not exist!");

  const WVisualGraphPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return WStatus("Connect Node: The given pin does not exist!");

  WVisualGraphObjectManager::CanConnectResult res;
  W_SUCCEED_OR_RETURN(pManager->CanConnect(m_pConnectionObject->GetType(), *pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return WStatus(W_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// WNodeCommands
////////////////////////////////////////////////////////////////////////

// static
WStatus WNodeCommands::AddAndConnectCommand(WCommandHistory* pHistory, const WRTTI* pConnectionType, const WVisualGraphPin& sourcePin, const WVisualGraphPin& targetPin)
{
  WAddObjectCommand addCmd;
  addCmd.m_pType = pConnectionType;
  addCmd.m_NewObjectGuid = WUuid::MakeUuid();
  addCmd.m_Index = -1;

  W_SUCCEED_OR_RETURN(pHistory->AddCommand(addCmd));

  constexpr WStringView propertyNames[] = {
    "Source"_wsv,
    "Target"_wsv,
    "SourcePin"_wsv,
    "TargetPin"_wsv,
  };
  WVariant propertyValues[] = {
    sourcePin.GetParent()->GetGuid(),
    targetPin.GetParent()->GetGuid(),
    sourcePin.GetName(),
    targetPin.GetName(),
  };
  static_assert(W_ARRAY_SIZE(propertyNames) == W_ARRAY_SIZE(propertyValues));

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(propertyNames); ++i)
  {
    WSetObjectPropertyCommand propCmd;
    propCmd.m_Object = addCmd.m_NewObjectGuid;
    propCmd.m_sProperty = propertyNames[i];
    propCmd.m_NewValue = propertyValues[i];

    W_SUCCEED_OR_RETURN(pHistory->AddCommand(propCmd));
  }

  WConnectNodePinsCommand connectCmd;
  connectCmd.m_ConnectionObject = addCmd.m_NewObjectGuid;
  connectCmd.m_ObjectSource = sourcePin.GetParent()->GetGuid();
  connectCmd.m_ObjectTarget = targetPin.GetParent()->GetGuid();
  connectCmd.m_sSourcePin = sourcePin.GetName();
  connectCmd.m_sTargetPin = targetPin.GetName();

  return pHistory->AddCommand(connectCmd);
}

// static
WStatus WNodeCommands::DisconnectAndRemoveCommand(WCommandHistory* pHistory, const WUuid& connectionObject)
{
  WDisconnectNodePinsCommand cmd;
  cmd.m_ConnectionObject = connectionObject;

  WStatus res = pHistory->AddCommand(cmd);
  if (res.Succeeded())
  {
    WRemoveObjectCommand remove;
    remove.m_Object = cmd.m_ConnectionObject;

    res = pHistory->AddCommand(remove);
  }

  return res;
}
