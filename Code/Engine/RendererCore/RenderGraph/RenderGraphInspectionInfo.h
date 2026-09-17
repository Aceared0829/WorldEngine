#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WStreamReader;
class WStreamWriter;

/// Identifies a render graph execution that was observed during the last frame.
struct WRenderGraphExecutionSummary
{
  WUInt64 m_uiRenderGraphId = 0;
  WString m_sGraphName;
  WString m_sUserName;
  WEnum<WRenderGraphPhase> m_Phase;
  WInt32 m_uiExecutionOrder = -1; ///< execution oder in the phase. -1 if not executed this frame.
};

/// Describes a swap-chain that can be used as a preview target by the render graph inspector.
struct WRenderGraphSwapChainSummary
{
  W_DECLARE_POD_TYPE();

  WUInt32 m_uiSwapChainId = 0;
  WUInt32 m_uiWidth = 0;
  WUInt32 m_uiHeight = 0;
};

/// Aggregates the render graphs and preview targets currently known to the inspector.
struct WRenderGraphInspectionSummary
{
  WDynamicArray<WRenderGraphExecutionSummary> m_RenderGraphs;
  WDynamicArray<WRenderGraphSwapChainSummary> m_AvailableSwapChains;
};

W_RENDERERCORE_DLL void operator<<(WStreamWriter& inout_stream, const WRenderGraphExecutionSummary& value);
W_RENDERERCORE_DLL void operator>>(WStreamReader& inout_stream, WRenderGraphExecutionSummary& ref_value);
W_RENDERERCORE_DLL void operator<<(WStreamWriter& inout_stream, const WRenderGraphSwapChainSummary& value);
W_RENDERERCORE_DLL void operator>>(WStreamReader& inout_stream, WRenderGraphSwapChainSummary& ref_value);
W_RENDERERCORE_DLL void operator<<(WStreamWriter& inout_stream, const WRenderGraphInspectionSummary& value);
W_RENDERERCORE_DLL void operator>>(WStreamReader& inout_stream, WRenderGraphInspectionSummary& ref_value);

/// Read-only snapshot of a compiled render graph's structure, taken after compilation. Used for visualization and debugging without exposing internal graph state.
struct W_RENDERERCORE_DLL WRenderGraphInspectionInfo
{
  void Swap(WRenderGraphInspectionInfo& ref_data);
  void Clear();

  /// Describes one declared pass and whether it remains active after graph compilation.
  struct PassInfo
  {
    WString m_sName;
    WEnum<WGALQueueType> m_QueueType;
    bool m_bHasSideEffects = false;
    bool m_bAlive = true; ///< false if the pass was culled during compilation.
  };

  /// Describes a texture resource used by the compiled render graph.
  struct TextureResourceInfo
  {
    W_DECLARE_POD_TYPE();
    WGALTextureCreationDescription m_Desc;
    bool m_bImported = false;
    WUInt16 m_uiFirstUsePassIndex = 0xFFFF;
    WUInt16 m_uiLastUsePassIndex = 0xFFFF;
    WUInt16 m_uiResolvedIndex = 0xFFFF;
  };

  /// Describes a buffer resource used by the compiled render graph.
  struct BufferResourceInfo
  {
    W_DECLARE_POD_TYPE();
    WGALBufferCreationDescription m_Desc;
    bool m_bImported = false;
    WUInt16 m_uiFirstUsePassIndex = 0xFFFF;
    WUInt16 m_uiLastUsePassIndex = 0xFFFF;
    WUInt16 m_uiResolvedIndex = 0xFFFF;
  };

  /// Describes a single resource access made by one pass in the compiled render graph.
  struct AccessInfo
  {
    W_DECLARE_POD_TYPE();
    WUInt16 m_uiPassIndex = 0;     ///< Index into m_Passes (declaration order).
    WUInt16 m_uiResourceIndex = 0; ///< Index into m_Textures or m_Buffers.
    WUInt16 m_uiAccessIndex = 0;
    bool m_bIsTexture = true;
    WBitflags<WGALResourceState> m_Access;
    WGALTextureRange m_TextureRange; ///< Only meaningful for textures.
  };

  /// All passes in declaration order. Culled passes have m_bAlive == false.
  WDynamicArray<PassInfo> m_Passes;

  WDynamicArray<TextureResourceInfo> m_Textures;
  WDynamicArray<BufferResourceInfo> m_Buffers;

  /// All resource accesses across all passes, flattened first by passId, then by access types.
  WDynamicArray<AccessInfo> m_Accesses;
};

W_RENDERERCORE_DLL void operator<<(WStreamWriter& inout_stream, const WRenderGraphInspectionInfo& value);
W_RENDERERCORE_DLL void operator>>(WStreamReader& inout_stream, WRenderGraphInspectionInfo& ref_value);
