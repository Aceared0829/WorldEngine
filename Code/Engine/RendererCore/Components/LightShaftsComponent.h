#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgUpdateLocalBounds;

class W_RENDERERCORE_DLL WLightShaftsRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WLightShaftsRenderData, WRenderData);

public:
  WVec3 m_vDirection;
  float m_fIntensity;
  float m_fMaxBrightness;
  float m_fBrightnessThreshold;
  float m_fDiskMaskRadius;
  WColorGammaUB m_TintColor;
};

using WLightShaftsComponentManager = WSettingsComponentManager<class WLightShaftsComponent>;

/// Adds a light shaft effect to the scene. Usually, this component is attached to the same game object as a directional light.
class W_RENDERERCORE_DLL WLightShaftsComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WLightShaftsComponent, WRenderComponent, WLightShaftsComponentManager);

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
  // WLightShaftsComponent

public:
  WLightShaftsComponent();
  ~WLightShaftsComponent();

  /// Sets the intensity of the light shafts.
  void SetIntensity(float fIntensity);                // [ property ]
  float GetIntensity() const { return m_fIntensity; } // [ property ]

  /// Sets the brightness threshold for the light shafts. Only pixels brighter than this value will contribute to the light shafts.
  void SetBrightnessThreshold(float fBrightnessThreshold);                // [ property ]
  float GetBrightnessThreshold() const { return m_fBrightnessThreshold; } // [ property ]

  /// The maximum brightness is used to prevent excessively bright light shafts, which can cause visual artifacts.
  void SetMaxBrightness(float fMaxBrightness);                // [ property ]
  float GetMaxBrightness() const { return m_fMaxBrightness; } // [ property ]

  /// The disk is used to mask only a small area around the sun as source for the light shafts.
  ///
  /// This is only needed if the sun has no other visual representation in the sky and the brightness threshold is too low to prevent artifacts.
  /// The radius is given in relative screen space, where 0.1 means 10% of the screen height.
  void SetDiskMaskRadius(float fDiskMaskRadius);                // [ property ]
  float GetDiskMaskRadius() const { return m_fDiskMaskRadius; } // [ property ]

  /// The light shafts pick its color from the sky pixels at the center. The tint color can be used to add an additional color tint to the light shafts.
  void SetTintColor(const WColorGammaUB& color);                    // [ property ]
  const WColorGammaUB& GetTintColor() const { return m_TintColor; } // [ property ]

private:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  float m_fIntensity = 1.0f;
  float m_fMaxBrightness = 10.0f;
  float m_fBrightnessThreshold = 0.0f;
  float m_fDiskMaskRadius = 0.1f;

  WColorGammaUB m_TintColor = WColor::White;
};
