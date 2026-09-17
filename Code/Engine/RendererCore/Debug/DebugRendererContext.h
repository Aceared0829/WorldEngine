#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <RendererCore/RendererCoreDLL.h>

class WWorld;
class WViewHandle;

/// Value used by containers for indices to indicate an invalid index.
#ifndef WInvalidIndex
#  define WInvalidIndex 0xFFFFFFFF
#endif

/// Used in WDebugRenderer to determine where debug geometry should be rendered
class W_RENDERERCORE_DLL WDebugRendererContext
{
public:
  WDebugRendererContext() = default;

  /// If this constructor is used, the geometry is rendered in all views for that scene.
  WDebugRendererContext(const WWorld* pWorld);

  /// If this constructor is used, the geometry is only rendered in this view.
  WDebugRendererContext(const WViewHandle& hView);

  W_ALWAYS_INLINE bool operator==(const WDebugRendererContext& other) const { return m_uiId == other.m_uiId; }

private:
  friend struct WHashHelper<WDebugRendererContext>;

  WUInt32 m_uiId = WInvalidIndex;
};


template <>
struct WHashHelper<WDebugRendererContext>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WDebugRendererContext value) { return WHashHelper<WUInt32>::Hash(value.m_uiId); }

  W_ALWAYS_INLINE static bool Equal(WDebugRendererContext a, WDebugRendererContext b) { return a == b; }
};
