#pragma once

#include <Foundation/Types/Id.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Forward declarations
class WRenderGraph;
class WRenderGraphPassBuilder;
class WRenderGraphContext;
class WRenderGraphManager;
struct WRenderGraphInspectionInfo;
struct WRenderGraphDebugTarget;
class WRenderGraphResourcePool;
class WRenderGraphResourceAllocator;
class WPooledRenderTexture;
class WPooledRenderBuffer;
struct WRenderGraphInspectionSummary;

/// Opaque handle to a texture within a render graph.
class WRenderGraphTextureHandle
{
  W_DECLARE_HANDLE_TYPE(WRenderGraphTextureHandle, WGAL::ez18_14Id);
  friend class WRenderGraph;
  friend class WRenderGraphPassBuilder;
  friend class WRenderGraphManager;
};

/// Opaque handle to a buffer within a render graph.
class WRenderGraphBufferHandle
{
  W_DECLARE_HANDLE_TYPE(WRenderGraphBufferHandle, WGAL::ez18_14Id);
  friend class WRenderGraph;
  friend class WRenderGraphPassBuilder;
  friend class WRenderGraphManager;
};

template <>
struct WHashHelper<WRenderGraphTextureHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WRenderGraphTextureHandle value)
  {
    return WHashHelper<WRenderGraphTextureHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  W_ALWAYS_INLINE static bool Equal(WRenderGraphTextureHandle a, WRenderGraphTextureHandle b)
  {
    return a == b;
  }
};

template <>
struct WHashHelper<WRenderGraphBufferHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WRenderGraphBufferHandle value)
  {
    return WHashHelper<WRenderGraphBufferHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  W_ALWAYS_INLINE static bool Equal(WRenderGraphBufferHandle a, WRenderGraphBufferHandle b)
  {
    return a == b;
  }
};



/// Coarse execution phase for render graphs. Graphs within the same phase execute in registration (FIFO) order.
struct WRenderGraphPhase
{
  using StorageType = WUInt8;

  enum Enum
  {
    PreRender,  ///< E.g. Subsystem setup: decals, shadows, reflections, RmlUi texture updates.
    Render,     ///< E.g. Main view rendering via render pipelines.
    PostRender, ///< E.g. Companion view blits, screen captures, readbacks.
    Default = Render
  };
};
