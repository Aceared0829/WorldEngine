#pragma once

#include <RendererCore/Components/RenderComponent.h>

using WAlwaysVisibleComponentManager = WComponentManager<class WAlwaysVisibleComponent, WBlockStorageType::Compact>;

/// Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class W_RENDERERCORE_DLL WAlwaysVisibleComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WAlwaysVisibleComponent, WRenderComponent, WAlwaysVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WAlwaysVisibleComponent

public:
  WAlwaysVisibleComponent();
  ~WAlwaysVisibleComponent();
};
