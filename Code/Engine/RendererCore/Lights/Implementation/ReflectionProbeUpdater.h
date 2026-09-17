#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

W_DECLARE_FLAGS(WUInt8, WReflectionProbeUpdaterFlags, SkyLight, HasCustomCubeMap);

/// Renders reflection probes and stores filtered mipmap chains into an atlas texture as well as computing sky irradiance
/// Rendering sky irradiance is optional and only done if m_iIrradianceOutputIndex != -1.
class WReflectionProbeUpdater
{
public:
  /// Defines the target specular reflection probe atlas and index as well as the sky irradiance atlas and index in case the rendered cube map is a sky light.
  struct TargetSlot
  {
    WGALTextureHandle m_hSpecularOutputTexture;   ///< Must be a valid cube map texture array handle.
    WGALTextureHandle m_hIrradianceOutputTexture; ///< Optional. Must be set if m_iIrradianceOutputIndex != -1.
    WInt32 m_iSpecularOutputIndex = -1;           ///< Must be a valid index into the atlas texture.
    WInt32 m_iIrradianceOutputIndex = -1;         ///< If -1, no irradiance is computed.
  };

public:
  WReflectionProbeUpdater();
  ~WReflectionProbeUpdater();

  /// Returns how many new probes can be started this frame.
  /// \param out_updatesFinished Contains the probes that finished last frame.
  /// \return The number of new probes can be started this frame.
  WUInt32 GetFreeUpdateSlots(WDynamicArray<WReflectionProbeRef>& out_updatesFinished);

  /// Starts rendering a new reflection probe.
  ///
  /// The six cube faces are normally rendered over six frames to spread the cost. A probe that has no content
  /// yet is instead allowed to render several faces per frame, so that it becomes usable sooner.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param globalTransform World position to be rendered.
  /// \param target Where the probe should be rendered into.
  /// \param bFirstBake Set if the probe has no usable content yet. Allows rendering multiple faces per frame.
  /// \param bSkyLight Set for the sky light, which renders all six faces at once as its result invalidates all other probes.
  /// \param bSharingBudget Set if probes of the other kind (first bake vs. refresh) are also waiting. The per frame budget is then split between them instead of going to this probe alone.
  /// \return Returns W_FAILURE if no more free slots are available.
  WResult StartDynamicUpdate(const WReflectionProbeRef& probe, const WReflectionProbeDesc& desc, const WTransform& globalTransform, const TargetSlot& target, bool bFirstBake = false, bool bSkyLight = false, bool bSharingBudget = false);

  /// Starts filtering an existing cube map into a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param sourceTexture Cube map that should be filtered into a reflection probe.
  /// \param target Where the probe should be rendered into.
  /// \return Returns W_FAILURE if no more free slots are available.
  WResult StartFilterUpdate(const WReflectionProbeRef& probe, const WReflectionProbeDesc& desc, WTextureCubeResourceHandle hSourceTexture, const TargetSlot& target);

  /// Returns whether a probe that has no content yet is currently being rendered.
  /// Used to decide whether the per frame budget has to be shared with such a probe.
  bool IsFirstBakeInProgress() const;

  /// Cancel a previously started update.
  void CancelUpdate(const WReflectionProbeRef& probe);

  /// Generates update steps. Should be called in PreExtraction phase.
  void GenerateUpdateSteps();

  /// Schedules probe rendering views. Should be called at some point during the extraction phase. Can be called multiple times. It will only do work on the first call after GenerateUpdateSteps.
  void ScheduleUpdateSteps();

private:
  struct ReflectionView
  {
    WViewHandle m_hView;
    WCamera m_Camera;
  };

  struct UpdateStep
  {
    using StorageType = WUInt8;

    enum Enum
    {
      RenderFace0,
      RenderFace1,
      RenderFace2,
      RenderFace3,
      RenderFace4,
      RenderFace5,
      Filter,

      ENUM_COUNT,

      Default = Filter
    };

    static bool IsRenderStep(Enum value) { return value >= UpdateStep::RenderFace0 && value <= UpdateStep::RenderFace5; }
    static Enum NextStep(Enum value) { return static_cast<UpdateStep::Enum>((value + 1) % UpdateStep::ENUM_COUNT); }
  };

  struct ProbeUpdateInfo
  {
    ProbeUpdateInfo();
    ~ProbeUpdateInfo();

    WBitflags<WReflectionProbeUpdaterFlags> m_flags;
    WReflectionProbeRef m_probe;
    WReflectionProbeDesc m_desc;
    WTransform m_globalTransform;
    WTextureCubeResourceHandle m_sourceTexture;
    TargetSlot m_TargetSlot;

    struct Step
    {
      W_DECLARE_POD_TYPE();

      WUInt8 m_uiViewIndex;
      WEnum<UpdateStep> m_UpdateStep;
    };

    bool m_bInUse = false;

    // Whether this probe had no usable content when the update was started.
    bool m_bFirstBake = false;

    WEnum<UpdateStep> m_LastUpdateStep;

    // How many cube faces this probe may render in a single frame. 1 for probes that already have content.
    WUInt8 m_uiRenderBurst = 1;

    WHybridArray<Step, 8> m_UpdateSteps;

    WGALTextureHandle m_hCubemap;
    WGALTextureHandle m_hCubemapProxies[6];
  };

private:
  static void CreateViews(WDynamicArray<ReflectionView>& views, WUInt32 uiNumViews, const char* szNameSuffix, const char* szRenderPipelineResource);
  static WUInt8 ComputeRenderBurst(bool bFirstBake, bool bSkyLight, bool bSharingBudget);
  void CreateReflectionViewsAndResources();

  void ResetProbeUpdateInfo(WUInt32 uiInfo);
  void AddViewToRender(const ProbeUpdateInfo::Step& step, ProbeUpdateInfo& updateInfo);

  bool m_bUpdateStepsFlushed = true;

  WDynamicArray<ReflectionView> m_RenderViews;
  WDynamicArray<ReflectionView> m_FilterViews;

  // Active Dynamic Updates
  WDynamicArray<WUniquePtr<ProbeUpdateInfo>> m_DynamicUpdates;
  WHybridArray<WReflectionProbeRef, 4> m_FinishedLastFrame;
};
