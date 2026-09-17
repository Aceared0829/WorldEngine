#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>

/// Contains all render data extracted from a view for one frame.
///
/// During the extraction phase, render components add their render data to this container,
/// organized by category (opaque, transparent, etc.). The data is then sorted and batched
/// for efficient rendering. Also stores camera data, view data, world time, and debug contexts.
class W_RENDERERCORE_DLL WExtractedRenderData
{
public:
  WExtractedRenderData();
  ~WExtractedRenderData();

  /// \name Initial setup
  ///@{

  W_ALWAYS_INLINE void SetCamera(const WCamera& camera) { m_Camera = camera; }
  W_ALWAYS_INLINE const WCamera& GetCamera() const { return m_Camera; }

  W_ALWAYS_INLINE void SetViewData(const WViewData& viewData) { m_ViewData = viewData; }
  W_ALWAYS_INLINE const WViewData& GetViewData() const { return m_ViewData; }

  W_ALWAYS_INLINE void SetWorldHandle(const WWorldHandle& hWorld) { m_hWorld = hWorld; }
  W_ALWAYS_INLINE const WWorldHandle& GetWorldHandle() const { return m_hWorld; }

  W_ALWAYS_INLINE void SetWorldTime(WTime time) { m_WorldTime = time; }
  W_ALWAYS_INLINE WTime GetWorldTime() const { return m_WorldTime; }

  W_ALWAYS_INLINE void SetWorldDebugContext(const WDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  W_ALWAYS_INLINE const WDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  W_ALWAYS_INLINE void SetViewDebugContext(const WDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  W_ALWAYS_INLINE const WDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  ///@}
  /// \name Add extracted data
  ///@{

  /// Adds render data for a specific rendering category.
  W_ALWAYS_INLINE void AddRenderData(const WRenderData* pRenderData, WRenderData::Category category);

  /// Adds frame-level data that is not tied to a specific render category.
  W_ALWAYS_INLINE void AddFrameData(const WRenderData* pFrameData);

  W_ALWAYS_INLINE void AddViewDependency(WGALTextureHandle hTexture, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  W_ALWAYS_INLINE void AddViewDependency(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Adds a texture barrier dependency that applies when its category is rendered. The category is taken from the dependency itself.
  /// Should not be called in user code, call WMsgExtractRenderData::AddDependency instead during extraction of object or call AddViewDependency.
  W_ALWAYS_INLINE void AddDependency(const WTextureDependency& dependency);

  /// Adds a buffer barrier dependency that applies when its category is rendered. The category is taken from the dependency itself.
  /// Should not be called in user code, call WMsgExtractRenderData::AddDependency instead during extraction of object or call AddViewDependency.
  W_ALWAYS_INLINE void AddDependency(const WBufferDependency& dependency);

  /// Adds a sampler that is added to the W_GAL_BIND_GROUP_FRAME during rendering.
  /// \sa WBindGroupBuilder
  void AddSamplerBinding(WTempHashedString sSlotName, WGALSamplerStateHandle hSampler);
  W_ALWAYS_INLINE void AddSamplerBinding(const WSamplerBinding& binding);

  /// Adds a buffer that is added to the W_GAL_BIND_GROUP_FRAME during rendering.
  /// \sa WBindGroupBuilder
  void AddBufferBinding(WTempHashedString sSlotName, WGALBufferHandle hBuffer, WGALBufferRange bufferRange = {}, WEnum<WGALResourceFormat> overrideTexelBufferFormat = WGALResourceFormat::Invalid);
  W_ALWAYS_INLINE void AddBufferBinding(const WBufferBinding& binding);

  /// Adds a texture that is added to the W_GAL_BIND_GROUP_FRAME during rendering.
  /// \sa WBindGroupBuilder
  void AddTextureBinding(WTempHashedString sSlotName, WGALTextureHandle hTexture, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);
  W_ALWAYS_INLINE void AddTextureBinding(const WTextureBinding& binding);

  void AddTextureBinding(WTempHashedString sSlotName, const WTexture2DResourceHandle& hTexture, WResourceAcquireMode acquireMode = WResourceAcquireMode::AllowLoadingFallback, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);
  void AddTextureBinding(WTempHashedString sSlotName, const WTexture3DResourceHandle& hTexture, WResourceAcquireMode acquireMode = WResourceAcquireMode::AllowLoadingFallback, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);
  void AddTextureBinding(WTempHashedString sSlotName, const WTextureCubeResourceHandle& hTexture, WResourceAcquireMode acquireMode = WResourceAcquireMode::AllowLoadingFallback, WGALTextureRange textureRange = {}, WEnum<WGALResourceFormat> overrideViewFormat = WGALResourceFormat::Invalid, WEnum<WGALTextureType> overrideViewType = WGALTextureType::Invalid);

  ///@}
  /// \name Read extracted data
  ///@{

  W_ALWAYS_INLINE WArrayPtr<const WTextureDependency> GetTextureViewDependencies() const { return m_ViewTextureDependencies; }
  W_ALWAYS_INLINE WArrayPtr<const WBufferDependency> GetBufferViewDependencies() const { return m_ViewBufferDependencies; }

  W_ALWAYS_INLINE WArrayPtr<const WSamplerBinding> GetSamplerBindings() const { return m_SamplerBindings; }
  W_ALWAYS_INLINE WArrayPtr<const WBufferBinding> GetBufferBindings() const { return m_BufferBindings; }
  W_ALWAYS_INLINE WArrayPtr<const WTextureBinding> GetTextureBindings() const { return m_TextureBindings; }

  /// Returns the texture barrier dependencies recorded for a specific category.
  WArrayPtr<const WTextureDependency> GetTextureDependenciesWithCategory(WRenderData::Category category) const;

  /// Returns the buffer barrier dependencies recorded for a specific category.
  WArrayPtr<const WBufferDependency> GetBufferDependenciesWithCategory(WRenderData::Category category) const;

  /// Returns all render data batches for a specific category.
  WRenderDataBatchList GetRenderDataBatchesWithCategory(WRenderData::Category category) const;

  /// Returns raw unsorted render data for a specific category.
  WArrayPtr<const WRenderDataBatch::SortableRenderData> GetRawRenderDataWithCategory(WRenderData::Category category) const;

  template <typename T>
  W_ALWAYS_INLINE const T* GetFrameData() const
  {
    return static_cast<const T*>(GetFrameData(WGetStaticRTTI<T>()));
  }

  ///@}
  /// \name Administration
  ///@{

  /// Sorts and batches all render data by category and sorting key for efficient rendering.
  void SortAndBatch();

  void Clear();

  ///@}

private:
  const WRenderData* GetFrameData(const WRTTI* pRtti) const;

  struct DataPerCategory
  {
    WDynamicArray<WRenderDataBatch> m_Batches;
    WDynamicArray<WRenderDataBatch::SortableRenderData> m_SortableRenderData;
    WDynamicArray<WInstanceableRenderData::DataOffsets> m_DataOffsets;
    WGALBufferHandle m_hDataOffsetsBuffer;
    WDynamicArray<WTextureDependency> m_TextureDependencies;
    WDynamicArray<WBufferDependency> m_BufferDependencies;
  };

  void SortAndBatchCategory(DataPerCategory& dataPerCategory, WRenderData::Category category);

  WCamera m_Camera;
  WViewData m_ViewData;
  WWorldHandle m_hWorld;
  WTime m_WorldTime;

  WDebugRendererContext m_WorldDebugContext;
  WDebugRendererContext m_ViewDebugContext;

  WHybridArray<DataPerCategory, 32> m_DataPerCategory;
  WHybridArray<const WRenderData*, 16> m_FrameData;
  WHybridArray<WTextureDependency, 4> m_ViewTextureDependencies;
  WHybridArray<WBufferDependency, 4> m_ViewBufferDependencies;

  WHybridArray<WSamplerBinding, 2> m_SamplerBindings;
  WHybridArray<WBufferBinding, 2> m_BufferBindings;
  WHybridArray<WTextureBinding, 2> m_TextureBindings;
};

#include <RendererCore/Pipeline/Implementation/ExtractedRenderData_inl.h>
