#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/Threading/TaskSystem.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

struct WBakingSettings;
class WTracerInterface;

namespace WBakingInternal
{
  class W_BAKINGPLUGIN_DLL SkyVisibilityTask : public WTask
  {
  public:
    SkyVisibilityTask(const WBakingSettings& settings, WTracerInterface& tracer, WArrayPtr<const WVec3> probePositions);
    ~SkyVisibilityTask();

    virtual void Execute() override;

    WArrayPtr<const WCompressedSkyVisibility> GetSkyVisibility() const { return m_SkyVisibility; }

  private:
    const WBakingSettings& m_Settings;

    WTracerInterface& m_Tracer;
    WArrayPtr<const WVec3> m_ProbePositions;

    WDynamicArray<WCompressedSkyVisibility> m_SkyVisibility;
  };
} // namespace WBakingInternal
