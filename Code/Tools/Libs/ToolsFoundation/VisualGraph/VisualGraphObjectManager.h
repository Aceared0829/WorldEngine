#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WVisualGraphPin;
class WVisualGraphConnection;

/// Event structure for visual graph changes
struct W_TOOLSFOUNDATION_DLL WVisualGraphObjectManagerEvent
{
  enum class Type
  {
    NodeMoved,
    AfterPinsConnected,
    BeforePinsDisonnected,
    BeforePinsChanged,
    AfterPinsChanged,
    BeforeNodeAdded,
    AfterNodeAdded,
    BeforeNodeRemoved,
    AfterNodeRemoved,
  };

  WVisualGraphObjectManagerEvent(Type eventType, const WDocumentObject* pObject = nullptr)
    : m_EventType(eventType)
    , m_pObject(pObject)
  {
  }

  Type m_EventType;
  const WDocumentObject* m_pObject;
};

/// Represents an active connection between two pins in a visual graph.
///
/// This class is created and managed by WVisualGraphObjectManager when pins are connected.
/// It holds references to both the source and target pins.
class WVisualGraphConnection final
{
public:
  const WVisualGraphPin& GetSourcePin() const { return m_SourcePin; }
  const WVisualGraphPin& GetTargetPin() const { return m_TargetPin; }
  const WDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class WVisualGraphObjectManager;

  WVisualGraphConnection(const WVisualGraphPin& sourcePin, const WVisualGraphPin& targetPin, const WDocumentObject* pParent)
    : m_SourcePin(sourcePin)
    , m_TargetPin(targetPin)
    , m_pParent(pParent)
  {
  }

  const WVisualGraphPin& m_SourcePin;
  const WVisualGraphPin& m_TargetPin;
  const WDocumentObject* m_pParent = nullptr;
};

/// Represents a connection point (input or output) on a visual graph node.
///
/// Pins are created by WVisualGraphObjectManager for each node based on the node type.
/// Derived classes can extend pins with additional metadata specific to their graph type.
class W_TOOLSFOUNDATION_DLL WVisualGraphPin : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WVisualGraphPin, WReflectedClass);

public:
  enum class Type
  {
    Input,
    Output
  };

  enum class Shape
  {
    Circle,
    Rect,
    RoundRect,
    Arrow,
    Default = Circle
  };

  WVisualGraphPin(Type type, WStringView sName, const WColorGammaUB& color, const WDocumentObject* pObject)
    : m_Type(type)
    , m_Color(color)
    , m_sName(sName)
    , m_pParent(pObject)
  {
  }

  Shape m_Shape = Shape::Default;

  Type GetType() const { return m_Type; }
  const char* GetName() const { return m_sName; }
  const WColorGammaUB& GetColor() const { return m_Color; }
  const WDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class WVisualGraphObjectManager;

  Type m_Type;
  WColorGammaUB m_Color;
  WString m_sName;
  const WDocumentObject* m_pParent = nullptr;
};

//////////////////////////////////////////////////////////////////////////

/// A property value that can be pre-filled when creating a node from a template
struct WVisualGraphNodeProperty
{
  WHashedString m_sPropertyName;
  WVariant m_Value;
};

/// Describes a template that will be used to create new nodes. In most cases this only contains the type
/// but it can also contain properties that are pre-filled when the node is created.
///
/// For example in visual script this allows us to have one generic node type for setting reflected properties
/// but we can expose all relevant reflected properties in the node creation menu so the user does not need to fill out the property name manually.
struct WVisualGraphNodeDesc
{
  const WRTTI* m_pType = nullptr;
  WStringView m_sTypeName;
  WHashedString m_sCategory;
  WArrayPtr<const WVisualGraphNodeProperty> m_PropertyValues;
};

//////////////////////////////////////////////////////////////////////////

/// Serializable base class for storing connection data in documents.
///
/// Derive from this class and overwrite WVisualGraphObjectManager::GetConnectionType if you need custom properties for connections.
/// This class stores the persistent connection information, while WVisualGraphConnection holds runtime connection references.
class W_TOOLSFOUNDATION_DLL WDocumentObject_ConnectionBase : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentObject_ConnectionBase, WReflectedClass);

public:
  WUuid m_Source;
  WUuid m_Target;
  WString m_SourcePin;
  WString m_TargetPin;
};

//////////////////////////////////////////////////////////////////////////

/// Document object manager for node-based visual graphs.
///
/// This base class manages the document-side representation of visual graphs, including nodes, pins, and connections.
/// It handles node creation, pin management, connection validation, and provides events for graph changes.
/// Derive from this class to implement specific graph types such as visual shaders, state machines, or visual scripts.
/// The visual representation is handled separately by WQtVisualGraphScene.
class W_TOOLSFOUNDATION_DLL WVisualGraphObjectManager : public WDocumentObjectManager
{
public:
  WEvent<const WVisualGraphObjectManagerEvent&> m_NodeEvents;

  WVisualGraphObjectManager();
  virtual ~WVisualGraphObjectManager();

  /// For node documents this function is called instead of GetCreateableTypes to get a list for the node creation menu.
  ///
  /// \see WVisualGraphNodeDesc
  virtual void GetNodeCreationTemplates(WDynamicArray<WVisualGraphNodeDesc>& out_templates) const;

  virtual const WRTTI* GetConnectionType() const;

  WVec2 GetNodePos(const WDocumentObject* pObject) const;
  const WVisualGraphConnection& GetConnection(const WDocumentObject* pObject) const;
  const WVisualGraphConnection* GetConnectionIfExists(const WDocumentObject* pObject) const;

  const WVisualGraphPin* GetInputPinByName(const WDocumentObject* pObject, WStringView sName) const;
  const WVisualGraphPin* GetOutputPinByName(const WDocumentObject* pObject, WStringView sName) const;
  WArrayPtr<const WUniquePtr<const WVisualGraphPin>> GetInputPins(const WDocumentObject* pObject) const;
  WArrayPtr<const WUniquePtr<const WVisualGraphPin>> GetOutputPins(const WDocumentObject* pObject) const;

  /// Specifies how many connections are allowed for a pair of pins
  enum class CanConnectResult
  {
    ConnectNever, ///< Pins can't be connected
    Connect1to1,  ///< Output pin can have 1 outgoing connection, Input pin can have 1 incoming connection
    Connect1toN,  ///< Output pin can have 1 outgoing connection, Input pin can have N incoming connections
    ConnectNto1,  ///< Output pin can have N outgoing connections, Input pin can have 1 incoming connection
    ConnectNtoN,  ///< Output pin can have N outgoing connections, Input pin can have N incoming connections
  };

  bool IsNode(const WDocumentObject* pObject) const;
  bool IsComment(const WDocumentObject* pObject) const;
  bool IsConnection(const WDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const;

  WArrayPtr<const WVisualGraphConnection* const> GetConnections(const WVisualGraphPin& pin) const;
  bool HasConnections(const WVisualGraphPin& pin) const;
  bool IsConnected(const WVisualGraphPin& source, const WVisualGraphPin& target) const;

  WStatus CanConnect(const WRTTI* pObjectType, const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& ref_result) const;
  WStatus CanDisconnect(const WVisualGraphConnection* pConnection) const;
  WStatus CanDisconnect(const WDocumentObject* pObject) const;
  WStatus CanMoveNode(const WDocumentObject* pObject, const WVec2& vPos) const;

  void Connect(const WDocumentObject* pObject, const WVisualGraphPin& source, const WVisualGraphPin& target);
  void Disconnect(const WDocumentObject* pObject);
  void MoveNode(const WDocumentObject* pObject, const WVec2& vPos);

  void AttachMetaDataBeforeSaving(WAbstractObjectGraph& ref_graph) const;
  void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable);

  void GetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const;
  bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph) const;
  bool PasteObjects(const WArrayPtr<WDocument::PasteInfo>& info, const WAbstractObjectGraph& objectGraph, const WVec2& vPickedPosition, bool bAllowPickedPosition);

protected:
  /// Tests whether pTarget can be reached from pSource by following the pin connections
  bool CanReachNode(const WDocumentObject* pSource, const WDocumentObject* pTarget, WSet<const WDocumentObject*>& Visited) const;

  /// Returns true if adding a connection between the two pins would create a circular graph
  bool WouldConnectionCreateCircle(const WVisualGraphPin& source, const WVisualGraphPin& target) const;

  WResult ResolveConnection(const WUuid& sourceObject, const WUuid& targetObject, WStringView sourcePin, WStringView targetPin, const WVisualGraphPin*& out_pSourcePin, const WVisualGraphPin*& out_pTargetPin) const;

  virtual void GetDynamicPinNames(const WDocumentObject* pObject, WStringView sPropertyName, WStringView sPinName, WDynamicArray<WString>& out_Names) const;
  virtual bool TryRecreatePins(const WDocumentObject* pObject);

  struct NodeInternal
  {
    WVec2 m_vPos = WVec2::MakeZero();
    WHybridArray<WUniquePtr<WVisualGraphPin>, 6> m_Inputs;
    WHybridArray<WUniquePtr<WVisualGraphPin>, 6> m_Outputs;
  };

private:
  virtual bool InternalIsNode(const WDocumentObject* pObject) const;
  virtual bool InternalIsConnection(const WDocumentObject* pObject) const;
  virtual bool InternalIsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const { return false; }
  virtual WStatus InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_Result) const;
  virtual WStatus InternalCanDisconnect(const WVisualGraphPin& source, const WVisualGraphPin& target) const { return WStatus(W_SUCCESS); }
  virtual WStatus InternalCanMoveNode(const WDocumentObject* pObject, const WVec2& vPos) const { return WStatus(W_SUCCESS); }
  virtual void InternalCreatePins(const WDocumentObject* pObject, NodeInternal& node) = 0;

  void ObjectHandler(const WDocumentObjectEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void PropertyEventsHandler(const WDocumentObjectPropertyEvent& e);

  void HandlePotentialDynamicPinPropertyChanged(const WDocumentObject* pObject, WStringView sPropertyName);

private:
  WHashTable<WUuid, NodeInternal> m_ObjectToNode;
  WHashTable<WUuid, WUniquePtr<WVisualGraphConnection>> m_ObjectToConnection;
  WMap<const WVisualGraphPin*, WHybridArray<const WVisualGraphConnection*, 6>> m_Connections;
};
