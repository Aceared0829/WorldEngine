#pragma once

#include <ProcGenPlugin/Components/ProcVolumeComponent.h>

struct WMsgSplineChanged;
struct WMsgExtractRenderData;
struct WDebugRendererLine;
class WSplineComponent;

using WProcVolumeSplineComponentManager = WComponentManager<class WProcVolumeSplineComponent, WBlockStorageType::Compact>;

class W_PROCGENPLUGIN_DLL WProcVolumeSplineComponent : public WProcVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WProcVolumeSplineComponent, WProcVolumeComponent, WProcVolumeSplineComponentManager);

public:
  WProcVolumeSplineComponent();
  ~WProcVolumeSplineComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void SetFalloff(float fFalloff);

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  void OnMsgSplineChanged(WMsgSplineChanged& ref_msg);
  void OnMsgUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const;
  void OnMsgExtractVolumes(WMsgExtractVolumes& ref_msg) const;
  void OnMsgExtractRenderData(WMsgExtractRenderData& ref_msg) const;

protected:
  const WSplineComponent* GetSplineComponent() const;

  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;

  WUInt32 m_uiLastChangeCounter = 0;

  mutable WDynamicArray<WDebugRendererLine> m_DebugLines;
};
