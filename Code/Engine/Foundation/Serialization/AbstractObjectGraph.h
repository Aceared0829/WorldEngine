#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class WAbstractObjectGraph;

class W_FOUNDATION_DLL WAbstractObjectNode
{
public:
  struct Property
  {
    WStringView m_sPropertyName;
    WVariant m_Value;
  };

  WAbstractObjectNode() = default;

  const WHybridArray<Property, 16>& GetProperties() const { return m_Properties; }

  void AddProperty(WStringView sName, const WVariant& value);

  void RemoveProperty(WStringView sName);

  void ChangeProperty(WStringView sName, const WVariant& value);

  void RenameProperty(WStringView sOldName, WStringView sNewName);

  void ClearProperties();

  // Inlines a custom variant type. Use to patch properties that have been turned into custom variant type.
  // \sa W_DEFINE_CUSTOM_VARIANT_TYPE, W_DECLARE_CUSTOM_VARIANT_TYPE
  WResult InlineProperty(WStringView sName);

  const WAbstractObjectGraph* GetOwner() const { return m_pOwner; }
  const WUuid& GetGuid() const { return m_Guid; }
  WUInt32 GetTypeVersion() const { return m_uiTypeVersion; }
  void SetTypeVersion(WUInt32 uiTypeVersion) { m_uiTypeVersion = uiTypeVersion; }
  WStringView GetType() const { return m_sType; }
  void SetType(WStringView sType);

  const Property* FindProperty(WStringView sName) const;
  Property* FindProperty(WStringView sName);

  WStringView GetNodeName() const { return m_sNodeName; }

private:
  friend class WAbstractObjectGraph;

  WAbstractObjectGraph* m_pOwner = nullptr;

  WUuid m_Guid;
  WUInt32 m_uiTypeVersion = 0;
  WStringView m_sType;
  WStringView m_sNodeName;

  WHybridArray<Property, 16> m_Properties;
};
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WAbstractObjectNode);

struct W_FOUNDATION_DLL WAbstractGraphDiffOperation
{
  enum class Op
  {
    NodeAdded,
    NodeRemoved,
    PropertyChanged
  };

  Op m_Operation;
  WUuid m_Node;            // prop parent or added / deleted node
  WString m_sProperty;     // prop name or type
  WUInt32 m_uiTypeVersion; // only used for NodeAdded
  WVariant m_Value;
};

struct W_FOUNDATION_DLL WObjectChangeType
{
  using StorageType = WInt8;

  enum Enum : WInt8
  {
    NodeAdded,
    NodeRemoved,
    PropertySet,
    PropertyInserted,
    PropertyRemoved,

    Default = NodeAdded
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WObjectChangeType);


struct W_FOUNDATION_DLL WDiffOperation
{
  WEnum<WObjectChangeType> m_Operation;
  WUuid m_Node;        // owner of m_sProperty
  WString m_sProperty; // property
  WVariant m_Index;
  WVariant m_Value;
};
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WDiffOperation);


/// An intermediate representation for serializing/deserializing reflected objects.
///
/// The AbstractObjectGraph represents objects and their properties in a type-independent way,
/// allowing for versioning, patching, and format conversion. It serves as the core of WorldEngine's
/// serialization system and enables advanced features like:
///
/// - Cross-format serialization (DDL, binary, JSON)
/// - Type versioning and migration
/// - Graph diffing and merging
/// - Prefab instantiation and overrides
/// - Undo/redo systems
///
/// Structure:
/// - Graph contains nodes (WAbstractObjectNode) identified by GUIDs
/// - Each node has a type name, version, and properties
/// - Properties store values as WVariant (including object references)
/// - References between objects are stored as GUIDs
class W_FOUNDATION_DLL WAbstractObjectGraph
{
public:
  WAbstractObjectGraph() = default;
  ~WAbstractObjectGraph();

  void Clear();

  using FilterFunction = WDelegate<bool(const WAbstractObjectNode*, const WAbstractObjectNode::Property*)>;
  WAbstractObjectNode* Clone(WAbstractObjectGraph& ref_cloneTarget, const WAbstractObjectNode* pRootNode = nullptr, FilterFunction filter = FilterFunction()) const;

  WStringView RegisterString(WStringView sString);

  const WAbstractObjectNode* GetNode(const WUuid& guid) const;
  WAbstractObjectNode* GetNode(const WUuid& guid);

  const WAbstractObjectNode* GetNodeByName(WStringView sName) const;
  WAbstractObjectNode* GetNodeByName(WStringView sName);

  WAbstractObjectNode* AddNode(const WUuid& guid, WStringView sType, WUInt32 uiTypeVersion, WStringView sNodeName = {});
  void RemoveNode(const WUuid& guid);

  const WMap<WUuid, WAbstractObjectNode*>& GetAllNodes() const { return m_Nodes; }
  WMap<WUuid, WAbstractObjectNode*>& GetAllNodes() { return m_Nodes; }

  /// Remaps all node guids by adding the given seed, or if bRemapInverse is true, by subtracting it/
  ///   This is mostly used to remap prefab instance graphs to their prefab template graph.
  void ReMapNodeGuids(const WUuid& seedGuid, bool bRemapInverse = false);

  /// Tries to remap the guids of this graph to those in rhsGraph by walking in both down the hierarchy, starting at root and
  /// rhsRoot.
  ///
  ///  Note that in case of array properties the remapping assumes element indices to be equal
  ///  on both sides which will cause all moves inside the arrays to be lost as there is no way of recovering this information without an
  ///  equality criteria. This function is mostly used to remap a graph from a native object to a graph from WDocumentObjects to allow
  ///  applying native side changes to the original WDocumentObject hierarchy using diffs.
  void ReMapNodeGuidsToMatchGraph(WAbstractObjectNode* pRoot, const WAbstractObjectGraph& rhsGraph, const WAbstractObjectNode* pRhsRoot);

  /// Finds everything accessible by the given root node.
  void FindTransitiveHull(const WUuid& rootGuid, WSet<WUuid>& out_reachableNodes) const;
  /// Deletes everything not accessible by the given root node.
  void PruneGraph(const WUuid& rootGuid);

  /// Allows a node to be modified as a native object and automatically syncs changes back.
  ///
  /// This temporarily converts the node (and its sub-hierarchy) to native objects, calls the provided
  /// callback to allow modifications, then converts the modified objects back to the graph representation.
  /// This is useful for applying complex modifications that are easier to implement on native objects.
  /// Changes to the object hierarchy (adding/removing children) will be reflected in the graph.
  void ModifyNodeViaNativeCounterpart(WAbstractObjectNode* pRootNode, WDelegate<void(void*, const WRTTI*)> callback);

  /// Allows to copy a node from another graph into this graph.
  WAbstractObjectNode* CopyNodeIntoGraph(const WAbstractObjectNode* pNode);

  WAbstractObjectNode* CopyNodeIntoGraph(const WAbstractObjectNode* pNode, FilterFunction& ref_filter);

  void CreateDiffWithBaseGraph(const WAbstractObjectGraph& base, WDeque<WAbstractGraphDiffOperation>& out_diffResult) const;

  void ApplyDiff(WDeque<WAbstractGraphDiffOperation>& ref_diff);

  void MergeDiffs(const WDeque<WAbstractGraphDiffOperation>& lhs, const WDeque<WAbstractGraphDiffOperation>& rhs, WDeque<WAbstractGraphDiffOperation>& ref_out) const;

private:
  W_DISALLOW_COPY_AND_ASSIGN(WAbstractObjectGraph);

  void RemapVariant(WVariant& value, const WHashTable<WUuid, WUuid>& guidMap);
  void MergeArrays(const WVariantArray& baseArray, const WVariantArray& leftArray, const WVariantArray& rightArray, WVariantArray& out) const;
  void ReMapNodeGuidsToMatchGraphRecursive(WHashTable<WUuid, WUuid>& guidMap, WAbstractObjectNode* lhs, const WAbstractObjectGraph& rhsGraph, const WAbstractObjectNode* rhs);

  WSet<WString> m_Strings;
  WMap<WUuid, WAbstractObjectNode*> m_Nodes;
  WMap<WStringView, WAbstractObjectNode*> m_NodesByName;
};
