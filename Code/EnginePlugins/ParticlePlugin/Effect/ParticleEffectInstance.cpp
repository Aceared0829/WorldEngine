#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/SpatialData.h>
#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Components/ParticleAttractorComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarBool cvar_ParticlesDebugWindSamples("Particles.DebugWindSamples", false, WCVarFlags::Default, "Enables debug visualization for wind sampling on particle effects.");
#endif

WParticleEffectInstance::WParticleEffectInstance()
{
  m_pTask = W_DEFAULT_NEW(WParticleEffectUpdateTask, this);
  m_pTask->ConfigureTask("Particle Effect Update", WTaskNesting::Maybe);

  m_pOwnerModule = nullptr;

  Destruct();
}

WParticleEffectInstance::~WParticleEffectInstance()
{
  Destruct();
}

void WParticleEffectInstance::Construct(WParticleEffectHandle hEffectHandle, const WParticleEffectResourceHandle& hResource, WWorld* pWorld, WParticleWorldModule* pOwnerModule, WUInt64 uiRandomSeed, bool bIsShared, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams)
{
  m_hEffectHandle = hEffectHandle;
  m_pWorld = pWorld;
  m_pOwnerModule = pOwnerModule;
  m_hResource = hResource;
  m_bIsSharedEffect = bIsShared;
  m_bEmitterEnabled = true;
  m_bIsFinishing = false;
  m_BoundingVolume = WBoundingBoxSphere::MakeInvalid();
  m_ElapsedTimeSinceUpdate = WTime::MakeZero();
  m_EffectIsVisible = WTime::MakeZero();
  m_iMinSimStepsToDo = 4;
  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_TotalEffectLifeTime = WTime::MakeZero();
  m_pVisibleIf = nullptr;
  m_uiRandomSeed = uiRandomSeed;

  if (uiRandomSeed == 0)
    m_Random.InitializeFromCurrentTime();
  else
    m_Random.Initialize(uiRandomSeed);

  Reconfigure(true, floatParams, colorParams);
}

void WParticleEffectInstance::Destruct()
{
  Interrupt();

  m_SharedInstances.Clear();
  m_hEffectHandle.Invalidate();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_bIsSharedEffect = false;
  m_pWorld = nullptr;
  m_hResource.Invalidate();
  m_hEffectHandle.Invalidate();
  m_uiReviveTimeout = 5;

  m_WindSampleGrids[0] = nullptr;
  m_WindSampleGrids[1] = nullptr;

  m_uiMaxAttractors = 0;
  m_uiAttractorSearchTimer = 0;
  m_AttractorHandles.Clear();
  m_AttractorData[0].Clear();
  m_AttractorData[1].Clear();

  m_EventQueue.Clear();
}

void WParticleEffectInstance::Interrupt()
{
  ClearParticleSystems();
  ClearEventReactions();
  m_bEmitterEnabled = false;
}

void WParticleEffectInstance::SetEmitterEnabled(bool bEnable)
{
  m_bEmitterEnabled = bEnable;

  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    }
  }
}


bool WParticleEffectInstance::HasActiveParticles() const
{
  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->HasActiveParticles())
        return true;
    }
  }

  return false;
}


void WParticleEffectInstance::ClearParticleSystem(WUInt32 index)
{
  if (m_ParticleSystems[index])
  {
    m_pOwnerModule->DestroySystemInstance(m_ParticleSystems[index]);
    m_ParticleSystems[index] = nullptr;
  }
}

void WParticleEffectInstance::ClearParticleSystems()
{
  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    ClearParticleSystem(i);
  }

  m_ParticleSystems.Clear();
}


void WParticleEffectInstance::ClearEventReactions()
{
  for (WUInt32 i = 0; i < m_EventReactions.GetCount(); ++i)
  {
    if (m_EventReactions[i])
    {
      m_EventReactions[i]->GetDynamicRTTI()->GetAllocator()->Deallocate(m_EventReactions[i]);
    }
  }

  m_EventReactions.Clear();
}

bool WParticleEffectInstance::IsContinuous() const
{
  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->IsContinuous())
        return true;
    }
  }

  return false;
}

void WParticleEffectInstance::PreSimulate()
{
  if (m_PreSimulateDuration.GetSeconds() == 0.0)
    return;

  PassTransformToSystems();

  // Pre-simulate the effect, if desired, to get it into a 'good looking' state

  // simulate in large steps to get close
  {
    const WTime tDiff = WTime::MakeFromSeconds(0.5);
    while (m_PreSimulateDuration.GetSeconds() > 10.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // finer steps
  {
    const WTime tDiff = WTime::MakeFromSeconds(0.2);
    while (m_PreSimulateDuration.GetSeconds() > 5.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // even finer
  {
    const WTime tDiff = WTime::MakeFromSeconds(0.1);
    while (m_PreSimulateDuration.GetSeconds() >= 0.1)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // final step if necessary
  if (m_PreSimulateDuration.GetSeconds() > 0.0)
  {
    StepSimulation(m_PreSimulateDuration);
    m_PreSimulateDuration = WTime::MakeFromSeconds(0);
  }

  // Ensure the first game-world update after pre-simulation is not throttled by the
  // invisible-update-rate check. PreSimulate() decrements m_iMinSimStepsToDo to 0, which
  // would cause Update() to skip simulation if the effect hasn't been rendered yet
  // (m_EffectIsVisible == 0). Resetting to 1 here forces exactly one non-throttled update,
  // making simulation deterministic regardless of how much real time elapsed during initialization.
  m_iMinSimStepsToDo = 1;

  if (!IsContinuous())
  {
    // Can't check this at the beginning, because the particle systems are only set up during StepSimulation.
    WLog::Warning("Particle pre-simulation is enabled on an effect that is not continuous.");
  }
}

void WParticleEffectInstance::SetIsVisible() const
{
  // if it is visible this frame, also render it the next few frames
  // this has multiple purposes:
  // 1) it fixes the transition when handing off an effect from a
  //    WParticleComponent to a WParticleFinisherComponent
  //    though this would only need one frame overlap
  // 2) The bounding volume for culling is only computed every couple of frames
  //    so it may be too small and culling could be imprecise
  //    by just rendering it the next 100ms, no matter what, the bounding volume
  //    does not need to be updated so frequently
  m_EffectIsVisible = WClock::GetGlobalClock()->GetAccumulatedTime() + WTime::MakeFromSeconds(0.1);
}


void WParticleEffectInstance::SetVisibleIf(WParticleEffectInstance* pOtherVisible)
{
  W_ASSERT_DEV(pOtherVisible != this, "Invalid effect");
  m_pVisibleIf = pOtherVisible;
}

bool WParticleEffectInstance::IsVisible() const
{
  if (m_pVisibleIf != nullptr)
  {
    return m_pVisibleIf->IsVisible();
  }

  return m_EffectIsVisible >= WClock::GetGlobalClock()->GetAccumulatedTime();
}

void WParticleEffectInstance::Reconfigure(bool bFirstTime, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams)
{
  if (!m_hResource.IsValid())
  {
    WLog::Error("Effect Reconfigure: Effect Resource is invalid");
    return;
  }

  WResourceLock<WParticleEffectResource> pResource(m_hResource, WResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pResource->GetDescriptor().m_Effect;
  const auto& systems = desc.GetParticleSystems();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_fApplyInstanceVelocity = desc.m_fApplyInstanceVelocity;
  m_bSimulateInLocalSpace = desc.m_bSimulateInLocalSpace;
  m_InvisibleUpdateRate = desc.m_InvisibleUpdateRate;

  m_vNumWindSamples.x = desc.m_vNumWindSamples.x;
  m_vNumWindSamples.y = desc.m_vNumWindSamples.y;
  m_vNumWindSamples.z = desc.m_vNumWindSamples.z;
  m_vNumWindSamples.w = desc.m_vNumWindSamples.x * desc.m_vNumWindSamples.y * desc.m_vNumWindSamples.z;
  m_WindSampleGrids[0] = nullptr;
  m_WindSampleGrids[1] = nullptr;

  // parameters
  {
    m_FloatParameters.Clear();
    m_ColorParameters.Clear();

    for (auto it = desc.m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(WTempHashedString(it.Key().GetData()), it.Value());
    }

    for (auto it = desc.m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(WTempHashedString(it.Key().GetData()), it.Value());
    }

    // shared effects do not support per-instance parameters
    if (m_bIsSharedEffect)
    {
      if (!floatParams.IsEmpty() || !colorParams.IsEmpty())
      {
        WLog::Warning("Shared particle effects do not support effect parameters");
      }
    }
    else
    {
      for (WUInt32 p = 0; p < floatParams.GetCount(); ++p)
      {
        SetParameter(floatParams[p].m_sName, floatParams[p].m_Value);
      }

      for (WUInt32 p = 0; p < colorParams.GetCount(); ++p)
      {
        SetParameter(colorParams[p].m_sName, colorParams[p].m_Value);
      }
    }
  }

  if (bFirstTime)
  {
    m_PreSimulateDuration = desc.m_PreSimulateDuration;
  }

  // TODO Check max number of particles etc. to reset

  if (m_ParticleSystems.GetCount() != systems.GetCount())
  {
    // reset everything
    ClearParticleSystems();
  }

  m_ParticleSystems.SetCount(systems.GetCount());

  struct MulCount
  {
    W_DECLARE_POD_TYPE();

    float m_fMultiplier = 1.0f;
    WUInt32 m_uiCount = 0;
  };

  WTempHybridArray<MulCount, 8> systemMaxParticles;
  {
    systemMaxParticles.SetCountUninitialized(systems.GetCount());
    for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      WUInt32 uiMaxParticlesAbs = 0, uiMaxParticlesPerSec = 0;
      for (const WParticleEmitterFactory* pEmitter : systems[i]->GetEmitterFactories())
      {
        WUInt32 uiMaxParticlesAbs0 = 0, uiMaxParticlesPerSec0 = 0;
        pEmitter->QueryMaxParticleCount(uiMaxParticlesAbs0, uiMaxParticlesPerSec0);

        uiMaxParticlesAbs += uiMaxParticlesAbs0;
        uiMaxParticlesPerSec += uiMaxParticlesPerSec0;
      }

      const WTime tLifetime = systems[i]->GetAvgLifetime();

      const WUInt32 uiMaxParticles = WMath::Max(32u, WMath::Max(uiMaxParticlesAbs, (WUInt32)(uiMaxParticlesPerSec * tLifetime.GetSeconds())));

      float fMultiplier = 1.0f;

      for (const WParticleInitializerFactory* pInitializer : systems[i]->GetInitializerFactories())
      {
        fMultiplier *= pInitializer->GetSpawnCountMultiplier(this);
      }

      systemMaxParticles[i].m_fMultiplier = WMath::Max(0.0f, fMultiplier);
      systemMaxParticles[i].m_uiCount = (WUInt32)(uiMaxParticles * systemMaxParticles[i].m_fMultiplier);
    }
  }
  // delete all that have important changes
  {
    for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        if (m_ParticleSystems[i]->GetMaxParticles() != systemMaxParticles[i].m_uiCount)
          ClearParticleSystem(i);
      }
    }
  }

  // recreate where necessary
  {
    for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] == nullptr)
      {
        m_ParticleSystems[i] = m_pOwnerModule->CreateSystemInstance(systemMaxParticles[i].m_uiCount, m_pWorld, this, systemMaxParticles[i].m_fMultiplier);
      }
    }
  }

  const WVec3 vStartVelocity = m_vVelocity * m_fApplyInstanceVelocity;

  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->ConfigureFromTemplate(systems[i]);
    m_ParticleSystems[i]->SetTransform(m_Transform, vStartVelocity);
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    m_ParticleSystems[i]->Finalize();
  }

  // recreate event reactions
  {
    ClearEventReactions();

    m_EventReactions.SetCount(desc.GetEventReactions().GetCount());

    const auto& er = desc.GetEventReactions();
    for (WUInt32 i = 0; i < er.GetCount(); ++i)
    {
      if (m_EventReactions[i] == nullptr)
      {
        m_EventReactions[i] = er[i]->CreateEventReaction(this);
      }
    }
  }
}

bool WParticleEffectInstance::Update(const WTime& diff)
{
  W_PROFILE_SCOPE("PFX: Effect Update");

  WTime tMinStep = WTime::MakeFromSeconds(0);

  if (!IsVisible() && m_iMinSimStepsToDo == 0)
  {
    // shared effects always get paused when they are invisible
    if (IsSharedEffect())
      return true;

    switch (m_InvisibleUpdateRate)
    {
      case WEffectInvisibleUpdateRate::FullUpdate:
        tMinStep = WTime::MakeFromSeconds(1.0 / 60.0);
        break;

      case WEffectInvisibleUpdateRate::Max20fps:
        tMinStep = WTime::MakeFromMilliseconds(50);
        break;

      case WEffectInvisibleUpdateRate::Max10fps:
        tMinStep = WTime::MakeFromMilliseconds(100);
        break;

      case WEffectInvisibleUpdateRate::Max5fps:
        tMinStep = WTime::MakeFromMilliseconds(200);
        break;

      case WEffectInvisibleUpdateRate::Pause:
      {
        if (m_bEmitterEnabled)
        {
          // during regular operation, pause
          return m_uiReviveTimeout > 0;
        }

        // otherwise do infrequent updates to shut the effect down
        tMinStep = WTime::MakeFromMilliseconds(200);
        break;
      }

      case WEffectInvisibleUpdateRate::Discard:
        Interrupt();
        return false;
    }
  }

  m_ElapsedTimeSinceUpdate += diff;
  PassTransformToSystems();

  // if the time step is too big, iterate multiple times
  {
    const WTime tMaxTimeStep = WTime::MakeFromMilliseconds(200); // in sync with Max5fps
    while (m_ElapsedTimeSinceUpdate > tMaxTimeStep)
    {
      m_ElapsedTimeSinceUpdate -= tMaxTimeStep;

      if (!StepSimulation(tMaxTimeStep))
        return false;
    }
  }

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return m_uiReviveTimeout > 0;

  // do the remainder
  const WTime tUpdateDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate = WTime::MakeZero();

  return StepSimulation(tUpdateDiff);
}

bool WParticleEffectInstance::StepSimulation(const WTime& tDiff)
{
  m_TotalEffectLifeTime += tDiff;

  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i] != nullptr)
    {
      auto state = m_ParticleSystems[i]->Update(tDiff);

      if (state == WParticleSystemState::Inactive)
      {
        ClearParticleSystem(i);
      }
      else if (state != WParticleSystemState::OnlyReacting)
      {
        // this is used to delay particle effect death by a couple of frames
        // that way, if an event is in the pipeline that might trigger a reacting emitter,
        // or particles are in the spawn queue, but not yet created, we don't kill the effect too early
        m_uiReviveTimeout = 3;
      }
    }
  }

  m_iMinSimStepsToDo = WMath::Max<WInt8>(m_iMinSimStepsToDo - 1, 0);

  --m_uiReviveTimeout;
  return m_uiReviveTimeout > 0;
}


void WParticleEffectInstance::AddParticleEvent(const WParticleEvent& pe)
{
  // drop events when the capacity is full
  if (m_EventQueue.GetCount() == m_EventQueue.GetCapacity())
    return;

  m_EventQueue.PushBack(pe);
}

void WParticleEffectInstance::RequestWindSamples()
{
  const WUInt32 uiTotalNumSamples = m_vNumWindSamples.w;

  for (auto& grid : m_WindSampleGrids)
  {
    if (grid == nullptr)
    {
      grid = W_NEW(WFoundation::GetAlignedAllocator(), WindSampleGrid);
      grid->m_vMinPos.Set(1000.0f);
      grid->m_vMaxPos.Set(-1000.0f);
      grid->m_vInvCellSize.SetZero();
      grid->m_Samples.SetCountUninitialized(uiTotalNumSamples);
      WMemoryUtils::ZeroFill(grid->m_Samples.GetData(), uiTotalNumSamples);
    }
  }
}

void WParticleEffectInstance::UpdateWindSamples(WTime diff)
{
  const WUInt64 uiFrameCounter = WRenderWorld::GetFrameCounter();
  const WUInt32 uiDataIdx = uiFrameCounter & 1;
  if (m_WindSampleGrids[uiDataIdx] == nullptr || m_BoundingVolume.IsValid() == false)
    return;

  auto& grid = *m_WindSampleGrids[uiDataIdx];
  const auto& oldGrid = *m_WindSampleGrids[(uiDataIdx + 1) & 1];

  const WUInt32 uiNumSamplesX = m_vNumWindSamples.x;
  const WUInt32 uiNumSamplesY = m_vNumWindSamples.y;
  const WUInt32 uiNumSamplesZ = m_vNumWindSamples.z;
  const WUInt32 uiTotalNumSamples = m_vNumWindSamples.w;
  W_ASSERT_DEBUG(grid.m_Samples.GetCount() == uiTotalNumSamples && oldGrid.m_Samples.GetCount() == uiTotalNumSamples, "Invalid number of samples");

  const WSimdVec4f interpolationFactor = WSimdVec4f(1.0f - WMath::Pow(0.1f, diff.AsFloatInSeconds()));

  const WSimdBBox boundingBox = WSimdConversion::ToBBox(m_BoundingVolume.GetBox());
  const WSimdVec4f boundsSize = boundingBox.GetExtents();
  const WSimdVec4f gridSize = WSimdVec4i(uiNumSamplesX, uiNumSamplesY, uiNumSamplesZ).ToFloat();
  WSimdVec4f cellSize = boundsSize.CompDiv(gridSize);
  const WSimdVec4f minPos = boundingBox.m_Min + cellSize * 0.5f;
  const WSimdVec4f maxPos = boundingBox.m_Max - cellSize * 0.5f;

  const WSimdVec4b oldGridValid = oldGrid.m_vMinPos < oldGrid.m_vMaxPos;
  grid.m_vMinPos = WSimdVec4f::Select(oldGridValid, WSimdVec4f::Lerp(oldGrid.m_vMinPos, minPos, interpolationFactor), minPos);
  grid.m_vMaxPos = WSimdVec4f::Select(oldGridValid, WSimdVec4f::Lerp(oldGrid.m_vMaxPos, maxPos, interpolationFactor), maxPos);

  const WSimdVec4f finalGridSize = grid.m_vMaxPos - grid.m_vMinPos;
  const WSimdVec4f maxIndices = gridSize - WSimdVec4f(1.0f);
  cellSize = WSimdVec4f::Select(maxIndices != WSimdVec4f::MakeZero(), finalGridSize.CompDiv(maxIndices), WSimdVec4f::MakeZero());
  grid.m_vInvCellSize = WSimdVec4f::Select(cellSize != WSimdVec4f::MakeZero(), WSimdVec4f(1.0f).CompDiv(cellSize), WSimdVec4f::MakeZero());
  W_ASSERT_DEBUG(grid.m_vInvCellSize.IsValid<3>(), "");

  if (auto pWind = GetWorld()->GetModuleReadOnly<WWindWorldModuleInterface>())
  {
    for (WUInt32 i = 0; i < uiTotalNumSamples; ++i)
    {
      WUInt32 index = i;
      const WUInt32 z = i / (uiNumSamplesX * uiNumSamplesY);
      index -= z * (uiNumSamplesX * uiNumSamplesY);
      const WUInt32 y = index / uiNumSamplesX;
      const WUInt32 x = index - (y * uiNumSamplesX);

      const WSimdVec4f samplePos = grid.m_vMinPos + cellSize.CompMul(WSimdVec4i(x, y, z).ToFloat());

      grid.m_Samples[i] = WSimdVec4f::Lerp(oldGrid.m_Samples[i], pWind->GetWindAtSimd(samplePos), interpolationFactor);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      if (cvar_ParticlesDebugWindSamples)
      {
        const WColor c = WColorScheme::GetColor(WColorScheme::Blue, 8);

        const WVec3 samplePos0 = WSimdConversion::ToVec3(samplePos);
        WDebugRenderer::DrawCross(GetWorld(), samplePos0, 0.1f, c);

        const WVec3 vWind = WSimdConversion::ToVec3(grid.m_Samples[i]);
        const float fWindStrength = vWind.GetLength();
        WDebugRenderer::Draw3DText(GetWorld(), WFmt("{} m/s", WArgF(fWindStrength, 2)), samplePos0, c);

        if (fWindStrength > 0.01f)
        {
          const WQuat q = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), vWind);
          WDebugRenderer::DrawArrow(GetWorld(), fWindStrength, c, WTransform::Make(samplePos0, q));
        }
      }
#endif
    }
  }
  else
  {
    for (auto& sample : grid.m_Samples)
    {
      sample.SetZero();
    }
  }
}

void WParticleEffectInstance::RequestAttractorSamples(WUInt8 uiMaxAttractors)
{
  m_uiMaxAttractors = WMath::Clamp<WUInt8>(uiMaxAttractors, m_uiMaxAttractors, 4);
}

void WParticleEffectInstance::FindNearbyAttractors(WTime diff)
{
  if (m_uiMaxAttractors == 0)
    return;

  // Refresh which attractors are nearby roughly once per second.
  if (m_uiAttractorSearchTimer == 0)
  {
    m_uiAttractorSearchTimer = 90;
    m_AttractorHandles.Clear();

    WSpatialSystem* pSpatialSystem = m_pWorld->GetSpatialSystem();
    if (pSpatialSystem != nullptr)
    {
      const WSpatialData::Category category = WParticleAttractorComponent::GetSpatialCategory();

      WBoundingBoxSphere effectBounds;
      GetBoundingVolume(effectBounds);

      const WVec3 worldCenter = m_Transform.TransformPosition(effectBounds.m_vCenter);
      const float fSearchRadius = WMath::Max(0.0f, effectBounds.m_fSphereRadius);

      WSpatialSystem::QueryParams queryParams;
      queryParams.m_uiCategoryBitmask = category.GetBitmask();

      struct SortEntry
      {
        W_DECLARE_POD_TYPE();
        float fDistSqr;
        WComponentHandle hComponent;
      };

      WTempHybridArray<SortEntry, 8> candidates;

      pSpatialSystem->FindObjectsInSphere(
        WBoundingSphere::MakeFromCenterAndRadius(worldCenter, fSearchRadius),
        queryParams,
        [&](WGameObject* pObject) -> WVisitorExecution::Enum
        {
          WParticleAttractorComponent* pAttractor = nullptr;
          if (pObject->TryGetComponentOfBaseType(pAttractor))
          {
            SortEntry entry;
            entry.hComponent = pAttractor->GetHandle();
            entry.fDistSqr = (pObject->GetGlobalPosition() - worldCenter).GetLengthSquared();
            candidates.PushBack(entry);
          }
          return WVisitorExecution::Continue;
        });

      candidates.Sort([](const SortEntry& a, const SortEntry& b)
        { return a.fDistSqr < b.fDistSqr; });

      const WUInt32 uiCount = WMath::Min(candidates.GetCount(), (WUInt32)m_uiMaxAttractors);
      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        m_AttractorHandles.PushBack(candidates[i].hComponent);
      }
    }
  }
  else
  {
    m_uiAttractorSearchTimer--;
  }

  // Every frame: resolve handles and read live position/properties into the write slot.
  // Worker threads read from the opposite slot (written last frame), so there is no data race.
  const WUInt32 uiWriteIdx = WRenderWorld::GetFrameCounter() & 1;
  auto& writeBuffer = m_AttractorData[uiWriteIdx];
  writeBuffer.Clear();
  for (const WComponentHandle& hComponent : m_AttractorHandles)
  {
    WParticleAttractorComponent* pAttractor = nullptr;
    if (m_pWorld->TryGetComponent(hComponent, pAttractor))
    {
      WParticleAttractorData& data = writeBuffer.ExpandAndGetRef();
      data.m_vPosition = pAttractor->GetOwner()->GetGlobalPosition();
      data.m_fStrength = pAttractor->m_fStrength;
      data.m_fRadius = pAttractor->m_fRadius;
      data.m_fMinDistance = pAttractor->m_fMinDistance;
      data.m_fKillDistance = pAttractor->m_fKillDistance;
    }
  }
}

WArrayPtr<const WParticleAttractorData> WParticleEffectInstance::GetAttractorData() const
{
  // Read from the slot opposite to what the main thread is currently writing.
  const WUInt32 uiReadIdx = (WRenderWorld::GetFrameCounter() + 1) & 1;
  return m_AttractorData[uiReadIdx];
}

WUInt64 WParticleEffectInstance::GetNumActiveParticles() const
{
  WUInt64 num = 0;

  for (auto pSystem : m_ParticleSystems)
  {
    if (pSystem)
    {
      num += pSystem->GetNumActiveParticles();
    }
  }

  return num;
}

void WParticleEffectInstance::SetTransform(const WTransform& transform, const WVec3& vParticleStartVelocity)
{
  m_Transform = transform;
  m_TransformForNextFrame = transform;

  m_vVelocity = vParticleStartVelocity;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

void WParticleEffectInstance::SetTransformForNextFrame(const WTransform& transform, const WVec3& vParticleStartVelocity)
{
  m_TransformForNextFrame = transform;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

WSimdVec4f WParticleEffectInstance::GetWindAt(const WSimdVec4f& vPosition) const
{
  const WUInt64 uiFrameCounter = WRenderWorld::GetFrameCounter();
  const WUInt32 uiDataIdx = (uiFrameCounter + 1) & 1;
  if (m_WindSampleGrids[uiDataIdx] == nullptr)
  {
    return WSimdVec4f::MakeZero();
  }

  auto& grid = *m_WindSampleGrids[uiDataIdx];

  const WUInt32 uiTotalNumSamples = m_vNumWindSamples.w;
  W_ASSERT_DEBUG(grid.m_Samples.GetCount() == uiTotalNumSamples, "Invalid sample count");

  // Sample grid with trilinear interpolation
  WSimdVec4f gridSpacePos = (vPosition - grid.m_vMinPos).CompMul(grid.m_vInvCellSize);
  gridSpacePos = gridSpacePos.CompMax(WSimdVec4f::MakeZero());

  const WSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  const WSimdVec4f weights = gridSpacePos - gridSpacePosFloor;

  const WSimdVec4i maxIndices = WSimdConversion::ToVec4i(m_vNumWindSamples) - WSimdVec4i(1);
  const WSimdVec4i pos0 = WSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  const WSimdVec4i pos1 = (pos0 + WSimdVec4i(1)).CompMin(maxIndices);

  const WInt32 xCount = m_vNumWindSamples.x;
  const WInt32 xyCount = xCount * m_vNumWindSamples.y;
  const WSimdVec4i cXcXYcXcXY = WSimdVec4i(xCount, xyCount, xCount, xyCount);
  const WSimdVec4i y0z0y1z1 = pos0.GetCombined<WSwizzle::YZYZ>(pos1).CompMul(cXcXYcXcXY);
  const WSimdVec4i y0y0y1y1 = y0z0y1z1.Get<WSwizzle::XXZZ>();
  const WSimdVec4i x0x0x1x1 = pos0.GetCombined<WSwizzle::XXXX>(pos1);
  const WSimdVec4i x0x1x0x1 = x0x0x1x1.Get<WSwizzle::XZXZ>();
  const WSimdVec4i y0y0y1y1_plus_x0x1x0x1 = y0y0y1y1 + x0x1x0x1;

  const WSimdVec4f wX = weights.Get<WSwizzle::XXXX>();
  const WSimdVec4f wY = weights.Get<WSwizzle::YYYY>();

  const WSimdVec4f* pSamples = grid.m_Samples.GetData();

  const WSimdVec4i indices_z0 = y0z0y1z1.Get<WSwizzle::YYYY>() + y0y0y1y1_plus_x0x1x0x1;
  const WSimdVec4f sample_z0y0x0 = pSamples[indices_z0.x()];
  const WSimdVec4f sample_z0y0x1 = pSamples[indices_z0.y()];
  const WSimdVec4f res_z0y0 = WSimdVec4f::Lerp(sample_z0y0x0, sample_z0y0x1, wX);

  const WSimdVec4f sample_z0y1x0 = pSamples[indices_z0.z()];
  const WSimdVec4f sample_z0y1x1 = pSamples[indices_z0.w()];
  const WSimdVec4f res_z0y1 = WSimdVec4f::Lerp(sample_z0y1x0, sample_z0y1x1, wX);

  const WSimdVec4f res_z0 = WSimdVec4f::Lerp(res_z0y0, res_z0y1, wY);

  const WSimdVec4i indices_z1 = y0z0y1z1.Get<WSwizzle::WWWW>() + y0y0y1y1_plus_x0x1x0x1;
  const WSimdVec4f sample_z1y0x0 = pSamples[indices_z1.x()];
  const WSimdVec4f sample_z1y0x1 = pSamples[indices_z1.y()];
  const WSimdVec4f res_z1y0 = WSimdVec4f::Lerp(sample_z1y0x0, sample_z1y0x1, wX);

  const WSimdVec4f sample_z1y1x0 = pSamples[indices_z1.z()];
  const WSimdVec4f sample_z1y1x1 = pSamples[indices_z1.w()];
  const WSimdVec4f res_z1y1 = WSimdVec4f::Lerp(sample_z1y1x0, sample_z1y1x1, wX);

  const WSimdVec4f res_z1 = WSimdVec4f::Lerp(res_z1y0, res_z1y1, wY);

  const WSimdVec4f wZ = weights.Get<WSwizzle::ZZZZ>();
  const WSimdVec4f res = WSimdVec4f::Lerp(res_z0, res_z1, wZ);

  return res;
}

void WParticleEffectInstance::PassTransformToSystems()
{
  if (!m_bSimulateInLocalSpace)
  {
    const WVec3 vStartVel = m_vVelocity * m_fApplyInstanceVelocity;

    for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        m_ParticleSystems[i]->SetTransform(m_Transform, vStartVel);
      }
    }
  }
}

void WParticleEffectInstance::AddSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Insert(pSharedInstanceOwner);
}

void WParticleEffectInstance::RemoveSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Remove(pSharedInstanceOwner);
}

bool WParticleEffectInstance::ShouldBeUpdated() const
{
  if (m_hEffectHandle.IsInvalidated())
    return false;

  // do not update shared instances when there is no one watching
  if (m_bIsSharedEffect && m_SharedInstances.GetCount() == 0)
    return false;

  return true;
}

void WParticleEffectInstance::GetBoundingVolume(WBoundingBoxSphere& ref_volume) const
{
  if (!m_BoundingVolume.IsValid())
  {
    ref_volume = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), 0.25f);
    return;
  }

  ref_volume = m_BoundingVolume;

  if (!m_bSimulateInLocalSpace)
  {
    // transform the bounding volume to local space, unless it was already created there
    const WMat4 invTrans = GetTransform().GetAsMat4().GetInverse();
    ref_volume.Transform(invTrans);
  }
}

void WParticleEffectInstance::CombineSystemBoundingVolumes()
{
  WBoundingBoxSphere effectVolume = WBoundingBoxSphere::MakeInvalid();

  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      const WBoundingBoxSphere& systemVolume = m_ParticleSystems[i]->GetBoundingVolume();
      if (systemVolume.IsValid())
      {
        effectVolume.ExpandToInclude(systemVolume);
      }
    }
  }

  m_BoundingVolume = effectVolume;
}

void WParticleEffectInstance::ProcessEventQueues()
{
  m_Transform = m_TransformForNextFrame;
  m_vVelocity = m_vVelocityForNextFrame;

  if (m_EventQueue.IsEmpty())
    return;

  W_PROFILE_SCOPE("PFX: Effect Event Queue");
  for (WUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->ProcessEventQueue(m_EventQueue);
    }
  }

  for (const WParticleEvent& e : m_EventQueue)
  {
    WUInt32 rnd = m_Random.UIntInRange(100);

    for (WParticleEventReaction* pReaction : m_EventReactions)
    {
      if (pReaction->m_sEventName != e.m_EventType)
        continue;

      if (pReaction->m_uiProbability > rnd)
      {
        pReaction->ProcessEvent(e);
        break;
      }

      rnd -= pReaction->m_uiProbability;
    }
  }

  m_EventQueue.Clear();
}

WParticleEffectUpdateTask::WParticleEffectUpdateTask(WParticleEffectInstance* pEffect)
{
  m_pEffect = pEffect;
  m_UpdateDiff = WTime::MakeZero();
}

void WParticleEffectUpdateTask::Execute()
{
  if (HasBeenCanceled())
    return;

  if (m_UpdateDiff.GetSeconds() != 0.0)
  {
    m_pEffect->PreSimulate();

    if (!m_pEffect->Update(m_UpdateDiff))
    {
      const WParticleEffectHandle hEffect = m_pEffect->GetHandle();
      W_ASSERT_DEBUG(!hEffect.IsInvalidated(), "Invalid particle effect handle");

      m_pEffect->GetOwnerWorldModule()->DestroyEffectInstance(hEffect, true, nullptr);
    }
  }
}

void WParticleEffectInstance::SetParameter(const WTempHashedString& sName, float value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (WUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
    {
      m_FloatParameters[i].m_fValue = value;
      return;
    }
  }

  auto& ref = m_FloatParameters.ExpandAndGetRef();
  ref.m_uiNameHash = sName.GetHash();
  ref.m_fValue = value;
}

void WParticleEffectInstance::SetParameter(const WTempHashedString& sName, const WColor& value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (WUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
    {
      m_ColorParameters[i].m_Value = value;
      return;
    }
  }

  auto& ref = m_ColorParameters.ExpandAndGetRef();
  ref.m_uiNameHash = sName.GetHash();
  ref.m_Value = value;
}

WInt32 WParticleEffectInstance::FindFloatParameter(const WTempHashedString& sName) const
{
  for (WUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
      return i;
  }

  return -1;
}

float WParticleEffectInstance::GetFloatParameter(const WTempHashedString& sName, float fDefaultValue) const
{
  if (sName.IsEmpty())
    return fDefaultValue;

  for (WUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
      return m_FloatParameters[i].m_fValue;
  }

  return fDefaultValue;
}

WInt32 WParticleEffectInstance::FindColorParameter(const WTempHashedString& sName) const
{
  for (WUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
      return i;
  }

  return -1;
}

const WColor& WParticleEffectInstance::GetColorParameter(const WTempHashedString& sName, const WColor& defaultValue) const
{
  if (sName.IsEmpty())
    return defaultValue;

  for (WUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
      return m_ColorParameters[i].m_Value;
  }

  return defaultValue;
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectInstance);
