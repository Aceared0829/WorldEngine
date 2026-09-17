#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WObjectChangeType, 1)
  W_ENUM_CONSTANTS(WObjectChangeType::NodeAdded, WObjectChangeType::NodeRemoved)
  W_ENUM_CONSTANTS(WObjectChangeType::PropertySet, WObjectChangeType::PropertyInserted, WObjectChangeType::PropertyRemoved)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WAbstractObjectNode, WNoBase, 1, WRTTIDefaultAllocator<WAbstractObjectNode>)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WDiffOperation, WNoBase, 1, WRTTIDefaultAllocator<WDiffOperation>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Operation", WObjectChangeType, m_Operation),
    W_MEMBER_PROPERTY("Node", m_Node),
    W_MEMBER_PROPERTY("Property", m_sProperty),
    W_MEMBER_PROPERTY("Index", m_Index),
    W_MEMBER_PROPERTY("Value", m_Value),
  }
    W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WAbstractObjectGraph::~WAbstractObjectGraph()
{
  Clear();
}

void WAbstractObjectGraph::Clear()
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    W_DEFAULT_DELETE(it.Value());
  }
  m_Nodes.Clear();
  m_NodesByName.Clear();
  m_Strings.Clear();
}


WAbstractObjectNode* WAbstractObjectGraph::Clone(WAbstractObjectGraph& ref_cloneTarget, const WAbstractObjectNode* pRootNode, FilterFunction filter) const
{
  ref_cloneTarget.Clear();

  if (pRootNode == nullptr)
  {
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (filter.IsValid())
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value(), filter);
      }
      else
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value());
      }
    }
    return nullptr;
  }
  else
  {
    W_ASSERT_DEV(pRootNode->GetOwner() == this, "The given root node must be part of this document");
    WSet<WUuid> reachableNodes;
    FindTransitiveHull(pRootNode->GetGuid(), reachableNodes);

    for (const WUuid& guid : reachableNodes)
    {
      if (auto* pNode = GetNode(guid))
      {
        if (filter.IsValid())
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode, filter);
        }
        else
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode);
        }
      }
    }

    return ref_cloneTarget.GetNode(pRootNode->GetGuid());
  }
}

WStringView WAbstractObjectGraph::RegisterString(WStringView sString)
{
  auto it = m_Strings.Insert(sString);
  W_ASSERT_DEV(it.IsValid(), "");
  return it.Key();
}

WAbstractObjectNode* WAbstractObjectGraph::GetNode(const WUuid& guid)
{
  return m_Nodes.GetValueOrDefault(guid, nullptr);
}

const WAbstractObjectNode* WAbstractObjectGraph::GetNode(const WUuid& guid) const
{
  return const_cast<WAbstractObjectGraph*>(this)->GetNode(guid);
}

const WAbstractObjectNode* WAbstractObjectGraph::GetNodeByName(WStringView sName) const
{
  return const_cast<WAbstractObjectGraph*>(this)->GetNodeByName(sName);
}

WAbstractObjectNode* WAbstractObjectGraph::GetNodeByName(WStringView sName)
{
  return m_NodesByName.GetValueOrDefault(sName, nullptr);
}

WAbstractObjectNode* WAbstractObjectGraph::AddNode(const WUuid& guid, WStringView sType, WUInt32 uiTypeVersion, WStringView sNodeName)
{
  W_ASSERT_DEV(!m_Nodes.Contains(guid), "object {0} must not yet exist", guid);
  if (!sNodeName.IsEmpty())
  {
    sNodeName = RegisterString(sNodeName);
  }
  else
  {
    sNodeName = {};
  }

  WAbstractObjectNode* pNode = W_DEFAULT_NEW(WAbstractObjectNode);
  pNode->m_Guid = guid;
  pNode->m_pOwner = this;
  pNode->m_sType = RegisterString(sType);
  pNode->m_uiTypeVersion = uiTypeVersion;
  pNode->m_sNodeName = sNodeName;

  m_Nodes[guid] = pNode;

  if (!sNodeName.IsEmpty())
  {
    m_NodesByName[sNodeName] = pNode;
  }

  return pNode;
}

void WAbstractObjectGraph::RemoveNode(const WUuid& guid)
{
  auto it = m_Nodes.Find(guid);

  if (it.IsValid())
  {
    WAbstractObjectNode* pNode = it.Value();
    if (!pNode->m_sNodeName.IsEmpty())
      m_NodesByName.Remove(pNode->m_sNodeName);

    m_Nodes.Remove(guid);
    W_DEFAULT_DELETE(pNode);
  }
}

void WAbstractObjectNode::AddProperty(WStringView sName, const WVariant& value)
{
  auto& prop = m_Properties.ExpandAndGetRef();
  prop.m_sPropertyName = m_pOwner->RegisterString(sName);
  prop.m_Value = value;
}

void WAbstractObjectNode::ChangeProperty(WStringView sName, const WVariant& value)
{
  for (WUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties[i].m_Value = value;
      return;
    }
  }

  W_REPORT_FAILURE("Property '{0}' is unknown", sName);
}

void WAbstractObjectNode::RenameProperty(WStringView sOldName, WStringView sNewName)
{
  for (WUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sOldName)
    {
      m_Properties[i].m_sPropertyName = m_pOwner->RegisterString(sNewName);
      return;
    }
  }
}

void WAbstractObjectNode::ClearProperties()
{
  m_Properties.Clear();
}

WResult WAbstractObjectNode::InlineProperty(WStringView sName)
{
  for (WUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    Property& prop = m_Properties[i];
    if (prop.m_sPropertyName == sName)
    {
      if (!prop.m_Value.IsA<WUuid>())
        return W_FAILURE;

      WUuid guid = prop.m_Value.Get<WUuid>();
      WAbstractObjectNode* pNode = m_pOwner->GetNode(guid);
      if (!pNode)
        return W_FAILURE;

      class InlineContext : public WRttiConverterContext
      {
      public:
        void RegisterObject(const WUuid& guid, const WRTTI* pRtti, void* pObject) override
        {
          W_IGNORE_UNUSED(pRtti);
          W_IGNORE_UNUSED(pObject);
          m_SubTree.PushBack(guid);
        }
        WTempHybridArray<WUuid, 1> m_SubTree;
      };

      InlineContext context;
      WRttiConverterReader reader(m_pOwner, &context);
      void* pObject = reader.CreateObjectFromNode(pNode);
      if (!pObject)
        return W_FAILURE;

      prop.m_Value.MoveTypedObject(pObject, WRTTI::FindTypeByName(pNode->GetType()));

      // Delete old objects.
      for (WUuid& uuid : context.m_SubTree)
      {
        m_pOwner->RemoveNode(uuid);
      }
      return W_SUCCESS;
    }
  }
  return W_FAILURE;
}

void WAbstractObjectNode::RemoveProperty(WStringView sName)
{
  for (WUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties.RemoveAtAndSwap(i);
      return;
    }
  }
}

void WAbstractObjectNode::SetType(WStringView sType)
{
  m_sType = m_pOwner->RegisterString(sType);
}

const WAbstractObjectNode::Property* WAbstractObjectNode::FindProperty(WStringView sName) const
{
  for (WUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

WAbstractObjectNode::Property* WAbstractObjectNode::FindProperty(WStringView sName)
{
  for (WUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

void WAbstractObjectGraph::ReMapNodeGuids(const WUuid& seedGuid, bool bRemapInverse /*= false*/)
{
  WTempHybridArray<WAbstractObjectNode*, 16> nodes;
  nodes.Reserve(m_Nodes.GetCount());
  WHashTable<WUuid, WUuid> guidMap;
  guidMap.Reserve(m_Nodes.GetCount());

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    WUuid newGuid = it.Key();

    if (bRemapInverse)
      newGuid.RevertCombinationWithSeed(seedGuid);
    else
      newGuid.CombineWithSeed(seedGuid);

    guidMap[it.Key()] = newGuid;

    nodes.PushBack(it.Value());
  }

  m_Nodes.Clear();

  // go through all nodes to remap guids
  for (auto* pNode : nodes)
  {
    pNode->m_Guid = guidMap[pNode->m_Guid];

    // check every property
    for (auto& prop : pNode->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
    }
    m_Nodes[pNode->m_Guid] = pNode;
  }
}


void WAbstractObjectGraph::ReMapNodeGuidsToMatchGraph(WAbstractObjectNode* pRoot, const WAbstractObjectGraph& rhsGraph, const WAbstractObjectNode* pRhsRoot)
{
  WHashTable<WUuid, WUuid> guidMap;
  W_ASSERT_DEV(pRoot->GetType() == pRhsRoot->GetType(), "Roots must have the same type to be able re-map guids!");

  ReMapNodeGuidsToMatchGraphRecursive(guidMap, pRoot, rhsGraph, pRhsRoot);

  // go through all nodes to remap remaining occurrences of remapped guids
  for (auto it : m_Nodes)
  {
    // check every property
    for (auto& prop : it.Value()->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
    }
    m_Nodes[it.Value()->m_Guid] = it.Value();
  }
}

void WAbstractObjectGraph::ReMapNodeGuidsToMatchGraphRecursive(WHashTable<WUuid, WUuid>& guidMap, WAbstractObjectNode* lhs, const WAbstractObjectGraph& rhsGraph, const WAbstractObjectNode* rhs)
{
  if (lhs->GetType() != rhs->GetType())
  {
    // Types differ, remapping ends as this is a removal and add of a new object.
    return;
  }

  if (lhs->GetGuid() != rhs->GetGuid())
  {
    guidMap[lhs->GetGuid()] = rhs->GetGuid();
    m_Nodes.Remove(lhs->GetGuid());
    lhs->m_Guid = rhs->GetGuid();
    m_Nodes.Insert(rhs->GetGuid(), lhs);
  }

  for (WAbstractObjectNode::Property& prop : lhs->m_Properties)
  {
    if (prop.m_Value.IsA<WUuid>() && prop.m_Value.Get<WUuid>().IsValid())
    {
      // if the guid is an owned object in the graph, remap to rhs.
      auto it = m_Nodes.Find(prop.m_Value.Get<WUuid>());
      if (it.IsValid())
      {
        if (const WAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
        {
          if (rhsProp->m_Value.IsA<WUuid>() && rhsProp->m_Value.Get<WUuid>().IsValid())
          {
            if (const WAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsProp->m_Value.Get<WUuid>()))
            {
              ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
            }
          }
        }
      }
    }
    // Arrays may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<WVariantArray>())
    {
      const WVariantArray& values = prop.m_Value.Get<WVariantArray>();
      for (WUInt32 i = 0; i < values.GetCount(); i++)
      {
        auto& subValue = values[i];
        if (subValue.IsA<WUuid>() && subValue.Get<WUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to array element.
          auto it = m_Nodes.Find(subValue.Get<WUuid>());
          if (it.IsValid())
          {
            if (const WAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<WVariantArray>())
              {
                const WVariantArray& rhsValues = rhsProp->m_Value.Get<WVariantArray>();
                if (i < rhsValues.GetCount())
                {
                  const auto& rhsElemValue = rhsValues[i];
                  if (rhsElemValue.IsA<WUuid>() && rhsElemValue.Get<WUuid>().IsValid())
                  {
                    if (const WAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<WUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    // Maps may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<WVariantDictionary>())
    {
      const WVariantDictionary& values = prop.m_Value.Get<WVariantDictionary>();
      for (auto lhsIt = values.GetIterator(); lhsIt.IsValid(); ++lhsIt)
      {
        auto& subValue = lhsIt.Value();
        if (subValue.IsA<WUuid>() && subValue.Get<WUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to map element.
          auto it = m_Nodes.Find(subValue.Get<WUuid>());
          if (it.IsValid())
          {
            if (const WAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<WVariantDictionary>())
              {
                const WVariantDictionary& rhsValues = rhsProp->m_Value.Get<WVariantDictionary>();
                if (rhsValues.Contains(lhsIt.Key()))
                {
                  const auto& rhsElemValue = *rhsValues.GetValue(lhsIt.Key());
                  if (rhsElemValue.IsA<WUuid>() && rhsElemValue.Get<WUuid>().IsValid())
                  {
                    if (const WAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<WUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}


void WAbstractObjectGraph::FindTransitiveHull(const WUuid& rootGuid, WSet<WUuid>& ref_reachableNodes) const
{
  ref_reachableNodes.Clear();
  WSet<WUuid> inProgress;
  inProgress.Insert(rootGuid);

  while (!inProgress.IsEmpty())
  {
    WUuid current = *inProgress.GetIterator();
    auto it = m_Nodes.Find(current);
    if (it.IsValid())
    {
      const WAbstractObjectNode* pNode = it.Value();
      for (auto& prop : pNode->m_Properties)
      {
        if (prop.m_Value.IsA<WUuid>())
        {
          const WUuid& guid = prop.m_Value.Get<WUuid>();
          if (!ref_reachableNodes.Contains(guid))
          {
            inProgress.Insert(guid);
          }
        }
        // Arrays may be of uuids
        else if (prop.m_Value.IsA<WVariantArray>())
        {
          const WVariantArray& values = prop.m_Value.Get<WVariantArray>();
          for (auto& subValue : values)
          {
            if (subValue.IsA<WUuid>())
            {
              const WUuid& guid = subValue.Get<WUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
        else if (prop.m_Value.IsA<WVariantDictionary>())
        {
          const WVariantDictionary& values = prop.m_Value.Get<WVariantDictionary>();
          for (auto& subValue : values)
          {
            if (subValue.Value().IsA<WUuid>())
            {
              const WUuid& guid = subValue.Value().Get<WUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
      }
    }
    // Even if 'current' is not in the graph add it anyway to early out if it is found again.
    ref_reachableNodes.Insert(current);
    inProgress.Remove(current);
  }
}

void WAbstractObjectGraph::PruneGraph(const WUuid& rootGuid)
{
  WSet<WUuid> reachableNodes;
  FindTransitiveHull(rootGuid, reachableNodes);

  // Determine nodes to be removed by subtracting valid ones from all nodes.
  WSet<WUuid> removeSet;
  for (auto it = GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    removeSet.Insert(it.Key());
  }
  removeSet.Difference(reachableNodes);

  // Remove nodes.
  for (const WUuid& guid : removeSet)
  {
    RemoveNode(guid);
  }
}

void WAbstractObjectGraph::ModifyNodeViaNativeCounterpart(WAbstractObjectNode* pRootNode, WDelegate<void(void*, const WRTTI*)> callback)
{
  W_ASSERT_DEV(pRootNode->GetOwner() == this, "Node must be from this graph.");

  // Clone sub graph
  WAbstractObjectGraph origGraph;
  WAbstractObjectNode* pOrigRootNode = nullptr;
  {
    pOrigRootNode = Clone(origGraph, pRootNode);
  }

  // Create native object
  WRttiConverterContext context;
  WRttiConverterReader convRead(&origGraph, &context);
  void* pNativeRoot = convRead.CreateObjectFromNode(pOrigRootNode);
  const WRTTI* pType = WRTTI::FindTypeByName(pOrigRootNode->GetType());
  W_SCOPE_EXIT(pType->GetAllocator()->Deallocate(pNativeRoot););

  // Make changes to native object
  if (callback.IsValid())
  {
    callback(pNativeRoot, pType);
  }

  // Create native object graph
  WAbstractObjectGraph graph;
  {
    // The WApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those
    // of the object manager.
    WApplyNativePropertyChangesContext nativeChangesContext(context, origGraph);
    WRttiConverterWriter rttiConverter(&graph, &nativeChangesContext, true, true);
    nativeChangesContext.RegisterObject(pOrigRootNode->GetGuid(), pType, pNativeRoot);
    rttiConverter.AddObjectToGraph(pType, pNativeRoot, "Object");
  }

  // Create diff from native to cloned sub-graph and then apply the diff to the original graph.
  WDeque<WAbstractGraphDiffOperation> diffResult;
  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  ApplyDiff(diffResult);
}

WAbstractObjectNode* WAbstractObjectGraph::CopyNodeIntoGraph(const WAbstractObjectNode* pNode)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  for (const auto& props : pNode->GetProperties())
  {
    pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
  }

  return pNewNode;
}

WAbstractObjectNode* WAbstractObjectGraph::CopyNodeIntoGraph(const WAbstractObjectNode* pNode, FilterFunction& ref_filter)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  if (ref_filter.IsValid())
  {
    for (const auto& props : pNode->GetProperties())
    {
      if (!ref_filter(pNode, &props))
        continue;

      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
    }
  }
  else
  {
    for (const auto& props : pNode->GetProperties())
      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
  }

  return pNewNode;
}

void WAbstractObjectGraph::CreateDiffWithBaseGraph(const WAbstractObjectGraph& base, WDeque<WAbstractGraphDiffOperation>& out_diffResult) const
{
  out_diffResult.Clear();

  // check whether any nodes have been deleted
  {
    for (auto itNodeBase = base.GetAllNodes().GetIterator(); itNodeBase.IsValid(); ++itNodeBase)
    {
      if (GetNode(itNodeBase.Key()) == nullptr)
      {
        // does not exist in this graph -> has been deleted from base
        WAbstractGraphDiffOperation op;
        op.m_Node = itNodeBase.Key();
        op.m_Operation = WAbstractGraphDiffOperation::Op::NodeRemoved;
        op.m_sProperty = itNodeBase.Value()->m_sType;
        op.m_Value = itNodeBase.Value()->m_sNodeName;

        out_diffResult.PushBack(op);
      }
    }
  }

  // check whether any nodes have been added
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      if (base.GetNode(itNodeThis.Key()) == nullptr)
      {
        // does not exist in base graph -> has been added
        WAbstractGraphDiffOperation op;
        op.m_Node = itNodeThis.Key();
        op.m_Operation = WAbstractGraphDiffOperation::Op::NodeAdded;
        op.m_sProperty = itNodeThis.Value()->m_sType;
        op.m_Value = itNodeThis.Value()->m_sNodeName;

        out_diffResult.PushBack(op);

        // set all properties
        for (const auto& prop : itNodeThis.Value()->GetProperties())
        {
          op.m_Operation = WAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value = prop.m_Value;

          out_diffResult.PushBack(op);
        }
      }
    }
  }

  // check whether any properties have been modified
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      const auto pBaseNode = base.GetNode(itNodeThis.Key());

      if (pBaseNode == nullptr)
        continue;

      for (const WAbstractObjectNode::Property& prop : itNodeThis.Value()->GetProperties())
      {
        bool bDifferent = true;

        for (const WAbstractObjectNode::Property& baseProp : pBaseNode->GetProperties())
        {
          if (baseProp.m_sPropertyName == prop.m_sPropertyName)
          {
            if (baseProp.m_Value == prop.m_Value)
            {
              bDifferent = false;
              break;
            }

            bDifferent = true;
            break;
          }
        }

        if (bDifferent)
        {
          WAbstractGraphDiffOperation op;
          op.m_Node = itNodeThis.Key();
          op.m_Operation = WAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value = prop.m_Value;

          out_diffResult.PushBack(op);
        }
      }
    }
  }
}


void WAbstractObjectGraph::ApplyDiff(WDeque<WAbstractGraphDiffOperation>& ref_diff)
{
  for (const auto& op : ref_diff)
  {
    switch (op.m_Operation)
    {
      case WAbstractGraphDiffOperation::Op::NodeAdded:
      {
        AddNode(op.m_Node, op.m_sProperty, op.m_uiTypeVersion, op.m_Value.Get<WString>());
      }
      break;

      case WAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        RemoveNode(op.m_Node);
      }
      break;

      case WAbstractGraphDiffOperation::Op::PropertyChanged:
      {
        auto* pNode = GetNode(op.m_Node);
        if (pNode)
        {
          auto* pProp = pNode->FindProperty(op.m_sProperty);

          if (!pProp)
            pNode->AddProperty(op.m_sProperty, op.m_Value);
          else
            pProp->m_Value = op.m_Value;
        }
      }
      break;
    }
  }
}


void WAbstractObjectGraph::MergeDiffs(const WDeque<WAbstractGraphDiffOperation>& lhs, const WDeque<WAbstractGraphDiffOperation>& rhs, WDeque<WAbstractGraphDiffOperation>& ref_out) const
{
  struct Prop
  {
    Prop() = default;
    Prop(WUuid node, WStringView sProperty)
      : m_Node(node)
      , m_sProperty(sProperty)
    {
    }
    WUuid m_Node;
    WStringView m_sProperty;

    bool operator<(const Prop& rhs) const
    {
      if (m_Node == rhs.m_Node)
        return m_sProperty < rhs.m_sProperty;

      return m_Node < rhs.m_Node;
    }

    bool operator==(const Prop& rhs) const { return m_Node == rhs.m_Node && m_sProperty == rhs.m_sProperty; }
  };

  WMap<Prop, WTempHybridArray<const WAbstractGraphDiffOperation*, 2>> propChanges;
  WSet<WUuid> removed;
  WMap<WUuid, WUInt32> added;
  for (const WAbstractGraphDiffOperation& op : lhs)
  {
    if (op.m_Operation == WAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      removed.Insert(op.m_Node);
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == WAbstractGraphDiffOperation::Op::NodeAdded)
    {
      added[op.m_Node] = ref_out.GetCount();
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == WAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }
  for (const WAbstractGraphDiffOperation& op : rhs)
  {
    if (op.m_Operation == WAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      if (!removed.Contains(op.m_Node))
        ref_out.PushBack(op);
    }
    else if (op.m_Operation == WAbstractGraphDiffOperation::Op::NodeAdded)
    {
      if (added.Contains(op.m_Node))
      {
        WAbstractGraphDiffOperation& leftOp = ref_out[added[op.m_Node]];
        leftOp.m_sProperty = op.m_sProperty; // Take type from rhs.
      }
      else
      {
        ref_out.PushBack(op);
      }
    }
    else if (op.m_Operation == WAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }

  for (auto it = propChanges.GetIterator(); it.IsValid(); ++it)
  {
    const Prop& key = it.Key();
    const WTempHybridArray<const WAbstractGraphDiffOperation*, 2>& value = it.Value();

    if (value.GetCount() == 1)
    {
      ref_out.PushBack(*value[0]);
    }
    else
    {
      const WAbstractGraphDiffOperation& leftProp = *value[0];
      const WAbstractGraphDiffOperation& rightProp = *value[1];

      if (leftProp.m_Value.GetType() == WVariantType::VariantArray && rightProp.m_Value.GetType() == WVariantType::VariantArray)
      {
        const WVariantArray& leftArray = leftProp.m_Value.Get<WVariantArray>();
        const WVariantArray& rightArray = rightProp.m_Value.Get<WVariantArray>();

        const WAbstractObjectNode* pNode = GetNode(key.m_Node);
        if (pNode)
        {
          WStringBuilder sTemp(key.m_sProperty);
          const WAbstractObjectNode::Property* pProperty = pNode->FindProperty(sTemp);
          if (pProperty && pProperty->m_Value.GetType() == WVariantType::VariantArray)
          {
            // Do 3-way array merge
            const WVariantArray& baseArray = pProperty->m_Value.Get<WVariantArray>();
            WVariantArray res;
            MergeArrays(baseArray, leftArray, rightArray, res);
            ref_out.PushBack(rightProp);
            ref_out.PeekBack().m_Value = res;
          }
          else
          {
            ref_out.PushBack(rightProp);
          }
        }
        else
        {
          ref_out.PushBack(rightProp);
        }
      }
      else
      {
        ref_out.PushBack(rightProp);
      }
    }
  }
}

void WAbstractObjectGraph::RemapVariant(WVariant& value, const WHashTable<WUuid, WUuid>& guidMap)
{
  WStringBuilder tmp;

  // if the property is a guid, we check if we need to remap it
  if (value.IsA<WUuid>())
  {
    const WUuid& guid = value.Get<WUuid>();

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = *found;
    }
  }
  else if (value.IsA<WString>() && WConversionUtils::IsStringUuid(value.Get<WString>()))
  {
    const WUuid guid = WConversionUtils::ConvertStringToUuid(value.Get<WString>());

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = WConversionUtils::ToString(*found, tmp).GetData();
    }
  }
  // Arrays may be of uuids
  else if (value.IsA<WVariantArray>())
  {
    const WVariantArray& values = value.Get<WVariantArray>();
    bool bNeedToRemap = false;
    for (auto& subValue : values)
    {
      if (subValue.IsA<WUuid>() && guidMap.Contains(subValue.Get<WUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<WString>() && WConversionUtils::IsStringUuid(subValue.Get<WString>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<WVariantArray>())
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      WVariantArray newValues = values;
      for (auto& subValue : newValues)
      {
        RemapVariant(subValue, guidMap);
      }
      value = newValues;
    }
  }
  // Maps may be of uuids
  else if (value.IsA<WVariantDictionary>())
  {
    const WVariantDictionary& values = value.Get<WVariantDictionary>();
    bool bNeedToRemap = false;
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      const WVariant& subValue = it.Value();

      if (subValue.IsA<WUuid>() && guidMap.Contains(subValue.Get<WUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<WString>() && WConversionUtils::IsStringUuid(subValue.Get<WString>()))
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      WVariantDictionary newValues = values;
      for (auto it = newValues.GetIterator(); it.IsValid(); ++it)
      {
        RemapVariant(it.Value(), guidMap);
      }
      value = newValues;
    }
  }
}

void WAbstractObjectGraph::MergeArrays(const WDynamicArray<WVariant>& baseArray, const WDynamicArray<WVariant>& leftArray, const WDynamicArray<WVariant>& rightArray, WDynamicArray<WVariant>& out) const
{
  // Find element type.
  WVariantType::Enum type = WVariantType::Invalid;
  if (!baseArray.IsEmpty())
    type = baseArray[0].GetType();
  if (type != WVariantType::Invalid && !leftArray.IsEmpty())
    type = leftArray[0].GetType();
  if (type != WVariantType::Invalid && !rightArray.IsEmpty())
    type = rightArray[0].GetType();

  if (type == WVariantType::Invalid)
    return;

  // For now, assume non-uuid types are arrays, uuids are sets.
  if (type != WVariantType::Uuid)
  {
    // Any size changes?
    WUInt32 uiSize = baseArray.GetCount();
    if (leftArray.GetCount() != baseArray.GetCount())
      uiSize = leftArray.GetCount();
    if (rightArray.GetCount() != baseArray.GetCount())
      uiSize = rightArray.GetCount();

    out.SetCount(uiSize);
    for (WUInt32 i = 0; i < uiSize; i++)
    {
      if (i < baseArray.GetCount())
        out[i] = baseArray[i];
    }

    WUInt32 uiCountLeft = WMath::Min(uiSize, leftArray.GetCount());
    for (WUInt32 i = 0; i < uiCountLeft; i++)
    {
      if (leftArray[i] != baseArray[i])
        out[i] = leftArray[i];
    }

    WUInt32 uiCountRight = WMath::Min(uiSize, rightArray.GetCount());
    for (WUInt32 i = 0; i < uiCountRight; i++)
    {
      if (rightArray[i] != baseArray[i])
        out[i] = rightArray[i];
    }
    return;
  }

  // Move distance is NP-complete so try greedy algorithm
  struct Element
  {
    Element(const WVariant* pValue = nullptr, WInt32 iBaseIndex = -1, WInt32 iLeftIndex = -1, WInt32 iRightIndex = -1)
      : m_pValue(pValue)
      , m_iBaseIndex(iBaseIndex)
      , m_iLeftIndex(iLeftIndex)
      , m_iRightIndex(iRightIndex)
      , m_fIndex(WMath::MaxValue<float>())
    {
    }
    bool IsDeleted() const { return m_iBaseIndex != -1 && (m_iLeftIndex == -1 || m_iRightIndex == -1); }
    bool operator<(const Element& rhs) const { return m_fIndex < rhs.m_fIndex; }

    const WVariant* m_pValue;
    WInt32 m_iBaseIndex;
    WInt32 m_iLeftIndex;
    WInt32 m_iRightIndex;
    float m_fIndex;
  };
  WDynamicArray<Element> baseOrder;
  baseOrder.Reserve(leftArray.GetCount() + rightArray.GetCount());

  // First, add up all unique elements and their position in each array.
  for (WInt32 i = 0; i < (WInt32)baseArray.GetCount(); i++)
  {
    baseOrder.PushBack(Element(&baseArray[i], i));
    baseOrder.PeekBack().m_fIndex = (float)i;
  }

  WDynamicArray<WInt32> leftOrder;
  leftOrder.SetCountUninitialized(leftArray.GetCount());
  for (WInt32 i = 0; i < (WInt32)leftArray.GetCount(); i++)
  {
    const WVariant& val = leftArray[i];
    bool bFound = false;
    for (WInt32 j = 0; j < (WInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iLeftIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iLeftIndex = i;
        leftOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      leftOrder[i] = (WInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&leftArray[i], -1, i));
    }
  }

  WDynamicArray<WInt32> rightOrder;
  rightOrder.SetCountUninitialized(rightArray.GetCount());
  for (WInt32 i = 0; i < (WInt32)rightArray.GetCount(); i++)
  {
    const WVariant& val = rightArray[i];
    bool bFound = false;
    for (WInt32 j = 0; j < (WInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iRightIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iRightIndex = i;
        rightOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      rightOrder[i] = (WInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&rightArray[i], -1, -1, i));
    }
  }

  // Re-order greedy
  float fLastElement = -0.5f;
  for (WInt32 i = 0; i < (WInt32)leftOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[leftOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = WMath::MaxValue<float>();
    for (WInt32 j = i + 1; j < (WInt32)leftOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[leftOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }

  fLastElement = -0.5f;
  for (WInt32 i = 0; i < (WInt32)rightOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[rightOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = WMath::MaxValue<float>();
    for (WInt32 j = i + 1; j < (WInt32)rightOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[rightOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }


  // Sort
  baseOrder.Sort();
  out.Reserve(baseOrder.GetCount());
  for (const Element& elem : baseOrder)
  {
    if (!elem.IsDeleted())
    {
      out.PushBack(*elem.m_pValue);
    }
  }
}

W_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_AbstractObjectGraph);
