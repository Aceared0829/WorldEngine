#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Declarations.h>

using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;

/// Event data for render data extraction phase.
struct WRenderWorldExtractionEvent
{
  enum class Type
  {
    BeginExtraction,      ///< Fired before any view extraction begins.
    BeforeViewExtraction, ///< Fired before extracting data for a specific view.
    AfterViewExtraction,  ///< Fired after extracting data for a specific view.
    EndExtraction         ///< Fired after all views have been extracted.
  };

  Type m_Type;
  const WView* m_pView = nullptr;
  WExtractedRenderData* m_pExtractedRenderData = nullptr;
  WUInt64 m_uiFrameCounter = 0;
};

/// Event data for render execution phase.
struct WRenderWorldRenderEvent
{
  enum class Type
  {
    BeginRender,             ///< Fired before rendering begins.
    BeforePipelineExecution, ///< Fired before executing a render pipeline.
    AfterPipelineExecution,  ///< Fired after executing a render pipeline.
    EndRender,               ///< Fired after rendering is complete.
  };

  Type m_Type;
  const WRenderViewContext* m_pRenderViewContext = nullptr;
  WUInt64 m_uiFrameCounter = 0;
};

/// Central hub for rendering operations and view management.
///
/// Manages views, render data extraction, and rendering execution. Handles double-buffering
/// of render data when multithreaded rendering is enabled. Provides events for hooking into
/// various stages of the rendering pipeline.
class W_RENDERERCORE_DLL WRenderWorld
{
public:
  static WViewHandle CreateView(WStringView sName, WView*& out_pView);
  static void DeleteView(const WViewHandle& hView);

  static bool TryGetView(const WViewHandle& hView, WView*& out_pView);

  /// Searches for an WView with the desired usage hint or alternative usage hint.
  static WView* GetViewByUsageHint(WCameraUsageHint::Enum usageHint, WCameraUsageHint::Enum alternativeUsageHint = WCameraUsageHint::None, const WWorld* pWorld = nullptr);

  static void AddMainView(const WViewHandle& hView);
  static void RemoveMainView(const WViewHandle& hView);
  static void ClearMainViews();
  static WArrayPtr<WViewHandle> GetMainViews();
  static bool IsRenderingScheduled();

  /// Caches render data for an object to avoid re-extraction if unchanged.
  ///
  /// Cached render data needs to be deleted/invalidated manually if any data changes. The dependency arrays carry per-category render-graph barrier dependencies that were recorded during extraction and are cached alongside the render data.
  static void CacheRenderData(const WView& view, const WGameObjectHandle& hOwnerObject, const WComponentHandle& hOwnerComponent, WUInt16 uiComponentVersion, WArrayPtr<WInternal::RenderDataCacheEntry> cacheEntries, WArrayPtr<const WTextureDependency> textureDependencies = {}, WArrayPtr<const WBufferDependency> bufferDependencies = {});

  /// Deletes all cached render data globally.
  static void DeleteAllCachedRenderData();

  /// Deletes cached render data for a specific component.
  static void DeleteCachedRenderData(const WGameObjectHandle& hOwnerObject, const WComponentHandle& hOwnerComponent);

  /// Deletes cached render data for a game object.
  static void DeleteCachedRenderDataForObject(const WGameObject* pOwnerObject);

  /// Recursively deletes cached render data for a game object and all its children.
  static void DeleteCachedRenderDataForObjectRecursive(const WGameObject* pOwnerObject);

  /// Resets the render data cache for a specific view.
  static void ResetRenderDataCache(WView& ref_view);

  /// Retrieves cached render data if available and still valid.
  static WArrayPtr<const WInternal::RenderDataCacheEntry> GetCachedRenderData(const WView& view, const WGameObjectHandle& hOwner, WUInt16 uiComponentVersion, WArrayPtr<const WTextureDependency>& out_textureDependencies, WArrayPtr<const WBufferDependency>& out_bufferDependencies);

  static void AddViewToRender(const WViewHandle& hView);

  /// Declares that the given view needs the specified texture in the given resource state when its render graph executes. Must be called during extraction.
  /// Usually this function is called alongside WRenderWorld::AddViewToRender to declare the required state of the output the view produced.
  static void AddViewDependency(const WView& consumerView, WGALTextureHandle hTexture, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Declares that the given view needs the specified buffer in the given resource state when its render graph executes. Must be called during extraction.
  /// Usually this function is called alongside WRenderWorld::AddViewToRender to declare the required state of the output the view produced.
  static void AddViewDependency(const WView& consumerView, WGALBufferHandle hBuffer, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  static void ExtractMainViews();

  static void Render(WRenderContext* pRenderContext);

  static void BeginFrame();
  static void EndFrame();

  static WEvent<WView*, WMutex> s_ViewCreatedEvent;
  static WEvent<WView*, WMutex> s_ViewDeletedEvent;

  static const WEvent<const WRenderWorldExtractionEvent&, WMutex>& GetExtractionEvent() { return s_ExtractionEvent; }
  static const WEvent<const WRenderWorldRenderEvent&, WMutex>& GetRenderEvent() { return s_RenderEvent; }

  static bool GetUseMultithreadedRendering();

  /// Resets the frame counter to zero. Only for test purposes !
  W_ALWAYS_INLINE static void ResetFrameCounter() { s_uiFrameCounter = 0; }

  W_ALWAYS_INLINE static WUInt64 GetFrameCounter() { return s_uiFrameCounter; }

  W_FORCE_INLINE static WUInt32 GetDataIndexForExtraction() { return GetUseMultithreadedRendering() ? (s_uiFrameCounter & 1) : 0; }

  W_FORCE_INLINE static WUInt32 GetDataIndexForRendering() { return GetUseMultithreadedRendering() ? ((s_uiFrameCounter + 1) & 1) : 0; }

  static bool IsRenderingThread();

  /// \name Render To Texture
  /// @{
public:
  struct CameraConfig
  {
    WRenderPipelineResourceHandle m_hRenderPipeline;
  };

  static void BeginModifyCameraConfigs();
  static void EndModifyCameraConfigs();
  static void ClearCameraConfigs();
  static void SetCameraConfig(const char* szName, const CameraConfig& config);
  static const CameraConfig* FindCameraConfig(const char* szName);

  static WEvent<void*> s_CameraConfigsModifiedEvent;

private:
  static bool s_bModifyingCameraConfigs;
  static WMap<WString, CameraConfig> s_CameraConfigs;

  /// @}

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderWorld);
  friend class WView;
  friend class WRenderPipeline;

  static void DeleteCachedRenderDataInternal(const WGameObjectHandle& hOwnerObject);
  static void ClearRenderDataCache();
  static void UpdateRenderDataCache();

  static void AddRenderPipelineToRebuild(WRenderPipeline* pRenderPipeline, const WViewHandle& hView);
  static void RebuildPipelines();

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static WEvent<const WRenderWorldExtractionEvent&, WMutex> s_ExtractionEvent;
  static WEvent<const WRenderWorldRenderEvent&, WMutex> s_RenderEvent;
  static WUInt64 s_uiFrameCounter;
};
