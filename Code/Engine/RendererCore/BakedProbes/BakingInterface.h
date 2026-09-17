#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/RendererCoreDLL.h>

struct W_RENDERERCORE_DLL WBakingSettings
{
  WVec3 m_vProbeSpacing = WVec3(4);
  WUInt32 m_uiNumSamplesPerProbe = 128;
  float m_fMaxRayDistance = 1000.0f;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WBakingSettings);

class WWorld;

class WBakingInterface
{
public:
  /// Renders a debug view of the baking scene
  virtual WResult RenderDebugView(const WWorld& world, const WMat4& mInverseViewProjection, WUInt32 uiWidth, WUInt32 uiHeight, WDynamicArray<WColorGammaUB>& out_pixels, WProgress& ref_progress) const = 0;
};
