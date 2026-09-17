#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct WMsgExtractRenderData;

using WDebugTextComponentManager = WComponentManager<class WDebugTextComponent, WBlockStorageType::Compact>;

/// This component prints debug text at the owner object's position.
class W_RENDERERCORE_DLL WDebugTextComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WDebugTextComponent, WComponent, WDebugTextComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent
public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WDebugTextComponent
public:
  WDebugTextComponent();
  ~WDebugTextComponent();

  WString m_sText;                                               // [ property ]
  WColorGammaUB m_Color;                                         // [ property ]

  float m_fValue0 = 0.0f;                                         // [ property ]
  float m_fValue1 = 0.0f;                                         // [ property ]
  float m_fValue2 = 0.0f;                                         // [ property ]
  float m_fValue3 = 0.0f;                                         // [ property ]

  float m_fMaxDistance = 10.0f;                                   // [ property ]

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const; // [ msg handler ]
};
