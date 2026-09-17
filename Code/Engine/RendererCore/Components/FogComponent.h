#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgUpdateLocalBounds;

/// The render data object for height fog.
class W_RENDERERCORE_DLL WFogRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WFogRenderData, WRenderData);

public:
  WColor m_Color;
  float m_fDensity;
  float m_fBaseHeight;
  float m_fHeightFalloff;
  float m_fInvSkyDistance;
  float m_fFogStartDistance;
};

using WFogComponentManager = WSettingsComponentManager<class WFogComponent>;

class W_RENDERERCORE_DLL WFogComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WFogComponent, WComponent, WFogComponentManager);

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
  // WFogComponent

public:
  WFogComponent();
  ~WFogComponent();

  void SetColor(WColor color);                 // [ property ]
  WColor GetColor() const;                     // [ property ]

  void SetDensity(float fDensity);              // [ property ]
  float GetDensity() const;                     // [ property ]

  void SetHeightFalloff(float fHeightFalloff);  // [ property ]
  float GetHeightFalloff() const;               // [ property ]

  void SetModulateWithSkyColor(bool bModulate); // [ property ]
  bool GetModulateWithSkyColor() const;         // [ property ]

  void SetSkyDistance(float fDistance);         // [ property ]
  float GetSkyDistance() const;                 // [ property ]

  void SetStartDistance(float fDistance);       // [ property ]
  float GetStartDistance() const;               // [ property ]

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  WColor m_Color = WColor(0.2f, 0.2f, 0.3f);
  float m_fDensity = 1.0f;
  float m_fHeightFalloff = 10.0f;
  float m_fSkyDistance = 1000.0f;
  float m_fStartDistance = 0.0f;
  bool m_bModulateWithSkyColor = false;
};
