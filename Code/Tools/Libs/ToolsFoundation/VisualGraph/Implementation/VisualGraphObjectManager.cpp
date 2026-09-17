#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommentNode.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualGraphPin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WVisualGraphObjectManager Internal
////////////////////////////////////////////////////////////////////////

struct DocumentNodeManager_NodeMetaData
{
  WVec2 m_Pos = WVec2::MakeZero();
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, DocumentNodeManager_NodeMetaData);

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_NodeMetaData, WNoBase, 1, WRTTIDefaultAllocator<DocumentNodeManager_NodeMetaData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Node::Pos", m_Pos),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

struct DocumentNodeManager_ConnectionMetaData
{
  WUuid m_Source;
  WUuid m_Target;
  WString m_SourcePin;
  WString m_TargetPin;

  bool IsValid() const { return m_Source.IsValid() && m_Target.IsValid(); }
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, DocumentNodeManager_ConnectionMetaData);

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_ConnectionMetaData, WNoBase, 1, WRTTIDefaultAllocator<DocumentNodeManager_ConnectionMetaData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Connection::Source", m_Source),
    W_MEMBER_PROPERTY("Connection::Target", m_Target),
    W_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),
    W_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

class DocumentNodeManager_DefaultConnection : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(DocumentNodeManager_DefaultConnection, WReflectedClass);
};

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(DocumentNodeManager_DefaultConnection, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WDocumentObject_ConnectionBase
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentObject_ConnectionBase, 1, WRTTIDefaultAllocator<WDocumentObject_ConnectionBase>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Source", m_Source)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Target", m_Target)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("SourcePin", m_SourcePin)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("TargetPin", m_TargetPin)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WVisualGraphObjectManager
////////////////////////////////////////////////////////////////////////

WVisualGraphObjectManager::WVisualGraphObjectManager()
{
  m_ObjectEvents.AddEventHandler(WMakeDelegate(&WVisualGraphObjectManager::ObjectHandler, this));
  m_StructureEvents.AddEventHandler(WMakeDelegate(&WVisualGraphObjectManager::StructureEventHandler, this));
  m_PropertyEvents.AddEventHandler(WMakeDelegate(&WVisualGraphObjectManager::PropertyEventsHandler, this));
}

WVisualGraphObjectManager::~WVisualGraphObjectManager()
{
  m_ObjectEvents.RemoveEventHandler(WMakeDelegate(&WVisualGraphObjectManager::ObjectHandler, this));
  m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WVisualGraphObjectManager::StructureEventHandler, this));
  m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WVisualGraphObjectManager::PropertyEventsHandler, this));
}

void WVisualGraphObjectManager::GetNodeCreationTemplates(WDynamicArray<WVisualGraphNodeDesc>& out_templates) const
{
  WTempHybridArray<const WRTTI*, 32> types;
  GetCreateableTypes(types);

  for (auto pType : types)
  {
    auto& nodeTemplate = out_templates.ExpandAndGetRef();
    nodeTemplate.m_pType = pType;
  }
}

const WRTTI* WVisualGraphObjectManager::GetConnectionType() const
{
  return WGetStaticRTTI<WDocumentObject_ConnectionBase>();
}

WVec2 WVisualGraphObjectManager::GetNodePos(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  W_ASSERT_DEV(it.IsValid(), "Can't get pos of objects that aren't nodes!");
  return it.Value().m_vPos;
}

const WVisualGraphConnection& WVisualGraphObjectManager::GetConnection(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  W_ASSERT_DEV(it.IsValid(), "Can't get connection for objects that aren't connections!");
  return *it.Value();
}

const WVisualGraphConnection* WVisualGraphObjectManager::GetConnectionIfExists(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  return it.IsValid() ? it.Value().Borrow() : nullptr;
}

const WVisualGraphPin* WVisualGraphObjectManager::GetInputPinByName(const WDocumentObject* pObject, WStringView sName) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  W_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Inputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

const WVisualGraphPin* WVisualGraphObjectManager::GetOutputPinByName(const WDocumentObject* pObject, WStringView sName) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  W_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Outputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

WArrayPtr<const WUniquePtr<const WVisualGraphPin>> WVisualGraphObjectManager::GetInputPins(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  W_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return WMakeArrayPtr((WUniquePtr<const WVisualGraphPin>*)it.Value().m_Inputs.GetData(), it.Value().m_Inputs.GetCount());
}

WArrayPtr<const WUniquePtr<const WVisualGraphPin>> WVisualGraphObjectManager::GetOutputPins(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  W_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return WMakeArrayPtr((WUniquePtr<const WVisualGraphPin>*)it.Value().m_Outputs.GetData(), it.Value().m_Outputs.GetCount());
}

bool WVisualGraphObjectManager::IsNode(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsNode(pObject);
}

bool WVisualGraphObjectManager::IsConnection(const WDocumentObject* pObject) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsConnection(pObject);
}

bool WVisualGraphObjectManager::IsComment(const WDocumentObject* pObject) const
{
  if (pObject == nullptr)
    return false;

  return pObject->GetType()->IsDerivedFrom<WVisualGraphComment>();
}

bool WVisualGraphObjectManager::IsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const
{
  if (IsNode(pObject) == false)
    return false;

  if (pProp == nullptr)
    return false;

  return InternalIsDynamicPinProperty(pObject, pProp);
}

WArrayPtr<const WVisualGraphConnection* const> WVisualGraphObjectManager::GetConnections(const WVisualGraphPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  if (it.IsValid())
  {
    return it.Value();
  }

  return WArrayPtr<const WVisualGraphConnection* const>();
}

bool WVisualGraphObjectManager::HasConnections(const WVisualGraphPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  return it.IsValid() && it.Value().IsEmpty() == false;
}

bool WVisualGraphObjectManager::IsConnected(const WVisualGraphPin& source, const WVisualGraphPin& target) const
{
  auto it = m_Connections.Find(&source);
  if (it.IsValid())
  {
    for (auto pConnection : it.Value())
    {
      if (&pConnection->GetTargetPin() == &target)
        return true;
    }
  }

  return false;
}

WStatus WVisualGraphObjectManager::CanConnect(const WRTTI* pObjectType, const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNever;

  if (pObjectType == nullptr || pObjectType->IsDerivedFrom(GetConnectionType()) == false)
    return WStatus("Invalid connection object type");

  if (source.m_Type != WVisualGraphPin::Type::Output)
    return WStatus("Source pin is not an output pin.");
  if (target.m_Type != WVisualGraphPin::Type::Input)
    return WStatus("Target pin is not an input pin.");

  if (source.m_pParent == target.m_pParent)
    return WStatus("Nodes cannot be connect with themselves.");

  if (IsConnected(source, target))
    return WStatus("Pins already connected.");

  return InternalCanConnect(source, target, out_result);
}

WStatus WVisualGraphObjectManager::CanDisconnect(const WVisualGraphConnection* pConnection) const
{
  if (pConnection == nullptr)
    return WStatus("Invalid connection");

  return InternalCanDisconnect(pConnection->GetSourcePin(), pConnection->GetTargetPin());
}

WStatus WVisualGraphObjectManager::CanDisconnect(const WDocumentObject* pObject) const
{
  if (!IsConnection(pObject))
    return WStatus("Invalid connection object");

  const WVisualGraphConnection& connection = GetConnection(pObject);
  return InternalCanDisconnect(connection.GetSourcePin(), connection.GetTargetPin());
}

WStatus WVisualGraphObjectManager::CanMoveNode(const WDocumentObject* pObject, const WVec2& vPos) const
{
  W_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (!IsNode(pObject) && !IsComment(pObject))
    return WStatus("The given object is not a node!");

  return InternalCanMoveNode(pObject, vPos);
}

void WVisualGraphObjectManager::Connect(const WDocumentObject* pObject, const WVisualGraphPin& source, const WVisualGraphPin& target)
{
  WVisualGraphObjectManager::CanConnectResult res = CanConnectResult::ConnectNever;
  W_IGNORE_UNUSED(res);
  W_ASSERT_DEBUG(CanConnect(pObject->GetType(), source, target, res).Succeeded(), "Connect: Sanity check failed!");

  W_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("Source") == source.GetParent()->GetGuid(), "Property should have been set at this point already");
  W_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("Target") == target.GetParent()->GetGuid(), "Property should have been set at this point already");
  W_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("SourcePin") == source.GetName(), "Property should have been set at this point already");
  W_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("TargetPin") == target.GetName(), "Property should have been set at this point already");

  auto pConnection = W_DEFAULT_NEW(WVisualGraphConnection, source, target, pObject);
  m_ObjectToConnection.Insert(pObject->GetGuid(), pConnection);

  m_Connections[&source].PushBack(pConnection);
  m_Connections[&target].PushBack(pConnection);

  {
    WVisualGraphObjectManagerEvent e(WVisualGraphObjectManagerEvent::Type::AfterPinsConnected, pObject);
    m_NodeEvents.Broadcast(e);
  }
}

void WVisualGraphObjectManager::Disconnect(const WDocumentObject* pObject)
{
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  W_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");
  W_ASSERT_DEBUG(CanDisconnect(pObject).Succeeded(), "Disconnect: Sanity check failed!");

  {
    WVisualGraphObjectManagerEvent e(WVisualGraphObjectManagerEvent::Type::BeforePinsDisonnected, pObject);
    m_NodeEvents.Broadcast(e);
  }

  auto& pConnection = it.Value();
  const WVisualGraphPin& source = pConnection->GetSourcePin();
  const WVisualGraphPin& target = pConnection->GetTargetPin();
  m_Connections[&source].RemoveAndCopy(pConnection.Borrow());
  m_Connections[&target].RemoveAndCopy(pConnection.Borrow());

  m_ObjectToConnection.Remove(it);
}

void WVisualGraphObjectManager::MoveNode(const WDocumentObject* pObject, const WVec2& vPos)
{
  W_ASSERT_DEBUG(CanMoveNode(pObject, vPos).Succeeded(), "MoveNode: Sanity check failed!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  W_ASSERT_DEBUG(it.IsValid(), "Moveable node does not exist, CanMoveNode impl invalid!");
  it.Value().m_vPos = vPos;

  WVisualGraphObjectManagerEvent e(WVisualGraphObjectManagerEvent::Type::NodeMoved, pObject);
  m_NodeEvents.Broadcast(e);
}

void WVisualGraphObjectManager::AttachMetaDataBeforeSaving(WAbstractObjectGraph& ref_graph) const
{
  auto pNodeMetaDataType = WGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = WGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  WRttiConverterContext context;
  WRttiConverterWriter rttiConverter(&ref_graph, &context, true, true);

  for (auto it = ref_graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    auto* pAbstractObject = it.Value();
    const WUuid& guid = pAbstractObject->GetGuid();

    {
      auto it2 = m_ObjectToNode.Find(guid);
      if (it2.IsValid())
      {
        const NodeInternal& node = it2.Value();

        DocumentNodeManager_NodeMetaData nodeMetaData;
        nodeMetaData.m_Pos = node.m_vPos;
        rttiConverter.AddProperties(pAbstractObject, pNodeMetaDataType, &nodeMetaData);
      }
    }
  }
}

void WVisualGraphObjectManager::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  WCommandHistory* history = GetDocument()->GetCommandHistory();

  auto pNodeMetaDataType = WGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = WGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  WRttiConverterContext context;
  WRttiConverterReader rttiConverter(&graph, &context);

  // Ensure that all nodes have their pins created
  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    WDocumentObject* pObject = GetObject(pAbstractObject->GetGuid());
    if (pObject != nullptr && IsNode(pObject))
    {
      auto& nodeInternal = m_ObjectToNode[pObject->GetGuid()];
      if (nodeInternal.m_Inputs.IsEmpty() && nodeInternal.m_Outputs.IsEmpty())
      {
        InternalCreatePins(pObject, nodeInternal);
      }
    }
  }

  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    WDocumentObject* pObject = GetObject(pAbstractObject->GetGuid());
    if (pObject == nullptr)
      continue;

    if (IsNode(pObject) || IsComment(pObject))
    {
      DocumentNodeManager_NodeMetaData nodeMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pNodeMetaDataType, &nodeMetaData);

      if (CanMoveNode(pObject, nodeMetaData.m_Pos).Succeeded())
      {
        if (bUndoable)
        {
          WMoveNodeCommand move;
          move.m_Object = pObject->GetGuid();
          move.m_NewPos = nodeMetaData.m_Pos;
          history->AddCommand(move).LogFailure();
        }
        else
        {
          MoveNode(pObject, nodeMetaData.m_Pos);
        }
      }

      W_ASSERT_DEV(pAbstractObject->FindProperty("Node::Connections") == nullptr, "Old file format detected that is not supported anymore. Re-save the document with a previous version of W. ({})", GetDocument()->GetDocumentPath());
    }
    else if (IsConnection(pObject))
    {
      WVariant sourceVar = pObject->GetTypeAccessor().GetValue("Source");
      WVariant targetVar = pObject->GetTypeAccessor().GetValue("Target");
      WVariant sourcePinVar = pObject->GetTypeAccessor().GetValue("SourcePin");
      WVariant targetPinVar = pObject->GetTypeAccessor().GetValue("TargetPin");
      W_ASSERT_DEV(sourceVar.IsA<WUuid>() && targetVar.IsA<WUuid>() && sourcePinVar.IsA<WString>() && targetPinVar.IsA<WString>(), "Invalid connection object");

      WUuid source = sourceVar.Get<WUuid>();
      WUuid target = targetVar.Get<WUuid>();
      WStringView sourcePin = sourcePinVar.Get<WString>();
      WStringView targetPin = targetPinVar.Get<WString>();

      const WVisualGraphPin* pSourcePin = nullptr;
      const WVisualGraphPin* pTargetPin = nullptr;
      if (ResolveConnection(source, target, sourcePin, targetPin, pSourcePin, pTargetPin).Failed())
      {
        // Try to restore from metadata
        DocumentNodeManager_ConnectionMetaData connectionMetaData;
        rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);
        if (connectionMetaData.IsValid())
        {
          pObject->GetTypeAccessor().SetValue("Source", connectionMetaData.m_Source);
          pObject->GetTypeAccessor().SetValue("Target", connectionMetaData.m_Target);
          pObject->GetTypeAccessor().SetValue("SourcePin", connectionMetaData.m_SourcePin);
          pObject->GetTypeAccessor().SetValue("TargetPin", connectionMetaData.m_TargetPin);

          source = connectionMetaData.m_Source;
          target = connectionMetaData.m_Target;
          sourcePin = connectionMetaData.m_SourcePin;
          targetPin = connectionMetaData.m_TargetPin;
        }
      }

      if (ResolveConnection(source, target, sourcePin, targetPin, pSourcePin, pTargetPin).Succeeded())
      {
        if (bUndoable)
        {
          WConnectNodePinsCommand cmd;
          cmd.m_ConnectionObject = pObject->GetGuid();
          cmd.m_ObjectSource = pSourcePin->GetParent()->GetGuid();
          cmd.m_ObjectTarget = pTargetPin->GetParent()->GetGuid();
          cmd.m_sSourcePin = pSourcePin->GetName();
          cmd.m_sTargetPin = pTargetPin->GetName();
          history->AddCommand(cmd).LogFailure();
        }
        else
        {
          Connect(pObject, *pSourcePin, *pTargetPin);
        }
      }
      else
      {
        WStringBuilder sError;
        sError.SetFormat("Connection from pin '{}' to pin '{}' could not be restored because a pin no longer exists. The connection has been removed.",
          sourcePinVar.Get<WString>(), targetPinVar.Get<WString>());
        GetDocument()->AddLoadingError(sError);
        RemoveObject(pObject);
        DestroyObject(pObject);
      }
    }
    else
    {
      DocumentNodeManager_ConnectionMetaData connectionMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);

      if (connectionMetaData.IsValid() == false)
        continue;

      const WVisualGraphPin* pSourcePin = nullptr;
      const WVisualGraphPin* pTargetPin = nullptr;
      if (ResolveConnection(connectionMetaData.m_Source, connectionMetaData.m_Target, connectionMetaData.m_SourcePin, connectionMetaData.m_TargetPin, pSourcePin, pTargetPin).Succeeded())
      {
        WDocumentObject* pNewConnectionObject = CreateObject(GetConnectionType());
        pNewConnectionObject->GetTypeAccessor().SetValue("Source", connectionMetaData.m_Source);
        pNewConnectionObject->GetTypeAccessor().SetValue("Target", connectionMetaData.m_Target);
        pNewConnectionObject->GetTypeAccessor().SetValue("SourcePin", connectionMetaData.m_SourcePin);
        pNewConnectionObject->GetTypeAccessor().SetValue("TargetPin", connectionMetaData.m_TargetPin);
        AddObject(pNewConnectionObject, nullptr, "", -1);

        W_ASSERT_DEV(bUndoable == false, "This code path should only be taken by document loading code");
        Connect(pNewConnectionObject, *pSourcePin, *pTargetPin);
      }

      RemoveObject(pObject);
      DestroyObject(pObject);
    }
  }
}

void WVisualGraphObjectManager::GetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const
{
  if (IsNode(pObject) || IsComment(pObject))
  {
    // The node position is not hashed here since the hash is only used for asset transform
    // and for that the node position is irrelevant.
  }
  else if (IsConnection(pObject))
  {
    const WVisualGraphConnection& connection = GetConnection(pObject);
    const WVisualGraphPin& sourcePin = connection.GetSourcePin();
    const WVisualGraphPin& targetPin = connection.GetTargetPin();

    inout_uiHash = WHashingUtils::xxHash64(&sourcePin.GetParent()->GetGuid(), sizeof(WUuid), inout_uiHash);
    inout_uiHash = WHashingUtils::xxHash64(&targetPin.GetParent()->GetGuid(), sizeof(WUuid), inout_uiHash);
    inout_uiHash = WHashingUtils::xxHash64String(sourcePin.GetName(), inout_uiHash);
    inout_uiHash = WHashingUtils::xxHash64String(targetPin.GetName(), inout_uiHash);
  }
}

bool WVisualGraphObjectManager::CopySelectedObjects(WAbstractObjectGraph& out_objectGraph) const
{
  const auto& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return false;

  WDocumentObjectConverterWriter writer(&out_objectGraph, this);

  WHashSet<const WDocumentObject*> copiedNodes;
  for (const WDocumentObject* pObject : selection)
  {
    // Only add nodes here, connections are then collected below to ensure
    // that we always include only valid connections within the copied subgraph no matter if they are selected or not.
    if (IsNode(pObject) || IsComment(pObject))
    {
      // objects are required to be named root but this is not enforced or obvious by the interface.
      writer.AddObjectToGraph(pObject, "root");
      copiedNodes.Insert(pObject);
    }
  }

  WHashSet<const WDocumentObject*> copiedConnections;
  for (const WDocumentObject* pNodeObject : selection)
  {
    if (!IsNode(pNodeObject) && !IsComment(pNodeObject))
      continue;

    auto outputs = GetOutputPins(pNodeObject);
    for (auto& pSourcePin : outputs)
    {
      auto connections = GetConnections(*pSourcePin);
      for (const WVisualGraphConnection* pConnection : connections)
      {
        const WDocumentObject* pConnectionObject = pConnection->GetParent();

        W_ASSERT_DEV(pSourcePin == &pConnection->GetSourcePin(), "");
        if (copiedConnections.Contains(pConnectionObject) == false && copiedNodes.Contains(pConnection->GetTargetPin().GetParent()))
        {
          writer.AddObjectToGraph(pConnectionObject, "root");
          copiedConnections.Insert(pConnectionObject);
        }
      }
    }
  }

  AttachMetaDataBeforeSaving(out_objectGraph);

  return true;
}

bool WVisualGraphObjectManager::PasteObjects(const WArrayPtr<WDocument::PasteInfo>& info, const WAbstractObjectGraph& objectGraph, const WVec2& vPickedPosition, bool bAllowPickedPosition)
{
  bool bAddedAll = true;
  WDeque<const WDocumentObject*> AddedObjects;

  for (const auto& pi : info)
  {
    // only add nodes that are allowed to be added
    if (CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", pi.m_Index).Succeeded())
    {
      AddedObjects.PushBack(pi.m_pObject);
      AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      bAddedAll = false;
    }
  }

  RestoreMetaDataAfterLoading(objectGraph, true);

  if (!AddedObjects.IsEmpty() && bAllowPickedPosition)
  {
    WCommandHistory* history = GetDocument()->GetCommandHistory();

    WVec2 vAvgPos(0);
    WUInt32 nodeCount = 0;
    for (const WDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject) || IsComment(pObject))
      {
        vAvgPos += GetNodePos(pObject);
        ++nodeCount;
      }
    }

    vAvgPos /= (float)nodeCount;
    const WVec2 vMoveNode = -vAvgPos + vPickedPosition;

    for (const WDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject) || IsComment(pObject))
      {
        WMoveNodeCommand move;
        move.m_Object = pObject->GetGuid();
        move.m_NewPos = GetNodePos(pObject) + vMoveNode;
        history->AddCommand(move).LogFailure();
      }
    }

    if (!bAddedAll)
    {
      WLog::Info("[EditorStatus]Not all nodes were allowed to be added to the document");
    }
  }

  GetDocument()->GetSelectionManager()->SetSelection(AddedObjects);
  return true;
}

bool WVisualGraphObjectManager::CanReachNode(const WDocumentObject* pSource, const WDocumentObject* pTarget, WSet<const WDocumentObject*>& Visited) const
{
  if (pSource == pTarget)
    return true;

  if (Visited.Contains(pSource))
    return false;

  Visited.Insert(pSource);

  auto outputs = GetOutputPins(pSource);
  for (auto& pSourcePin : outputs)
  {
    auto connections = GetConnections(*pSourcePin);
    for (const WVisualGraphConnection* pConnection : connections)
    {
      if (CanReachNode(pConnection->GetTargetPin().GetParent(), pTarget, Visited))
        return true;
    }
  }

  return false;
}


bool WVisualGraphObjectManager::WouldConnectionCreateCircle(const WVisualGraphPin& source, const WVisualGraphPin& target) const
{
  const WDocumentObject* pSourceNode = source.GetParent();
  const WDocumentObject* pTargetNode = target.GetParent();
  WSet<const WDocumentObject*> Visited;

  return CanReachNode(pTargetNode, pSourceNode, Visited);
}

WResult WVisualGraphObjectManager::ResolveConnection(const WUuid& sourceObject, const WUuid& targetObject, WStringView sourcePin, WStringView targetPin, const WVisualGraphPin*& out_pSourcePin, const WVisualGraphPin*& out_pTargetPin) const
{
  const WDocumentObject* pSource = GetObject(sourceObject);
  const WDocumentObject* pTarget = GetObject(targetObject);
  if (pSource == nullptr || pTarget == nullptr)
  {
    return W_FAILURE;
  }

  const WVisualGraphPin* pSourcePin = GetOutputPinByName(pSource, sourcePin);
  if (pSourcePin == nullptr)
  {
    WLog::Error("Unknown output pin '{}' on '{}'. The connection has been removed.", sourcePin, pSource->GetType()->GetTypeName());
    return W_FAILURE;
  }

  const WVisualGraphPin* pTargetPin = GetInputPinByName(pTarget, targetPin);
  if (pTargetPin == nullptr)
  {
    WLog::Error("Unknown input pin '{}' on '{}'. The connection has been removed.", targetPin, pTarget->GetType()->GetTypeName());
    return W_FAILURE;
  }

  out_pSourcePin = pSourcePin;
  out_pTargetPin = pTargetPin;
  return W_SUCCESS;
}

void WVisualGraphObjectManager::GetDynamicPinNames(const WDocumentObject* pObject, WStringView sPropertyName, WStringView sPinName, WDynamicArray<WString>& out_Names) const
{
  out_Names.Clear();

  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sPropertyName);
  if (pProp == nullptr)
  {
    WLog::Warning("Property '{0}' not found in type '{1}'", sPropertyName, pObject->GetType()->GetTypeName());
    return;
  }

  WStringBuilder sTemp;
  WVariant value = pObject->GetTypeAccessor().GetValue(sPropertyName);

  if (pProp->GetCategory() == WPropertyCategory::Member)
  {
    if (value.CanConvertTo<WUInt32>())
    {
      WUInt32 uiCount = value.ConvertTo<WUInt32>();
      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
  else if (pProp->GetCategory() == WPropertyCategory::Array)
  {
    auto pArrayProp = static_cast<const WAbstractArrayProperty*>(pProp);

    auto& a = value.Get<WVariantArray>();
    const WUInt32 uiCount = a.GetCount();

    auto variantType = pArrayProp->GetSpecificType()->GetVariantType();
    if (variantType >= WVariantType::Int8 && variantType <= WVariantType::UInt64)
    {
      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}", a[i]);
        out_Names.PushBack(sTemp);
      }
    }
    else if (variantType == WVariantType::String || variantType == WVariantType::HashedString)
    {
      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        out_Names.PushBack(a[i].ConvertTo<WString>());
      }
    }
    else if (pArrayProp->GetSpecificType()->GetTypeFlags().IsSet(WTypeFlags::Class))
    {
      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        auto pInnerObject = GetObject(a[i].Get<WUuid>());
        if (pInnerObject == nullptr)
          continue;

        WVariant nameVar = pInnerObject->GetTypeAccessor().GetValue("Name");
        if (nameVar.IsString() || nameVar.IsHashedString())
        {
          out_Names.PushBack(nameVar.ConvertTo<WString>());
        }
        else
        {
          sTemp.SetFormat("{}[{}]", sPinName, i);
          out_Names.PushBack(sTemp);
        }
      }
    }
    else
    {
      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
}

bool WVisualGraphObjectManager::TryRecreatePins(const WDocumentObject* pObject)
{
  if (!IsNode(pObject))
    return false;

  auto& nodeInternal = m_ObjectToNode[pObject->GetGuid()];

  for (auto& pPin : nodeInternal.m_Inputs)
  {
    if (HasConnections(*pPin))
    {
      WLog::Error("Can't re-create pins if they are still connected");
      return false;
    }
  }

  for (auto& pPin : nodeInternal.m_Outputs)
  {
    if (HasConnections(*pPin))
    {
      WLog::Error("Can't re-create pins if they are still connected");
      return false;
    }
  }

  {
    WVisualGraphObjectManagerEvent e(WVisualGraphObjectManagerEvent::Type::BeforePinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  nodeInternal.m_Inputs.Clear();
  nodeInternal.m_Outputs.Clear();
  InternalCreatePins(pObject, nodeInternal);

  {
    WVisualGraphObjectManagerEvent e(WVisualGraphObjectManagerEvent::Type::AfterPinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  return true;
}

bool WVisualGraphObjectManager::InternalIsNode(const WDocumentObject* pObject) const
{
  return true;
}

bool WVisualGraphObjectManager::InternalIsConnection(const WDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom(GetConnectionType());
}

WStatus WVisualGraphObjectManager::InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return WStatus(W_SUCCESS);
}

void WVisualGraphObjectManager::ObjectHandler(const WDocumentObjectEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectEvent::Type::AfterObjectCreated:
    {
      if (IsNode(e.m_pObject) || IsComment(e.m_pObject))
      {
        W_ASSERT_DEBUG(!m_ObjectToNode.Contains(e.m_pObject->GetGuid()), "Sanity check failed!");
        m_ObjectToNode[e.m_pObject->GetGuid()] = NodeInternal();
      }
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are created in Connect method.
      }
    }
    break;
    case WDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      if (IsNode(e.m_pObject) || IsComment(e.m_pObject))
      {
        auto it = m_ObjectToNode.Find(e.m_pObject->GetGuid());
        W_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");

        m_ObjectToNode.Remove(it);
      }
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are removed in Disconnect method.
      }
    }
    break;
    default:
      W_ASSERT_NOT_IMPLEMENTED
  }
}

void WVisualGraphObjectManager::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        WVisualGraphObjectManagerEvent e2(WVisualGraphObjectManagerEvent::Type::BeforeNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        auto& nodeInternal = m_ObjectToNode[e.m_pObject->GetGuid()];
        if (nodeInternal.m_Inputs.IsEmpty() && nodeInternal.m_Outputs.IsEmpty())
        {
          InternalCreatePins(e.m_pObject, nodeInternal);
          // TODO: Sanity check pins (duplicate names etc).
        }
      }

      if (IsNode(e.m_pObject) || IsComment(e.m_pObject))
      {
        WVisualGraphObjectManagerEvent e2(WVisualGraphObjectManagerEvent::Type::AfterNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
      else
      {
        HandlePotentialDynamicPinPropertyChanged(e.m_pNewParent, e.m_sParentProperty);
      }
    }
    break;
    case WDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      if (IsNode(e.m_pObject) || IsComment(e.m_pObject))
      {
        WVisualGraphObjectManagerEvent e2(WVisualGraphObjectManagerEvent::Type::BeforeNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (IsNode(e.m_pObject) || IsComment(e.m_pObject))
      {
        WVisualGraphObjectManagerEvent e2(WVisualGraphObjectManagerEvent::Type::AfterNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
      else
      {
        HandlePotentialDynamicPinPropertyChanged(e.m_pPreviousParent, e.m_sParentProperty);
      }
    }
    break;

    default:
      break;
  }
}

void WVisualGraphObjectManager::PropertyEventsHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == nullptr)
    return;

  HandlePotentialDynamicPinPropertyChanged(e.m_pObject, e.m_sProperty);

  if (const WDocumentObject* pParent = e.m_pObject->GetParent())
  {
    HandlePotentialDynamicPinPropertyChanged(pParent, e.m_pObject->GetParentProperty());
  }
}

void WVisualGraphObjectManager::HandlePotentialDynamicPinPropertyChanged(const WDocumentObject* pObject, WStringView sPropertyName)
{
  if (pObject == nullptr)
    return;

  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sPropertyName);
  if (pProp == nullptr)
    return;

  if (IsDynamicPinProperty(pObject, pProp))
  {
    TryRecreatePins(pObject);
  }
}
