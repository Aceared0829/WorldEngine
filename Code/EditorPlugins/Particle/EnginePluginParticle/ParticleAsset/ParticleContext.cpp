#include <EnginePluginParticle/EnginePluginParticlePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleContext, 1, WRTTIDefaultAllocator<WParticleContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Particle Effect"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleContext::WParticleContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

WParticleContext::~WParticleContext() = default;

void WParticleContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WSimulationSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages

    auto msg = static_cast<const WSimulationSettingsMsgToEngine*>(pMsg);

    m_pWorld->SetWorldSimulationEnabled(msg->m_bSimulateWorld);
    m_pWorld->GetClock().SetSpeed(msg->m_fSimulationSpeed);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WEditorEngineRestartSimulationMsg>())
  {
    RestartEffect();
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WEditorEngineLoopAnimationMsg>())
  {
    SetAutoRestartEffect(((const WEditorEngineLoopAnimationMsg*)pMsg)->m_bLoop);
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg);
}

void WParticleContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WParticleComponentManager* pCompMan = pWorld->GetOrCreateComponentManager<WParticleComponentManager>();


  // Preview Effect
  {
    WGameObjectDesc obj;
    WGameObject* pObj;
    obj.m_sName.Assign("ParticlePreview");
    pWorld->CreateObject(obj, pObj);

    pCompMan->CreateComponent(pObj, m_pComponent);
    m_pComponent->m_OnFinishedAction = WOnComponentFinishedAction2::Restart;
    m_pComponent->m_MinRestartDelay = WTime::MakeFromSeconds(0.5);

    WStringBuilder sParticleGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sParticleGuid);
    m_hParticle = WResourceManager::LoadResource<WParticleEffectResource>(sParticleGuid);

    m_pComponent->SetParticleEffect(m_hParticle);
  }

  const char* szMeshName = "ParticlePreviewBackgroundMesh";
  m_hPreviewMeshResource = WResourceManager::GetExistingResource<WMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "ParticlePreviewBackgroundMeshBuffer";

    WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      WGeometry geom;

      geom.AddBox(WVec3(4, 4, 4), true);
      geom.ComputeTangents();

      WMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }
    {
      WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);

      WMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.WMaterialAsset
      md.ComputeBounds();

      m_hPreviewMeshResource = WResourceManager::GetOrCreateResource<WMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  WPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();

  // Background Mesh
  {
    WGameObjectDesc obj;
    obj.m_sName.Assign("ParticleBackground");

    const WColor bgColor(0.3f, 0.3f, 0.3f);

    for (int y = -1; y <= 5; ++y)
    {
      for (int x = -5; x <= 5; ++x)
      {
        WGameObject* pObj;
        obj.m_LocalPosition.Set(6, (float)x * 4, 1 + (float)y * 4);
        pWorld->CreateObject(obj, pObj);

        WMeshComponent* pMesh;
        WMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, WVec3(4, 4, 4));
      }
    }

    for (int y = -5; y <= 5; ++y)
    {
      for (int x = -5; x <= 1; ++x)
      {
        WGameObject* pObj;
        obj.m_LocalPosition.Set((float)x * 4, (float)y * 4, -3);
        pWorld->CreateObject(obj, pObj);

        WMeshComponent* pMesh;
        WMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, WVec3(4, 4, 4));
      }
    }

    for (int x = -5; x <= 5; ++x)
    {
      WGameObject* pObj;
      obj.m_LocalPosition.Set(4, (float)x * 4, -2);
      obj.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(45));
      pWorld->CreateObject(obj, pObj);

      WMeshComponent* pMesh;
      WMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
      pMesh->SetColor(bgColor);

      if (pPhysicsInterface)
        pPhysicsInterface->AddStaticCollisionBox(pObj, WVec3(4, 4, 4));
    }
  }
}

WEngineProcessViewContext* WParticleContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WParticleViewContext, this);
}

void WParticleContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

void WParticleContext::OnThumbnailViewContextRequested()
{
  m_ThumbnailBoundingVolume = WBoundingBoxSphere::MakeInvalid();
}

bool WParticleContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  WParticleViewContext* pParticleViewContext = static_cast<WParticleViewContext*>(pThumbnailViewContext);

  if (!m_ThumbnailBoundingVolume.IsValid())
  {
    W_LOCK(m_pWorld->GetWriteMarker());

    const bool bWorldPaused = m_pWorld->GetClock().GetPaused();

    // make sure the component restarts as soon as possible
    {
      const WTime restartDelay = m_pComponent->m_MinRestartDelay;
      const auto onFinished = m_pComponent->m_OnFinishedAction;
      const double fClockSpeed = m_pWorld->GetClock().GetSpeed();

      m_pComponent->m_MinRestartDelay = WTime::MakeZero();
      m_pComponent->m_OnFinishedAction = WOnComponentFinishedAction2::Restart;

      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(false);
      m_pWorld->GetClock().SetSpeed(10);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->GetClock().SetSpeed(fClockSpeed);
      m_pWorld->SetWorldSimulationEnabled(false);

      m_pComponent->m_MinRestartDelay = restartDelay;
      m_pComponent->m_OnFinishedAction = onFinished;
    }

    if (m_pComponent && !m_pComponent->m_EffectController.IsAlive())
    {
      // if this happens, the effect has finished and we need to wait for it to restart, so that it can be reconfigured for the screenshot
      // not very clean solution

      pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
      return false;
    }

    if (m_pComponent && m_pComponent->m_EffectController.IsAlive())
    {
      // set a fixed random seed
      m_pComponent->m_uiRandomSeed = 11;

      const WUInt32 uiMinSimSteps = 3;
      WUInt32 uiSimStepsNeeded = uiMinSimSteps;

      if (m_pComponent->m_EffectController.IsContinuousEffect())
      {
        uiSimStepsNeeded = 30;
      }
      else
      {
        m_pComponent->InterruptEffect();
        m_pComponent->StartEffect();

        WUInt64 uiMostParticles = 0;
        WUInt64 uiMostParticlesStep = 0;

        for (WUInt32 step = 0; step < 30; ++step)
        {
          // step once, to get the initial bbox out of the way
          m_pComponent->m_EffectController.ForceVisible();
          m_pComponent->m_EffectController.Tick(WTime::MakeFromSeconds(0.05));

          if (!m_pComponent->m_EffectController.IsAlive())
            break;

          const WUInt64 numParticles = m_pComponent->m_EffectController.GetNumActiveParticles();

          if (step == uiMinSimSteps && numParticles > 0)
          {
            uiMostParticles = 0;
          }

          if (numParticles > uiMostParticles)
          {
            // this is the step with the largest number of particles
            // but usually a few steps later is the best step to capture

            uiMostParticles = numParticles;
            uiMostParticlesStep = step;
            uiSimStepsNeeded = step;
          }
          else if ((numParticles > uiMostParticles * 0.8f) && (step < uiMostParticlesStep + 5))
          {
            // if a few steps later we still have a decent amount of particles (so it didn't drop significantly),
            // prefer to use that step
            uiSimStepsNeeded = step;
          }
        }
      }

      m_pComponent->InterruptEffect();
      m_pComponent->StartEffect();

      for (WUInt32 step = 0; step < uiSimStepsNeeded; ++step)
      {
        m_pComponent->m_EffectController.ForceVisible();
        m_pComponent->m_EffectController.Tick(WTime::MakeFromSeconds(0.05));

        if (m_pComponent->m_EffectController.IsAlive())
        {
          m_pComponent->m_EffectController.GetBoundingVolume(m_ThumbnailBoundingVolume);

          // shrink the bbox to zoom in
          m_ThumbnailBoundingVolume.m_fSphereRadius *= 0.7f;
          m_ThumbnailBoundingVolume.m_vBoxHalfExtents *= 0.7f;
        }
      }

      m_pComponent->m_uiRandomSeed = 0;

      // tick the world once more, so that the effect passes on its bounding box to the culling system
      // otherwise the effect is not rendered, when only the thumbnail is updated, but the document is not open
      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(true);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->SetWorldSimulationEnabled(false);
    }
  }

  pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
  return true;
}

void WParticleContext::RestartEffect()
{
  W_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->InterruptEffect();
    m_pComponent->StartEffect();
  }
}

void WParticleContext::SetAutoRestartEffect(bool loop)
{
  W_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->m_OnFinishedAction = loop ? WOnComponentFinishedAction2::Restart : WOnComponentFinishedAction2::None;
  }
}
