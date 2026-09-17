#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

struct WMsgExtractRenderData;

using WRenderTargetComponentManager = WComponentManager<class WRenderTargetActivatorComponent, WBlockStorageType::Compact>;

/// Attach this component to an object that uses a render target for reading, to ensure that the render target gets written to.
///
/// If you build a monitor that displays the output of a security camera in your level, the engine needs to know when it should
/// update the render target that displays the security camera footage, and when it can skip that part to not waste performance.
/// Thus, by default, the engine will not update the render target, as long as this isn't requested.
/// This component implements this request functionality.
///
/// It is a render component, which means that it tracks when it is visible and when visible, it will 'activate' the desired
/// render target, so that it will be updated.
/// By attaching it to an object, like the monitor, it activates the render target whenever the monitor object itself gets rendered.
class W_RENDERERCORE_DLL WRenderTargetActivatorComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WRenderTargetActivatorComponent, WRenderComponent, WRenderTargetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent
public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // WRenderTargetActivatorComponent

public:
  WRenderTargetActivatorComponent();
  ~WRenderTargetActivatorComponent();

  /// Sets the WRenderToTexture2DResource to render activate.
  void SetRenderTarget(const WRenderToTexture2DResourceHandle& hResource);                    // [ property ]
  const WRenderToTexture2DResourceHandle& GetRenderTarget() const { return m_hRenderTarget; } // [ property ]

private:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  WRenderToTexture2DResourceHandle m_hRenderTarget;
};
