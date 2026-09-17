#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeUpdater.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Math/Color16f.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

WCVarInt cvar_RenderingReflectionPoolMaxUpdatesPerFrame("Rendering.ReflectionPool.MaxUpdatesPerFrame", 1, WCVarFlags::Default, "How many cube faces of a reflection probe may be rendered per frame. Higher values make probes resolve faster at a higher peak cost per frame. Probes that have no content yet always render at least 2 faces, and a sky light without content always renders all 6 at once, so that it does not hold up every other probe.");

// A probe never needs more than one view per cube face, and never more than one filter step at a time.
// The views themselves are cheap (a view plus a blackboard, the render targets live on ProbeUpdateInfo),
// so the pool is fixed at the maximum instead of following the CVar above: a smaller pool could not
// satisfy the burst sizes below, which are allowed to exceed the configured budget.
static constexpr WUInt32 s_uiNumRenderViews = 6;
static constexpr WUInt32 s_uiNumFilterViews = 1;



//////////////////////////////////////////////////////////////////////////
/// ProbeUpdateInfo

WReflectionProbeUpdater::ProbeUpdateInfo::ProbeUpdateInfo()
{
  m_globalTransform.SetIdentity();

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = s_uiReflectionCubeMapSize;
    desc.m_uiHeight = s_uiReflectionCubeMapSize;
    desc.m_uiMipLevelCount = GetMipLevels();
    desc.m_Format = WGALResourceFormat::RGBAHalf;
    desc.m_Type = WGALTextureType::TextureCube;
    desc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hCubemap = WGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
    pDevice->GetTexture(m_hCubemap)->SetDebugName("Reflection Cubemap");
  }

  WStringBuilder sName;
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_hCubemapProxies); ++i)
  {
    m_hCubemapProxies[i] = WGALDevice::GetDefaultDevice()->CreateProxyTexture(m_hCubemap, i);

    sName.SetFormat("Reflection Cubemap Proxy {}", i);
    pDevice->GetTexture(m_hCubemapProxies[i])->SetDebugName(sName);
  }
}

WReflectionProbeUpdater::ProbeUpdateInfo::~ProbeUpdateInfo()
{
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_hCubemapProxies); ++i)
  {
    WGALDevice::GetDefaultDevice()->DestroyProxyTexture(m_hCubemapProxies[i]);
  }

  if (!m_hCubemap.IsInvalidated())
  {
    WGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(m_hCubemap);
  }
}


//////////////////////////////////////////////////////////////////////////
/// WReflectionProbeUpdater

WReflectionProbeUpdater::WReflectionProbeUpdater() = default;

WReflectionProbeUpdater::~WReflectionProbeUpdater()
{
  for (auto& renderView : m_RenderViews)
  {
    WRenderWorld::DeleteView(renderView.m_hView);
  }

  for (auto& filterView : m_FilterViews)
  {
    WRenderWorld::DeleteView(filterView.m_hView);
  }
}

WUInt32 WReflectionProbeUpdater::GetFreeUpdateSlots(WDynamicArray<WReflectionProbeRef>& out_updatesFinished)
{
  out_updatesFinished = m_FinishedLastFrame;
  m_FinishedLastFrame.Clear();
  WUInt32 uiCount = 0;
  for (auto& slot : m_DynamicUpdates)
  {
    if (!slot->m_bInUse)
      ++uiCount;
  }
  return uiCount;
}

WResult WReflectionProbeUpdater::StartDynamicUpdate(const WReflectionProbeRef& probe, const WReflectionProbeDesc& desc, const WTransform& globalTransform, const TargetSlot& target, bool bFirstBake /*= false*/, bool bSkyLight /*= false*/, bool bSharingBudget /*= false*/)
{
  W_ASSERT_DEBUG(target.m_hIrradianceOutputTexture.IsInvalidated() == (target.m_iIrradianceOutputIndex == -1), "Invalid irradiance output settings.");
  W_ASSERT_DEBUG(!target.m_hSpecularOutputTexture.IsInvalidated() && target.m_iSpecularOutputIndex != -1, "Specular output invalid.");
  for (auto& slot : m_DynamicUpdates)
  {
    if (!slot->m_bInUse)
    {
      slot->m_flags = target.m_iIrradianceOutputIndex != -1 ? WReflectionProbeUpdaterFlags::SkyLight : WReflectionProbeUpdaterFlags::Default;
      slot->m_probe = probe;
      slot->m_desc = desc;
      slot->m_globalTransform = globalTransform;
      // Ignore scale when rendering the probe. Zero scaled components will otherwise cause asserts.
      slot->m_globalTransform.m_vScale = WVec3(1.0f);
      slot->m_sourceTexture.Invalidate();
      slot->m_TargetSlot = target;
      slot->m_uiRenderBurst = ComputeRenderBurst(bFirstBake, bSkyLight, bSharingBudget);
      slot->m_bFirstBake = bFirstBake;
      slot->m_bInUse = true;
      return W_SUCCESS;
    }
  }
  return W_FAILURE;
}

WUInt8 WReflectionProbeUpdater::ComputeRenderBurst(bool bFirstBake, bool bSkyLight, bool bSharingBudget)
{
  constexpr WInt32 iNumFaces = (WInt32)UpdateStep::RenderFace5 + 1;

  if (bSkyLight && bFirstBake)
  {
    // Completing the sky light marks all other probes dirty, so anything rendered before that is wasted.
    // Rendering all faces at once gets it done in a single frame, no matter how small the budget is.
    return (WUInt8)iNumFaces;
  }

  const WInt32 iBudget = WMath::Clamp<WInt32>(cvar_RenderingReflectionPoolMaxUpdatesPerFrame, 1, iNumFaces);

  if (!bFirstBake)
  {
    // A probe that already has content is not worth rushing, so it stays within the budget. While a probe
    // without content is waiting, it drops to a single face so that the other one can use the rest.
    return bSharingBudget ? (WUInt8)1 : (WUInt8)iBudget;
  }

  // A probe without any content renders at least 2 faces per frame even if the budget is lower, so that it
  // becomes usable quickly. While a refresh is waiting, one face of the budget is left to it.
  return (WUInt8)WMath::Max(2, bSharingBudget ? iBudget - 1 : iBudget);
}

WResult WReflectionProbeUpdater::StartFilterUpdate(const WReflectionProbeRef& probe, const WReflectionProbeDesc& desc, WTextureCubeResourceHandle hSourceTexture, const TargetSlot& target)
{
  W_ASSERT_DEBUG(target.m_hIrradianceOutputTexture.IsInvalidated() == (target.m_iIrradianceOutputIndex == -1), "Invalid irradiance output settings.");
  W_ASSERT_DEBUG(!target.m_hSpecularOutputTexture.IsInvalidated() && target.m_iSpecularOutputIndex != -1, "Specular output invalid.");
  for (auto& slot : m_DynamicUpdates)
  {
    if (!slot->m_bInUse)
    {
      slot->m_flags = WReflectionProbeUpdaterFlags::HasCustomCubeMap;
      if (target.m_iIrradianceOutputIndex != -1)
      {
        slot->m_flags.Add(WReflectionProbeUpdaterFlags::SkyLight);
      }
      slot->m_probe = probe;
      slot->m_desc = desc;
      slot->m_globalTransform.SetIdentity();
      slot->m_sourceTexture = hSourceTexture;
      slot->m_TargetSlot = target;
      slot->m_bFirstBake = false;
      slot->m_bInUse = true;
      return W_SUCCESS;
    }
  }
  return W_FAILURE;
}

bool WReflectionProbeUpdater::IsFirstBakeInProgress() const
{
  for (const auto& slot : m_DynamicUpdates)
  {
    if (slot->m_bInUse && slot->m_bFirstBake)
      return true;
  }
  return false;
}

void WReflectionProbeUpdater::CancelUpdate(const WReflectionProbeRef& probe)
{
  m_FinishedLastFrame.RemoveAndSwap(probe);
  for (WUInt32 uiInfo = m_DynamicUpdates.GetCount(); uiInfo-- > 0;)
  {
    if (m_DynamicUpdates[uiInfo]->m_probe == probe)
    {
      ResetProbeUpdateInfo(uiInfo);
      // A probe can be in the queue multiple times so don't exit the loop.
    }
  }
}

void WReflectionProbeUpdater::GenerateUpdateSteps()
{
  if (!m_bUpdateStepsFlushed)
    return;

  m_bUpdateStepsFlushed = false;
  WUInt32 uiRenderViewIndex = 0;
  WUInt32 uiFilterViewIndex = 0;

  WUInt32 uiSortedUpdateInfoIndex = 0;
  while (uiSortedUpdateInfoIndex < m_DynamicUpdates.GetCount())
  {
    auto pUpdateInfo = m_DynamicUpdates[uiSortedUpdateInfoIndex].Borrow();
    if (!pUpdateInfo->m_bInUse)
    {
      ++uiSortedUpdateInfoIndex;
      continue;
    }

    auto& updateSteps = pUpdateInfo->m_UpdateSteps;
    UpdateStep::Enum nextStep = UpdateStep::NextStep(updateSteps.IsEmpty() ? pUpdateInfo->m_LastUpdateStep : updateSteps.PeekBack().m_UpdateStep);

    if (pUpdateInfo->m_flags.IsSet(WReflectionProbeUpdaterFlags::HasCustomCubeMap))
      nextStep = UpdateStep::Filter;

    bool bNextProbe = false;

    if (UpdateStep::IsRenderStep(nextStep))
    {
      // Probes that already have content render a single face per frame. Only probes without any content
      // are allowed to consume several render views at once, so the steady state cost stays unchanged.
      const bool bBurstExhausted = updateSteps.GetCount() >= pUpdateInfo->m_uiRenderBurst;

      if (uiRenderViewIndex < m_RenderViews.GetCount() && !bBurstExhausted)
      {
        updateSteps.PushBack({(WUInt8)uiRenderViewIndex, nextStep});
        ++uiRenderViewIndex;
      }
      else
      {
        bNextProbe = true;
      }
    }
    else if (nextStep == UpdateStep::Filter)
    {
      // The filter reads the cube map that the render steps write, so it may only run in the same frame if
      // all six faces are queued in that frame as well. ScheduleUpdateSteps submits the steps in reverse and
      // WRenderWorld renders the last added view first, so the faces are rendered before the filter reads
      // them. Probes that render their faces over several frames keep filtering in a separate frame.
      const bool bAllFacesThisFrame = updateSteps.GetCount() == (WUInt32)UpdateStep::RenderFace5 + 1;

      if (uiFilterViewIndex < m_FilterViews.GetCount() && (updateSteps.IsEmpty() || bAllFacesThisFrame))
      {
        updateSteps.PushBack({(WUInt8)uiFilterViewIndex, nextStep});
        ++uiFilterViewIndex;
      }
      bNextProbe = true;
    }

    // break if no more views are available
    if (uiRenderViewIndex == m_RenderViews.GetCount() && uiFilterViewIndex == m_FilterViews.GetCount())
    {
      break;
    }

    if (bNextProbe)
    {
      ++uiSortedUpdateInfoIndex;
    }
  }
}

void WReflectionProbeUpdater::ScheduleUpdateSteps()
{
  if (m_bUpdateStepsFlushed)
  {
    return;
  }
  m_bUpdateStepsFlushed = true;

  // #TODO: would like to do that in the ctor but then the renderer tests assert that don't have the base asset directory set up.
  CreateReflectionViewsAndResources();

  // Iterate in reverse as ResetProbeUpdateInfo will move the current index to the back of the array.
  for (WUInt32 uiInfo = m_DynamicUpdates.GetCount(); uiInfo-- > 0;)
  {
    ProbeUpdateInfo& info = *m_DynamicUpdates[uiInfo];
    if (info.m_bInUse)
    {
      bool bDone = false;
      if (!info.m_UpdateSteps.IsEmpty())
      {
        // Render steps are done in reverse order so the last committed view is rendered first. Thus we need to iterate the array in reverse order.
        for (WUInt32 uiStep = info.m_UpdateSteps.GetCount(); uiStep-- > 0;)
        {
          if (info.m_UpdateSteps[uiStep].m_UpdateStep == UpdateStep::Filter)
          {
            bDone = true;
          }

          AddViewToRender(info.m_UpdateSteps[uiStep], info);
        }

        bool bIsLoadingResources = false;
        if (info.m_desc.m_Mode == WReflectionProbeMode::Static && info.m_sourceTexture.IsValid())
        {
          // Wait until the input texture is fully loaded.
          WResourceLock<WTextureCubeResource> pTexture(info.m_sourceTexture, WResourceAcquireMode::AllowLoadingFallback);
          if (pTexture->GetLoadingState() != WResourceState::Loaded || pTexture->GetNumQualityLevelsLoadable() > 0 || pTexture->GetResourceHandle() != info.m_sourceTexture)
          {
            bIsLoadingResources = true;
          }
        }
        else
        {
          bIsLoadingResources = WResourceManager::IsAnyLoadingInProgress();
        }

        if (!bIsLoadingResources)
        {
          info.m_LastUpdateStep = info.m_UpdateSteps.PeekBack().m_UpdateStep;
        }
        info.m_UpdateSteps.Clear();
        if (bDone && !bIsLoadingResources)
        {
          m_FinishedLastFrame.PushBack(info.m_probe);
          ResetProbeUpdateInfo(uiInfo);
        }
      }
    }
  }
}

void WReflectionProbeUpdater::CreateViews(WDynamicArray<ReflectionView>& views, WUInt32 uiNumViews, const char* szNameSuffix, const char* szRenderPipelineResource)
{
  WStringBuilder sName;

  for (WUInt32 i = views.GetCount(); i < uiNumViews; ++i)
  {
    auto& renderView = views.ExpandAndGetRef();

    sName.SetFormat("Reflection Probe {} {}", szNameSuffix, i);

    WView* pView = nullptr;
    renderView.m_hView = WRenderWorld::CreateView(sName, pView);

    pView->SetCameraUsageHint(WCameraUsageHint::Reflection);
    pView->SetViewport(WRectFloat(0.0f, 0.0f, static_cast<float>(s_uiReflectionCubeMapSize), static_cast<float>(s_uiReflectionCubeMapSize)));

    pView->SetRenderPipelineResource(WResourceManager::LoadResource<WRenderPipelineResource>(szRenderPipelineResource));

    renderView.m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    pView->SetCamera(&renderView.m_Camera);

    sName.Append(" Blackboard");
    pView->SetBlackboard(WBlackboard::Create(sName));
  }
}

void WReflectionProbeUpdater::CreateReflectionViewsAndResources()
{
  // ReflectionRenderPipeline.WRenderPipelineAsset
  CreateViews(m_RenderViews, s_uiNumRenderViews, "Render", "{ 734898e8-b1a2-0da2-c4ae-701912983c2f }");

  // ReflectionFilterPipeline.WRenderPipelineAsset
  CreateViews(m_FilterViews, s_uiNumFilterViews, "Filter", "{ 3437db17-ddf1-4b67-b80f-9999d6b0c352 }");

  if (m_DynamicUpdates.IsEmpty())
  {
    for (WUInt32 i = 0; i < 2; i++)
    {
      m_DynamicUpdates.PushBack(W_DEFAULT_NEW(ProbeUpdateInfo));
    }
  }
}

void WReflectionProbeUpdater::ResetProbeUpdateInfo(WUInt32 uiInfo)
{
  // Reset and move to the end of the queue.
  WUniquePtr<ProbeUpdateInfo> info = std::move(m_DynamicUpdates[uiInfo]);
  info->m_bInUse = false;
  info->m_flags = {};
  info->m_probe.m_Id.Invalidate();
  info->m_probe.m_uiWorldIndex = 0;
  info->m_globalTransform.SetIdentity();
  info->m_sourceTexture.Invalidate();
  info->m_LastUpdateStep = UpdateStep::Default;
  info->m_uiRenderBurst = 1;
  info->m_bFirstBake = false;
  info->m_UpdateSteps.Clear();

  m_DynamicUpdates.RemoveAtAndCopy(uiInfo);
  m_DynamicUpdates.PushBack(std::move(info));
}

void WReflectionProbeUpdater::AddViewToRender(const ProbeUpdateInfo::Step& step, ProbeUpdateInfo& updateInfo)
{
  WVec3 vForward[6] = {
    WVec3(1.0f, 0.0f, 0.0f),
    WVec3(-1.0f, 0.0f, 0.0f),
    WVec3(0.0f, 0.0f, 1.0f),
    WVec3(0.0f, 0.0f, -1.0f),
    WVec3(0.0f, -1.0f, 0.0f),
    WVec3(0.0f, 1.0f, 0.0f),
  };

  WVec3 vUp[6] = {
    WVec3(0.0f, 0.0f, 1.0f),
    WVec3(0.0f, 0.0f, 1.0f),
    WVec3(0.0f, 1.0f, 0.0f),
    WVec3(0.0f, -1.0f, 0.0f),
    WVec3(0.0f, 0.0f, 1.0f),
    WVec3(0.0f, 0.0f, 1.0f),
  };

  // Setup view and camera
  {
    ReflectionView* pReflectionView = nullptr;
    WUInt32 uiFaceIndex = 0;

    if (step.m_UpdateStep == UpdateStep::Filter)
    {
      pReflectionView = &m_FilterViews[step.m_uiViewIndex];
    }
    else
    {
      pReflectionView = &m_RenderViews[step.m_uiViewIndex];
      uiFaceIndex = step.m_UpdateStep;
    }

    WView* pView = nullptr;
    WRenderWorld::TryGetView(pReflectionView->m_hView, pView);

    pView->m_IncludeTags = updateInfo.m_desc.m_IncludeTags;
    pView->m_ExcludeTags = updateInfo.m_desc.m_ExcludeTags;
    WWorld* pWorld = WWorld::GetWorld(updateInfo.m_probe.m_uiWorldIndex);
    pView->SetWorld(pWorld);

    WGALRenderTargets renderTargets;
    if (step.m_UpdateStep == UpdateStep::Filter)
    {
      renderTargets.m_hRTs[0] = updateInfo.m_TargetSlot.m_hSpecularOutputTexture;

      if (updateInfo.m_flags.IsSet(WReflectionProbeUpdaterFlags::SkyLight))
      {
        renderTargets.m_hRTs[2] = updateInfo.m_TargetSlot.m_hIrradianceOutputTexture;
      }
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ReflectionFilterPass.DiffuseIntensity"), updateInfo.m_desc.m_fDiffuseIntensity);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ReflectionFilterPass.DiffuseSaturation"), updateInfo.m_desc.m_fDiffuseSaturation);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ReflectionFilterPass.SpecularIntensity"), updateInfo.m_desc.m_fSpecularIntensity);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ReflectionFilterPass.SpecularOutputIndex"), updateInfo.m_TargetSlot.m_iSpecularOutputIndex);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ReflectionFilterPass.IrradianceOutputIndex"), updateInfo.m_TargetSlot.m_iIrradianceOutputIndex);

      WGALTextureHandle hSourceTexture = updateInfo.m_hCubemap;
      if (updateInfo.m_desc.m_Mode == WReflectionProbeMode::Static)
      {
        if (updateInfo.m_flags.IsSet(WReflectionProbeUpdaterFlags::HasCustomCubeMap))
        {
          WResourceLock<WTextureCubeResource> pTexture(updateInfo.m_sourceTexture, WResourceAcquireMode::BlockTillLoaded);
          // #TODO Currently even in static mode we render the 6 sides and only change the filter stage to point to the static texture if available. Rendering the 6 sides is intended only in the editor as a preview for non-baked probes. We will need to find a way to quickly determine if we need to do this fallback at a much earlier stage.
          if (pTexture->GetLoadingState() == WResourceState::Loaded && pTexture->GetResourceHandle() == updateInfo.m_sourceTexture)
          {
            hSourceTexture = pTexture->GetGALTexture();
          }
        }
      }
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("ReflectionFilterPass.InputCubemap"), hSourceTexture.GetInternalID().m_Data);
    }
    else
    {
      renderTargets.m_hRTs[0] = updateInfo.m_hCubemapProxies[uiFaceIndex];
    }
    pView->SetRenderTargets(renderTargets);

    WVec3 vPosition = updateInfo.m_globalTransform * updateInfo.m_desc.m_vCaptureOffset;
    WVec3 vForward2 = updateInfo.m_globalTransform.TransformDirection(vForward[uiFaceIndex]);
    WVec3 vUp2 = updateInfo.m_globalTransform.TransformDirection(vUp[uiFaceIndex]);
    if (updateInfo.m_flags.IsSet(WReflectionProbeUpdaterFlags::SkyLight))
    {
      vForward2 = vForward[uiFaceIndex];
      vUp2 = vUp[uiFaceIndex];
    }

    const float fFar = updateInfo.m_desc.m_fFarPlane;
    float fNear = updateInfo.m_desc.m_fNearPlane;
    if (fNear >= fFar)
    {
      fNear = fFar - 0.001f;
    }
    else if (fNear == 0.0f)
    {
      fNear = fFar / 1000.0f;
    }

    pReflectionView->m_Camera.LookAt(vPosition, vPosition + vForward2, vUp2);
    pReflectionView->m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90.0f, fNear, fFar);
    WRenderWorld::AddViewToRender(pReflectionView->m_hView);
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeUpdater);
