#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

using WDirectionalLightComponentManager = WComponentManager<class WDirectionalLightComponent, WBlockStorageType::Compact>;

/// The render data object for directional lights.
class W_RENDERERCORE_DLL WDirectionalLightRenderData : public WLightRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WDirectionalLightRenderData, WLightRenderData);

public:
  WVec3 m_vDirection;
  bool m_bScreenSpaceShadows;
};

/// A directional lightsource shines light into one fixed direction and has infinite size. It is usually used for sunlight.
///
/// It is very rare to use more than one directional lightsource at the same time.
/// Directional lightsources are used to fake the large scale light of the sun (or moon).
/// They use cascaded shadow maps to reduce the performance overhead for dynamic shadows of such large lights.
class W_RENDERERCORE_DLL WDirectionalLightComponent : public WLightComponent
{
  W_DECLARE_COMPONENT_TYPE(WDirectionalLightComponent, WLightComponent, WDirectionalLightComponentManager);

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
  // WDirectionalLightComponent

public:
  WDirectionalLightComponent();
  ~WDirectionalLightComponent();

  /// Sets whether to use screen space shadows. Improves contact shadows and self-shadowing for small objects.
  void SetScreenSpaceShadows(bool bShadows); // [ property ]
  bool GetScreenSpaceShadows() const;        // [ property ]

  /// Angular diameter of the emitter disc (the "sun disc") as seen from the ground.
  ///
  /// A non-zero value produces softer specular highlights via representative-point shading. Has no effect on attenuation since directional lights are treated as infinitely far away.
  /// Reference values: sun ≈ 0.53°, full moon ≈ 0.52°. Values above a few degrees are physically implausible but may be used for stylised looks.
  void SetSourceAngle(WAngle sourceAngle); // [ property ]
  WAngle GetSourceAngle() const;           // [ property ]


  /// Sets how many shadow map cascades to use. Typically between 2 and 4.
  void SetNumCascades(WUInt32 uiNumCascades); // [ property ]
  WUInt32 GetNumCascades() const;             // [ property ]

  /// Sets the distance around the main camera in which to apply dynamic shadows.
  void SetMinShadowRange(float fMinShadowRange); // [ property ]
  float GetMinShadowRange() const;               // [ property ]

  /// The factor (0 to 1) at which relative distance to start fading out the shadow map. Typically 0.8 or 0.9.
  void SetFadeOutStart(float fFadeOutStart); // [ property ]
  float GetFadeOutStart() const;             // [ property ]

  /// Has something to do with shadow map cascades (TODO: figure out what).
  void SetSplitModeWeight(float fSplitModeWeight); // [ property ]
  float GetSplitModeWeight() const;                // [ property ]

  /// Has something to do with shadow map cascades (TODO: figure out what).
  void SetNearPlaneOffset(float fNearPlaneOffset); // [ property ]
  float GetNearPlaneOffset() const;                // [ property ]

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  bool m_bScreenSpaceShadows = false;

  WAngle m_SourceAngle = WAngle::MakeFromDegree(0.0f);
  WUInt32 m_uiNumCascades = 3;
  float m_fMinShadowRange = 50.0f;
  float m_fFadeOutStart = 0.8f;
  float m_fSplitModeWeight = 0.7f;
  float m_fNearPlaneOffset = 100.0f;
};

/// Visualizer attribute for the angular size (source angle) of a directional light.
///
/// Shows a sphere as an intuitive size reference; its world-space size is proportional to the configured angle.
class W_RENDERERCORE_DLL WDirectionalLightVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDirectionalLightVisualizerAttribute, WVisualizerAttribute);

public:
  WDirectionalLightVisualizerAttribute();
  WDirectionalLightVisualizerAttribute(const char* szAngleProperty, const char* szColorProperty);

  const WUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty2; }
};
