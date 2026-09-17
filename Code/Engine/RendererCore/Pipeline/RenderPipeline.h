#pragma once

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>

class WProfilingId;
class WView;
class WCamera;
struct WViewData;
class WRenderPipelinePass;
class WFrameDataProviderBase;
struct WPermutationVar;
class WDGMLGraph;
class WFrustum;
class WRasterizerView;
class WRenderGraph;
struct WRenderGraphRenderEvent;
class WRenderGraphContext;

class W_RENDERERCORE_DLL WRenderPipeline : public WRefCounted
{
public:
  enum class PipelineState
  {
    Uninitialized,
    RebuildError,
    Initialized,
    RenderGraphBuilt,
  };

  WRenderPipeline(WDynamicArray<WUniquePtr<WRenderPipelinePass>>&& passes, WDynamicArray<WUniquePtr<WExtractor>>&& extractors, WArrayPtr<const WRenderPipelineResourceLoaderConnection> connections);
  ~WRenderPipeline();

  void GetPasses(WDynamicArray<const WRenderPipelinePass*>& ref_passes) const;
  void GetPasses(WDynamicArray<WRenderPipelinePass*>& ref_passes);
  WRenderPipelinePass* GetPassByName(const WStringView& sPassName);
  WHashedString GetViewName() const;

  void GetExtractors(WDynamicArray<const WExtractor*>& ref_extractors) const;
  void GetExtractors(WDynamicArray<WExtractor*>& ref_extractors);
  WExtractor* GetExtractorByName(const WStringView& sExtractorName);

  WArrayPtr<const WRenderPipelinePassGraph::SwitchInfo> GetSwitches() const { return m_PassGraph.GetSwitches(); }
  bool SetSwitchValue(WUInt32 uiSwitchIndex, WInt32 iValue) { return m_PassGraph.SetSwitchValue(uiSwitchIndex, iValue); }
  bool SetSwitchToDefault(WUInt32 uiSwitchIndex) { return m_PassGraph.SetSwitchToDefault(uiSwitchIndex); }

  template <typename T>
  W_ALWAYS_INLINE T* GetFrameDataProvider() const
  {
    return static_cast<T*>(GetFrameDataProvider(WGetStaticRTTI<T>()));
  }

  const WExtractedRenderData& GetRenderData() const;
  WRenderDataBatchList GetRenderDataBatchesWithCategory(WRenderData::Category category) const;

  /// Returns the texture barrier dependencies recorded during extraction for a specific category.
  WArrayPtr<const WTextureDependency> GetTextureDependenciesWithCategory(WRenderData::Category category) const;

  /// Returns the buffer barrier dependencies recorded during extraction for a specific category.
  WArrayPtr<const WBufferDependency> GetBufferDependenciesWithCategory(WRenderData::Category category) const;

  /// Adds a texture dependency to the current extraction frame's data.
  /// Must only be called during extraction.
  void AddViewDependency(WGALTextureHandle hTexture, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Adds a buffer dependency to the current extraction frame's data.
  /// Must only be called during extraction.
  void AddViewDependency(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  using RenderDataProcessor = WDelegate<void(WExtractedRenderData&)>;
  WUInt32 AddRenderDataProcessor(RenderDataProcessor processor);

  /// Creates a DGML graph of all passes and textures. Can be used to verify that no accidental temp textures are created due to poorly constructed pipelines or errors in code.
  void CreateDgmlGraph(WDGMLGraph& ref_graph);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  static WCVarBool cvar_SpatialCullingVis;
#endif

  W_DISALLOW_COPY_AND_ASSIGN(WRenderPipeline);

private:
  friend class WRenderWorld;
  friend class WView;

  /// Returns whether the pipeline should be rendered. E.g. returns false if the associated world has been destroyed.
  bool ShouldRender() const;

  // Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const WView& view);
  bool RebuildInternal(const WView& view);
  bool RebuildRenderGraph(const WViewData& viewData, const WCamera& camera);
  bool AddRenderPasses(const WViewData& viewData, const WCamera& camera);
  bool UpdateTextureProviders();
  void UpdateViewData(const WView& view, WUInt32 uiDataIndex);

  WFrameDataProviderBase* GetFrameDataProvider(const WRTTI* pRtti) const;

  void ExtractData(const WView& view);
  void FindVisibleObjects(const WView& view);

  void EnqueueRenderGraph(WRenderContext* pRenderer);
  void UpdateRenderContext(WRenderGraphContext& ctx);

  WRasterizerView* PrepareOcclusionCulling(const WFrustum& frustum, const WView& view);
  void PreviewOcclusionBuffer(const WRasterizerView& rasterizer, const WView& view);

  void OnRenderEvent(const WRenderGraphRenderEvent& e);

private: // Member data
  // Thread data
  WThreadID m_CurrentExtractThread = (WThreadID)0;
  WThreadID m_CurrentRenderThread = (WThreadID)0;

  // Pipeline render data
  WExtractedRenderData m_Data[2];
  WDynamicArray<const WGameObject*> m_VisibleObjects;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WTime m_AverageCullingTime;
#endif

  WHashedString m_sName;
  WUInt64 m_uiLastExtractionFrame = -1;
  WUInt64 m_uiLastRenderFrame = -1;

  // Render pass graph data
  PipelineState m_PipelineState = PipelineState::Uninitialized;

  WRenderPipelinePassGraph m_PassGraph;

  /// Render Graph
  WSharedPtr<WRenderGraph> m_pRenderGraph;
  WRenderViewContext m_RenderViewContext;
  WUInt32 m_uiSettingsModificationCounter = 0;

  // Data Providers
  mutable WDynamicArray<WUniquePtr<WFrameDataProviderBase>> m_DataProviders;
  mutable WHashTable<const WRTTI*, WUInt32> m_TypeToDataProviderIndex;

  WDynamicArray<RenderDataProcessor> m_RenderDataProcessors;

  WDynamicArray<WPermutationVar> m_PermutationVars;

  // Occlusion Culling
  WGALTextureHandle m_hOcclusionDebugViewTexture;
};
