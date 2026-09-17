#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/Status.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class WGALCommandEncoder;
class WGALDevice;
class WRenderGraphPassObserver;
class WRenderGraphResourcePool;
class WRenderGraphResourceAllocator;
class WGALResourceStateTracker;
struct WRenderGraphInspectionInfo;

/// Render graph usable for a single frame. Contains passes that declare transient resource dependencies. After all passes are added, the graph is compiled to cull unused passes, allocate and alias transient resources, and compute barrier placement, then executed. Passes are always executed in declaration order.
///
/// The graph object is designed to persist across the lifetime of the program. Every frame you want to use the graph, call Reset() before re-declaring passes. Note that only one WRenderGraphPassBuilder can exist at any time so calls to `Add*Pass` must be scoped individually. Finally, call WRenderGraphManager::EnqueueRenderGraph to submit the graph for execution this frame.
class W_RENDERERCORE_DLL WRenderGraph : public WRefCounted
{
public:
  ~WRenderGraph();

  WGALDevice* GetDevice() const { return m_pDevice; }

  /// Returns the immutable name assigned at creation.
  const WString& GetGraphName() const { return m_sGraphName; }

  /// Sets the user name that can be changed at any time.
  void SetUserName(WStringView sName) { m_sUserName = sName; }
  const WString& GetUserName() const { return m_sUserName; }

  /// User data is accessible during execution callbacks via `ctx.GetUserData<T>()`.
  void SetUserData(WReflectedClass* pUserData) { m_pUserData = pUserData; }
  WReflectedClass* GetUserData() const { return m_pUserData; }

  /// \name Transient Resource Creation
  ///@{

  /// Create a transient texture. The GPU resource is allocated or aliased from a pool during compilation. There is no need to set any flags on the texture. Whenever a pass operates on a texture its flags are extended to support that operation.
  WRenderGraphTextureHandle CreateTexture(const WGALTextureCreationDescription& desc);

  /// Create a transient buffer. The GPU resource is allocated or aliased from a pool during compilation. There is no need to set any flags on the buffer. Whenever a pass operates on a buffer its flags are extended to support that operation.
  WRenderGraphBufferHandle CreateBuffer(const WGALBufferCreationDescription& desc);

  ///@}
  /// \name Resource Queries
  ///@{

  /// Returns the creation description of a transient or imported texture. Note that this reference is not stable and can become invalid when new textures are added to the graph so it is best to store a copy of the returned value.
  const WGALTextureCreationDescription& GetTextureDesc(WRenderGraphTextureHandle hTexture) const;

  /// Returns the creation description of a transient or imported buffer. Note that this reference is not stable and can become invalid when new buffers are added to the graph so it is best to store a copy of the returned value.
  const WGALBufferCreationDescription& GetBufferDesc(WRenderGraphBufferHandle hBuffer) const;

  ///@}
  /// \name Import External Resources
  ///@{

  /// Import an existing GPU texture into the graph, returning a graph handle so it can be used inside the graph. You can optionally set `access` and `stage` to force a barrier on the imported resource at the start of the graph.
  WRenderGraphTextureHandle ImportTexture(WGALTextureHandle hTexture, WBitflags<WGALResourceState> access = WGALResourceState::Unknown,
    WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Import an existing GPU buffer into the graph, returning a graph handle so it can be used inside the graph. You can optionally set `access` and `stage` to force a barrier on the imported resource at the start of the graph.
  WRenderGraphBufferHandle ImportBuffer(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> access = WGALResourceState::Unknown,
    WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Use to replace a graph texture with an actual GPU resource. The `hNewTexture` must not be already imported into the graph. Mainly used when graphs are built greedily and the target swachchain image is not clear at which point it fints into the pipeline. Thus, the graph can be built entirely with transient graph textures and only at the end the replacement can be done. If this call failed the given texture does not fulfil the requirements made to the graph texture.
  WStatus ReplaceImportedTexture(WRenderGraphTextureHandle hGraphTexture, WGALTextureHandle hNewTexture);

  ///@}
  /// \name Pass Creation
  ///@{

  /// Add a graphics pass. Returns a builder through which resource accesses and the execution callback must be declared before compilation.
  WRenderGraphPassBuilder AddGraphicsPass(WStringView sName);

  /// Add a compute pass.
  WRenderGraphPassBuilder AddComputePass(WStringView sName);

  /// Add a transfer pass.
  WRenderGraphPassBuilder AddTransferPass(WStringView sName);

  /// Insert a debug marker that will be pushed during execution at the/ current recording position (before the next pass).
  void PushMarker(WStringView sMarker);

  /// Insert a pop-marker at the current recording position.
  void PopMarker();

  ///@}
  /// \name Compilation & Execution
  ///@{

  /// Clear all declared passes and resources so the graph can be rebuilt. This is done automatically for enqueued graphs at the end of each frame, but should be called regardless every time you intent to record into the graph.
  void Reset();

  /// Checks that all imported textures and buffers still exist on the device. Call before registering the graph each frame to detect stale resources.
  WResult ValidateImportedResources() const;

  /// Compile the graph: cull unused passes, allocate and alias transient resources. After this call, no new passes can be added until the graph is reset.
  WResult Compile();

  /// Compute texture and buffer barriers for each pass based on resource state tracking.
  void ComputeBarriers(WGALResourceStateTracker& ref_tracker, WArrayPtr<WRenderGraphPassObserver*> observers = {});

  /// Execute all passes in compiled order.
  void Execute(WRenderGraphContext& ref_ctx, WArrayPtr<WRenderGraphPassObserver*> observers = {});

private:
  friend class WRenderGraphManager;
  friend class WRenderGraphPassBuilder;

  WRenderGraph(WGALDevice* pDevice, WStringView sName, WEnum<WRenderGraphPhase> phase);

  void StartPassBuilder(WEnum<WGALQueueType> queueType, WStringView sName);
  void EndPassBuilder();

  WResult GetInspectionInfo(WRenderGraphInspectionInfo& out_info) const;

  /// Find a pass by name and return its original index. Returns s_Unused if not found.
  WUInt16 FindPassByName(WStringView sName) const;

  /// Find the texture handle used in the Nth texture access of the given pass.
  WRenderGraphTextureHandle FindTextureAccessInPass(WUInt16 uiOriginalPassIndex, WUInt16 uiAccessIndex) const;

private:
  static constexpr WUInt16 s_Unused = 0xFFFF;

  enum class RenderGraphState
  {
    Recording,       // New passes can be added, initial state
    Enqueued,        // No new passes can be added.
    Compiled,        // BuildDependencyGraph, CullDeadPasses, BuildSortedPassList, ComputeResourceLifetimes, AllocateTransientResources, BuildRenderingSetups
    BarriersCreated, // ComputeBarriers
  };

  struct TextureInfo
  {
    WRenderGraphTextureHandle m_hTexture;
    WGALTextureRange m_range;
    WBitflags<WGALShaderStageFlags> m_stage;
    WBitflags<WGALResourceState> m_access;
  };

  struct BufferInfo
  {
    WRenderGraphBufferHandle m_hBuffer;
    WBitflags<WGALShaderStageFlags> m_stage;
    WBitflags<WGALResourceState> m_access;
  };

  struct ColorTargetInfo
  {
    WRenderGraphTextureHandle m_hTexture;
    WGALRenderTargetRange m_range;
    WEnum<WGALRenderTargetLoadOp> m_loadOp;
    WEnum<WGALRenderTargetStoreOp> m_storeOp;
    WEnum<WGALResourceFormat> m_overrideViewFormat;
    WEnum<WGALTextureType> m_overrideViewType;
  };

  struct DepthStencilTargetInfo
  {
    WRenderGraphTextureHandle m_hTexture;
    WGALRenderTargetRange m_range;
    WEnum<WGALRenderTargetLoadOp> m_depthLoadOp;
    WEnum<WGALRenderTargetStoreOp> m_depthStoreOp;
    WEnum<WGALRenderTargetLoadOp> m_stencilLoadOp;
    WEnum<WGALRenderTargetStoreOp> m_stencilStoreOp;
    bool m_bReadOnly = false;
  };

  struct DepthStencilTargetClearInfo
  {
    float fDepthClear = 1.0f;
    WUInt8 uiStencilClear = 0;
  };

  struct ImportedTexture
  {
    WGALTextureHandle m_hTextureHandle;
    WBitflags<WGALResourceState> m_access;
    WBitflags<WGALShaderStageFlags> m_stage;
  };
  struct ImportedBuffer
  {
    WGALBufferHandle m_hBufferHandle;
    WBitflags<WGALResourceState> m_access;
    WBitflags<WGALShaderStageFlags> m_stage;
  };

  struct Pass
  {
    W_ALWAYS_INLINE WArrayPtr<const TextureInfo> GetReadTextures(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_ReadTextures.GetData() + m_uiReadTextureIndex, m_uiReadTextureCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const TextureInfo> GetWriteTextures(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_WriteTextures.GetData() + m_uiWriteTextureIndex, m_uiWriteTextureCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const BufferInfo> GetReadBuffers(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_ReadBuffers.GetData() + m_uiReadBufferIndex, m_uiReadBufferCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const BufferInfo> GetWriteBuffers(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_WriteBuffers.GetData() + m_uiWriteBufferIndex, m_uiWriteBufferCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const ColorTargetInfo> GetColorTargets(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_ColorTargets.GetData() + m_uiColorTargetIndex, m_uiColorTargetCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const DepthStencilTargetInfo> GetDepthStencilTargets(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_DepthStencilTargets.GetData() + m_uiDepthStencilTargetIndex, m_uiDepthStencilTargetCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const WColor> GetClearColors(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_ClearColors.GetData() + m_uiClearColorIndex, m_uiClearColorCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const DepthStencilTargetClearInfo> GetClearDepthStencils(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_ClearDepthStencils.GetData() + m_uiDepthStencilClearIndex, m_uiDepthStencilClearCount);
    }

    /// Returns the adjacency slice for this pass from the flat storage. Populated during BuildDependencyGraph.
    W_ALWAYS_INLINE WArrayPtr<const WUInt16> GetAdjacency(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_AdjacencyStorage.GetData() + m_uiAdjacencyIndex, m_uiAdjacencyCount);
    }

    WUInt16 m_uiReadTextureIndex = 0;
    WUInt16 m_uiWriteTextureIndex = 0;
    WUInt16 m_uiReadBufferIndex = 0;
    WUInt16 m_uiWriteBufferIndex = 0;
    WUInt16 m_uiColorTargetIndex = 0;
    WUInt16 m_uiDepthStencilTargetIndex = 0;
    WUInt16 m_uiClearColorIndex = 0;
    WUInt16 m_uiDepthStencilClearIndex = 0;
    WUInt16 m_uiAdjacencyIndex = 0;

    WUInt8 m_uiReadTextureCount = 0;
    WUInt8 m_uiWriteTextureCount = 0;
    WUInt8 m_uiReadBufferCount = 0;
    WUInt8 m_uiWriteBufferCount = 0;
    WUInt8 m_uiColorTargetCount = 0;
    WUInt8 m_uiDepthStencilTargetCount = 0;
    WUInt8 m_uiClearColorCount = 0;
    WUInt8 m_uiDepthStencilClearCount = 0;
    WUInt8 m_uiAdjacencyCount = 0;
    WRenderGraphExecuteFunction m_ExecuteFunction;
    bool m_bHasSideEffects = false;
    bool m_bStereoscopic = false;
    WEnum<WGALQueueType> m_QueueType;
  };

  struct IntermediatePass
  {
    W_ALWAYS_INLINE WArrayPtr<const WUInt16> GetAcquireTextures(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_AcquireTextures.GetData() + m_uiAcquireTextureIndex, m_uiAcquireTextureCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const WUInt16> GetReleaseTextures(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_ReleaseTextures.GetData() + m_uiReleaseTextureIndex, m_uiReleaseTextureCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const WUInt16> GetAcquireBuffers(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_AcquireBuffers.GetData() + m_uiAcquireBufferIndex, m_uiAcquireBufferCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const WUInt16> GetReleaseBuffers(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_ReleaseBuffers.GetData() + m_uiReleaseBufferIndex, m_uiReleaseBufferCount);
    }

    WUInt16 uiOriginalPassIndex = 0;
    WUInt8 m_uiAcquireTextureCount = 0;
    WUInt8 m_uiReleaseTextureCount = 0;
    WUInt8 m_uiAcquireBufferCount = 0;
    WUInt8 m_uiReleaseBufferCount = 0;

    WUInt16 m_uiAcquireTextureIndex = 0;
    WUInt16 m_uiReleaseTextureIndex = 0;
    WUInt16 m_uiAcquireBufferIndex = 0;
    WUInt16 m_uiReleaseBufferIndex = 0;
  };

  struct CompiledPass
  {
    W_ALWAYS_INLINE WArrayPtr<const WGALTextureBarrier> GetTextureBarriers(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_CompiledTextureBarriers.GetData() + m_uiTextureBarrierIndex, m_uiTextureBarrierCount);
    }
    W_ALWAYS_INLINE WArrayPtr<const WGALBufferBarrier> GetBufferBarriers(const WRenderGraph* pGraph) const
    {
      return WMakeArrayPtr(pGraph->m_CompiledBufferBarriers.GetData() + m_uiBufferBarrierIndex, m_uiBufferBarrierCount);
    }

    WUInt16 m_uiOriginalPassIndex = 0;
    WUInt32 m_uiTextureBarrierIndex = 0;
    WUInt32 m_uiBufferBarrierIndex = 0;
    WUInt16 m_uiTextureBarrierCount = 0;
    WUInt16 m_uiBufferBarrierCount = 0;
    WGALRenderingSetup m_RenderingSetup;
  };

private:
  void ResetInternal(RenderGraphState renderGraphState);

  // --- Compilation ---

  /// Validate that all passes have execute-callbacks and that all referenced handles are valid.
  WResult ValidateGraph();

  /// Build per-pass dependency adjacency lists from resource read/write patterns.
  void BuildDependencyGraph();

  /// Remove passes that do not contribute to any side-effecting output.
  void CullDeadPasses();

  /// Build the sorted pass list from alive passes in declaration order.
  void BuildSortedPassList();

  /// Determine the first and last use (in sorted order) of each transient resource.
  void ComputeResourceLifetimes();

  /// Allocate concrete GPU resources for transient handles using the resource pool.
  void AllocateTransientResources();

  /// Build WGALRenderingSetup for each graphics pass from its declared color
  /// and depth-stencil targets.
  void BuildRenderingSetups();

private:
  WGALDevice* m_pDevice = nullptr;
  WUniquePtr<WRenderGraphResourceAllocator> m_pAllocator;
  RenderGraphState m_RenderGraphState = RenderGraphState::Recording;
  WString m_sUserName;
  WString m_sGraphName;
  WEnum<WRenderGraphPhase> m_Phase;
  WReflectedClass* m_pUserData = nullptr;

  /// \name Recording State
  ///@{

  Pass* m_pCurrentPass = nullptr;
  WDynamicArray<WString> m_PassNames;
  WHashTable<WString, WUInt32> m_UniquePassNames;
  WDynamicArray<Pass> m_Passes;
  // Storage heaps for pass data
  WDynamicArray<TextureInfo> m_ReadTextures;
  WDynamicArray<TextureInfo> m_WriteTextures;
  WDynamicArray<BufferInfo> m_ReadBuffers;
  WDynamicArray<BufferInfo> m_WriteBuffers;
  WDynamicArray<ColorTargetInfo> m_ColorTargets;
  WDynamicArray<DepthStencilTargetInfo> m_DepthStencilTargets;
  WDynamicArray<WColor> m_ClearColors;
  WDynamicArray<DepthStencilTargetClearInfo> m_ClearDepthStencils;

  struct MarkerEvent
  {
    WUInt16 m_uiPassIndex; ///< Emitted before this pass index.
    bool m_bPush;           ///< true = PushMarker, false = PopMarker.
    WString m_sName;       ///< Only used for push.
  };
  WDynamicArray<MarkerEvent> m_MarkerEvents;

  // Create / Import Texture / Buffer result
  WDynamicArray<WGALTextureCreationDescription> m_TextureCreationDescriptions;
  WDynamicArray<WGALBufferCreationDescription> m_BufferCreationDescriptions;

  // Imported textures / Buffers
  WMap<WRenderGraphTextureHandle, ImportedTexture> m_HandleToImportTexture;
  WMap<WGALTextureHandle, WRenderGraphTextureHandle> m_ImportTextureToHandle;
  WMap<WRenderGraphBufferHandle, ImportedBuffer> m_HandleToImportBuffer;
  WMap<WGALBufferHandle, WRenderGraphBufferHandle> m_ImportBufferToHandle;

  ///@}
  /// \name Intermediate State
  ///@{

  // BuildDependencyGraph (pass index to...)
  WDynamicArray<WUInt16> m_AdjacencyStorage;
  // CullDeadPasses (pass index to...)
  WDynamicArray<bool> m_Alive;

  // BuildSortedPassList (list of pass indices)
  WDynamicArray<IntermediatePass> m_AlivePasses;

  // ComputeResourceLifetimes (texture / buffer index to sorted pass index)
  WDynamicArray<WUInt16> m_TextureFirstUse;
  WDynamicArray<WUInt16> m_TextureLastUse;
  WDynamicArray<WUInt16> m_BufferFirstUse;
  WDynamicArray<WUInt16> m_BufferLastUse;
  // Heap storage for acquire / release calls
  WDynamicArray<WUInt16> m_AcquireTextures;
  WDynamicArray<WUInt16> m_ReleaseTextures;
  WDynamicArray<WUInt16> m_AcquireBuffers;
  WDynamicArray<WUInt16> m_ReleaseBuffers;

  // AllocateTransientResources (texture / buffer index to resolved index)
  WDynamicArray<WUInt16> m_TextureToResolvedTexture;
  WDynamicArray<WUInt16> m_BufferToResolvedBuffer;
  /// List of resolved resources. Barriers will happen on this array. Size will be less or equal to handle count (aliasing of resources).
  WDynamicArray<WGALTextureHandle> m_ResolvedTextures;
  WDynamicArray<WGALBufferHandle> m_ResolvedBuffers;

  ///@}
  /// \name Compiled state
  ///@{

  // BuildRenderingSetups
  WDynamicArray<CompiledPass> m_CompiledPasses;

  ///@}
  /// \name BarriersCreated state
  ///@{

  // ComputeBarriers
  WDynamicArray<WGALTextureBarrier> m_CompiledTextureBarriers;
  WDynamicArray<WGALBufferBarrier> m_CompiledBufferBarriers;

  ///@}
};
