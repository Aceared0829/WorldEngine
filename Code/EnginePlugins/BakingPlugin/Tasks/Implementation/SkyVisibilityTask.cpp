#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerInterface.h>
#include <RendererCore/BakedProbes/BakingInterface.h>

using namespace WBakingInternal;

SkyVisibilityTask::SkyVisibilityTask(const WBakingSettings& settings, WTracerInterface& tracer, WArrayPtr<const WVec3> probePositions)
  : m_Settings(settings)
  , m_Tracer(tracer)
  , m_ProbePositions(probePositions)
{
}

SkyVisibilityTask::~SkyVisibilityTask() = default;

void SkyVisibilityTask::Execute()
{
  m_SkyVisibility.SetCountUninitialized(m_ProbePositions.GetCount());

  const WUInt32 uiNumSamples = m_Settings.m_uiNumSamplesPerProbe;
  WHybridArray<WTracerInterface::Ray, 128> rays(WFrameAllocator::GetCurrentAllocator());
  rays.SetCountUninitialized(uiNumSamples);

  WAmbientCube<float> weightNormalization;
  for (WUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
  {
    auto& ray = rays[uiSampleIndex];
    ray.m_vDir = WBakingUtils::FibonacciSphere(uiSampleIndex, uiNumSamples);
    ray.m_fDistance = m_Settings.m_fMaxRayDistance;

    weightNormalization.AddSample(ray.m_vDir, 1.0f);
  }

  for (WUInt32 i = 0; i < WAmbientCubeBasis::NumDirs; ++i)
  {
    weightNormalization.m_Values[i] = 1.0f / weightNormalization.m_Values[i];
  }

  WHybridArray<WTracerInterface::Hit, 128> hits(WFrameAllocator::GetCurrentAllocator());
  hits.SetCountUninitialized(uiNumSamples);

  for (WUInt32 uiProbeIndex = 0; uiProbeIndex < m_ProbePositions.GetCount(); ++uiProbeIndex)
  {
    WVec3 probePos = m_ProbePositions[uiProbeIndex];
    for (WUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      rays[uiSampleIndex].m_vStartPos = probePos;
    }

    m_Tracer.TraceRays(rays, hits);

    WAmbientCube<float> skyVisibility;
    for (WUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      const auto& ray = rays[uiSampleIndex];
      const auto& hit = hits[uiSampleIndex];
      const float value = hit.m_fDistance < 0.0f ? 1.0f : 0.0f;

      skyVisibility.AddSample(ray.m_vDir, value);
    }

    for (WUInt32 i = 0; i < WAmbientCubeBasis::NumDirs; ++i)
    {
      skyVisibility.m_Values[i] *= weightNormalization.m_Values[i];
    }
    auto& compressedSkyVisibility = m_SkyVisibility[uiProbeIndex];
    compressedSkyVisibility = WBakingUtils::CompressSkyVisibility(skyVisibility);
  }
}
