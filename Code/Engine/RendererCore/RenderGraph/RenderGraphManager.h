#pragma once

#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WGALDeviceEvent;
class WGALResourceStateTracker;
class WRenderGraph;
class WRenderGraphPassObserver;
class WRenderGraphResourcePool;
struct WTextureValidationError;
struct WBufferValidationError;

struct WRenderGraphRenderEvent
{
  enum class Type
  {
    BeginRender,          ///< Fired before rendering begins.
    BeforeGraphExecution, ///< Fired before executing a render graph.
    AfterGraphExecution,  ///< Fired after executing a render graph.
    EndRender,            ///< Fired after rendering is complete.
  };

  Type m_Type;
  WRenderGraph* m_pGraph = nullptr;
  WRenderGraphContext* m_pContext = nullptr;
};

/// Manages WRenderGraph lifetime and execution.
class W_RENDERERCORE_DLL WRenderGraphManager
{
public:
  /// Creates a new render graph. The caller holds a reference via the returned scoped pointer.
  /// When all references are released, the graph is deleted at the end of the frame.
  static WSharedPtr<WRenderGraph> CreateRenderGraph(WStringView sName, WEnum<WRenderGraphPhase> phase = WRenderGraphPhase::Render);

  /// Enqueue a render graph for execution in the current frame. Must be called before ExecuteRenderGraphs. Usually in WRenderWorldRenderEvent::Type::BeginRender or WGALDeviceEvent::AfterBeginFrame.
  static void EnqueueRenderGraph(const WSharedPtr<WRenderGraph>& pRenderGraph);

  /// Must be called after WGALDeviceEvent::BeforeBeginFrame and before WGALDeviceEvent::AfterEndFrame.
  static void ExecuteRenderGraphs(WGALDevice* pDevice);

  /// Access the shared resource pool. Only valid after device init.
  static WRenderGraphResourcePool* GetResourcePool();

  /// \name Pass Observers
  ///@{

  /// Fills a summary with the render graphs and available swapchains.
  /// Only safe to call from an WRenderGraphRenderEvent::Type::AfterGraphExecution callback registered through s_RenderEvent.
  static void GetExecutionSummary(WRenderGraphInspectionSummary& out_summary);

  /// Fills an inspection info of a specific render graph.
  /// Only safe to call from an WRenderGraphRenderEvent::Type::AfterGraphExecution callback registered through s_RenderEvent.
  static WResult GetRenderGraphInspectionInfo(WUInt64 uiRenderGraphId, WRenderGraphInspectionInfo& out_inspectionInfo);

  static WSharedPtr<WRenderGraphPassObserver> CreateObserver();

  ///@}

  static WEvent<const WRenderGraphRenderEvent&, WMutex> s_RenderEvent;

  /// Prints all operations and barriers affecting a texture resource up to the current execution point.
  /// Only operations overlapping the given range are printed.
  static void PrintTextureResourceHistory(const WTextureValidationError& error);

  /// Prints all operations and barriers affecting a buffer resource up to the current execution point.
  /// Call from validation failure handlers to diagnose barrier issues.
  static void PrintBufferResourceHistory(const WBufferValidationError& error);

private:
  friend class WRenderGraph;
  friend class WRenderGraphPassObserver;

  static void OnEngineStartup();
  static void OnEngineShutdown();
  static void GALDeviceEventHandler(const WGALDeviceEvent& e);

  static void InitPool(WGALDevice* pDevice);
  static void DeinitPool(WGALDevice* pDevice);
  static void BeginFrame();
  static void OnGraphDestroyed(WRenderGraph* pGraph);
  static WUInt64 GetRenderGraphId(WRenderGraph* pGraph);
  static WRenderGraph* GetRenderGraphById(WUInt64 uiRenderGraphId);
  static WUInt32 GetSwapChainId(WGALSwapChainHandle hSwapChain);
  static WGALSwapChainHandle GetSwapChainById(WUInt32 uiSwapChainId);

  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderGraphManager);

  static WMutex s_Mutex;
  static WDynamicArray<WSharedPtr<WRenderGraph>> s_EnqueuedRenderGraphs[3];
  static WDynamicArray<WRenderGraph*> s_AllRenderGraphs[3];
  static WUniquePtr<WRenderGraphResourcePool> s_pPool;
  static WUniquePtr<WGALResourceStateTracker> s_pStateTracker;

  // Pass observers
  static WDynamicArray<WSharedPtr<WRenderGraphPassObserver>> s_Observers;
  static WSharedPtr<WRenderGraph> s_pObserverGraph;

  // Execution tracking for diagnostics
  static WDynamicArray<WRenderGraph*> s_ExecutingGraphs; ///< Executed this frame, kept alive via s_EnqueuedRenderGraphs
  static WDynamicArray<WRenderGraphPassObserver*> s_ExecutingObservers;
  static WUInt32 s_uiCurrentGraphIndex;
  static WUInt32 s_uiCurrentPassIndex;
};
