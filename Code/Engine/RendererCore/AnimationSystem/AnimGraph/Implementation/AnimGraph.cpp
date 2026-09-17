#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

WAnimGraph::WAnimGraph()
{
  Clear();
}

WAnimGraph::~WAnimGraph() = default;

void WAnimGraph::Clear()
{
  WMemoryUtils::ZeroFillArray(m_uiInputPinCounts);
  WMemoryUtils::ZeroFillArray(m_uiPinInstanceDataOffset);
  m_From.Clear();
  m_Nodes.Clear();
  m_bPreparedForUse = true;
  m_InstanceDataAllocator.ClearDescs();

  for (auto& r : m_OutputPinToInputPinMapping)
  {
    r.Clear();
  }
}

WAnimGraphNode* WAnimGraph::AddNode(WUniquePtr<WAnimGraphNode>&& pNode)
{
  m_bPreparedForUse = false;

  m_Nodes.PushBack(std::move(pNode));
  return m_Nodes.PeekBack().Borrow();
}

void WAnimGraph::AddConnection(const WAnimGraphNode* pSrcNode, WStringView sSrcPinName, WAnimGraphNode* pDstNode, WStringView sDstPinName)
{
  // TODO: assert pSrcNode and pDstNode exist

  m_bPreparedForUse = false;
  WStringView sIdx;

  WAbstractMemberProperty* pPinPropSrc = (WAbstractMemberProperty*)pSrcNode->GetDynamicRTTI()->FindPropertyByName(sSrcPinName);

  auto& to = m_From[pSrcNode].m_To.ExpandAndGetRef();
  to.m_sSrcPinName = sSrcPinName;
  to.m_pDstNode = pDstNode;
  to.m_sDstPinName = sDstPinName;
  to.m_pSrcPin = (WAnimGraphPin*)pPinPropSrc->GetPropertyPointer(pSrcNode);

  if (const char* szIdx = sDstPinName.FindSubString("["))
  {
    sIdx = WStringView(szIdx + 1, sDstPinName.GetEndPointer() - 1);
    sDstPinName = WStringView(sDstPinName.GetStartPointer(), szIdx);

    WAbstractArrayProperty* pPinPropDst = (WAbstractArrayProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);
    const WDynamicPinAttribute* pDynPinAttr = pPinPropDst->GetAttributeByType<WDynamicPinAttribute>();

    const WTypedMemberProperty<WUInt8>* pPinSizeProp = (const WTypedMemberProperty<WUInt8>*)pDstNode->GetDynamicRTTI()->FindPropertyByName(pDynPinAttr->GetProperty());
    WUInt8 uiArraySize = pPinSizeProp->GetValue(pDstNode);
    pPinPropDst->SetCount(pDstNode, uiArraySize);

    WUInt32 uiIdx;
    WConversionUtils::StringToUInt(sIdx, uiIdx).AssertSuccess();

    to.m_pDstPin = (WAnimGraphPin*)pPinPropDst->GetValuePointer(pDstNode, uiIdx);
  }
  else
  {
    WAbstractMemberProperty* pPinPropDst = (WAbstractMemberProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);

    to.m_pDstPin = (WAnimGraphPin*)pPinPropDst->GetPropertyPointer(pDstNode);
  }
}

void WAnimGraph::PreparePinMapping()
{
  WUInt16 uiOutputPinCounts[WAnimGraphPin::Type::ENUM_COUNT];
  WMemoryUtils::ZeroFillArray(uiOutputPinCounts);

  for (const auto& consFrom : m_From)
  {
    for (const ConnectionTo& to : consFrom.Value().m_To)
    {
      uiOutputPinCounts[to.m_pSrcPin->GetPinType()]++;
    }
  }

  for (WUInt32 i = 0; i < WAnimGraphPin::ENUM_COUNT; ++i)
  {
    m_OutputPinToInputPinMapping[i].Clear();
    m_OutputPinToInputPinMapping[i].SetCount(uiOutputPinCounts[i]);
  }
}

void WAnimGraph::AssignInputPinIndices()
{
  WMemoryUtils::ZeroFillArray(m_uiInputPinCounts);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      // there may be multiple connections to this pin
      // only assign the index the first time we see a connection to this pin
      // otherwise only count up the number of connections

      if (to.m_pDstPin->m_iPinIndex == -1)
      {
        to.m_pDstPin->m_iPinIndex = m_uiInputPinCounts[to.m_pDstPin->GetPinType()]++;
      }

      ++to.m_pDstPin->m_uiNumConnections;
    }
  }
}

void WAnimGraph::AssignOutputPinIndices()
{
  WInt16 iPinTypeCount[WAnimGraphPin::Type::ENUM_COUNT];
  WMemoryUtils::ZeroFillArray(iPinTypeCount);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      const WUInt8 pinType = to.m_pSrcPin->GetPinType();

      // there may be multiple connections from this pin
      // only assign the index the first time we see a connection from this pin

      if (to.m_pSrcPin->m_iPinIndex == -1)
      {
        to.m_pSrcPin->m_iPinIndex = iPinTypeCount[pinType]++;
      }

      // store the indices of all the destination pins
      m_OutputPinToInputPinMapping[pinType][to.m_pSrcPin->m_iPinIndex].PushBack(to.m_pDstPin->m_iPinIndex);
    }
  }
}

WUInt16 WAnimGraph::ComputeNodePriority(const WAnimGraphNode* pNode, WMap<const WAnimGraphNode*, WUInt16>& inout_Prios, WUInt16& inout_uiOutputPrio) const
{
  auto itPrio = inout_Prios.Find(pNode);
  if (itPrio.IsValid())
  {
    // priority already computed -> return it
    return itPrio.Value();
  }

  const auto itConsFrom = m_From.Find(pNode);

  WUInt16 uiOwnPrio = 0xFFFF;

  if (itConsFrom.IsValid())
  {
    // look at all outgoing priorities and take the smallest dst priority - 1
    for (const ConnectionTo& to : itConsFrom.Value().m_To)
    {
      uiOwnPrio = WMath::Min<WUInt16>(uiOwnPrio, ComputeNodePriority(to.m_pDstNode, inout_Prios, inout_uiOutputPrio) - 1);
    }
  }
  else
  {
    // has no outgoing connections at all -> max priority value
    uiOwnPrio = inout_uiOutputPrio;
    inout_uiOutputPrio -= 64;
  }

  W_ASSERT_DEBUG(uiOwnPrio != 0xFFFF, "");

  inout_Prios[pNode] = uiOwnPrio;
  return uiOwnPrio;
}

void WAnimGraph::SortNodesByPriority()
{
  // this is important so that we can step all nodes in linear order,
  // and have them generate their output such that it is ready before
  // dependent nodes are stepped

  WUInt16 uiOutputPrio = 0xFFFE;
  WMap<const WAnimGraphNode*, WUInt16> prios;
  for (const auto& pNode : m_Nodes)
  {
    ComputeNodePriority(pNode.Borrow(), prios, uiOutputPrio);
  }

  m_Nodes.Sort([&](const auto& lhs, const auto& rhs) -> bool
    { return prios[lhs.Borrow()] < prios[rhs.Borrow()]; });
}

void WAnimGraph::PrepareForUse()
{
  if (m_bPreparedForUse)
    return;

  m_bPreparedForUse = true;

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      to.m_pSrcPin->m_iPinIndex = -1;
      to.m_pSrcPin->m_uiNumConnections = 0;
      to.m_pDstPin->m_iPinIndex = -1;
      to.m_pDstPin->m_uiNumConnections = 0;
    }
  }

  SortNodesByPriority();
  PreparePinMapping();
  AssignInputPinIndices();
  AssignOutputPinIndices();

  m_InstanceDataAllocator.ClearDescs();
  for (const auto& pNode : m_Nodes)
  {
    WInstanceDataDesc desc;
    if (pNode->GetInstanceDataDesc(desc))
    {
      pNode->m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
    }
  }

  // EXTEND THIS if a new type is introduced
  {
    WInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(WInt8);
    desc.m_uiTypeSize = sizeof(WInt8) * m_uiInputPinCounts[WAnimGraphPin::Type::Trigger];
    m_uiPinInstanceDataOffset[WAnimGraphPin::Type::Trigger] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    WInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(double);
    desc.m_uiTypeSize = sizeof(double) * m_uiInputPinCounts[WAnimGraphPin::Type::Number];
    m_uiPinInstanceDataOffset[WAnimGraphPin::Type::Number] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    WInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(bool);
    desc.m_uiTypeSize = sizeof(bool) * m_uiInputPinCounts[WAnimGraphPin::Type::Bool];
    m_uiPinInstanceDataOffset[WAnimGraphPin::Type::Bool] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    WInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(WUInt16);
    desc.m_uiTypeSize = sizeof(WUInt16) * m_uiInputPinCounts[WAnimGraphPin::Type::BoneWeights];
    m_uiPinInstanceDataOffset[WAnimGraphPin::Type::BoneWeights] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    WInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(WUInt16);
    desc.m_uiTypeSize = sizeof(WUInt16) * m_uiInputPinCounts[WAnimGraphPin::Type::ModelPose];
    m_uiPinInstanceDataOffset[WAnimGraphPin::Type::ModelPose] = m_InstanceDataAllocator.AddDesc(desc);
  }
}

WResult WAnimGraph::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(10);

  const WUInt32 uiNumNodes = m_Nodes.GetCount();
  inout_stream << uiNumNodes;

  WMap<const WAnimGraphNode*, WUInt32> nodeToIdx;

  for (WUInt32 n = 0; n < m_Nodes.GetCount(); ++n)
  {
    const WAnimGraphNode* pNode = m_Nodes[n].Borrow();

    nodeToIdx[pNode] = n;

    inout_stream << pNode->GetDynamicRTTI()->GetTypeName();
    W_SUCCEED_OR_RETURN(pNode->SerializeNode(inout_stream));
  }

  inout_stream << m_From.GetCount();
  for (auto itFrom : m_From)
  {
    inout_stream << nodeToIdx[itFrom.Key()];

    const auto& toAll = itFrom.Value().m_To;
    inout_stream << toAll.GetCount();

    for (const auto& to : toAll)
    {
      inout_stream << to.m_sSrcPinName;
      inout_stream << nodeToIdx[to.m_pDstNode];
      inout_stream << to.m_sDstPinName;
    }
  }

  return W_SUCCESS;
}

WResult WAnimGraph::Deserialize(WStreamReader& inout_stream)
{
  Clear();

  const WTypeVersion version = inout_stream.ReadVersion(10);

  if (version < 10)
    return W_FAILURE;

  WUInt32 uiNumNodes = 0;
  inout_stream >> uiNumNodes;

  WDynamicArray<WAnimGraphNode*> idxToNode;
  idxToNode.SetCount(uiNumNodes);

  WStringBuilder sTypeName;

  for (WUInt32 n = 0; n < uiNumNodes; ++n)
  {
    inout_stream >> sTypeName;
    WUniquePtr<WAnimGraphNode> pNode = WRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<WAnimGraphNode>();
    W_SUCCEED_OR_RETURN(pNode->DeserializeNode(inout_stream));

    idxToNode[n] = AddNode(std::move(pNode));
  }

  WUInt32 uiNumConnectionsFrom = 0;
  inout_stream >> uiNumConnectionsFrom;

  WStringBuilder sPinSrc, sPinDst;

  for (WUInt32 cf = 0; cf < uiNumConnectionsFrom; ++cf)
  {
    WUInt32 nodeIdx;
    inout_stream >> nodeIdx;
    const WAnimGraphNode* ptrNodeFrom = idxToNode[nodeIdx];

    WUInt32 uiNumConnectionsTo = 0;
    inout_stream >> uiNumConnectionsTo;

    for (WUInt32 ct = 0; ct < uiNumConnectionsTo; ++ct)
    {
      inout_stream >> sPinSrc;

      inout_stream >> nodeIdx;
      WAnimGraphNode* ptrNodeTo = idxToNode[nodeIdx];

      inout_stream >> sPinDst;

      AddConnection(ptrNodeFrom, sPinSrc, ptrNodeTo, sPinDst);
    }
  }

  m_bPreparedForUse = false;
  return W_SUCCESS;
}
