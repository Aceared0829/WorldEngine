#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Event generated on mapping changes.
/// \sa WReflectionProbeMapping::m_Events
struct WReflectionProbeMappingEvent
{
  enum class Type
  {
    ProbeMapped,          ///< The given probe was mapped to the atlas.
    ProbeUnmapped,        ///<  The given probe was unmapped from the atlas.
    ProbeUpdateRequested, ///< The given probe needs to be updated after which WReflectionProbeMapping::ProbeUpdateFinished must be called.
  };

  WReflectionProbeId m_Id;
  Type m_Type;

  /// Only valid for ProbeUpdateRequested. The probe's priority this frame. Used to order pending updates.
  float m_fPriority = 0.0f;

  /// Only valid for ProbeUpdateRequested. Set if the probe has never completed an update, i.e. it has no
  /// usable content yet. Such probes are updated before refreshes of probes that already have content.
  bool m_bFirstBake = false;
};

/// This class creates a reflection probe atlas and controls the mapping of added probes to the available atlas indices.
class WReflectionProbeMapping
{
public:
  /// Creates a reflection probe atlas and mapping of the given size.
  /// \param uiAtlasSize How many probes the atlas can contain.
  WReflectionProbeMapping(WUInt32 uiAtlasSize);
  ~WReflectionProbeMapping();

  /// \name Probe management
  ///@{

  /// Adds a probe that will be considered for mapping into the atlas.
  void AddProbe(WReflectionProbeId probe, WBitflags<WProbeFlags> flags);

  /// Marks previously added probe as dirty and potentially changes its flags.
  void UpdateProbe(WReflectionProbeId probe, WBitflags<WProbeFlags> flags);

  /// Should be called once a requested WReflectionProbeMappingEvent::Type::ProbeUpdateRequested event has been completed.
  /// \param probe The probe that has finished its update.
  void ProbeUpdateFinished(WReflectionProbeId probe);

  /// Removes a probe. If the probe was mapped, WReflectionProbeMappingEvent::Type::ProbeUnmapped will be fired when calling this function.
  void RemoveProbe(WReflectionProbeId probe);

  ///@}
  /// \name Render helpers
  ///@{

  /// Returns the index at which a given probe is mapped.
  /// \param probe The probe that is being queried.
  /// \param bForExtraction If set, returns whether the index can be used for using the probe during rendering. If the probe was just mapped but not updated yet, -1 will be returned for bForExtraction = true but a valid index for bForExtraction = false so that the index can be rendered into.
  /// \return Returns the mapped index in the atlas or -1 of the probe is not mapped.
  WInt32 GetReflectionIndex(WReflectionProbeId probe, bool bForExtraction = false) const;

  /// Returns the atlas texture.
  /// \return The texture handle of the cube map atlas.
  WGALTextureHandle GetTexture() const { return m_hReflectionSpecularTexture; }

  ///@}
  /// \name Compute atlas mapping
  ///@{

  /// Should be called in the PreExtraction phase. This will reset all probe weights.
  void PreExtraction();

  /// Adds weight to a probe. Should be called during extraction of the probe. The mapping will map the probes with the highest weights in the atlas over time. This can be called multiple times in a frame for a probe if it is visible in multiple views. The maximum weight is then taken.
  void AddWeight(WReflectionProbeId probe, float fPriority);

  /// Should be called in the PostExtraction phase. This will compute the best probe mapping and potentially fire WReflectionProbeMappingEvent events to map / unmap or request updates of probes.
  void PostExtraction();

  ///@}

public:
  WEvent<const WReflectionProbeMappingEvent&> m_Events;

private:
  struct WProbeMappingFlags
  {
    using StorageType = WUInt8;

    enum Enum
    {
      SkyLight = WProbeFlags::SkyLight,
      HasCustomCubeMap = WProbeFlags::HasCustomCubeMap,
      Sphere = WProbeFlags::Sphere,
      Box = WProbeFlags::Box,
      Dynamic = WProbeFlags::Dynamic,
      Dirty = W_BIT(5),
      Usable = W_BIT(6),
      Default = 0
    };

    struct Bits
    {
      StorageType SkyLight : 1;
      StorageType HasCustomCubeMap : 1;
      StorageType Sphere : 1;
      StorageType Box : 1;
      StorageType Dynamic : 1;
      StorageType Dirty : 1;
      StorageType Usable : 1;
    };
  };

  // W_DECLARE_FLAGS_OPERATORS(WProbeMappingFlags);

  struct SortedProbes
  {
    W_DECLARE_POD_TYPE();

    W_ALWAYS_INLINE bool operator<(const SortedProbes& other) const
    {
      if (m_fPriority != other.m_fPriority) // we want to sort descending (higher priority first)
        return m_fPriority > other.m_fPriority;

      return m_uiIndex < other.m_uiIndex;
    }

    WReflectionProbeId m_uiIndex;
    float m_fPriority = 0.0f;
  };

  struct ProbeDataInternal
  {
    WBitflags<WProbeMappingFlags> m_Flags;
    WInt32 m_uiReflectionIndex = -1;
    float m_fPriority = 0.0f;
    WReflectionProbeId m_id;
  };

private:
  void MapProbe(WReflectionProbeId id, WInt32 iReflectionIndex);
  void UnmapProbe(WReflectionProbeId id);
  void RequestUpdate(const ProbeDataInternal& probeData);
  bool IsSkyLightRefreshDue() const;

private:
  WDynamicArray<ProbeDataInternal> m_RegisteredProbes;
  WReflectionProbeId m_SkyLight;

  // Frame in which the sky light last completed an update. Its completion marks every other probe dirty,
  // so refreshing it every frame would keep the entire scene from ever settling.
  WUInt64 m_uiLastSkyLightUpdateFrame = 0;
  bool m_bSkyLightUpdatedOnce = false;

  WUInt32 m_uiAtlasSize = 32;
  WDynamicArray<WReflectionProbeId> m_MappedCubes;

  // GPU Data
  WGALTextureHandle m_hReflectionSpecularTexture;

  // Cleared every frame:
  WDynamicArray<SortedProbes> m_SortedProbes; // All probes exiting in the scene, sorted by priority.
  WDynamicArray<SortedProbes> m_ActiveProbes; // Probes that are currently mapped in the atlas.
  WDynamicArray<WInt32> m_UnusedProbeSlots;  // Probe slots are are currently unused in the atlas.
  WDynamicArray<SortedProbes> m_AddProbes;    // Probes that should be added to the atlas
};
