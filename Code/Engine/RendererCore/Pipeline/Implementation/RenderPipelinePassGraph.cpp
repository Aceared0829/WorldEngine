#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/ViewData.h>

WRenderPipelinePassGraph::WRenderPipelinePassGraph(WDynamicArray<WUniquePtr<WRenderPipelinePass>>&& passes, WDynamicArray<WUniquePtr<WExtractor>>&& extractors, WArrayPtr<const WRenderPipelineResourceLoaderConnection> connections)
  : m_Passes(std::move(passes))
  , m_Extractors(std::move(extractors))
{
  SortExtractors();

  m_PassInfos.Reserve(m_Passes.GetCount());
  WUInt32 uiInputPinCount = 0;
  WUInt32 uiOutputPinCount = 0;
  WHashTable<const WRenderPipelineNodePin*, WUInt16, WHashHelper<const WRenderPipelineNodePin*>, WTempAllocatorWrapper> PinToIndex;

  auto ResolvePin = [&](WUInt32 uiPassIdx, const WRenderPipelineNodePin* pPin) -> WUInt16
  {
    bool bExisted = false;
    WUInt16& uiPinIdx = PinToIndex.FindOrAdd(pPin, &bExisted);
    if (!bExisted)
    {
      uiPinIdx = m_Pins.GetCount();
      PinInfo& pinInfo = m_Pins.ExpandAndGetRef();
      pinInfo.m_uiPassIndex = uiPassIdx;
      pinInfo.m_uiInputPinIndex = pPin->m_uiInputIndex;
      pinInfo.m_uiOutputPinIndex = pPin->m_uiOutputIndex;
      pinInfo.m_Flags = pPin->m_Type;
      if (pinInfo.m_Flags.IsSet(WRenderPipelineNodePin::Type::TextureProvider) && !pinInfo.m_Flags.IsSet(WRenderPipelineNodePin::Type::Buffer))
      {
        m_TextureProviderPins.PushBack(uiPinIdx);
      }
    }
    return uiPinIdx;
  };

  // Calculate data sizes
  const WUInt32 uiPassCount = m_Passes.GetCount();
  for (WUInt32 uiPassIdx = 0; uiPassIdx < uiPassCount; ++uiPassIdx)
  {
    WRenderPipelinePass* pPass = m_Passes[uiPassIdx].Borrow();
    pPass->InitializePins();
    uiInputPinCount += pPass->GetInputPins().GetCount();
    uiOutputPinCount += pPass->GetOutputPins().GetCount();

    if (WSwitchBasePass* pSwitch = WDynamicCast<WSwitchBasePass*>(pPass))
    {
      SwitchInfo& switchInfo = m_Switches.ExpandAndGetRef();
      switchInfo.m_pSwitch = pSwitch;
      switchInfo.m_sBlackboardProperty.Assign(pSwitch->m_sBlackboardProperty);
    }
  }
  m_Pins.Reserve(uiInputPinCount + uiOutputPinCount);
  PinToIndex.Reserve(uiInputPinCount + uiOutputPinCount);
  m_InputConnectionsStorage.Reserve(uiInputPinCount);
  m_OutputConnectionsStorage.Reserve(uiOutputPinCount);
  m_InputPinsStorage.Reserve(connections.GetCount());

  // Fill m_NodeInfos, m_Pins and PinToIndex
  for (WUInt32 uiPassIdx = 0; uiPassIdx < uiPassCount; ++uiPassIdx)
  {
    PassInfo& passInfo = m_PassInfos.ExpandAndGetRef();
    WRenderPipelinePass* pPass = m_Passes[uiPassIdx].Borrow();

    const WArrayPtr<const WRenderPipelineNodePin* const> inputs = pPass->GetInputPins();
    WUInt16 uiInputStartCount = m_InputConnectionsStorage.GetCount();
    for (WUInt32 uiPinIdx = 0; uiPinIdx < inputs.GetCount(); ++uiPinIdx)
    {
      m_InputConnectionsStorage.PushBack(s_uiInvalidIndex);
      ResolvePin(uiPassIdx, inputs[uiPinIdx]);
    }
    passInfo.m_uiInputConnections = WMakeArrayPtr<WUInt16>(m_InputConnectionsStorage.GetData() + uiInputStartCount, inputs.GetCount());

    const WArrayPtr<const WRenderPipelineNodePin* const> outputs = pPass->GetOutputPins();
    WUInt16 uiOutputStartCount = m_OutputConnectionsStorage.GetCount();
    for (WUInt32 uiPinIdx = 0; uiPinIdx < outputs.GetCount(); ++uiPinIdx)
    {
      m_OutputConnectionsStorage.PushBack(s_uiInvalidIndex);
      ResolvePin(uiPassIdx, outputs[uiPinIdx]);
    }
    passInfo.m_uiOutputConnections = WMakeArrayPtr<WUInt16>(m_OutputConnectionsStorage.GetData() + uiOutputStartCount, outputs.GetCount());
  }

  struct Connection
  {
    W_DECLARE_POD_TYPE();
    WUInt16 m_uiSource;
    WUInt16 m_uiTarget;
    WUInt16 m_uiSourcePinIndex;
    WUInt16 m_uiTargetPinIndex;
  };
  WDynamicArray<Connection> resolvedConnections;
  resolvedConnections.Reserve(connections.GetCount());
  // Resolve connections
  for (WUInt32 uiConnectionIdx = 0; uiConnectionIdx < connections.GetCount(); ++uiConnectionIdx)
  {
    const WRenderPipelineResourceLoaderConnection& loaderConn = connections[uiConnectionIdx];
    const WRenderPipelineNodePin* pSourcePin = m_Passes[loaderConn.m_uiSource]->GetPinByName(WTempHashedString(loaderConn.m_sSourcePin));
    const WRenderPipelineNodePin* pTargetPin = m_Passes[loaderConn.m_uiTarget]->GetPinByName(WTempHashedString(loaderConn.m_sTargetPin));
    if (pSourcePin == nullptr)
    {
      WLog::Warning("Failed to resolve pin '{0}' on node of type '{1}'", loaderConn.m_sSourcePin, m_Passes[loaderConn.m_uiSource]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pTargetPin == nullptr)
    {
      WLog::Warning("Failed to resolve pin '{0}' on node of type '{1}'", loaderConn.m_sTargetPin, m_Passes[loaderConn.m_uiTarget]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pSourcePin->m_uiOutputIndex == 0xFF)
    {
      WLog::Error("Failed to connect pin '{0}' of node type '{1}', because it is not an output pin.", loaderConn.m_sSourcePin, m_Passes[loaderConn.m_uiSource]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pTargetPin->m_uiInputIndex == 0xFF)
    {
      WLog::Error("Failed to connect to pin '{0}' of node type '{1}', because it is not an input pin.", loaderConn.m_sTargetPin, m_Passes[loaderConn.m_uiTarget]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pSourcePin->m_Type.IsSet(WRenderPipelineNodePin::Type::Buffer) != pTargetPin->m_Type.IsSet(WRenderPipelineNodePin::Type::Buffer))
    {
      WLog::Error("Failed to connect pin '{0}' of node type '{1}' to pin '{2}' of node type '{3}', because texture pins can't be connected to buffer pins.", loaderConn.m_sSourcePin, m_Passes[loaderConn.m_uiSource]->GetDynamicRTTI()->GetTypeName(), loaderConn.m_sTargetPin, m_Passes[loaderConn.m_uiTarget]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    Connection& conn = resolvedConnections.ExpandAndGetRef();
    conn.m_uiSource = loaderConn.m_uiSource;
    conn.m_uiTarget = loaderConn.m_uiTarget;
    conn.m_uiSourcePinIndex = pSourcePin->m_uiOutputIndex;
    conn.m_uiTargetPinIndex = pTargetPin->m_uiInputIndex;
  }

  // Sort so we can create arrayPtr out of these.
  resolvedConnections.Sort([](const Connection& lhs, const Connection& rhs) -> bool
    {
      if (lhs.m_uiSource != rhs.m_uiSource)
        return lhs.m_uiSource < rhs.m_uiSource;

      if (lhs.m_uiSourcePinIndex != rhs.m_uiSourcePinIndex)
        return lhs.m_uiSourcePinIndex < rhs.m_uiSourcePinIndex;

      if (lhs.m_uiTarget != rhs.m_uiTarget)
        return lhs.m_uiTarget < rhs.m_uiTarget;

      return lhs.m_uiTargetPinIndex < rhs.m_uiTargetPinIndex; });

  // Add connections by iterating the contiguous chunks of connections
  WUInt16 uiCurrentSource = s_uiInvalidIndex;
  WUInt16 uiCurrentSourcePin = s_uiInvalidIndex;
  WUInt32 uiStartIndex = m_InputPinsStorage.GetCount();
  auto FinishBlock = [&]()
  {
    const WUInt16 uiConnection = m_Connections.GetCount();
    m_PassInfos[uiCurrentSource].m_uiOutputConnections[uiCurrentSourcePin] = uiConnection;
    const WRenderPipelinePass* pSourcePass = m_Passes[uiCurrentSource].Borrow();
    ConnectionInfo& newConnection = m_Connections.ExpandAndGetRef();
    newConnection.m_uiOutputPin = ResolvePin(uiCurrentSource, pSourcePass->GetOutputPins()[uiCurrentSourcePin]);
    newConnection.m_uiInputPins = WMakeArrayPtr<WUInt16>(m_InputPinsStorage.GetData() + uiStartIndex, m_InputPinsStorage.GetCount() - uiStartIndex);

    for (WUInt16 uiInputPin : newConnection.m_uiInputPins)
    {
      const PinInfo& inputPin = m_Pins[uiInputPin];
      m_PassInfos[inputPin.m_uiPassIndex].m_uiInputConnections[inputPin.m_uiInputPinIndex] = uiConnection;
    }
  };

  for (WUInt32 uiConnectionIdx = 0; uiConnectionIdx < resolvedConnections.GetCount(); ++uiConnectionIdx)
  {
    const Connection& conn = resolvedConnections[uiConnectionIdx];
    if (uiCurrentSource != conn.m_uiSource || uiCurrentSourcePin != conn.m_uiSourcePinIndex)
    {
      // Finish old block
      if (uiCurrentSource != s_uiInvalidIndex)
      {
        FinishBlock();
      }

      // Start new block
      uiCurrentSource = conn.m_uiSource;
      uiCurrentSourcePin = conn.m_uiSourcePinIndex;
      uiStartIndex = m_InputPinsStorage.GetCount();
    }
    const WRenderPipelinePass* pTargetPass = m_Passes[conn.m_uiTarget].Borrow();
    m_InputPinsStorage.PushBack(ResolvePin(conn.m_uiTarget, pTargetPass->GetInputPins()[conn.m_uiTargetPinIndex]));
  }

  // Finish last block
  if (uiCurrentSource != s_uiInvalidIndex)
  {
    FinishBlock();
  }

  m_ConnectionsRenderGraph.SetCount(m_Connections.GetCount());
  for (WUInt32 i = 0; i < m_Connections.GetCount(); ++i)
  {
    const PinInfo& outputPin = m_Pins[m_Connections[i].m_uiOutputPin];
    const auto connectivity = outputPin.m_Flags.IsSet(WRenderPipelineNodePin::Type::Buffer) ? WRenderPipelinePinConnection::Connectivity::Buffer : WRenderPipelinePinConnection::Connectivity::Texture;
    m_ConnectionsRenderGraph[i] = WRenderPipelinePinConnection(connectivity);
  }
}

void WRenderPipelinePassGraph::SortExtractors()
{
  WDynamicArray<WUniquePtr<WExtractor>> sortedExtractors;
  sortedExtractors.Reserve(m_Extractors.GetCount());

  while (!m_Extractors.IsEmpty())
  {
    bool bMadeProgress = false;
    for (WUInt32 i = 0; i < m_Extractors.GetCount(); ++i)
    {
      bool bDependenciesFound = true;
      for (const WHashedString& sDependency : m_Extractors[i]->m_DependsOn)
      {
        bool bFound = false;
        for (const WUniquePtr<WExtractor>& pExtractor : sortedExtractors)
        {
          if (sDependency == WTempHashedString(pExtractor->GetDynamicRTTI()->GetTypeNameHash()))
          {
            bFound = true;
            break;
          }
        }
        if (!bFound)
        {
          bDependenciesFound = false;
          break;
        }
      }

      if (bDependenciesFound)
      {
        sortedExtractors.PushBack(std::move(m_Extractors[i]));
        m_Extractors.RemoveAtAndCopy(i);
        bMadeProgress = true;
        break;
      }
    }

    if (!bMadeProgress)
    {
      WLog::Error("GPU pipeline contains missing or cyclic extractor dependencies.");
      break;
    }
  }

  for (WUniquePtr<WExtractor>& pExtractor : m_Extractors)
  {
    sortedExtractors.PushBack(std::move(pExtractor));
  }
  m_Extractors = std::move(sortedExtractors);
}

WRenderPipelinePass* WRenderPipelinePassGraph::GetPassByName(WStringView sName) const
{
  for (const WUniquePtr<WRenderPipelinePass>& pPass : m_Passes)
  {
    if (sName.IsEqual(pPass->GetName()))
      return pPass.Borrow();
  }
  return nullptr;
}

WExtractor* WRenderPipelinePassGraph::GetExtractorByName(WStringView sName) const
{
  for (const WUniquePtr<WExtractor>& pExtractor : m_Extractors)
  {
    if (sName.IsEqual(pExtractor->GetName()))
      return pExtractor.Borrow();
  }
  return nullptr;
}

bool WRenderPipelinePassGraph::SetSwitchValue(WUInt32 uiSwitchIndex, WInt32 iValue)
{
  W_ASSERT_DEV(uiSwitchIndex < m_Switches.GetCount(), "Invalid GPU pipeline switch index");
  return m_Switches[uiSwitchIndex].m_pSwitch->SetSwitchValue(iValue);
}

bool WRenderPipelinePassGraph::SetSwitchToDefault(WUInt32 uiSwitchIndex)
{
  W_ASSERT_DEV(uiSwitchIndex < m_Switches.GetCount(), "Invalid GPU pipeline switch index");
  WSwitchBasePass* pSwitch = m_Switches[uiSwitchIndex].m_pSwitch;
  if (pSwitch->m_Values.IsEmpty())
    return false;

  return pSwitch->SetSwitchValue(pSwitch->m_Values[0]);
}

WResult WRenderPipelinePassGraph::CullDeadPasses()
{
  m_AlivePasses.Clear();
  m_AlivePasses.SetCount(m_Passes.GetCount(), false);
  m_AliveConnections.Clear();
  m_AliveConnections.SetCount(m_Connections.GetCount(), false);

  WTempArray<WUInt16> stack;

  // Passes without outputs are the observable roots of the pipeline.
  for (WUInt16 i = 0; i < m_PassInfos.GetCount(); ++i)
  {
    if (m_PassInfos[i].m_uiOutputConnections.IsEmpty())
    {
      m_AlivePasses.SetBit(i);
      stack.PushBack(i);
    }
  }

  // Walk backwards through all required input connections. Switch passes only keep the input
  // selected by m_uiSelectedValueIndex, so their other branches are never reached.
  while (!stack.IsEmpty())
  {
    const WUInt16 uiPass = stack.PeekBack();
    stack.PopBack();

    const PassInfo& pass = m_PassInfos[uiPass];
    WUInt32 uiFirstInput = 0;
    WUInt32 uiInputCount = pass.m_uiInputConnections.GetCount();

    if (const WSwitchBasePass* pSwitch = WDynamicCast<const WSwitchBasePass*>(m_Passes[uiPass].Borrow()))
    {
      if (pSwitch->m_uiSelectedValueIndex >= pSwitch->m_Values.GetCount() || pSwitch->m_uiSelectedValueIndex >= pass.m_uiInputConnections.GetCount())
      {
        WLog::Error("Switch '{}' has an invalid selected value index {}.", pSwitch->GetName(), pSwitch->m_uiSelectedValueIndex);
        return W_FAILURE;
      }

      uiFirstInput = pSwitch->m_uiSelectedValueIndex;
      uiInputCount = 1;
    }

    for (WUInt32 i = 0; i < uiInputCount; ++i)
    {
      const WUInt16 uiConnection = pass.m_uiInputConnections[uiFirstInput + i];
      if (uiConnection == s_uiInvalidIndex)
        continue;

      m_AliveConnections.SetBit(uiConnection);
      const WUInt16 uiSourcePass = m_Pins[m_Connections[uiConnection].m_uiOutputPin].m_uiPassIndex;
      if (!m_AlivePasses.IsBitSet(uiSourcePass))
      {
        m_AlivePasses.SetBit(uiSourcePass);
        stack.PushBack(uiSourcePass);
      }
    }
  }

  return W_SUCCESS;
}

WResult WRenderPipelinePassGraph::SortPasses()
{
  using TempBitfield = WBitfield<WTempArray<WUInt32>>;

  m_SortedPasses.Clear();
  WTempArray<WUInt32> totalDependencies;
  WTempArray<WUInt32> fulfilledDependencies;
  totalDependencies.SetCount(m_Passes.GetCount(), 0);
  fulfilledDependencies.SetCount(m_Passes.GetCount(), 0);

  WUInt32 uiAlivePassCount = 0;
  for (WUInt32 uiPass = 0; uiPass < m_Passes.GetCount(); ++uiPass)
  {
    if (m_AlivePasses.IsBitSet(uiPass))
      ++uiAlivePassCount;
  }
  m_SortedPasses.Reserve(uiAlivePassCount);

  // Count source dependencies and the additional sibling-consumer dependencies required by
  // pass-through inputs.
  for (WUInt32 uiConnection = 0; uiConnection < m_Connections.GetCount(); ++uiConnection)
  {
    if (!m_AliveConnections.IsBitSet(uiConnection))
      continue;

    const ConnectionInfo& connection = m_Connections[uiConnection];
    for (WUInt16 uiInputPin : connection.m_uiInputPins)
    {
      const WUInt16 uiTargetPass = m_Pins[uiInputPin].m_uiPassIndex;
      if (!m_AlivePasses.IsBitSet(uiTargetPass))
        continue;

      ++totalDependencies[uiTargetPass];

      if (m_Pins[uiInputPin].m_Flags.IsSet(WRenderPipelineNodePin::Type::PassThrough))
      {
        // A pass-through pass may modify the resource in place. Make it depend on every other alive consumer of the same connection so all readers of the original resource execute first. This trick cheaply allows to model this dependency without the need to iterate the connections every time to check for completion.
        for (WUInt16 uiConsumerPin : connection.m_uiInputPins)
        {
          const WUInt16 uiConsumerPass = m_Pins[uiConsumerPin].m_uiPassIndex;
          if (uiConsumerPass != uiTargetPass && m_AlivePasses.IsBitSet(uiConsumerPass))
            ++totalDependencies[uiTargetPass];
        }
      }
    }
  }

  TempBitfield done;
  done.SetCount(m_Passes.GetCount(), false);
  WTempArray<WUInt16> usable;
  usable.Reserve(m_Passes.GetCount());
  for (WUInt32 uiPass = 0; uiPass < m_Passes.GetCount(); ++uiPass)
  {
    if (m_AlivePasses.IsBitSet(uiPass) && totalDependencies[uiPass] == 0)
      usable.PushBack(uiPass);
  }

  auto DependencyFulfilled = [&](WUInt16 uiPass)
  {
    W_ASSERT_DEBUG(fulfilledDependencies[uiPass] < totalDependencies[uiPass], "GPU pipeline dependency counted more than once");
    ++fulfilledDependencies[uiPass];
    if (fulfilledDependencies[uiPass] == totalDependencies[uiPass])
      usable.PushBack(uiPass);
  };

  while (!usable.IsEmpty())
  {
    const WUInt16 uiPass = usable.PeekBack();
    usable.PopBack();

    W_ASSERT_DEBUG(!done.IsBitSet(uiPass), "GPU pipeline pass was queued more than once");
    done.SetBit(uiPass);
    m_SortedPasses.PushBack(uiPass);

    // Completing the source pass fulfills the regular dependency of every alive consumer.
    for (WUInt16 uiConnection : m_PassInfos[uiPass].m_uiOutputConnections)
    {
      if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
        continue;

      for (WUInt16 uiInputPin : m_Connections[uiConnection].m_uiInputPins)
      {
        const WUInt16 uiTargetPass = m_Pins[uiInputPin].m_uiPassIndex;
        if (m_AlivePasses.IsBitSet(uiTargetPass))
          DependencyFulfilled(uiTargetPass);
      }
    }

    // Completing a consumer fulfills the additional ordering dependency of pass-through consumers that share the same input connection. Once all normal consumers have been fulfilled, the passthrough pin will have all its dependencies fulfilled as well and can be run. This works because there can only ever be one passthrough pin in a single connection.
    for (WUInt16 uiConnection : m_PassInfos[uiPass].m_uiInputConnections)
    {
      if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
        continue;

      for (WUInt16 uiInputPin : m_Connections[uiConnection].m_uiInputPins)
      {
        const WUInt16 uiTargetPass = m_Pins[uiInputPin].m_uiPassIndex;
        if (uiTargetPass != uiPass && m_AlivePasses.IsBitSet(uiTargetPass) && m_Pins[uiInputPin].m_Flags.IsSet(WRenderPipelineNodePin::Type::PassThrough))
          DependencyFulfilled(uiTargetPass);
      }
    }
  }

  if (m_SortedPasses.GetCount() != uiAlivePassCount)
  {
    WLog::Error("GPU pipeline contains a cycle or unresolved alive dependency.");
    for (WUInt16 i = 0; i < m_Passes.GetCount(); ++i)
    {
      if (m_AlivePasses.IsBitSet(i) && !done.IsBitSet(i))
        WLog::Error("Failed to sort pass '{}' of type '{}'.", m_Passes[i]->GetName(), m_Passes[i]->GetDynamicRTTI()->GetTypeName());
    }
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WStatus WRenderPipelinePassGraph::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph)
{
  WHybridArray<WRenderPipelinePinConnection, 10> inputs(WFrameAllocator::GetCurrentAllocator());
  WHybridArray<WRenderPipelinePinConnection, 10> outputs(WFrameAllocator::GetCurrentAllocator());

  for (WUInt16 uiPass : m_SortedPasses)
  {
    WRenderPipelinePass* pPass = m_Passes[uiPass].Borrow();
    const PassInfo& passInfo = m_PassInfos[uiPass];

    if (camera.IsStereoscopic() && !pPass->IsStereoAware())
    {
      WLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", viewData.m_sName, pPass->GetName());
    }

    inputs.SetCount(passInfo.m_uiInputConnections.GetCount());
    for (WUInt32 i = 0; i < inputs.GetCount(); ++i)
    {
      const WUInt16 uiConnection = passInfo.m_uiInputConnections[i];
      inputs[i] = uiConnection != s_uiInvalidIndex && m_AliveConnections.IsBitSet(uiConnection) ? m_ConnectionsRenderGraph[uiConnection] : WRenderPipelinePinConnection();
    }

    outputs.SetCount(passInfo.m_uiOutputConnections.GetCount());
    for (WUInt32 i = 0; i < outputs.GetCount(); ++i)
    {
      const WUInt16 uiConnection = passInfo.m_uiOutputConnections[i];
      if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
      {
        outputs[i] = WRenderPipelinePinConnection();
        continue;
      }

      outputs[i] = m_ConnectionsRenderGraph[uiConnection];
      const WRenderPipelineNodePin* pPin = pPass->GetOutputPins()[i];
      if (pPin->m_Type.IsSet(WRenderPipelineNodePin::Type::PassThrough))
      {
        outputs[i] = inputs[pPin->m_uiInputIndex];
      }
    }

    ref_graph.PushMarker(pPass->GetName());
    const WStatus result = pPass->m_bActive ? pPass->AddRenderPasses(viewData, camera, ref_graph, inputs, outputs) : pPass->AddRenderPassesInactive(viewData, camera, ref_graph, inputs, outputs);
    ref_graph.PopMarker();
    if (result.Failed())
      return result;

    for (WUInt32 i = 0; i < passInfo.m_uiOutputConnections.GetCount(); ++i)
    {
      const WUInt16 uiConnection = passInfo.m_uiOutputConnections[i];
      if (uiConnection != s_uiInvalidIndex && m_AliveConnections.IsBitSet(uiConnection))
      {
        m_ConnectionsRenderGraph[uiConnection] = outputs[i];
      }
    }
  }

  return W_SUCCESS;
}

WStatus WRenderPipelinePassGraph::UpdateTextureProviders(WRenderGraph& ref_graph)
{
  WHashTable<WRenderGraphTextureHandle, WUInt16> updates(WFrameAllocator::GetCurrentAllocator());
  updates.Reserve(m_TextureProviderPins.GetCount());

  for (WUInt16 uiPin : m_TextureProviderPins)
  {
    const PinInfo& pin = m_Pins[uiPin];
    const PassInfo& pass = m_PassInfos[pin.m_uiPassIndex];
    const WUInt16 uiConnection = pin.m_uiInputPinIndex != 0xFF ? pass.m_uiInputConnections[pin.m_uiInputPinIndex] : pass.m_uiOutputConnections[pin.m_uiOutputPinIndex];
    if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
      continue;

    const WRenderGraphTextureHandle hTexture = m_ConnectionsRenderGraph[uiConnection].m_TextureHandle;
    if (updates.Contains(hTexture))
      return WStatus("Two texture provider pins reference the same texture.");
    updates.Insert(hTexture, uiPin);
  }

  for (auto it : updates)
  {
    const PinInfo& pin = m_Pins[it.Value()];
    WRenderPipelinePass* pPass = m_Passes[pin.m_uiPassIndex].Borrow();
    const WRenderPipelineNodePin* pNodePin = pin.m_uiInputPinIndex != 0xFF ? pPass->GetInputPins()[pin.m_uiInputPinIndex] : pPass->GetOutputPins()[pin.m_uiOutputPinIndex];
    const WGALTextureCreationDescription& desc = ref_graph.GetTextureDesc(it.Key());
    const WGALTextureHandle hTexture = pPass->QueryTextureProvider(pNodePin, desc);
    if (!hTexture.IsInvalidated())
    {
      W_SUCCEED_OR_RETURN(ref_graph.ReplaceImportedTexture(it.Key(), hTexture));
    }
  }

  return W_SUCCESS;
}
