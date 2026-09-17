#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class W_RENDERERCORE_DLL WBoxReflectionProbeComponentManager final : public WComponentManager<class WBoxReflectionProbeComponent, WBlockStorageType::Compact>
{
public:
  WBoxReflectionProbeComponentManager(WWorld* pWorld);
};

/// Box reflection probe component.
///
/// The generated reflection cube map is projected on a box defined by this component's extents. The influence volume can be smaller than the projection which is defined by a scale and shift parameter. Each side of the influence volume has a separate falloff parameter to smoothly blend the probe into others.
class W_RENDERERCORE_DLL WBoxReflectionProbeComponent : public WReflectionProbeComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WBoxReflectionProbeComponent, WReflectionProbeComponentBase, WBoxReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WBoxReflectionProbeComponent

public:
  WBoxReflectionProbeComponent();
  ~WBoxReflectionProbeComponent();

  const WVec3& GetExtents() const;                                       // [ property ]
  void SetExtents(const WVec3& vExtents);                                // [ property ]

  const WVec3& GetInfluenceScale() const;                                // [ property ]
  void SetInfluenceScale(const WVec3& vInfluenceScale);                  // [ property ]
  const WVec3& GetInfluenceShift() const;                                // [ property ]
  void SetInfluenceShift(const WVec3& vInfluenceShift);                  // [ property ]

  void SetPositiveFalloff(const WVec3& vFalloff);                        // [ property ]
  const WVec3& GetPositiveFalloff() const { return m_vPositiveFalloff; } // [ property ]
  void SetNegativeFalloff(const WVec3& vFalloff);                        // [ property ]
  const WVec3& GetNegativeFalloff() const { return m_vNegativeFalloff; } // [ property ]

  void SetBoxProjection(bool bBoxProjection);                             // [ property ]
  bool GetBoxProjection() const { return m_bBoxProjection; }              // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const WAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void OnTransformChanged(WMsgTransformChanged& msg);

protected:
  WVec3 m_vExtents = WVec3(5.0f);
  WVec3 m_vInfluenceScale = WVec3(1.0f);
  WVec3 m_vInfluenceShift = WVec3(0.0f);
  WVec3 m_vPositiveFalloff = WVec3(0.1f, 0.1f, 0.0f);
  WVec3 m_vNegativeFalloff = WVec3(0.1f, 0.1f, 0.0f);
  bool m_bBoxProjection = true;
};

/// A special visualizer attribute for box reflection probes
class W_RENDERERCORE_DLL WBoxReflectionProbeVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WBoxReflectionProbeVisualizerAttribute, WVisualizerAttribute);

public:
  WBoxReflectionProbeVisualizerAttribute();

  WBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty);

  const WUntrackedString& GetExtentsProperty() const { return m_sProperty1; }
  const WUntrackedString& GetInfluenceScaleProperty() const { return m_sProperty2; }
  const WUntrackedString& GetInfluenceShiftProperty() const { return m_sProperty3; }
};
