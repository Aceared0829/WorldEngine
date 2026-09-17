#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/SerializationContext.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/SubGraphNode.h>

namespace
{
  bool IsInputBoundaryNode(const WRTTI* pType)
  {
    return pType == WGetStaticRTTI<WSubGraphTextureInputNode>() || pType == WGetStaticRTTI<WSubGraphBufferInputNode>();
  }

  bool IsOutputBoundaryNode(const WRTTI* pType)
  {
    return pType == WGetStaticRTTI<WSubGraphTextureOutputNode>() || pType == WGetStaticRTTI<WSubGraphBufferOutputNode>();
  }

  bool IsBoundaryNode(const WRTTI* pType)
  {
    return IsInputBoundaryNode(pType) || IsOutputBoundaryNode(pType);
  }
} // namespace

////////////////////////////////////////////////////////////////////////
// WVisualGraphObjectManager Internal
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineResourceLoaderConnection, WNoBase, 1, WRTTIDefaultAllocator<WRenderPipelineResourceLoaderConnection>)
{
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WRenderPipelineResourceLoaderConnection::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_uiSource;
  inout_stream << m_uiTarget;
  inout_stream << m_sSourcePin;
  inout_stream << m_sTargetPin;

  return W_SUCCESS;
}

WResult WRenderPipelineResourceLoaderConnection::Deserialize(WStreamReader& inout_stream)
{
  W_VERIFY(WTypeVersionReadContext::GetContext()->GetTypeVersion(WGetStaticRTTI<WRenderPipelineResourceLoaderConnection>()) == 1, "Unknown version");

  inout_stream >> m_uiSource;
  inout_stream >> m_uiTarget;
  inout_stream >> m_sSourcePin;
  inout_stream >> m_sTargetPin;

  return W_SUCCESS;
}

constexpr WTypeVersion s_RenderPipelineDescriptorVersion = 1;

// static
WStatus WRenderPipelineResourceLoader::ImportPipeline(WStreamReader& ref_streamReader, WDynamicArray<WUniquePtr<WRenderPipelinePass>>& out_passes, WDynamicArray<WUniquePtr<WExtractor>>& out_extractors, WDynamicArray<WRenderPipelineResourceLoaderConnection>& out_connections)
{
  out_passes.Clear();
  out_extractors.Clear();
  out_connections.Clear();

  const auto uiVersion = ref_streamReader.ReadVersion(s_RenderPipelineDescriptorVersion);
  W_IGNORE_UNUSED(uiVersion);

  WStringDeduplicationReadContext stringDeduplicationReadContext(ref_streamReader);
  WTypeVersionReadContext typeVersionReadContext(ref_streamReader);

  WStringBuilder sTypeName;

  // Passes
  {
    WUInt32 uiNumPasses = 0;
    ref_streamReader >> uiNumPasses;
    out_passes.Reserve(uiNumPasses);

    for (WUInt32 i = 0; i < uiNumPasses; ++i)
    {
      ref_streamReader >> sTypeName;
      const WRTTI* pType = WRTTI::FindTypeByName(sTypeName);
      if (pType == nullptr)
        return WStatus(WFmt("Render pipeline pass type '{}' is unknown.", sTypeName));
      if (!pType->IsDerivedFrom<WRenderPipelinePass>())
        return WStatus(WFmt("Render pipeline pass type '{}' is not derived from WRenderPipelinePass.", sTypeName));
      if (pType->GetAllocator() == nullptr || !pType->GetAllocator()->CanAllocate())
        return WStatus(WFmt("Render pipeline pass type '{}' cannot be allocated.", sTypeName));

      WUniquePtr<WRenderPipelinePass> pPass = pType->GetAllocator()->Allocate<WRenderPipelinePass>();
      if (pPass->Deserialize(ref_streamReader).Failed())
        return WStatus(WFmt("Failed to deserialize render pipeline pass of type '{}'.", sTypeName));

      out_passes.PushBack(std::move(pPass));
    }
  }

  // Extractors
  {
    WUInt32 uiNumExtractors = 0;
    ref_streamReader >> uiNumExtractors;
    out_extractors.Reserve(uiNumExtractors);

    for (WUInt32 i = 0; i < uiNumExtractors; ++i)
    {
      ref_streamReader >> sTypeName;
      const WRTTI* pType = WRTTI::FindTypeByName(sTypeName);
      if (pType == nullptr)
        return WStatus(WFmt("Render pipeline extractor type '{}' is unknown.", sTypeName));
      if (!pType->IsDerivedFrom<WExtractor>())
        return WStatus(WFmt("Render pipeline extractor type '{}' is not derived from WExtractor.", sTypeName));
      if (pType->GetAllocator() == nullptr || !pType->GetAllocator()->CanAllocate())
        return WStatus(WFmt("Render pipeline extractor type '{}' cannot be allocated.", sTypeName));

      WUniquePtr<WExtractor> pExtractor = pType->GetAllocator()->Allocate<WExtractor>();
      if (pExtractor->Deserialize(ref_streamReader).Failed())
        return WStatus(WFmt("Failed to deserialize render pipeline extractor of type '{}'.", sTypeName));

      out_extractors.PushBack(std::move(pExtractor));
    }
  }

  // Connections
  {
    WUInt32 uiNumConnections = 0;
    ref_streamReader >> uiNumConnections;
    out_connections.SetCount(uiNumConnections);

    for (WUInt32 i = 0; i < uiNumConnections; ++i)
    {
      if (out_connections[i].Deserialize(ref_streamReader).Failed())
        return WStatus(WFmt("Failed to deserialize render pipeline connection {}.", i));

      if (out_connections[i].m_uiSource >= out_passes.GetCount() || out_connections[i].m_uiTarget >= out_passes.GetCount())
        return WStatus(WFmt("Render pipeline connection {} references a pass index outside of the {} passes in the pipeline.", i, out_passes.GetCount()));
    }
  }

  return WStatus(W_SUCCESS);
}

WStatus WRenderPipelineResourceLoader::InlineImportedSubGraphs(WDynamicArray<WRenderPipelineNode*>& ref_nodes, WDynamicArray<WUniquePtr<WRenderPipelinePass>>& ref_ownedPasses, WDynamicArray<WExtractor*>& ref_extractors, WDynamicArray<WUniquePtr<WExtractor>>& ref_ownedExtractors, WDynamicArray<WRenderPipelineResourceLoaderConnection>& ref_connections, const ImportPipelineCallback& importPipeline)
{
  WSet<const WRTTI*> extractorTypes;
  for (const WExtractor* pExtractor : ref_extractors)
  {
    extractorTypes.Insert(pExtractor->GetDynamicRTTI());
  }

  for (WUInt32 iSub = 0; iSub < ref_nodes.GetCount(); ++iSub)
  {
    const WSubGraphNode* pSubGraph = WDynamicCast<const WSubGraphNode*>(ref_nodes[iSub]);
    if (pSubGraph == nullptr)
      continue;

    WDynamicArray<WUniquePtr<WRenderPipelinePass>> importedPasses;
    WDynamicArray<WUniquePtr<WExtractor>> importedExtractors;
    WDynamicArray<WRenderPipelineResourceLoaderConnection> importedConnections;

    // Sub-pipeline binaries are already fully inlined because dependencies are transformed first.
    // Therefore, no imported pipeline can contain another Subgraph node at this point.
    WStatus res = importPipeline(pSubGraph->m_sPipeline, importedPasses, importedExtractors, importedConnections);
    if (res.Failed())
      return WStatus(WFmt("Failed to import sub-graph pipeline '{}': {}", pSubGraph->m_sPipeline, res.GetMessageString()));

    for (const WRenderPipelineResourceLoaderConnection& connection : importedConnections)
    {
      if (connection.m_uiSource >= importedPasses.GetCount() || connection.m_uiTarget >= importedPasses.GetCount())
        return WStatus(WFmt("Sub-graph pipeline '{}' contains a connection with an invalid node index.", pSubGraph->m_sPipeline));
    }

    // Map sub-graph pass indices to parent pass indices. Boundary nodes are eliminated during
    // inlining and retain WInvalidIndex in this mapping.
    WDynamicArray<WUInt32> subToParentIndex;
    subToParentIndex.SetCount(importedPasses.GetCount(), WInvalidIndex);
    for (WUInt32 iSubNode = 0; iSubNode < importedPasses.GetCount(); ++iSubNode)
    {
      WRenderPipelinePass* pPass = importedPasses[iSubNode].Borrow();
      if (IsBoundaryNode(pPass->GetDynamicRTTI()))
        continue;

      subToParentIndex[iSubNode] = ref_nodes.GetCount();
      ref_nodes.PushBack(pPass);
    }

    // Add internal sub-graph connections between non-boundary passes.
    for (const WRenderPipelineResourceLoaderConnection& subConn : importedConnections)
    {
      const bool bSourceIsBoundary = IsBoundaryNode(importedPasses[subConn.m_uiSource]->GetDynamicRTTI());
      const bool bTargetIsBoundary = IsBoundaryNode(importedPasses[subConn.m_uiTarget]->GetDynamicRTTI());
      if (bSourceIsBoundary || bTargetIsBoundary)
        continue;

      WRenderPipelineResourceLoaderConnection& newConn = ref_connections.ExpandAndGetRef();
      newConn = subConn;
      newConn.m_uiSource = subToParentIndex[subConn.m_uiSource];
      newConn.m_uiTarget = subToParentIndex[subConn.m_uiTarget];
    }

    struct BoundaryPassthrough
    {
      WString m_sOutput;
      WUInt32 m_uiSource = WInvalidIndex;
      WString m_sSourcePin;
    };

    // Find connections that connect an input directly to an output node (i.e. direct passthrough)
    WDynamicArray<BoundaryPassthrough> boundaryPassthroughs;
    for (const WRenderPipelineResourceLoaderConnection& subConn : importedConnections)
    {
      WRenderPipelinePass* pInputBoundary = importedPasses[subConn.m_uiSource].Borrow();
      WRenderPipelinePass* pOutputBoundary = importedPasses[subConn.m_uiTarget].Borrow();
      if (!IsInputBoundaryNode(pInputBoundary->GetDynamicRTTI()) || !IsOutputBoundaryNode(pOutputBoundary->GetDynamicRTTI()))
        continue;

      BoundaryPassthrough& forward = boundaryPassthroughs.ExpandAndGetRef();
      forward.m_sOutput = pOutputBoundary->GetName();

      for (const WRenderPipelineResourceLoaderConnection& parentConn : ref_connections)
      {
        if (parentConn.m_uiTarget == iSub && parentConn.m_sTargetPin == pInputBoundary->GetName())
        {
          forward.m_uiSource = parentConn.m_uiSource;
          forward.m_sSourcePin = parentConn.m_sSourcePin;
          break;
        }
      }
    }

    // Remap connections to SubGraph input pins to every internal consumer of the matching input
    // boundary. Iterate only over existing parent connections because fan-out adds new entries.
    const WUInt32 uiConnCountBeforeInputRemap = ref_connections.GetCount();
    for (WUInt32 iConn = 0; iConn < uiConnCountBeforeInputRemap; ++iConn)
    {
      if (ref_connections[iConn].m_uiTarget != iSub)
        continue;

      const WString sPinName = ref_connections[iConn].m_sTargetPin;
      bool bPinFound = false;
      bool bRemapped = false;
      for (WUInt32 iSubNode = 0; iSubNode < importedPasses.GetCount(); ++iSubNode)
      {
        WRenderPipelinePass* pBoundary = importedPasses[iSubNode].Borrow();
        if (!IsInputBoundaryNode(pBoundary->GetDynamicRTTI()) || pBoundary->GetName() != sPinName)
          continue;

        bPinFound = true;
        for (const WRenderPipelineResourceLoaderConnection& subConn : importedConnections)
        {
          if (subConn.m_uiSource != iSubNode || subConn.m_sSourcePin != "Value")
            continue;
          if (subToParentIndex[subConn.m_uiTarget] == WInvalidIndex)
            continue;

          if (!bRemapped)
          {
            ref_connections[iConn].m_uiTarget = subToParentIndex[subConn.m_uiTarget];
            ref_connections[iConn].m_sTargetPin = subConn.m_sTargetPin;
            bRemapped = true;
          }
          else
          {
            WRenderPipelineResourceLoaderConnection extra = ref_connections[iConn];
            extra.m_uiTarget = subToParentIndex[subConn.m_uiTarget];
            extra.m_sTargetPin = subConn.m_sTargetPin;
            ref_connections.PushBack(std::move(extra));
          }
        }
        break;
      }

      if (!bPinFound)
        return WStatus(WFmt("Sub-graph '{}' no longer has an input pin named '{}'.", pSubGraph->m_sPipeline, sPinName));
      if (!bRemapped)
        ref_connections[iConn].m_uiSource = WInvalidIndex;
    }

    // Remap connections from SubGraph output pins to the internal producer of the matching output
    // boundary. Direct input-to-output connections use the captured parent input endpoint.
    for (WRenderPipelineResourceLoaderConnection& parentConn : ref_connections)
    {
      if (parentConn.m_uiSource != iSub)
        continue;

      const WString sPinName = parentConn.m_sSourcePin;
      bool bPinFound = false;
      bool bRemapped = false;
      for (WUInt32 iSubNode = 0; iSubNode < importedPasses.GetCount(); ++iSubNode)
      {
        WRenderPipelinePass* pBoundary = importedPasses[iSubNode].Borrow();
        if (!IsOutputBoundaryNode(pBoundary->GetDynamicRTTI()) || pBoundary->GetName() != sPinName)
          continue;

        bPinFound = true;
        for (const WRenderPipelineResourceLoaderConnection& subConn : importedConnections)
        {
          if (subConn.m_uiTarget != iSubNode || subConn.m_sTargetPin != "Value")
            continue;
          if (subToParentIndex[subConn.m_uiSource] == WInvalidIndex)
          {
            for (const BoundaryPassthrough& forward : boundaryPassthroughs)
            {
              if (forward.m_sOutput == sPinName)
              {
                parentConn.m_uiSource = forward.m_uiSource;
                parentConn.m_sSourcePin = forward.m_sSourcePin;
                bRemapped = forward.m_uiSource != WInvalidIndex;
                break;
              }
            }
            break;
          }

          parentConn.m_uiSource = subToParentIndex[subConn.m_uiSource];
          parentConn.m_sSourcePin = subConn.m_sSourcePin;
          bRemapped = true;
          break;
        }
        break;
      }

      if (!bPinFound)
        return WStatus(WFmt("Sub-graph '{}' no longer has an output pin named '{}'.", pSubGraph->m_sPipeline, sPinName));
      if (!bRemapped)
        parentConn.m_uiSource = WInvalidIndex;
    }

    // Parent extractors override imported extractors of the same type.
    for (WUniquePtr<WExtractor>& pExtractor : importedExtractors)
    {
      if (extractorTypes.Contains(pExtractor->GetDynamicRTTI()))
        continue;

      extractorTypes.Insert(pExtractor->GetDynamicRTTI());
      ref_extractors.PushBack(pExtractor.Borrow());
      ref_ownedExtractors.PushBack(std::move(pExtractor));
    }

    // Keep imported passes alive until the flattened pipeline has been serialized.
    for (WUniquePtr<WRenderPipelinePass>& pPass : importedPasses)
    {
      ref_ownedPasses.PushBack(std::move(pPass));
    }

    // Remove connections to unconnected boundaries.
    for (WUInt32 iConn = ref_connections.GetCount(); iConn-- > 0;)
    {
      if (ref_connections[iConn].m_uiSource == WInvalidIndex || ref_connections[iConn].m_uiTarget == WInvalidIndex)
        ref_connections.RemoveAtAndCopy(iConn);
    }

    // Remove the SubGraph placeholder and update indices shifted by RemoveAtAndCopy.
    ref_nodes.RemoveAtAndCopy(iSub);
    for (WRenderPipelineResourceLoaderConnection& conn : ref_connections)
    {
      if (conn.m_uiSource > iSub)
        --conn.m_uiSource;
      if (conn.m_uiTarget > iSub)
        --conn.m_uiTarget;
    }

    --iSub;
  }

  return WStatus(W_SUCCESS);
}

// static
WInternal::NewInstance<WRenderPipeline> WRenderPipelineResourceLoader::CreateRenderPipeline(const WRenderPipelineResourceDescriptor& desc)
{
  WRawMemoryStreamReader stream(desc.m_SerializedPipeline);
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  WDynamicArray<WUniquePtr<WExtractor>> extractors;
  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  const WStatus res = ImportPipeline(stream, passes, extractors, connections);
  if (res.Failed())
  {
    WLog::Error("Failed to import render pipeline '{}': {}", desc.m_sPath, res.GetMessageString());
    return nullptr;
  }

  return W_DEFAULT_NEW(WRenderPipeline, std::move(passes), std::move(extractors), connections.GetArrayPtr());
}

WResult WRenderPipelineResourceLoader::ExportPipeline(WArrayPtr<const WRenderPipelinePass* const> passes, WArrayPtr<const WExtractor* const> extractors, WArrayPtr<const WRenderPipelineResourceLoaderConnection> connections, WStreamWriter& ref_streamWriter)
{
  ref_streamWriter.WriteVersion(s_RenderPipelineDescriptorVersion);

  WStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_streamWriter);
  WTypeVersionWriteContext typeVersionWriteContext;
  auto& stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  // passes
  {
    const WUInt32 uiNumPasses = passes.GetCount();
    stream << uiNumPasses;

    for (auto& pass : passes)
    {
      auto pPassType = pass->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pPassType);

      stream << pPassType->GetTypeName();
      W_SUCCEED_OR_RETURN(pass->Serialize(stream));
    }
  }

  // extractors
  {
    const WUInt32 uiNumExtractors = extractors.GetCount();
    stream << uiNumExtractors;

    for (auto& extractor : extractors)
    {
      auto pExtractorType = extractor->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pExtractorType);

      stream << pExtractorType->GetTypeName();
      W_SUCCEED_OR_RETURN(extractor->Serialize(stream));
    }
  }

  // Connections
  {
    const WUInt32 uiNumConnections = connections.GetCount();
    stream << uiNumConnections;

    typeVersionWriteContext.AddType(WGetStaticRTTI<WRenderPipelineResourceLoaderConnection>());

    for (auto& connection : connections)
    {
      W_SUCCEED_OR_RETURN(connection.Serialize(stream));
    }
  }

  W_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  W_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return W_SUCCESS;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
