#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Status.h>
#include <RendererCore/RendererCoreDLL.h>

class WRenderPipeline;
struct WRenderPipelineResourceDescriptor;
class WStreamReader;
class WStreamWriter;
class WRenderPipelinePass;
class WRenderPipelineNode;
class WExtractor;

struct W_RENDERERCORE_DLL WRenderPipelineResourceLoaderConnection
{
  WUInt32 m_uiSource;
  WUInt32 m_uiTarget;
  WString m_sSourcePin;
  WString m_sTargetPin;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineResourceLoaderConnection);

struct W_RENDERERCORE_DLL WRenderPipelineResourceLoader
{
  /// Loads a transformed render pipeline by asset GUID or path. Used to resolve WSubGraphNode references.
  using ImportPipelineCallback = WDelegate<WStatus(WStringView, WDynamicArray<WUniquePtr<WRenderPipelinePass>>&, WDynamicArray<WUniquePtr<WExtractor>>&, WDynamicArray<WRenderPipelineResourceLoaderConnection>&)>;

  /// Reads passes, extractors and connections from the binary format written by ExportPipeline.
  static WStatus ImportPipeline(WStreamReader& ref_streamReader, WDynamicArray<WUniquePtr<WRenderPipelinePass>>& out_passes, WDynamicArray<WUniquePtr<WExtractor>>& out_extractors, WDynamicArray<WRenderPipelineResourceLoaderConnection>& out_connections);

  /// Replaces every WSubGraphNode node with the contents of the pipeline it references.
  ///
  /// Connections to the sub-graph's pins are rerouted to the passes behind the corresponding boundary nodes, and connections to unconnected boundaries are dropped. Extractors that the root graph already has are not imported again. The imported objects are kept alive through ref_ownedPasses / ref_ownedExtractors, so those have to outlive ref_nodes and ref_extractors.
  static WStatus InlineImportedSubGraphs(WDynamicArray<WRenderPipelineNode*>& ref_nodes, WDynamicArray<WUniquePtr<WRenderPipelinePass>>& ref_ownedPasses, WDynamicArray<WExtractor*>& ref_extractors, WDynamicArray<WUniquePtr<WExtractor>>& ref_ownedExtractors, WDynamicArray<WRenderPipelineResourceLoaderConnection>& ref_connections, const ImportPipelineCallback& importPipeline);

  static WInternal::NewInstance<WRenderPipeline> CreateRenderPipeline(const WRenderPipelineResourceDescriptor& desc);
  static WResult ExportPipeline(WArrayPtr<const WRenderPipelinePass* const> passes, WArrayPtr<const WExtractor* const> extractors, WArrayPtr<const WRenderPipelineResourceLoaderConnection> connections, WStreamWriter& ref_streamWriter);
};
