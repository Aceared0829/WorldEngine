#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

/// Version of the file that wraps the serialized pipeline. Has to match the version that WRenderPipelineResource expects.
static constexpr WUInt8 s_uiBinRenderPipelineVersion = 2;

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderPipelineContext, 1, WRTTIDefaultAllocator<WRenderPipelineContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "RenderPipeline"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRenderPipelineContext::WRenderPipelineContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

void WRenderPipelineContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  WEngineProcessDocumentContext::HandleMessage(pMsg);
}

void WRenderPipelineContext::OnInitialize()
{
}

WEngineProcessViewContext* WRenderPipelineContext::CreateViewContext()
{
  W_ASSERT_DEV(false, "Should not be called");
  return nullptr;
}

void WRenderPipelineContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_ASSERT_DEV(false, "Should not be called");
}

/// Loads a binary render pipeline from an asset GUID string (resolving via the asset redirection table).
static WStatus ImportSubPipeline(WStringView sGuidOrPath, WDynamicArray<WUniquePtr<WRenderPipelinePass>>& out_passes, WDynamicArray<WUniquePtr<WExtractor>>& out_extractors, WDynamicArray<WRenderPipelineResourceLoaderConnection>& out_connections)
{
  WFileReader file;
  if (file.Open(sGuidOrPath).Failed())
    return WStatus(WFmt("Failed to open render pipeline '{}'.", sGuidOrPath));

  WAssetFileHeader header;
  if (header.Read(file).Failed())
    return WStatus(WFmt("Render pipeline '{}' has an invalid asset header.", sGuidOrPath));

  WUInt8 uiVersion = 0;
  file >> uiVersion;
  if (uiVersion != s_uiBinRenderPipelineVersion)
    return WStatus(WFmt("Render pipeline '{}' has version {}, expected {}.", sGuidOrPath, uiVersion, s_uiBinRenderPipelineVersion));

  WUInt32 uiSize = 0;
  file >> uiSize;
  W_IGNORE_UNUSED(uiSize);

  // The pipeline data follows directly; deserialize it from the file stream.
  return WRenderPipelineResourceLoader::ImportPipeline(file, out_passes, out_extractors, out_connections);
}

WStatus WRenderPipelineContext::ExportDocument(const WExportDocumentMsgToEngine* pMsg)
{
  WDynamicArray<WRenderPipelineNode*> nodes;
  WDynamicArray<WUuid> nodeUuids;
  WDynamicArray<WExtractor*> extractors;
  WDynamicArray<WDocumentObject_ConnectionBase*> toolConnections;
  m_Context.GetObjectsByType(nodes, &nodeUuids);
  m_Context.GetObjectsByType(extractors);
  m_Context.GetObjectsByType(toolConnections);

  // Build the UUID-to-index map used by serialized connections.
  WHashTable<WUuid, WUInt32> uuidToIndex;
  for (WUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    uuidToIndex.Insert(nodeUuids[i], i);
    if (WDynamicCast<WRenderPipelinePass*>(nodes[i]) == nullptr && WDynamicCast<WSubGraphNode*>(nodes[i]) == nullptr)
      return WStatus(WFmt("Unsupported GPU pipeline node type '{}'.", nodes[i]->GetDynamicRTTI()->GetTypeName()));
  }

  // Validate that the root graph contains at most one extractor of each type. Imported extractors
  // with matching types are ignored later so root extractors take precedence.
  WSet<const WRTTI*> extractorTypes;
  for (const WExtractor* pExtractor : extractors)
  {
    const WRTTI* pType = pExtractor->GetDynamicRTTI();
    if (extractorTypes.Contains(pType))
      return WStatus(WFmt("The pipeline contains more than one extractor of type '{}'.", pType->GetTypeName()));
    extractorTypes.Insert(pType);
  }

  // Convert tool connection UUIDs to indices in the temporary compiled node list.
  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  connections.Reserve(toolConnections.GetCount());
  for (const WDocumentObject_ConnectionBase* pConn : toolConnections)
  {
    auto& conn = connections.ExpandAndGetRef();
    if (!uuidToIndex.TryGetValue(pConn->m_Source, conn.m_uiSource))
      return WStatus(WFmt("Connection source UUID '{}' was not found in the pipeline node list.", pConn->m_Source));
    if (!uuidToIndex.TryGetValue(pConn->m_Target, conn.m_uiTarget))
      return WStatus(WFmt("Connection target UUID '{}' was not found in the pipeline node list.", pConn->m_Target));
    conn.m_sSourcePin = pConn->m_SourcePin;
    conn.m_sTargetPin = pConn->m_TargetPin;
  }

  // Inline all SubGraph placeholders by importing their already transformed render pipeline data.
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
  WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
  W_SUCCEED_OR_RETURN(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ImportSubPipeline));

  WDynamicArray<WRenderPipelinePass*> passes;
  passes.Reserve(nodes.GetCount());
  for (WRenderPipelineNode* pNode : nodes)
  {
    WRenderPipelinePass* pPass = WDynamicCast<WRenderPipelinePass*>(pNode);
    if (pPass == nullptr)
      return WStatus("Failed to inline all GPU pipeline sub-graphs.");
    passes.PushBack(pPass);
  }

  WDefaultMemoryStreamStorage storage;
  {
    // Export Resource Data
    WMemoryStreamWriter writer(&storage);
    W_SUCCEED_OR_RETURN(WRenderPipelineResourceLoader::ExportPipeline(passes.GetArrayPtr(), extractors.GetArrayPtr(), connections.GetArrayPtr(), writer));
  }

  WDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  {
    // File Header
    WAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(file).AssertSuccess();

    file << s_uiBinRenderPipelineVersion;
  }

  {
    // Resource Data
    WUInt32 uiSize = storage.GetStorageSize32();
    file << uiSize;
    if (storage.CopyToStream(file).Failed())
      return WStatus(WFmt("Failed to copy pipeline data to '{}'.", pMsg->m_sOutputFile));
  }

  // do the actual file writing
  if (file.Close().Failed())
    return WStatus(WFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return WStatus(W_SUCCESS);
}
