#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureCubeResource.h>

using WPointLightComponentManager = WComponentManager<class WPointLightComponent, WBlockStorageType::Compact>;

/// The render data object for point lights.
class W_RENDERERCORE_DLL WPointLightRenderData : public WLightRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WPointLightRenderData, WLightRenderData);

public:
  float m_fRange;
  float m_fLength;
  WQuat m_qGlobalRotation;
};

/// Adds a dynamic point light to the scene, optionally casting shadows.
///
/// For performance reasons, prefer to use WSpotLightComponent where possible.
/// Do not use shadows just to limit the light cone, when a spot light could achieve the same.
class W_RENDERERCORE_DLL WPointLightComponent : public WLightComponent
{
  W_DECLARE_COMPONENT_TYPE(WPointLightComponent, WLightComponent, WPointLightComponentManager);

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
  // WPointLightComponent

public:
  WPointLightComponent();
  ~WPointLightComponent();

  /// Sets the radius of the lightsource. If zero, the radius is automatically determined from the intensity.
  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  /// Returns the final radius of the lightsource.
  float GetEffectiveRange() const;

  /// Sets the length of the tube. Zero means the light is a point light.
  void SetLength(float fLength); // [ property ]
  float GetLength() const;       // [ property ]

  /// Radius of the tube's cross-section. Affects the size of specular highlights. Zero means the light is a point light.
  void SetRadius(float fRadius); // [ property ]
  float GetRadius() const;       // [ property ]

  /// Sets the radius that is used to determine when to fade out shadows. If zero the radius of the lightsource is used.
  void SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;      // [ property ]

  // void SetProjectedTextureFile(const char* szFile); // [ property ]
  // const char* GetProjectedTextureFile() const;      // [ property ]

  // void SetProjectedTexture(const WTextureCubeResourceHandle& hProjectedTexture);
  // const WTextureCubeResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  float m_fLength = 0.0f;
  float m_fRadius = 0.0f;
  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;
  float m_fShadowFadeOutRange = 0.0f;

  // WTextureCubeResourceHandle m_hProjectedTexture;
};

/// Visualizer attribute for point lights. Also renders a tube (capsule) when Length or Radius is non-zero.
class W_RENDERERCORE_DLL WPointLightVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WPointLightVisualizerAttribute, WVisualizerAttribute);

public:
  WPointLightVisualizerAttribute();
  WPointLightVisualizerAttribute(
    const char* szLengthProperty, const char* szRadiusProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const WUntrackedString& GetLengthProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const WUntrackedString& GetRangeProperty() const { return m_sProperty3; }
  const WUntrackedString& GetIntensityProperty() const { return m_sProperty4; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty5; }
};
