#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ReflectionPool)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WReflectionPool::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WReflectionPool::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////
/// WReflectionPool

WReflectionProbeId WReflectionPool::RegisterReflectionProbe(const WWorld* pWorld, const WReflectionProbeDesc& desc, const WReflectionProbeComponentBase* pComponent)
{
  W_LOCK(s_pData->m_Mutex);

  Data::ProbeData probe;
  s_pData->UpdateProbeData(probe, desc, pComponent);
  return s_pData->AddProbe(pWorld, std::move(probe));
}

void WReflectionPool::DeregisterReflectionProbe(const WWorld* pWorld, WReflectionProbeId id)
{
  W_LOCK(s_pData->m_Mutex);
  s_pData->RemoveProbe(pWorld, id);
}

void WReflectionPool::UpdateReflectionProbe(const WWorld* pWorld, WReflectionProbeId id, const WReflectionProbeDesc& desc, const WReflectionProbeComponentBase* pComponent)
{
  W_LOCK(s_pData->m_Mutex);
  WReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  s_pData->UpdateProbeData(probeData, desc, pComponent);
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

void WReflectionPool::ExtractReflectionProbe(const WComponent* pComponent, WMsgExtractRenderData& ref_msg, WReflectionProbeRenderData* pRenderData0, const WWorld* pWorld, WReflectionProbeId id, float fPriority)
{
  W_LOCK(s_pData->m_Mutex);
  s_pData->m_ReflectionProbeUpdater.ScheduleUpdateSteps();

  const WUInt32 uiWorldIndex = pWorld->GetIndex();
  WReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
  data.m_mapping.AddWeight(id, fPriority);
  const WInt32 iMappedIndex = data.m_mapping.GetReflectionIndex(id, true);

  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);

  if (pComponent->GetOwner()->IsDynamic())
  {
    WTransform globalTransform = pComponent->GetOwner()->GetGlobalTransform();
    if (!probeData.m_Flags.IsSet(WProbeFlags::Dynamic) && probeData.m_GlobalTransform != globalTransform)
    {
      data.m_mapping.UpdateProbe(id, probeData.m_Flags);
    }
    probeData.m_GlobalTransform = globalTransform;
  }

  // The sky light is always active and not added to the render data (always passes in nullptr as pRenderData).
  if (pRenderData0 && iMappedIndex > 0)
  {
    // Index and flags are stored in m_uiIndex so we can't just overwrite it.
    pRenderData0->m_uiIndex |= (WUInt32)iMappedIndex;
    ref_msg.AddRenderData(pRenderData0, WDefaultRenderDataCategories::ReflectionProbe, WRenderData::Caching::Never);
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WUInt32 uiMipLevels = GetMipLevels();
  if (probeData.m_desc.m_bShowDebugInfo && s_pData->m_hDebugMaterial.IsValid())
  {
    if (ref_msg.m_OverrideCategory == WInvalidRenderDataCategory)
    {
      WInt32 activeIndex = 0;
      if (s_pData->m_ActiveDynamicUpdate.Contains(WReflectionProbeRef{uiWorldIndex, id}))
      {
        activeIndex = 1;
      }

      WStringBuilder sEnum;
      WReflectionUtils::BitflagsToString(probeData.m_Flags, sEnum, WReflectionUtils::EnumConversionMode::ValueNameOnly);
      WStringBuilder s;
      s.SetFormat("\n RefIdx: {}\nUpdating: {}\nFlags: {}\n", iMappedIndex, activeIndex, sEnum);
      WDebugRenderer::Draw3DText(pWorld, s, pComponent->GetOwner()->GetGlobalPosition(), WColorScheme::LightUI(WColorScheme::Violet));
    }

    // Not mapped in the atlas - cannot render it.
    if (iMappedIndex < 0)
      return;

    const WGameObject* pOwner = pComponent->GetOwner();
    const WUInt32 uiUniqueID = WRenderComponent::GetUniqueIdForRendering(*pComponent);

    // This is debug rendering code so we don't want to trash the static instance data buffer every frame.
    const bool bDynamic = true;
    WGALDynamicBufferHandle hInstanceDataBuffer;
    auto instanceData = ref_msg.m_pRenderDataManager->GetOrCreateInstanceData(pComponent, bDynamic, hInstanceDataBuffer, probeData.m_DebugInstanceDataOffset, uiMipLevels);

    WUInt32 uiMipLevelsToRender = probeData.m_desc.m_bShowMipMaps ? uiMipLevels : 1;
    for (WUInt32 i = 0; i < uiMipLevelsToRender; i++)
    {
      WTransform t;
      t.m_vPosition = probeData.m_GlobalTransform * probeData.m_desc.m_vCaptureOffset;
      t.m_vPosition.z += s_fDebugSphereRadius * i * 2;
      t.m_qRotation = probeData.m_Flags.IsSet(WProbeFlags::SkyLight) ? WQuat::MakeIdentity() : probeData.m_GlobalTransform.m_qRotation;
      t.m_vScale = WVec3(1.0f);

      WVec4 customData = WVec4(static_cast<float>(iMappedIndex), static_cast<float>(i), 0, 0);

      WRenderDataManager::FillPerInstanceData(instanceData[i], pOwner, t, uiUniqueID, WColor::White, customData);
    }

    WMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(pOwner);
    pRenderData->Fill(probeData.m_DebugInstanceDataOffset, hInstanceDataBuffer, s_pData->m_hDebugMaterial, s_pData->m_hDebugSphere, 0, 0, uiMipLevelsToRender);

    ref_msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::LitOpaque, WRenderData::Caching::Never);
  }
#endif
}

//////////////////////////////////////////////////////////////////////////
/// SkyLight

WReflectionProbeId WReflectionPool::RegisterSkyLight(const WWorld* pWorld, WReflectionProbeDesc& ref_desc, const WSkyLightComponent* pComponent)
{
  W_LOCK(s_pData->m_Mutex);
  const WUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight |= W_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= W_BIT(uiWorldIndex);

  Data::ProbeData probe;
  s_pData->UpdateSkyLightData(probe, ref_desc, pComponent);

  WReflectionProbeId id = s_pData->AddProbe(pWorld, std::move(probe));
  return id;
}

void WReflectionPool::DeregisterSkyLight(const WWorld* pWorld, WReflectionProbeId id)
{
  W_LOCK(s_pData->m_Mutex);

  s_pData->RemoveProbe(pWorld, id);

  const WUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight &= ~W_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= W_BIT(uiWorldIndex);
}

void WReflectionPool::UpdateSkyLight(const WWorld* pWorld, WReflectionProbeId id, const WReflectionProbeDesc& desc, const WSkyLightComponent* pComponent)
{
  W_LOCK(s_pData->m_Mutex);
  WReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  if (s_pData->UpdateSkyLightData(probeData, desc, pComponent))
  {
    // s_pData->UnmapProbe(pWorld->GetIndex(), data, id);
  }
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

//////////////////////////////////////////////////////////////////////////
/// Misc

// static
void WReflectionPool::SetConstantSkyIrradiance(const WWorld* pWorld, const WAmbientCube<WColor>& skyIrradiance)
{
  W_LOCK(s_pData->m_Mutex);
  WUInt32 uiWorldIndex = pWorld->GetIndex();
  WAmbientCube<WColorLinear16f> skyIrradiance16f = skyIrradiance;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != skyIrradiance16f)
  {
    skyIrradianceStorage[uiWorldIndex] = skyIrradiance16f;

    s_pData->m_uiSkyIrradianceChanged |= W_BIT(uiWorldIndex);
  }
}

void WReflectionPool::ResetConstantSkyIrradiance(const WWorld* pWorld)
{
  W_LOCK(s_pData->m_Mutex);
  WUInt32 uiWorldIndex = pWorld->GetIndex();

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != WAmbientCube<WColorLinear16f>())
  {
    skyIrradianceStorage[uiWorldIndex] = WAmbientCube<WColorLinear16f>();

    s_pData->m_uiSkyIrradianceChanged |= W_BIT(uiWorldIndex);
  }
}

// static
WUInt32 WReflectionPool::GetReflectionCubeMapSize()
{
  return s_uiReflectionCubeMapSize;
}

// static
WGALTextureHandle WReflectionPool::GetReflectionSpecularTexture(WUInt32 uiWorldIndex, WEnum<WCameraUsageHint> cameraUsageHint)
{
  if (uiWorldIndex < s_pData->m_WorldReflectionData.GetCount() && cameraUsageHint != WCameraUsageHint::Reflection)
  {
    Data::WorldReflectionData* pData = s_pData->m_WorldReflectionData[uiWorldIndex].Borrow();
    if (pData)
      return pData->m_mapping.GetTexture();
  }
  return s_pData->m_hFallbackReflectionSpecularTexture;
}

// static
WGALTextureHandle WReflectionPool::GetSkyIrradianceTexture()
{
  return s_pData->m_hSkyIrradianceTexture;
}

//////////////////////////////////////////////////////////////////////////
/// Private Functions

// static
void WReflectionPool::OnEngineStartup()
{
  s_pData = W_DEFAULT_NEW(WReflectionPool::Data);

  WRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  WRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void WReflectionPool::OnEngineShutdown()
{
  WRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  WRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  W_DEFAULT_DELETE(s_pData);
}

// static
void WReflectionPool::OnExtractionEvent(const WRenderWorldExtractionEvent& e)
{
  if (e.m_Type == WRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    W_PROFILE_SCOPE("Reflection Pool BeginExtraction");
    s_pData->CreateSkyIrradianceTexture();
    s_pData->CreateReflectionViewsAndResources();
    s_pData->PreExtraction();
  }

  if (e.m_Type == WRenderWorldExtractionEvent::Type::EndExtraction)
  {
    W_PROFILE_SCOPE("Reflection Pool EndExtraction");
    s_pData->PostExtraction();
  }
}

// static
void WReflectionPool::OnRenderEvent(const WRenderWorldRenderEvent& e)
{
  if (e.m_Type != WRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hSkyIrradianceTexture.IsInvalidated())
    return;

  W_LOCK(s_pData->m_Mutex);

  WUInt64 uiWorldHasSkyLight = s_pData->m_uiWorldHasSkyLight;
  WUInt64& uiSkyIrradianceChanged = s_pData->m_uiSkyIrradianceChanged;
  if ((~uiWorldHasSkyLight & uiSkyIrradianceChanged) == 0)
    return;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  if (s_pData->m_pRenderGraph == nullptr)
    s_pData->m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("ReflectionPool", WRenderGraphPhase::PreRender);

  s_pData->m_pRenderGraph->Reset();

  struct IrradianceUpdates
  {
    WBoundingBoxu32 m_destBox;
    WAmbientCube<WColorLinear16f> m_cube;
  };
  WHybridArray<IrradianceUpdates, 4> irradianceUpdates;
  WTempHybridArray<WGALTextureHandle, 4> atlasToClear;

  for (WUInt32 i = 0; i < skyIrradianceStorage.GetCount(); ++i)
  {
    if ((uiWorldHasSkyLight & W_BIT(i)) == 0 && (uiSkyIrradianceChanged & W_BIT(i)) != 0)
    {
      IrradianceUpdates& cube = irradianceUpdates.ExpandAndGetRef();
      cube.m_destBox.m_vMin.Set(0, i, 0);
      cube.m_destBox.m_vMax.Set(6, i + 1, 1);
      cube.m_cube = skyIrradianceStorage[i];

      uiSkyIrradianceChanged &= ~W_BIT(i);

      if (i < s_pData->m_WorldReflectionData.GetCount() && s_pData->m_WorldReflectionData[i] != nullptr)
      {
        WReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[i];
        atlasToClear.PushBack(data.m_mapping.GetTexture());
      }
    }
  }

  // Transfer pass: update sky irradiance texture
  {
    WRenderGraphTextureHandle hSkyIrradiance = s_pData->m_pRenderGraph->ImportTexture(s_pData->m_hSkyIrradianceTexture);
    auto pass = s_pData->m_pRenderGraph->AddTransferPass("Sky Irradiance Texture Update");
    pass.WriteTexture(hSkyIrradiance, {}, WGALResourceState::CopyDestination);
    pass.HasSideEffects();
    pass.SetExecuteCallback([hSkyIrradiance, irradianceUpdates](const WRenderGraphContext& ctx)
      {
        for (WUInt32 i = 0; i < irradianceUpdates.GetCount(); ++i)
        {
          WGALSystemMemoryDescription memDesc;
          memDesc.m_uiRowPitch = sizeof(WAmbientCube<WColorLinear16f>);
          memDesc.m_pData = WMakeByteBlobPtr(reinterpret_cast<const WUInt8*>(&irradianceUpdates[i].m_cube.m_Values[0]), memDesc.m_uiRowPitch * 1);
          ctx.GetCommandEncoder()->UpdateTexture(ctx.ResolveTexture(hSkyIrradiance), WGALTextureSubresource(), irradianceUpdates[i].m_destBox, memDesc);
        } //
      } //
    );
  }

  // Graphics passes: clear specular sky reflection to black.
  {
    const WUInt32 uiNumMipMaps = GetMipLevels();
    for (WGALTextureHandle atlas : atlasToClear)
    {
      WRenderGraphTextureHandle hAtlas = s_pData->m_pRenderGraph->ImportTexture(atlas);
      for (WUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (WUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          WGALRenderTargetRange range;
          range.m_uiBaseArraySlice = static_cast<WUInt16>(uiFaceIndex);
          range.m_uiArraySlices = 1;
          range.m_uiBaseMipLevel = static_cast<WUInt8>(uiMipMapIndex);

          auto clearPass = s_pData->m_pRenderGraph->AddGraphicsPass("ClearSkySpecular");
          clearPass.AddColorTarget(hAtlas, range, {}, {}, {}, WGALTextureType::Texture2DArray);
          clearPass.SetClearColor(0, WColor(0, 0, 0, 1));
          clearPass.HasSideEffects();
        }
      }
    }
  }

  WRenderGraphManager::EnqueueRenderGraph(s_pData->m_pRenderGraph);
}


W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionPool);
