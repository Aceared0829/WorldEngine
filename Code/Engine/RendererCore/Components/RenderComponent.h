#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

/// Base class for objects that should be rendered.
class W_RENDERERCORE_DLL WRenderComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WRenderComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  WRenderComponent();
  ~WRenderComponent();

  /// Called by WRenderComponent::OnUpdateLocalBounds().
  ///
  /// If W_SUCCESS is returned, out_bounds and out_bAlwaysVisible will be integrated into the WMsgUpdateLocalBounds ref_msg,
  /// otherwise the out values are simply ignored.
  virtual WResult GetLocalBounds(WBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) = 0;

  /// Call this when some value was modified that affects the size of the local bounding box and it should be recomputed.
  void TriggerLocalBoundsUpdate();

  /// Like TriggerLocalBoundsUpdate(), but defers the update and is safe to call from async update functions.
  void QueueLocalBoundsUpdate();

  /// Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  static WUInt32 GetUniqueIdForRendering(const WComponent& component);

  /// Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  W_ALWAYS_INLINE WUInt32 GetUniqueIdForRendering() const
  {
    return GetUniqueIdForRendering(*this);
  }

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
};
