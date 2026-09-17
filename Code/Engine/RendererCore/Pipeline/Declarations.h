#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Shader/BindGroup.h>
class WCamera;
class WExtractedRenderData;
class WExtractor;
class WView;
class WRenderer;
class WRenderData;
class WRenderDataManager;
class WRenderDataBatch;
class WRenderPipeline;
class WRenderPipelinePass;
class WRenderContext;
class WDebugRendererContext;

struct WRenderPipelineNodePin;
struct WViewData;

namespace WInternal
{
  struct RenderDataCache;

  struct RenderDataCacheEntry
  {
    W_DECLARE_POD_TYPE();

    const WRenderData* m_pRenderData = nullptr;
    WUInt16 m_uiCategory = 0;
    WUInt16 m_uiComponentIndex = 0;
    WUInt16 m_uiPartIndex = 0;

    W_ALWAYS_INLINE bool operator==(const RenderDataCacheEntry& other) const { return m_pRenderData == other.m_pRenderData && m_uiCategory == other.m_uiCategory && m_uiComponentIndex == other.m_uiComponentIndex && m_uiPartIndex == other.m_uiPartIndex; }

    // Cache entries need to be sorted by component index and then by part index
    W_ALWAYS_INLINE bool operator<(const RenderDataCacheEntry& other) const
    {
      if (m_uiComponentIndex == other.m_uiComponentIndex)
        return m_uiPartIndex < other.m_uiPartIndex;

      return m_uiComponentIndex < other.m_uiComponentIndex;
    }
  };
} // namespace WInternal

class W_RENDERERCORE_DLL WRenderViewContext : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WRenderViewContext, WReflectedClass);
  // Updates global constants and encoder with the viewport information of the view data.
  void UpdateViewport() const;

  const WRenderPipeline* m_pPipeline = nullptr;
  const WCamera* m_pCamera = nullptr;
  const WViewData* m_pViewData = nullptr;
  WRenderContext* m_pRenderContext = nullptr;

  const WDebugRendererContext* m_pWorldDebugContext = nullptr;
  const WDebugRendererContext* m_pViewDebugContext = nullptr;
};

using WViewId = WGenericId<24, 8>;

class WViewHandle
{
  W_DECLARE_HANDLE_TYPE(WViewHandle, WViewId);

  friend class WRenderWorld;
};

/// HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct WHashHelper<WViewHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  W_ALWAYS_INLINE static bool Equal(WViewHandle a, WViewHandle b) { return a == b; }
};

/// Usage hint of a camera/view.
struct W_RENDERERCORE_DLL WCameraUsageHint
{
  using StorageType = WUInt8;

  enum Enum
  {
    None,         ///< No hint, camera may not be used, at all.
    MainView,     ///< The main camera from which the scene gets rendered. There should only be one camera with this hint.
    EditorView,   ///< The editor view shall be rendered from this camera.
    RenderTarget, ///< The camera is used to render to a render target.
    Culling,      ///< This camera should be used for culling only. Usually culling is done from the main view, but with a dedicated culling camera, one can debug the culling system.
    Shadow,       ///< This camera is used for rendering shadow maps.
    Reflection,   ///< This camera is used for rendering reflections.
    Thumbnail,    ///< This camera should be used for rendering a scene thumbnail when exporting from the editor.

    ENUM_COUNT,

    Default = None,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WCameraUsageHint);

/// Declares that a texture needs to be in a specific resource state when a render category is rendered.
/// Recorded during extraction, applied during rendering to ensure correct barriers. m_uiCategory is the raw value of the WRenderData::Category this dependency belongs to. View-level dependencies ignore the category value.
struct WTextureDependency
{
  W_DECLARE_POD_TYPE();
  WGALTextureHandle m_hTexture;
  WBitflags<WGALResourceState> m_RequiredState;
  WBitflags<WGALShaderStageFlags> m_Stage;
  WUInt16 m_uiCategory = 0xFFFF;
};

/// Declares that a buffer needs to be in a specific resource state when a render category is rendered.
/// Recorded during extraction, applied during rendering to ensure correct barriers. m_uiCategory is the raw value of the WRenderData::Category this dependency belongs to. View-level dependencies ignore the category value.
struct WBufferDependency
{
  W_DECLARE_POD_TYPE();
  WGALBufferHandle m_hBuffer;
  WBitflags<WGALResourceState> m_RequiredState;
  WBitflags<WGALShaderStageFlags> m_Stage;
  WUInt16 m_uiCategory = 0xFFFF;
};

struct WTextureBinding
{
  W_DECLARE_POD_TYPE();
  WTempHashedString m_sSlotName;
  WTextureBindGroupItem m_Texture;
};

struct WBufferBinding
{
  W_DECLARE_POD_TYPE();
  WTempHashedString m_sSlotName;
  WGALBufferBindGroupItem m_Buffer;
};

struct WSamplerBinding
{
  W_DECLARE_POD_TYPE();
  WTempHashedString m_sSlotName;
  WSamplerBindGroupItem m_Sampler;
};