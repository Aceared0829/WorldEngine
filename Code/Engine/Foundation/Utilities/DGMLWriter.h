#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/ArrayPtr.h>

class WFormatString;

/// This class encapsulates building a DGML compatible graph.
class W_FOUNDATION_DLL WDGMLGraph
{
public:
  enum class Direction : WUInt8
  {
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft
  };

  enum class Layout : WUInt8
  {
    Free,
    Tree,
    DependencyMatrix
  };

  enum class NodeShape : WUInt8
  {
    None,
    Rectangle,
    RoundedRectangle,
    Button
  };

  enum class GroupType : WUInt8
  {
    None,
    Expanded,
    Collapsed,
  };

  using NodeId = WUInt32;
  using PropertyId = WUInt32;
  using ConnectionId = WUInt32;
  using CategoryId = WUInt32;

  struct NodeDesc
  {
    WColor m_Color = WColor::White;
    NodeShape m_Shape = NodeShape::Rectangle;
  };

  /// Constructor for the graph.
  WDGMLGraph(Direction graphDirection = Direction::LeftToRight, Layout graphLayout = Layout::Tree);

  /// Adds a node to the graph.
  /// Adds a node to the graph and returns the node id which can be used to reference the node later to add connections etc.
  NodeId AddNode(WStringView sTitle, const NodeDesc* pDesc = nullptr);

  /// Adds a DGML node that can act as a group for other nodes
  NodeId AddGroup(WStringView sTitle, GroupType type, const NodeDesc* pDesc = nullptr);

  /// Inserts a node into an existing group node.
  void AddNodeToGroup(NodeId node, NodeId group);

  /// Adds a category with a display color. Returns the category id for use with AddConnection.
  CategoryId AddConnectionCategory(WStringView sName, const WColor& color);

  /// Adds a directed connection to the graph (an arrow pointing from source to target node).
  ConnectionId AddConnection(NodeId source, NodeId target, WStringView sLabel = {}, CategoryId category = WInvalidIndex);

  /// Adds a property type. All properties currently use the data type 'string'
  PropertyId AddPropertyType(WStringView sName);

  /// Adds a property of the specified type with the given value to a node
  void AddNodeProperty(NodeId node, PropertyId property, const WFormatString& fmt);

protected:
  friend class WDGMLGraphWriter;

  struct ConnectionCategory
  {
    WString m_sName;
    WColor m_Color;
  };

  struct Connection
  {
    NodeId m_Source;
    NodeId m_Target;
    WString m_sLabel;
    CategoryId m_uiCategory = WInvalidIndex;
  };

  struct PropertyType
  {
    WString m_Name;
  };

  struct PropertyValue
  {
    PropertyId m_PropertyId;
    WString m_sValue;
  };

  struct Node
  {
    WString m_Title;
    GroupType m_GroupType = GroupType::None;
    NodeId m_ParentGroup = 0xFFFFFFFF;
    NodeDesc m_Desc;
    WDynamicArray<PropertyValue> m_Properties;
  };

  WHybridArray<Node, 16> m_Nodes;

  WHybridArray<Connection, 32> m_Connections;

  WHybridArray<ConnectionCategory, 16> m_ConnectionCategories;

  WHybridArray<PropertyType, 16> m_PropertyTypes;

  Direction m_Direction;

  Layout m_Layout;
};

/// This class encapsulates the output of DGML compatible graphs to files and streams.
class W_FOUNDATION_DLL WDGMLGraphWriter
{
public:
  /// Helper method to write the graph to a file.
  static WResult WriteGraphToFile(WStringView sFileName, const WDGMLGraph& graph);

  /// Writes the graph as a DGML formatted document to the given string builder.
  static WResult WriteGraphToString(WStringBuilder& ref_sStringBuilder, const WDGMLGraph& graph);
};
