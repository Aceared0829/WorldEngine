#pragma once

#include <BakingPlugin/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>

struct WBakingSettings;

namespace WBakingInternal
{
  class W_BAKINGPLUGIN_DLL PlaceProbesTask : public WTask
  {
  public:
    PlaceProbesTask(const WBakingSettings& settings, const WBoundingBox& bounds, WArrayPtr<const Volume> volumes);
    ~PlaceProbesTask();

    virtual void Execute() override;

    WArrayPtr<const WVec3> GetProbePositions() const { return m_ProbePositions; }
    const WVec3& GetGridOrigin() const { return m_vGridOrigin; }
    const WVec3U32& GetProbeCount() const { return m_vProbeCount; }

  private:
    const WBakingSettings& m_Settings;

    WBoundingBox m_Bounds;
    WArrayPtr<const Volume> m_Volumes;

    WVec3 m_vGridOrigin = WVec3::MakeZero();
    WVec3U32 m_vProbeCount = WVec3U32::MakeZero();
    WDynamicArray<WVec3> m_ProbePositions;
  };
} // namespace WBakingInternal
