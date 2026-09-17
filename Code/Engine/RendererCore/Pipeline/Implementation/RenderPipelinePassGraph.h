#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/Passes/SwitchPass.h>
#include <RendererCore/Pipeline/SubGraphNode.h>
#include <RendererCore/RendererCoreDLL.h>

/// The static pass graph of an WRenderPipeline.
///
/// Owns the passes and extractors of a pipeline and stores their connectivity in a flat, index-based form. The graph itself never changes after construction, only which parts of it are considered alive, which depends on the current switch values. Culling and sorting have to run again whenever a switch value changed.
class W_RENDERERCORE_DLL WRenderPipelinePassGraph
{
  W_DISALLOW_COPY_AND_ASSIGN(WRenderPipelinePassGraph);

public:
  struct SwitchInfo
  {
    WSwitchBasePass* m_pSwitch = nullptr;
    WHashedString m_sBlackboardProperty;
  };

  /// Resolves the connections against the pins of the given passes. Connections that reference an unknown pin are dropped with a warning.
  WRenderPipelinePassGraph(WDynamicArray<WUniquePtr<WRenderPipelinePass>>&& passes, WDynamicArray<WUniquePtr<WExtractor>>&& extractors, WArrayPtr<const WRenderPipelineResourceLoaderConnection> connections);

  /// Determines which passes and connections contribute to the pipeline's output, starting from the passes that have no outputs.
  ///
  /// A switch only keeps the branch that feeds its selected input alive.
  WResult CullDeadPasses();

  /// Brings the alive passes into an execution order. Requires CullDeadPasses to have run.
  ///
  /// Fails if the alive part of the graph contains a cycle. Two pass-through consumers on the same output are also a cycle, because each of them has to run after the other.
  WResult SortPasses();

  WArrayPtr<WUniquePtr<WRenderPipelinePass>> GetPasses() { return m_Passes; }
  WArrayPtr<const WUniquePtr<WRenderPipelinePass>> GetPasses() const { return m_Passes; }
  WArrayPtr<WUniquePtr<WExtractor>> GetExtractors() { return m_Extractors; }
  WArrayPtr<const WUniquePtr<WExtractor>> GetExtractors() const { return m_Extractors; }
  WRenderPipelinePass* GetPassByName(WStringView sName) const;
  WExtractor* GetExtractorByName(WStringView sName) const;
  WArrayPtr<const SwitchInfo> GetSwitches() const { return m_Switches; }

  /// \return Whether the selection changed, in which case CullDeadPasses and SortPasses have to run again.
  bool SetSwitchValue(WUInt32 uiSwitchIndex, WInt32 iValue);

  /// Selects a switch's first value. Used when no blackboard provides a value for it.
  ///
  /// \return Whether the selection changed, in which case CullDeadPasses and SortPasses have to run again.
  bool SetSwitchToDefault(WUInt32 uiSwitchIndex);

  /// Runs all alive passes in the sorted order and lets them add their work to the render graph.
  WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph);

  /// Lets passes with a texture provider pin replace the imported texture of their connection with an externally owned one.
  WStatus UpdateTextureProviders(WRenderGraph& ref_graph);

private:
  void SortExtractors();

  static constexpr WUInt16 s_uiInvalidIndex = WMath::MaxValue<WUInt16>();

  struct PinInfo
  {
    WUInt16 m_uiPassIndex = 0;
    WUInt8 m_uiInputPinIndex = 0;  ///< 0xFF if this is not an input pin.
    WUInt8 m_uiOutputPinIndex = 0; ///< 0xFF if this is not an output pin.
    WBitflags<WRenderPipelineNodePin::Type> m_Flags;
  };

  /// One output pin and every input pin that it feeds.
  struct ConnectionInfo
  {
    WUInt16 m_uiOutputPin = 0;         ///< Pin index.
    WArrayPtr<WUInt16> m_uiInputPins; ///< Pin indices, points into m_InputPinsStorage.
  };

  struct PassInfo
  {
    WArrayPtr<WUInt16> m_uiInputConnections;  ///< Connection index per input pin, points into m_InputConnectionsStorage. s_uiInvalidIndex if the pin is unconnected.
    WArrayPtr<WUInt16> m_uiOutputConnections; ///< Connection index per output pin, points into m_OutputConnectionsStorage. s_uiInvalidIndex if the pin is unconnected.
  };

  /// \name Compact immutable graph state: written only once in ctor
  ///@{

  WDynamicArray<WUniquePtr<WRenderPipelinePass>> m_Passes;
  WDynamicArray<WUniquePtr<WExtractor>> m_Extractors;

  // The array pointers in PassInfo and ConnectionInfo reference these.
  WDynamicArray<WUInt16> m_InputConnectionsStorage;
  WDynamicArray<WUInt16> m_OutputConnectionsStorage;
  WDynamicArray<WUInt16> m_InputPinsStorage;

  WDynamicArray<PassInfo> m_PassInfos;           // pass idx
  WDynamicArray<PinInfo> m_Pins;                 // pin idx
  WDynamicArray<ConnectionInfo> m_Connections;   // connection idx
  WDynamicArray<WUInt16> m_TextureProviderPins; // pin idx
  WDynamicArray<SwitchInfo> m_Switches;

  ///@}
  /// \name CullDeadPasses / SortPasses result
  ///@{

  WDynamicBitfield m_AlivePasses;         // pass idx
  WDynamicBitfield m_AliveConnections;    // connection idx
  WDynamicArray<WUInt16> m_SortedPasses; // pass indices, in execution order

  ///@}

  // Temp array that hold the connection's WRenderGraphTextureHandle / WRenderGraphBufferHandle during AddRenderPasses
  WDynamicArray<WRenderPipelinePinConnection> m_ConnectionsRenderGraph; // connection idx
};
