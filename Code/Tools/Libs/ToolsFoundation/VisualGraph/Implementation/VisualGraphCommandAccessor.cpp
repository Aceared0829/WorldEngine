#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualGraphCommandAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WVisualGraphCommandAccessor::WVisualGraphCommandAccessor(WCommandHistory* pHistory)
  : WObjectCommandAccessor(pHistory)
{
}

WVisualGraphCommandAccessor::~WVisualGraphCommandAccessor() = default;

WStatus WVisualGraphCommandAccessor::SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  if (m_pHistory->InTemporaryTransaction() == false)
  {
    auto pNodeObject = pObject;
    auto pDynamicPinProperty = pProp;

    if (IsNode(pObject) == false)
    {
      auto pParent = pObject->GetParent();
      if (pParent != nullptr && IsNode(pParent))
      {
        pNodeObject = pParent;
        pDynamicPinProperty = pParent->GetType()->FindPropertyByName(pObject->GetParentProperty());
      }
    }

    if (IsDynamicPinProperty(pNodeObject, pDynamicPinProperty))
    {
      WTempHybridArray<ConnectionInfo, 16> oldConnections;
      W_SUCCEED_OR_RETURN(DisconnectAllPins(pNodeObject, oldConnections));

      // TODO: remap oldConnections

      W_SUCCEED_OR_RETURN(WObjectCommandAccessor::SetValue(pObject, pProp, newValue, index));

      return TryReconnectAllPins(pNodeObject, oldConnections);
    }
  }

  return WObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
}

WStatus WVisualGraphCommandAccessor::InsertValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    WTempHybridArray<ConnectionInfo, 16> oldConnections;
    W_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    W_SUCCEED_OR_RETURN(WObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return WObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

WStatus WVisualGraphCommandAccessor::RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index /*= WVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    WTempHybridArray<ConnectionInfo, 16> oldConnections;
    W_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    W_SUCCEED_OR_RETURN(WObjectCommandAccessor::RemoveValue(pObject, pProp, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return WObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

WStatus WVisualGraphCommandAccessor::MoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    WTempHybridArray<ConnectionInfo, 16> oldConnections;
    W_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    // TODO: remap oldConnections

    W_SUCCEED_OR_RETURN(WObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return WObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

WStatus WVisualGraphCommandAccessor::AddObject(const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid)
{
  if (IsDynamicPinProperty(pParent, pParentProp))
  {
    WTempHybridArray<ConnectionInfo, 16> oldConnections;
    W_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

    // TODO: remap oldConnections

    W_SUCCEED_OR_RETURN(WObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid));

    return TryReconnectAllPins(pParent, oldConnections);
  }
  else
  {
    return WObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

WStatus WVisualGraphCommandAccessor::RemoveObject(const WDocumentObject* pObject)
{
  if (const WDocumentObject* pParent = pObject->GetParent())
  {
    const WAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(pObject->GetParentProperty());
    if (IsDynamicPinProperty(pParent, pProp))
    {
      WTempHybridArray<ConnectionInfo, 16> oldConnections;
      W_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

      // TODO: remap oldConnections

      W_SUCCEED_OR_RETURN(WObjectCommandAccessor::RemoveObject(pObject));

      return TryReconnectAllPins(pParent, oldConnections);
    }
  }

  return WObjectCommandAccessor::RemoveObject(pObject);
}


bool WVisualGraphCommandAccessor::IsNode(const WDocumentObject* pObject) const
{
  auto pManager = static_cast<const WVisualGraphObjectManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsNode(pObject);
}

bool WVisualGraphCommandAccessor::IsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const
{
  auto pManager = static_cast<const WVisualGraphObjectManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsDynamicPinProperty(pObject, pProp);
}

WStatus WVisualGraphCommandAccessor::DisconnectAllPins(const WDocumentObject* pObject, WDynamicArray<ConnectionInfo>& out_oldConnections)
{
  auto pManager = static_cast<const WVisualGraphObjectManager*>(pObject->GetDocumentObjectManager());

  auto Disconnect = [&](WArrayPtr<const WVisualGraphConnection* const> connections) -> WStatus
  {
    for (const WVisualGraphConnection* pConnection : connections)
    {
      auto& connectionInfo = out_oldConnections.ExpandAndGetRef();
      connectionInfo.m_pSource = pConnection->GetSourcePin().GetParent();
      connectionInfo.m_pTarget = pConnection->GetTargetPin().GetParent();
      connectionInfo.m_sSourcePin = pConnection->GetSourcePin().GetName();
      connectionInfo.m_sTargetPin = pConnection->GetTargetPin().GetName();

      W_SUCCEED_OR_RETURN(WNodeCommands::DisconnectAndRemoveCommand(m_pHistory, pConnection->GetParent()->GetGuid()));
    }

    return WStatus(W_SUCCESS);
  };

  auto inputs = pManager->GetInputPins(pObject);
  for (auto& pInputPin : inputs)
  {
    W_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pInputPin)));
  }

  auto outputs = pManager->GetOutputPins(pObject);
  for (auto& pOutputPin : outputs)
  {
    W_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pOutputPin)));
  }

  return WStatus(W_SUCCESS);
}

WStatus WVisualGraphCommandAccessor::TryReconnectAllPins(const WDocumentObject* pObject, const WDynamicArray<ConnectionInfo>& oldConnections)
{
  auto pManager = static_cast<const WVisualGraphObjectManager*>(pObject->GetDocumentObjectManager());
  const WRTTI* pConnectionType = pManager->GetConnectionType();

  for (auto& connectionInfo : oldConnections)
  {
    const WVisualGraphPin* pSourcePin = pManager->GetOutputPinByName(connectionInfo.m_pSource, connectionInfo.m_sSourcePin);
    const WVisualGraphPin* pTargetPin = pManager->GetInputPinByName(connectionInfo.m_pTarget, connectionInfo.m_sTargetPin);

    // This connection can't be restored because a pin doesn't exist anymore, which is ok in this case.
    if (pSourcePin == nullptr || pTargetPin == nullptr)
      continue;

    // This connection is not valid anymore after pins have changed.
    WVisualGraphObjectManager::CanConnectResult res;
    if (pManager->CanConnect(pConnectionType, *pSourcePin, *pTargetPin, res).Failed())
      continue;

    W_SUCCEED_OR_RETURN(WNodeCommands::AddAndConnectCommand(m_pHistory, pConnectionType, *pSourcePin, *pTargetPin));
  }

  return WStatus(W_SUCCESS);
}
