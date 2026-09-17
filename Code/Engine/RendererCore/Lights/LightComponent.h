#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgSetColor;

/// Base class for light render data objects.
class W_RENDERERCORE_DLL WLightRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WLightRenderData, WRenderData);

public:
  static constexpr WUInt32 s_uiBaseSortingKey = 0x10000;

  virtual bool CanBatch(const WRenderData& other) const override;
  void FillSortingKey(float fScreenSpaceSize);
  void FillShadowDataOffsetAndFadeOut(WUInt32 uiDataOffset, float fFadeOut);

  WColorLinearUB m_LightColor;
  float m_fIntensity;
  float m_fSpecularMultiplier;
  float m_fRadius;
  WUInt32 m_uiShadowDataOffsetAndFadeOut;
};

/// Base class for dynamic light components.
class W_RENDERERCORE_DLL WLightComponent : public WRenderComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WLightComponent, WRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WLightComponent

public:
  WLightComponent();
  ~WLightComponent();

  /// Used to enable kelvin color values. This is a physical representation of light color using.
  /// for more detail: https://wikipedia.org/wiki/Color_temperature
  void SetUsingColorTemperature(bool bUseColorTemperature);
  bool GetUsingColorTemperature() const;

  void SetTemperature(WUInt32 uiTemperature);   // [ property ]
  WUInt32 GetTemperature() const;               // [ property ]

  void SetLightColor(WColorGammaUB lightColor); // [ property ]
  WColorGammaUB GetLightColor() const;          // [ property ]

  WColorGammaUB GetEffectiveColor() const;

  /// Sets the brightness of the light source.
  void SetIntensity(float fIntensity);                   // [ property ]
  float GetIntensity() const;                            // [ property ]

  void SetSpecularMultiplier(float fSpecularMultiplier); // [ property ]
  float GetSpecularMultiplier() const;                   // [ property ]

  /// Sets whether the light source shall cast dynamic shadows.
  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  /// Sets whether transparent objects should cast shadows (emulated through dithering).
  void SetTransparentShadows(bool bShadows); // [ property ]
  bool GetTransparentShadows() const;        // [ property ]

  /// Sets the fuzziness of the shadow edges.
  void SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;             // [ property ]

  /// Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;           // [ property ]

  /// Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetConstantBias(float fShadowBias);    // [ property ]
  float GetConstantBias() const;              // [ property ]

  void OnMsgSetColor(WMsgSetColor& ref_msg); // [ msg handler ]

  /// Calculates how far a light source would shine given the specified range and intensity.
  ///
  /// If fRange is zero, the range needed for the given intensity is returned.
  /// Otherwise the smaller value of that and fRange is returned.
  static float CalculateEffectiveRange(float fRange, float fIntensity);

  /// Calculates how large on screen (relative height) the light source would be.
  static float CalculateScreenSpaceSize(const WBoundingSphere& sphere, const WCamera& camera);

protected:
  float CalculateShadowFadeOut(const WBoundingSphere& sphere, float fShadowFadeOutRange, const WCamera& camera, float& out_fShadowScreenSize) const;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  void VisualizeScreenSpaceSize(WViewHandle hView, const WBoundingSphere& sphere, float fScreenSize, float fShadowScreenSize, float fShadowFadeOut) const;
#endif

  WColorGammaUB m_LightColor = WColor::White;
  WUInt32 m_uiTemperature = 6550;
  float m_fIntensity = 10.0f;
  float m_fSpecularMultiplier = 1.0f;
  float m_fPenumbraSize = 0.05f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
  bool m_bTransparentShadows = false;
  bool m_bUseColorTemperature = false;
};
