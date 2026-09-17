#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgSetColor;

struct WFillLightMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Additive,
    Subtractive,
    ModulateIndirect,

    Default = Additive
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WFillLightMode);

/// The render data object for fill lights.
class W_RENDERERCORE_DLL WFillLightRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WFillLightRenderData, WRenderData);

public:
  void FillSortingKey(float fScreenSpaceSize);

  WColorLinearUB m_LightColor;
  WEnum<WFillLightMode> m_LightMode;
  float m_fIntensity;
  float m_fRange;
  float m_fFalloffExponent;
  float m_fDirectionality;
};

using WFillLightComponentManager = WComponentManager<class WFillLightComponent, WBlockStorageType::Compact>;

/// Adds a fill light to the scene. This can be used to simulate bounced light or to light up dark areas.
/// It can also be used to modulate the indirect lighting.
class W_RENDERERCORE_DLL WFillLightComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WFillLightComponent, WRenderComponent, WFillLightComponentManager);

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
  // WFillLightComponent

public:
  WFillLightComponent();
  ~WFillLightComponent();

  /// In Additive mode the fill light adds light to scene like a regular light source.
  /// In ModulateIndirect mode it acts as a multiplier to the indirect light.
  void SetLightMode(WEnum<WFillLightMode> mode);                     // [ property ]
  WEnum<WFillLightMode> GetLightMode() const { return m_LightMode; } // [ property ]

  /// Used to enable kelvin color values. This is a physical representation of light color using.
  /// for more detail: https://wikipedia.org/wiki/Color_temperature
  void SetUsingColorTemperature(bool bUseColorTemperature);                // [ property ]
  bool GetUsingColorTemperature() const { return m_bUseColorTemperature; } // [ property ]

  void SetTemperature(WUInt32 uiTemperature);                             // [ property ]
  WUInt32 GetTemperature() const { return m_uiTemperature; }              // [ property ]

  void SetLightColor(WColorGammaUB lightColor);                           // [ property ]
  WColorGammaUB GetLightColor() const { return m_LightColor; }            // [ property ]

  WColorGammaUB GetEffectiveColor() const;

  /// In Additive mode this controls the brightness of the light source.
  /// In ModulateIndirect mode light color times intensity is multiplied with the indirect light,
  /// thus values below 1 darken the indirect light, values above 1 brighten the indirect light.
  void SetIntensity(float fIntensity);                // [ property ]
  float GetIntensity() const { return m_fIntensity; } // [ property ]

  /// Sets the radius of the light source.
  void SetRange(float fRange);                                    // [ property ]
  float GetRange() const { return m_fRange; }                     // [ property ]

  void SetFalloffExponent(float fFalloffExponent);                // [ property ]
  float GetFalloffExponent() const { return m_fFalloffExponent; } // [ property ]

  /// Controls how much the light wraps to the backside of lit objects.
  /// A directionality of 1 means no light will wrap to the backside and
  /// with a directionality of 0 light will equaly lit front and backsides.
  void SetDirectionality(float fDirectionality);                // [ property ]
  float GetDirectionality() const { return m_fDirectionality; } // [ property ]

protected:
  void OnMsgSetColor(WMsgSetColor& ref_msg);
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  WColorGammaUB m_LightColor = WColor::White;
  WUInt32 m_uiTemperature = 6550;
  float m_fIntensity = 10.0f;
  float m_fRange = 5.0f;
  float m_fFalloffExponent = 1.0f;
  float m_fDirectionality = 1.0f;
  WEnum<WFillLightMode> m_LightMode;
  bool m_bUseColorTemperature = false;
};
