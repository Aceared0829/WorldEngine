#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeUpdater.h>
#include <RendererCore/Pipeline/View.h>

class WSkyLightComponent;
class WSphereReflectionProbeComponent;
class WBoxReflectionProbeComponent;
class WRenderGraph;

static constexpr WUInt32 s_uiReflectionCubeMapSize = 128;
static constexpr WUInt32 s_uiNumReflectionProbeCubeMaps = 32;
static constexpr float s_fDebugSphereRadius = 0.3f;

inline WUInt32 GetMipLevels()
{
  return WMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
}

//////////////////////////////////////////////////////////////////////////
/// WReflectionPool::Data

struct WReflectionPool::Data
{
  Data();
  ~Data();

  struct ProbeData
  {
    WReflectionProbeDesc m_desc;
    WTransform m_GlobalTransform;
    WBitflags<WProbeFlags> m_Flags;
    WInstanceDataOffset m_DebugInstanceDataOffset;
    WTextureCubeResourceHandle m_hCubeMap; // static data or empty for dynamic.
  };

  struct WorldReflectionData
  {
    WorldReflectionData()
      : m_mapping(s_uiNumReflectionProbeCubeMaps)
    {
    }
    W_DISALLOW_COPY_AND_ASSIGN(WorldReflectionData);

    WIdTable<WReflectionProbeId, ProbeData> m_Probes;
    WReflectionProbeId m_SkyLight; // SkyLight is always fixed at reflectionIndex 0.
    WEventSubscriptionID m_mappingSubscriptionId = 0;
    WReflectionProbeMapping m_mapping;
  };

  // WorldReflectionData management
  WReflectionProbeId AddProbe(const WWorld* pWorld, ProbeData&& probeData);
  WReflectionPool::Data::WorldReflectionData& GetWorldData(const WWorld* pWorld);
  void RemoveProbe(const WWorld* pWorld, WReflectionProbeId id);
  void UpdateProbeData(ProbeData& ref_probeData, const WReflectionProbeDesc& desc, const WReflectionProbeComponentBase* pComponent);
  bool UpdateSkyLightData(ProbeData& ref_probeData, const WReflectionProbeDesc& desc, const WSkyLightComponent* pComponent);
  void OnReflectionProbeMappingEvent(const WUInt32 uiWorldIndex, const WReflectionProbeMappingEvent& e);

  void PreExtraction();
  void PostExtraction();

  // Update Queues (all worlds combined)

  struct QueuedUpdate
  {
    W_DECLARE_POD_TYPE();

    WReflectionProbeRef m_probe;
    float m_fPriority = 0.0f;
    WUInt64 m_uiEnqueuedFrame = 0; ///< Only used by m_RefreshQueue, to age waiting probes.
  };

  /// Removes a probe from one of the update queues, if it is in there.
  static void RemoveFromQueue(WDynamicArray<QueuedUpdate>& ref_queue, const WReflectionProbeRef& probe);

  /// Sorts a queue so that the probe to update next is at the front.
  /// \param fAgeWeight How much a probe's priority grows per frame that it has been waiting. Zero sorts
  ///   purely by priority, which can starve low priority probes.
  static void SortQueue(WDynamicArray<QueuedUpdate>& ref_queue, float fAgeWeight);

  // Probes that have never completed an update and thus have no usable content yet. Sorted by priority and
  // drained before m_RefreshQueue, so that a probe cannot be starved by probes that already look correct.
  WDynamicArray<QueuedUpdate> m_InitialBakeQueue;

  // Probes that already have content and want to re-render it. Ordered by priority as well, so that the
  // probes around the camera are corrected first after a sky light update dirtied everything. The priority
  // is raised the longer a probe waits, so that distant probes cannot be starved by closer ones.
  WDynamicArray<QueuedUpdate> m_RefreshQueue;

  // Dedup across both queues. A probe is in at most one of them.
  WHashSet<WReflectionProbeRef> m_PendingDynamicUpdate;

  WHashSet<WReflectionProbeRef> m_ActiveDynamicUpdate;
  WReflectionProbeUpdater m_ReflectionProbeUpdater;

  void CreateReflectionViewsAndResources();
  void CreateSkyIrradianceTexture();

  WMutex m_Mutex;
  WUInt64 m_uiWorldHasSkyLight = 0;
  WUInt64 m_uiSkyIrradianceChanged = 0;
  WHybridArray<WUniquePtr<WorldReflectionData>, 2> m_WorldReflectionData;

  // GPU storage
  WGALTextureHandle m_hFallbackReflectionSpecularTexture;
  WGALTextureHandle m_hSkyIrradianceTexture;
  WHybridArray<WAmbientCube<WColorLinear16f>, 64> m_SkyIrradianceStorage;

  // Debug data
  WMeshResourceHandle m_hDebugSphere;
  WMaterialResourceHandle m_hDebugMaterial;

  WSharedPtr<WRenderGraph> m_pRenderGraph;
};
