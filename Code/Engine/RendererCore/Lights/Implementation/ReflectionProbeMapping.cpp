#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

WCVarInt cvar_RenderingReflectionPoolSkyLightRefreshFrames("Rendering.ReflectionPool.SkyLightRefreshFrames", 60, WCVarFlags::Default, "How many frames must pass between two updates of a dynamic sky light. Each update invalidates every other reflection probe, so a low value keeps the scene busy re-rendering probes. Set to 0 to update the sky light every frame.");

WReflectionProbeMapping::WReflectionProbeMapping(WUInt32 uiAtlasSize)
  : m_uiAtlasSize(uiAtlasSize)
{
  m_MappedCubes.SetCount(m_uiAtlasSize);
  m_ActiveProbes.Reserve(m_uiAtlasSize);
  m_UnusedProbeSlots.Reserve(m_uiAtlasSize);
  m_AddProbes.Reserve(m_uiAtlasSize);

  W_ASSERT_DEV(m_hReflectionSpecularTexture.IsInvalidated(), "World data already created.");
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALTextureCreationDescription desc;
  desc.m_uiWidth = s_uiReflectionCubeMapSize;
  desc.m_uiHeight = s_uiReflectionCubeMapSize;
  desc.m_uiMipLevelCount = GetMipLevels();
  desc.m_uiArraySize = s_uiNumReflectionProbeCubeMaps;
  desc.m_Format = WGALResourceFormat::RGBAHalf;
  desc.m_Type = WGALTextureType::TextureCubeArray;
  desc.m_TextureFlags.Add(WGALTextureUsageFlags::UnorderedAccess | WGALTextureUsageFlags::RenderTarget);
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hReflectionSpecularTexture = pDevice->CreateTexture(desc);
  pDevice->GetTexture(m_hReflectionSpecularTexture)->SetDebugName("Reflection Specular Texture");
}

WReflectionProbeMapping::~WReflectionProbeMapping()
{
  W_ASSERT_DEV(!m_hReflectionSpecularTexture.IsInvalidated(), "World data not created.");
  WGALDevice::GetDefaultDevice()->DestroyTexture(m_hReflectionSpecularTexture);
  m_hReflectionSpecularTexture.Invalidate();
}

void WReflectionProbeMapping::AddProbe(WReflectionProbeId probe, WBitflags<WProbeFlags> flags)
{
  m_RegisteredProbes.EnsureCount(probe.m_InstanceIndex + 1);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  W_ASSERT_DEBUG(probeData.m_Flags == 0, "");
  probeData.m_id = probe;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(WProbeMappingFlags::Dirty);
  if (probeData.m_Flags.IsSet(WProbeMappingFlags::SkyLight))
  {
    m_SkyLight = probe;
    MapProbe(probe, 0);
  }
}

void WReflectionProbeMapping::UpdateProbe(WReflectionProbeId probe, WBitflags<WProbeFlags> flags)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (!probeData.m_Flags.IsSet(WProbeMappingFlags::SkyLight) && probeData.m_Flags.IsSet(WProbeMappingFlags::Dynamic) != flags.IsSet(WProbeFlags::Dynamic))
  {
    UnmapProbe(probe);
  }
  WBitflags<WProbeMappingFlags> preserveFlags = probeData.m_Flags & WProbeMappingFlags::Usable;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(preserveFlags | WProbeMappingFlags::Dirty);
}

void WReflectionProbeMapping::ProbeUpdateFinished(WReflectionProbeId probe)
{
  ProbeDataInternal& probeData0 = m_RegisteredProbes[probe.m_InstanceIndex];
  if (m_SkyLight == probe)
  {
    m_uiLastSkyLightUpdateFrame = WRenderWorld::GetFrameCounter();
    m_bSkyLightUpdatedOnce = true;
  }
  if (m_SkyLight == probe && probeData0.m_Flags.IsSet(WProbeMappingFlags::Dirty))
  {
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(WProbeMappingFlags::Dirty);
      }
    }
  }
  probeData0.m_Flags.Add(WProbeMappingFlags::Usable);
  probeData0.m_Flags.Remove(WProbeMappingFlags::Dirty);
}

void WReflectionProbeMapping::RemoveProbe(WReflectionProbeId probe)
{
  if (m_SkyLight == probe)
  {
    m_SkyLight.Invalidate();
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(WProbeMappingFlags::Dirty);
      }
    }
  }
  UnmapProbe(probe);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData = {};
}

WInt32 WReflectionProbeMapping::GetReflectionIndex(WReflectionProbeId probe, bool bForExtraction) const
{
  const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (bForExtraction && !probeData.m_Flags.IsSet(WProbeMappingFlags::Usable))
  {
    return -1;
  }
  return probeData.m_uiReflectionIndex;
}

void WReflectionProbeMapping::PreExtraction()
{
  // Reset priorities
  for (ProbeDataInternal& probeData : m_RegisteredProbes)
  {
    probeData.m_fPriority = 0.0f;
  }
  if (!m_SkyLight.IsInvalidated())
  {
    ProbeDataInternal& probeData = m_RegisteredProbes[m_SkyLight.m_InstanceIndex];
    probeData.m_fPriority = WMath::MaxValue<float>();
  }

  m_SortedProbes.Clear();
  m_ActiveProbes.Clear();
  m_UnusedProbeSlots.Clear();
  m_AddProbes.Clear();
}

void WReflectionProbeMapping::AddWeight(WReflectionProbeId probe, float fPriority)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData.m_fPriority = WMath::Max(probeData.m_fPriority, fPriority);
}

void WReflectionProbeMapping::PostExtraction()
{
  {
    // Sort all active non-skylight probes so we can find the best candidates to evict from the atlas.
    for (WUInt32 i = 1; i < s_uiNumReflectionProbeCubeMaps; i++)
    {
      auto id = m_MappedCubes[i];
      if (!id.IsInvalidated())
      {
        m_ActiveProbes.PushBack({id, m_RegisteredProbes[id.m_InstanceIndex].m_fPriority});
      }
      else
      {
        m_UnusedProbeSlots.PushBack(i);
      }
    }
    m_ActiveProbes.Sort();
  }

  {
    // Sort all exiting probes by priority.
    m_SortedProbes.Reserve(m_RegisteredProbes.GetCount());
    for (const ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated())
      {
        m_SortedProbes.PushBack({probeData.m_id, probeData.m_fPriority});
      }
    }
    m_SortedProbes.Sort();
  }

  {
    // Look at the first N best probes that would ideally be mapped in the atlas and find unmapped ones.
    const WUInt32 uiMaxCount = WMath::Min(m_uiAtlasSize, m_SortedProbes.GetCount());
    for (WUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_SortedProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];

      if (probeData.m_uiReflectionIndex < 0)
      {
        // We found a better probe to be mapped to the atlas.
        m_AddProbes.PushBack(probe);
      }
    }
  }

  {
    // Trigger resource loading of static or updates of dynamic probes.
    const WUInt32 uiMaxCount = m_AddProbes.GetCount();
    for (WUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_AddProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];
      // #TODO static probe resource loading
    }
  }

  // Unmap probes in case we need free slots using results from last frame
  {
    // Only unmap one probe per frame
    // #TODO better heuristic to decide how many if any should be unmapped.
    if (m_UnusedProbeSlots.GetCount() == 0 && m_AddProbes.GetCount() > 0)
    {
      const SortedProbes probe = m_ActiveProbes.PeekBack();
      UnmapProbe(probe.m_uiIndex);
    }
  }

  // Map probes with higher priority
  {
    const WUInt32 uiMaxCount = WMath::Min(m_AddProbes.GetCount(), m_UnusedProbeSlots.GetCount());
    for (WUInt32 i = 0; i < uiMaxCount; i++)
    {
      WInt32 iReflectionIndex = m_UnusedProbeSlots[i];
      const SortedProbes probe = m_AddProbes[i];
      MapProbe(probe.m_uiIndex, iReflectionIndex);
    }
  }

  // Enqueue probe updates
  {
    // Newly mapped probes were appended by MapProbe, so restore the priority order before enqueuing.
    m_ActiveProbes.Sort();

    // The sky light drives the ambient of every other probe and its completion marks them all dirty, so it
    // has to be requested before everything else. It is not part of m_ActiveProbes (it occupies the fixed
    // atlas index 0, which the loops above skip), so it is handled separately here.
    if (!m_SkyLight.IsInvalidated())
    {
      const ProbeDataInternal& skyLightData = m_RegisteredProbes[m_SkyLight.m_InstanceIndex];
      const bool bFirstBake = !skyLightData.m_Flags.IsSet(WProbeMappingFlags::Usable);

      // A sky light that has no content yet is always requested. Once it has content, refreshes are rate
      // limited, as each one invalidates all other probes and would otherwise keep the scene from settling.
      if (bFirstBake || IsSkyLightRefreshDue())
      {
        RequestUpdate(skyLightData);
      }
    }

    const WUInt32 uiMaxCount = m_ActiveProbes.GetCount();
    for (WUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes probe = m_ActiveProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];

      // #TODO Add static probes once resources are loaded.
      if (probeData.m_Flags.IsSet(WProbeMappingFlags::Dynamic) || probeData.m_Flags.IsSet(WProbeMappingFlags::Dirty))
      {
        RequestUpdate(probeData);
      }
    }
  }
}

void WReflectionProbeMapping::RequestUpdate(const ProbeDataInternal& probeData)
{
  WReflectionProbeMappingEvent e = {probeData.m_id, WReflectionProbeMappingEvent::Type::ProbeUpdateRequested};
  e.m_fPriority = probeData.m_fPriority;
  e.m_bFirstBake = !probeData.m_Flags.IsSet(WProbeMappingFlags::Usable);
  m_Events.Broadcast(e);
}

bool WReflectionProbeMapping::IsSkyLightRefreshDue() const
{
  if (!m_bSkyLightUpdatedOnce)
    return true;

  const WUInt64 uiInterval = (WUInt64)WMath::Max<WInt64>(0, cvar_RenderingReflectionPoolSkyLightRefreshFrames);

  const WUInt64 uiCurrentFrame = WRenderWorld::GetFrameCounter();
  return uiCurrentFrame >= m_uiLastSkyLightUpdateFrame + uiInterval;
}

void WReflectionProbeMapping::MapProbe(WReflectionProbeId id, WInt32 iReflectionIndex)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];

  probeData.m_uiReflectionIndex = iReflectionIndex;
  m_MappedCubes[probeData.m_uiReflectionIndex] = id;
  // Push the actual priority so that the update enqueue order below stays sorted. A freshly mapped probe
  // has no content yet, so sorting it to the back would delay exactly the probes that need an update most.
  m_ActiveProbes.PushBack({id, probeData.m_fPriority});

  WReflectionProbeMappingEvent e = {id, WReflectionProbeMappingEvent::Type::ProbeMapped};
  m_Events.Broadcast(e);
}

void WReflectionProbeMapping::UnmapProbe(WReflectionProbeId id)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];
  if (probeData.m_uiReflectionIndex != -1)
  {
    m_MappedCubes[probeData.m_uiReflectionIndex].Invalidate();
    probeData.m_uiReflectionIndex = -1;

    // The atlas slot is given up, so whatever content was rendered into it is gone. If the probe is mapped
    // again later it has to be treated as a first bake, otherwise it would be sampled before being rendered.
    probeData.m_Flags.Remove(WProbeMappingFlags::Usable);
    probeData.m_Flags.Add(WProbeMappingFlags::Dirty);

    WReflectionProbeMappingEvent e = {id, WReflectionProbeMappingEvent::Type::ProbeUnmapped};
    m_Events.Broadcast(e);
  }
}
