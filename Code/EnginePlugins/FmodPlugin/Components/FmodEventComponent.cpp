#include <FmodPlugin/FmodPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

static_assert(sizeof(WFmodParameterId) == sizeof(FMOD_STUDIO_PARAMETER_ID));

W_ALWAYS_INLINE FMOD_STUDIO_PARAMETER_ID ConvertEzToFmodId(WFmodParameterId paramId)
{
  return *reinterpret_cast<FMOD_STUDIO_PARAMETER_ID*>(&paramId);
}

W_ALWAYS_INLINE WFmodParameterId ConvertFmodToEzId(FMOD_STUDIO_PARAMETER_ID paramId)
{
  return *reinterpret_cast<WFmodParameterId*>(&paramId);
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgFmodSoundFinished);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgFmodSoundFinished, 1, WRTTIDefaultAllocator<WMsgFmodSoundFinished>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WCVarInt cvar_FmodOcclusionNumRays("FMOD.Occlusion.NumRays", 2, WCVarFlags::Default, "Number of occlusion rays per component per frame");

static WVec3 s_InSpherePositions[32];
static bool s_bInSpherePositionsInitialized = false;

WFmodEventComponentManager::WFmodEventComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
  if (!s_bInSpherePositionsInitialized)
  {
    s_bInSpherePositionsInitialized = true;

    WRandom rng;
    rng.Initialize(3);
    WRandomGauss rngGauss;
    rngGauss.Initialize(27, 0xFFFF);

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_InSpherePositions); ++i)
    {
      WVec3& pos = s_InSpherePositions[i];
      pos.x = (float)rngGauss.SignedValue();
      pos.y = (float)rngGauss.SignedValue();
      pos.z = (float)rngGauss.SignedValue();

      float fRadius = WMath::Pow((float)rng.DoubleZeroToOneExclusive(), 1.0f / 3.0f);
      fRadius = fRadius * 0.5f + 0.5f;
      pos.SetLength(fRadius).IgnoreResult();
    }
  }
}

void WFmodEventComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WFmodEventComponentManager::UpdateOcclusion, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_uiAsyncPhaseBatchSize = 8;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WFmodEventComponentManager::UpdateEvents, this);
    desc.m_Phase = WWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WFmodEventComponentManager::ResourceEventHandler, this));
}

void WFmodEventComponentManager::Deinitialize()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WFmodEventComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

WUInt32 WFmodEventComponentManager::AddOcclusionState(WFmodEventComponent* pComponent, WFmodParameterId occlusionParamId, float fRadius)
{
  auto& occlusionState = m_OcclusionStates.ExpandAndGetRef();
  occlusionState.m_pComponent = pComponent;
  occlusionState.m_OcclusionParamId = occlusionParamId;
  occlusionState.m_fRadius = fRadius;

  if (const auto pPhysicsWorldModule = GetWorld()->GetModule<WPhysicsWorldModuleInterface>())
  {
    WVec3 listenerPos = WFmod::GetSingleton()->GetListenerPosition();
    ShootOcclusionRays(occlusionState, listenerPos, 8, pPhysicsWorldModule, WTime::MakeFromSeconds(1000.0));
  }

  return m_OcclusionStates.GetCount() - 1;
}

void WFmodEventComponentManager::RemoveOcclusionState(WUInt32 uiIndex)
{
  if (uiIndex >= m_OcclusionStates.GetCount())
    return;

  m_OcclusionStates.RemoveAtAndSwap(uiIndex);

  if (uiIndex != m_OcclusionStates.GetCount())
  {
    m_OcclusionStates[uiIndex].m_pComponent->m_uiOcclusionStateIndex = uiIndex;
  }
}

void WFmodEventComponentManager::ShootOcclusionRays(OcclusionState& state, WVec3 listenerPos, WUInt32 uiNumRays, const WPhysicsWorldModuleInterface* pPhysicsWorldModule, WTime deltaTime)
{
  uiNumRays = WMath::Min(uiNumRays, 32u);

  WVec3 centerPos = state.m_pComponent->GetOwner()->GetGlobalPosition();
  WUInt8 uiCollisionLayer = state.m_pComponent->m_uiOcclusionCollisionLayer;
  WPhysicsCastResult hitResult;

  for (WUInt32 i = 0; i < uiNumRays; ++i)
  {
    WUInt32 uiRayIndex = state.m_uiNextRayIndex;
    WVec3 targetPos = centerPos + s_InSpherePositions[uiRayIndex] * state.m_fRadius;
    WVec3 dir = targetPos - listenerPos;
    float fDistance = dir.GetLengthAndNormalize();

    WPhysicsQueryParameters query(uiCollisionLayer);
    query.m_bIgnoreInitialOverlap = true;
    query.m_ShapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic;

    bool bHit = pPhysicsWorldModule->Raycast(hitResult, listenerPos, dir, fDistance, query);
    if (bHit)
    {
      state.m_uiRaycastHits |= (1 << uiRayIndex);
    }
    else
    {
      state.m_uiRaycastHits &= ~(1 << uiRayIndex);
    }

    state.m_uiNextRayIndex = (state.m_uiNextRayIndex + 1) % 32;
    state.m_uiNumUsedRays = WMath::Min<WUInt8>(state.m_uiNumUsedRays + 1, 32);
  }

  float fNewOcclusionValue = (float)WMath::CountBits(state.m_uiRaycastHits) / state.m_uiNumUsedRays;
  float fNormalizedDistance = WMath::Min((centerPos - listenerPos).GetLength() / state.m_fRadius, 1.0f);
  fNewOcclusionValue = WMath::Max(fNewOcclusionValue - 1.0f + fNormalizedDistance, 0.0f);
  state.m_fLastOcclusionValue = WMath::Lerp(state.m_fLastOcclusionValue, fNewOcclusionValue, WMath::Min(deltaTime.GetSeconds() * 8.0, 1.0));
}

void WFmodEventComponentManager::UpdateOcclusion(const WWorldModule::UpdateContext& context)
{
  if (const auto pPhysicsWorldModule = GetWorld()->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WVec3 listenerPos = WFmod::GetSingleton()->GetListenerPosition();
    WTime deltaTime = GetWorld()->GetClock().GetTimeDiff();

    WUInt32 uiNumRays = WMath::Clamp<int>(cvar_FmodOcclusionNumRays, 1, 32);

    for (auto& occlusionState : m_OcclusionStates)
    {
      ShootOcclusionRays(occlusionState, listenerPos, uiNumRays, pPhysicsWorldModule, deltaTime);
    }
  }
}

void WFmodEventComponentManager::UpdateEvents(const WWorldModule::UpdateContext& context)
{
  constexpr WUInt32 uiUpdatesPerSec = 20;
  constexpr WTime tUpdateRate = WTime::Milliseconds(1000 / uiUpdatesPerSec);

  const float fUpdateFraction = (GetWorld()->GetClock().GetTimeDiff() / tUpdateRate).AsFloatInSeconds();

  const WUInt32 uiNumComps = m_ComponentStorage.GetCount();

  if (m_uiFirstComponentIndex >= uiNumComps)
  {
    m_uiFirstComponentIndex = 0;
  }

  const WUInt32 uiNumUpdate = static_cast<WUInt32>(uiNumComps * fUpdateFraction) + 1;
  const WUInt32 uiLastCompP1 = WMath::Min(m_uiFirstComponentIndex + uiNumUpdate, uiNumComps);

  for (auto it = m_ComponentStorage.GetIterator(m_uiFirstComponentIndex, uiNumComps); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;

    // a lot of components will actually be inactive (waiting to be reused)
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }

  m_uiFirstComponentIndex = uiLastCompP1;
}

void WFmodEventComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WFmodSoundEventResource>())
  {
    WFmodSoundEventResourceHandle hResource((WFmodSoundEventResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hSoundEvent == hResource)
      {
        it->InvalidateResource(true);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WFmodEventComponent, 4, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    W_ACCESSOR_PROPERTY("Volume", GetVolume, SetVolume)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("Pitch", GetPitch, SetPitch)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("NoGlobalPitch", GetNoGlobalPitch, SetNoGlobalPitch),
    W_RESOURCE_ACCESSOR_PROPERTY("SoundEvent", GetSoundEvent, SetSoundEvent)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Fmod_Event", WDependencyFlags::Package), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("UseOcclusion", GetUseOcclusion, SetUseOcclusion),
    W_ACCESSOR_PROPERTY("OcclusionThreshold", GetOcclusionThreshold, SetOcclusionThreshold)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("OcclusionCollisionLayer", GetOcclusionCollisionLayer, SetOcclusionCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_ENUM_MEMBER_PROPERTY("OnFinishedAction", WOnComponentFinishedAction, m_OnFinishedAction),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    //W_FUNCTION_PROPERTY("Preview", StartOneShot), // This doesn't seem to be working anymore, and I cannot find code for exposing it in the UI either
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
    W_MESSAGE_HANDLER(WMsgSetFloatParameter, OnMsgSetFloatParameter),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_MESSAGESENDERS
  {
    W_MESSAGE_SENDER(m_SoundFinishedEventSender),
  }
  W_END_MESSAGESENDERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Play),
    W_SCRIPT_FUNCTION_PROPERTY(Pause),
    W_SCRIPT_FUNCTION_PROPERTY(Stop),
    W_SCRIPT_FUNCTION_PROPERTY(FadeOut),
    W_SCRIPT_FUNCTION_PROPERTY(StartOneShot),
    W_SCRIPT_FUNCTION_PROPERTY(SoundCue),
    W_SCRIPT_FUNCTION_PROPERTY(SetEventParameter, In, "ParamName", In, "Value"),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum
{
  ShowDebugInfoFlag = 0,
  NoGlobalPitch = 1,
};

WFmodEventComponent::WFmodEventComponent()
{
  m_pEventDesc = nullptr;
  m_pEventInstance = nullptr;
  m_bPaused = false;
  m_bUseOcclusion = false;
  m_uiOcclusionThreshold = 128;
  m_uiOcclusionCollisionLayer = 0;
  m_fPitch = 1.0f;
  m_fVolume = 1.0f;
}

WFmodEventComponent::~WFmodEventComponent() = default;

void WFmodEventComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bPaused;
  s << m_bUseOcclusion;
  s << m_uiOcclusionThreshold;
  s << m_uiOcclusionCollisionLayer;
  s << m_fPitch;
  s << m_fVolume;

  s << m_hSoundEvent;

  WOnComponentFinishedAction::StorageType type = m_OnFinishedAction;
  s << type;

  WInt32 iTimelinePosition = -1;

  if (m_pEventInstance)
  {
    FMOD_STUDIO_PLAYBACK_STATE state;
    m_pEventInstance->getPlaybackState(&state);

    if (state == FMOD_STUDIO_PLAYBACK_STARTING || state == FMOD_STUDIO_PLAYBACK_PLAYING || state == FMOD_STUDIO_PLAYBACK_SUSTAINING)
    {
      m_pEventInstance->getTimelinePosition(&iTimelinePosition);
    }
  }

  s << iTimelinePosition;
}

void WFmodEventComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_bPaused;

  if (uiVersion >= 3)
  {
    s >> m_bUseOcclusion;
  }

  if (uiVersion >= 4)
  {
    s >> m_uiOcclusionThreshold;
    s >> m_uiOcclusionCollisionLayer;
  }

  s >> m_fPitch;
  s >> m_fVolume;
  s >> m_hSoundEvent;

  WOnComponentFinishedAction::StorageType type;
  s >> type;
  m_OnFinishedAction = (WOnComponentFinishedAction::Enum)type;

  s >> m_iTimelinePosition;
}

void WFmodEventComponent::SetPaused(bool b)
{
  if (b == m_bPaused)
    return;

  m_bPaused = b;

  if (m_bPaused)
  {
    Pause();
  }
  else
  {
    Play();
  }
}

void WFmodEventComponent::SetUseOcclusion(bool b)
{
  if (b == GetUseOcclusion())
    return;

  m_bUseOcclusion = b;

  if (!b)
  {
    static_cast<WFmodEventComponentManager*>(GetOwningManager())->RemoveOcclusionState(m_uiOcclusionStateIndex);
    m_uiOcclusionStateIndex = WInvalidIndex;
  }
}

void WFmodEventComponent::SetOcclusionCollisionLayer(WUInt8 uiCollisionLayer)
{
  m_uiOcclusionCollisionLayer = uiCollisionLayer;
}

void WFmodEventComponent::SetOcclusionThreshold(float fThreshold)
{
  m_uiOcclusionThreshold = WMath::ColorFloatToByte(fThreshold);
}

float WFmodEventComponent::GetOcclusionThreshold() const
{
  return WMath::ColorByteToFloat(m_uiOcclusionThreshold);
}

void WFmodEventComponent::SetPitch(float f)
{
  if (f == m_fPitch)
    return;

  m_fPitch = f;

  if (m_pEventInstance != nullptr)
  {
    if (GetNoGlobalPitch())
    {
      W_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch));
    }
    else
    {
      W_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
    }
  }
}

void WFmodEventComponent::SetVolume(float f)
{
  if (f == m_fVolume)
    return;

  m_fVolume = f;

  if (m_pEventInstance != nullptr)
  {
    W_FMOD_ASSERT(m_pEventInstance->setVolume(m_fVolume));
  }
}

void WFmodEventComponent::SetSoundEvent(const WFmodSoundEventResourceHandle& hSoundEvent)
{
  if (m_pEventInstance)
  {
    Stop();

    W_FMOD_ASSERT(m_pEventInstance->release());
    m_pEventInstance = nullptr;
    m_iTimelinePosition = -1;
  }

  m_hSoundEvent = hSoundEvent;
}

void WFmodEventComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(ShowDebugInfoFlag, bShow);
}

bool WFmodEventComponent::GetShowDebugInfo() const
{
  return GetUserFlag(ShowDebugInfoFlag);
}

void WFmodEventComponent::SetNoGlobalPitch(bool bEnable)
{
  SetUserFlag(NoGlobalPitch, bEnable);
}

bool WFmodEventComponent::GetNoGlobalPitch() const
{
  return GetUserFlag(NoGlobalPitch);
}

void WFmodEventComponent::OnSimulationStarted()
{
  if (!m_bPaused)
  {
    Play();
  }
}

void WFmodEventComponent::OnDeactivated()
{
  if (GetUseOcclusion())
  {
    static_cast<WFmodEventComponentManager*>(GetOwningManager())->RemoveOcclusionState(m_uiOcclusionStateIndex);
    m_uiOcclusionStateIndex = WInvalidIndex;
  }

  FadeOut();
}

void WFmodEventComponent::Play()
{
  if (!m_hSoundEvent.IsValid() || !IsActiveAndSimulating())
    return;

  if (m_pEventInstance == nullptr)
  {
    WResourceLock<WFmodSoundEventResource> pEvent(m_hSoundEvent, WResourceAcquireMode::BlockTillLoaded);

    if (pEvent.GetAcquireResult() == WResourceAcquireResult::MissingFallback)
      return;

    m_pEventInstance = pEvent->CreateInstance();
    if (m_pEventInstance == nullptr)
    {
      WLog::Error("Failed to start sound event '{}'.", m_hSoundEvent.GetResourceIdOrDescription());
      return;
    }
  }

  if (m_bPaused)
  {
    m_bPaused = false;
    W_FMOD_ASSERT(m_pEventInstance->setPaused(false));
  }

  UpdateParameters(m_pEventInstance);

  FMOD_STUDIO_PLAYBACK_STATE state;
  W_FMOD_ASSERT(m_pEventInstance->getPlaybackState(&state));

  if (state != FMOD_STUDIO_PLAYBACK_PLAYING && state != FMOD_STUDIO_PLAYBACK_SUSTAINING)
  {
    // reset this, using the value is handled outside this function
    m_iTimelinePosition = -1;

    W_FMOD_ASSERT(m_pEventInstance->start());
  }
}

void WFmodEventComponent::Pause()
{
  if (m_bPaused)
    return;

  m_bPaused = true;

  if (m_pEventInstance == nullptr)
    return;

  W_FMOD_ASSERT(m_pEventInstance->setPaused(m_bPaused));
}

void WFmodEventComponent::FadeOut()
{
  if (m_pEventInstance == nullptr)
    return;

  W_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT));

  W_FMOD_ASSERT(m_pEventInstance->release());
  m_pEventInstance = nullptr;
  m_iTimelinePosition = -1;
}

void WFmodEventComponent::StartOneShot()
{
  if (!m_hSoundEvent.IsValid())
    return;

  WResourceLock<WFmodSoundEventResource> pEvent(m_hSoundEvent, WResourceAcquireMode::BlockTillLoaded);

  if (pEvent.GetAcquireResult() == WResourceAcquireResult::MissingFallback)
  {
    WLog::Debug("Cannot start one-shot sound event, resource is missing.");
    return;
  }

  if (pEvent->GetDescriptor() == nullptr)
  {
    WLog::Debug("Cannot start one-shot sound event, descriptor is null.");
    return;
  }

  bool bIsOneShot = false;
  pEvent->GetDescriptor()->isOneshot(&bIsOneShot);

  // do not start sounds that will not terminate
  if (!bIsOneShot)
  {
    WLog::Warning("WFmodEventComponent::StartOneShot: Request ignored, because sound event '{}' is not a one-shot event.", pEvent->GetResourceIdOrDescription());
    return;
  }

  FMOD::Studio::EventInstance* pEventInstance = pEvent->CreateInstance();

  if (pEventInstance == nullptr)
  {
    WLog::Debug("Cannot start one-shot sound event, instance could not be created.");
    return;
  }

  UpdateParameters(pEventInstance);

  W_FMOD_ASSERT(pEventInstance->start());
  W_FMOD_ASSERT(pEventInstance->release());
}

void WFmodEventComponent::Stop()
{
  if (m_pEventInstance == nullptr)
    return;

  m_iTimelinePosition = -1;
  W_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE));
}

void WFmodEventComponent::SoundCue()
{
  if (m_pEventInstance != nullptr && m_pEventInstance->isValid())
  {
#if ((FMOD_VERSION & 0x0000FF00) >> 8 >= 2)
    W_FMOD_ASSERT(m_pEventInstance->keyOff());
#else
    W_FMOD_ASSERT(m_pEventInstance->triggerCue());
#endif
  }
}

void WFmodEventComponent::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  WOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

WFmodParameterId WFmodEventComponent::FindParameter(const char* szName) const
{
  if (!m_hSoundEvent.IsValid())
    return WFmodParameterId();

  WResourceLock<WFmodSoundEventResource> pEvent(m_hSoundEvent, WResourceAcquireMode::BlockTillLoaded);

  FMOD::Studio::EventDescription* pEventDesc = pEvent->GetDescriptor();
  if (pEventDesc == nullptr || !pEventDesc->isValid())
    return WFmodParameterId();

  FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc;
  if (pEventDesc->getParameterDescriptionByName(szName, &paramDesc) != FMOD_OK)
    return WFmodParameterId();

  return ConvertFmodToEzId(paramDesc.id);
}

void WFmodEventComponent::SetParameter(WFmodParameterId paramId, float fValue)
{
  if (m_pEventInstance == nullptr || !m_pEventInstance->isValid() || paramId.IsInvalidated())
    return;

  m_pEventInstance->setParameterByID(ConvertEzToFmodId(paramId), fValue);
}

float WFmodEventComponent::GetParameter(WFmodParameterId paramId) const
{
  if (m_pEventInstance == nullptr || !m_pEventInstance->isValid() || paramId.IsInvalidated())
    return 0.0f;

  float value = 0;
  float fFinalValue = 0.0f;
  m_pEventInstance->getParameterByID(ConvertEzToFmodId(paramId), &value, &fFinalValue);
  return fFinalValue;
}

void WFmodEventComponent::SetEventParameter(const char* szParamName, float fValue)
{
  WFmodParameterId paramId = FindParameter(szParamName);
  if (paramId.IsInvalidated())
    return;

  SetParameter(paramId, fValue);
}

void WFmodEventComponent::OnMsgSetFloatParameter(WMsgSetFloatParameter& ref_msg)
{
  SetEventParameter(ref_msg.m_sParameterName, ref_msg.m_fValue);
}

void WFmodEventComponent::Update()
{
  FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_FORCEINT;

  if (m_pEventInstance)
  {
    if (!m_pEventInstance->isValid())
    {
      InvalidateResource(false);
      return;
    }

    UpdateParameters(m_pEventInstance);
    if (GetUseOcclusion())
    {
      UpdateOcclusion();
    }

    W_FMOD_ASSERT(m_pEventInstance->getPlaybackState(&state));

    if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
    {
      m_iTimelinePosition = -1;

      WMsgFmodSoundFinished msg;
      m_SoundFinishedEventSender.SendEventMessage(msg, this, GetOwner());

      WOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
    }
  }
  else if (m_iTimelinePosition >= 0 && !m_bPaused)
  {
    // Restore the event to the last playback position
    const WInt32 iTimelinePos = m_iTimelinePosition;

    Play(); // will reset m_iTimelinePosition, that's why it is copied first

    if (m_pEventInstance)
    {
      // we need to store this value again, because it is very likely during a resource reload that we
      // run into the InvalidateResource(false) case at the top of this function, right after this
      // so we can restore the timeline position again
      m_iTimelinePosition = iTimelinePos;
      m_pEventInstance->setTimelinePosition(iTimelinePos);
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  if (GetShowDebugInfo())
  {
    if (m_pEventInstance)
    {
      FMOD::Studio::EventDescription* pDesc = nullptr;
      m_pEventInstance->getDescription(&pDesc);

      bool is3D = false;
      pDesc->is3D(&is3D);
      if (is3D)
      {
        float minDistance = 0.0f;
        float maxDistance = 0.0f;
#  if ((FMOD_VERSION & 0x0000FF00) >> 8 >= 2)
        pDesc->getMinMaxDistance(&minDistance, &maxDistance);
#  else
        pDesc->getMinimumDistance(&minDistance);
        pDesc->getMaximumDistance(&maxDistance);
#  endif

        WDebugRenderer::DrawLineSphere(GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(GetOwner()->GetGlobalPosition(), minDistance), WColor::Blue);
        WDebugRenderer::DrawLineSphere(GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(GetOwner()->GetGlobalPosition(), maxDistance), WColor::Cyan);
      }

      char path[128];
      pDesc->getPath(path, W_ARRAY_SIZE(path), nullptr);

      const char* szStates[] = {"PLAYING", "SUSTAINING", "STOPPED", "STARTING", "STOPPING"};

      const char* szCurrentState = "Invalid";
      if (state != FMOD_STUDIO_PLAYBACK_FORCEINT)
      {
        szCurrentState = szStates[state];
      }

      WStringBuilder sb;
      sb.SetFormat("{}\n{}", path, szCurrentState);

      if (GetUseOcclusion())
      {
        auto& occlusionState = static_cast<WFmodEventComponentManager*>(GetOwningManager())->GetOcclusionState(m_uiOcclusionStateIndex);
        sb.AppendFormat("\nOcclusion: {}", occlusionState.GetOcclusionValue(GetOcclusionThreshold()));

        WVec3 centerPos = GetOwner()->GetGlobalPosition();
        for (WUInt32 uiRayIndex = 0; uiRayIndex < W_ARRAY_SIZE(s_InSpherePositions); ++uiRayIndex)
        {
          WVec3 targetPos = centerPos + s_InSpherePositions[uiRayIndex] * occlusionState.m_fRadius;
          WColor color = (occlusionState.m_uiRaycastHits & (1 << uiRayIndex)) ? WColor::Red : WColor::Green;
          WDebugRenderer::DrawCross(GetWorld(), targetPos, 0.1f, color);
        }
      }

      WDebugRenderer::Draw3DText(GetWorld(), sb, GetOwner()->GetGlobalPosition(), WColor::Cyan);
    }
  }
#endif
}

void WFmodEventComponent::UpdateParameters(FMOD::Studio::EventInstance* pInstance)
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetLinearVelocity();
  const auto fwd = GetOwner()->GetGlobalRotation() * WVec3::MakeAxisX();
  const auto up = GetOwner()->GetGlobalRotation() * WVec3::MakeAxisZ();

  FMOD_3D_ATTRIBUTES attr;
  attr.position.x = pos.x;
  attr.position.y = pos.y;
  attr.position.z = pos.z;
  attr.forward.x = fwd.x;
  attr.forward.y = fwd.y;
  attr.forward.z = fwd.z;
  attr.up.x = up.x;
  attr.up.y = up.y;
  attr.up.z = up.z;
  attr.velocity.x = vel.x;
  attr.velocity.y = vel.y;
  attr.velocity.z = vel.z;

  if (GetNoGlobalPitch())
  {
    W_FMOD_ASSERT(pInstance->setPitch(m_fPitch));
  }
  else
  {
    // have to update pitch every time, in case the clock speed changes
    W_FMOD_ASSERT(pInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
  }

  W_FMOD_ASSERT(pInstance->setVolume(m_fVolume));
  W_FMOD_ASSERT(pInstance->set3DAttributes(&attr));
}

void WFmodEventComponent::UpdateOcclusion()
{
  if (m_uiOcclusionStateIndex == WInvalidIndex)
  {
    WFmodParameterId occlusionParamId = FindParameter("Occlusion");
    if (occlusionParamId.IsInvalidated())
    {
      WLog::Warning("'Occlusion' FMOD Event Parameter could not be found.");
      m_bUseOcclusion = false;
      return;
    }

    float fRadius = 1.0f;
    float fMaxDist = 0;
    {
      FMOD::Studio::EventDescription* pDesc = nullptr;
      m_pEventInstance->getDescription(&pDesc);

      bool is3D = false;
      pDesc->is3D(&is3D);
      if (is3D)
      {
#if ((FMOD_VERSION & 0x0000FF00) >> 8 >= 2)
        pDesc->getMinMaxDistance(&fRadius, &fMaxDist);
#else
        pDesc->getMinimumDistance(&fRadius);
        pDesc->getMaximumDistance(&fMaxDist);
#endif
      }
    }

    m_uiOcclusionStateIndex = static_cast<WFmodEventComponentManager*>(GetOwningManager())->AddOcclusionState(this, occlusionParamId, fRadius);
  }

  auto& occlusionState = static_cast<WFmodEventComponentManager*>(GetOwningManager())->GetOcclusionState(m_uiOcclusionStateIndex);
  SetParameter(occlusionState.m_OcclusionParamId, occlusionState.GetOcclusionValue(GetOcclusionThreshold()));
}

void WFmodEventComponent::InvalidateResource(bool bTryToRestore)
{
  if (m_pEventInstance)
  {
    if (bTryToRestore)
    {
      FMOD_STUDIO_PLAYBACK_STATE state;
      m_pEventInstance->getPlaybackState(&state);

      if (state == FMOD_STUDIO_PLAYBACK_STARTING || state == FMOD_STUDIO_PLAYBACK_PLAYING || state == FMOD_STUDIO_PLAYBACK_SUSTAINING)
      {
        m_pEventInstance->getTimelinePosition(&m_iTimelinePosition);
      }
      else
      {
        // only reset when bTryToRestore is true, otherwise keep the old value
        m_iTimelinePosition = -1;
      }
    }

    // pointer is no longer valid!
    m_pEventInstance = nullptr;
  }
}

W_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodEventComponent);
