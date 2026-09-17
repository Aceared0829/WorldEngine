#pragma once

#include <Foundation/Math/Float16.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

using WSpotLightComponentManager = WComponentManager<class WSpotLightComponent, WBlockStorageType::Compact>;

/// The render data object for spot lights.
class W_RENDERERCORE_DLL WSpotLightRenderData : public WLightRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WSpotLightRenderData, WLightRenderData);

public:
  WQuat m_qGlobalRotation;
  float m_fRange;
  WAngle m_InnerSpotAngle;
  WAngle m_OuterSpotAngle;
  WDecalId m_CookieId;
};

/// Adds a spotlight to the scene, optionally casting shadows.
class W_RENDERERCORE_DLL WSpotLightComponent : public WLightComponent
{
  W_DECLARE_COMPONENT_TYPE(WSpotLightComponent, WLightComponent, WSpotLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // WSpotLightComponent

public:
  WSpotLightComponent();
  ~WSpotLightComponent();

  /// Sets the radius (or length of the cone) of the lightsource. If zero, it is automatically determined from the intensity.
  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  /// Returns the final radius of the lightsource.
  float GetEffectiveRange() const;

  /// Radius of the emitter disc at the spot light's origin. A non-zero value produces softer specular highlights and area-light style shading. Does not affect attenuation.
  void SetRadius(float fRadius); // [ property ]
  float GetRadius() const;       // [ property ]

  /// Sets the radius that is used to determine when to fade out shadows. If zero the radius of the lightsource is used.
  void SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;      // [ property ]

  /// Sets the inner angle where the spotlight has equal brightness.
  void SetInnerSpotAngle(WAngle spotAngle); // [ property ]
  WAngle GetInnerSpotAngle() const;         // [ property ]

  /// Sets the outer angle of the spotlight's cone. The light will fade out between the inner and outer angle.
  void SetOuterSpotAngle(WAngle spotAngle);                               // [ property ]
  WAngle GetOuterSpotAngle() const;                                       // [ property ]

  void SetCookie(const WTexture2DResourceHandle& hCookie);                // [ property ]
  const WTexture2DResourceHandle& GetCookie() const { return m_hCookie; } // [ property ]

  // adds SetCookieFile() and GetCookieFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Cookie, m_hCookie, SetCookie);

  void SetMaterial(const WMaterialResourceHandle& hMaterial);                // [ property ]
  const WMaterialResourceHandle& GetMaterial() const { return m_hMaterial; } // [ property ]

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Material, m_hMaterial, SetMaterial);

  void SetMaterialResolution(WUInt32 uiResolution);                                                     // [ property ]
  WUInt32 GetMaterialResolution() const { return m_uiMaterialResolution; }                              // [ property ]

  void SetMaterialUpdateInterval(WTime updateInterval);                                                 // [ property ]
  WTime GetMaterialUpdateInterval() const { return WTime::MakeFromSeconds(m_MaterialUpdateInterval); } // [ property ]

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  WBoundingSphere CalculateBoundingSphere(const WTransform& t, float fRange) const;

  void UpdateCookie();
  void DeleteCookie();

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;
  float m_fShadowFadeOutRange = 0.0f;
  float m_fRadius = 0.0f;

  WAngle m_InnerSpotAngle = WAngle::MakeFromDegree(15.0f);
  WAngle m_OuterSpotAngle = WAngle::MakeFromDegree(30.0f);

  WUInt16 m_uiMaterialResolution = 512;
  WFloat16 m_MaterialUpdateInterval = 0.0f;
  WMaterialResourceHandle m_hMaterial;

  WTexture2DResourceHandle m_hCookie;

  WDecalId m_CookieId;
};

/// A special visualizer attribute for spot lights
class W_RENDERERCORE_DLL WSpotLightVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WSpotLightVisualizerAttribute, WVisualizerAttribute);

public:
  WSpotLightVisualizerAttribute();
  WSpotLightVisualizerAttribute(
    const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty, const char* szRadiusProperty = nullptr);

  const WUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const WUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty4; }
  const WUntrackedString& GetRadiusProperty() const { return m_sProperty5; }
};
