#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/RendererCoreDLL.h>

struct WMsgUpdateLocalBounds;

using WAmbientLightComponentManager = WSettingsComponentManager<class WAmbientLightComponent>;

class W_RENDERERCORE_DLL WAmbientLightComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WAmbientLightComponent, WComponent, WAmbientLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WAmbientLightComponent

public:
  WAmbientLightComponent();
  ~WAmbientLightComponent();

  void SetTopColor(WColorGammaUB color);    // [ property ]
  WColorGammaUB GetTopColor() const;        // [ property ]

  void SetBottomColor(WColorGammaUB color); // [ property ]
  WColorGammaUB GetBottomColor() const;     // [ property ]

  void SetIntensity(float fIntensity);       // [ property ]
  float GetIntensity() const;                // [ property ]

private:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);
  void UpdateSkyIrradiance();

  WColorGammaUB m_TopColor = WColor(0.2f, 0.2f, 0.3f);
  WColorGammaUB m_BottomColor = WColor(0.1f, 0.1f, 0.15f);
  float m_fIntensity = 1.0f;
};
