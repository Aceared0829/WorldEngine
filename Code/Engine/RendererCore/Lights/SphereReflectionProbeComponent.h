#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class W_RENDERERCORE_DLL WSphereReflectionProbeComponentManager final : public WComponentManager<class WSphereReflectionProbeComponent, WBlockStorageType::Compact>
{
public:
  WSphereReflectionProbeComponentManager(WWorld* pWorld);
};

//////////////////////////////////////////////////////////////////////////
// WSphereReflectionProbeComponent

/// Sphere reflection probe component.
///
/// The generated reflection cube map is is projected to infinity. So parallax correction takes place.
class W_RENDERERCORE_DLL WSphereReflectionProbeComponent : public WReflectionProbeComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WSphereReflectionProbeComponent, WReflectionProbeComponentBase, WSphereReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WSphereReflectionProbeComponent

public:
  WSphereReflectionProbeComponent();
  ~WSphereReflectionProbeComponent();

  void SetRadius(float fRadius);                                   // [ property ]
  float GetRadius() const;                                         // [ property ]

  void SetFalloff(float fFalloff);                                 // [ property ]
  float GetFalloff() const { return m_fFalloff; }                  // [ property ]

  void SetSphereProjection(bool bSphereProjection);                // [ property ]
  bool GetSphereProjection() const { return m_bSphereProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const WAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void OnTransformChanged(WMsgTransformChanged& msg);
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.1f;
  bool m_bSphereProjection = true;
};
