#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/ResourceManager/Implementation/ResourceHandleReflection.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <MiniAudioPlugin/Components/MiniAudioSoundComponent.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>
#include <MiniAudioPlugin/Resources/MiniAudioSoundResource.h>

WMiniAudioSoundComponentManager::WMiniAudioSoundComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

void WMiniAudioSoundComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WMiniAudioSoundComponentManager::UpdateEvents, this);
    desc.m_Phase = WWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(desc);
  }
}

void WMiniAudioSoundComponentManager::Deinitialize()
{
  SUPER::Deinitialize();

  WMiniAudioSingleton::GetSingleton()->StopWorldSounds(GetWorld());
}

void WMiniAudioSoundComponentManager::UpdateEvents(const WWorldModule::UpdateContext& context)
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

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WMiniAudioSoundComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Sound", m_hSound)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_MiniAudio_Sound", WDependencyFlags::Package), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    W_ACCESSOR_PROPERTY("Volume", GetVolume, SetVolume)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("Pitch", GetPitch, SetPitch)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("NoGlobalPitch", GetNoGlobalPitch, SetNoGlobalPitch),
    W_ENUM_MEMBER_PROPERTY("OnFinishedAction", WOnComponentFinishedAction2, m_OnFinishedAction),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Play),
    W_SCRIPT_FUNCTION_PROPERTY(Pause),
    W_SCRIPT_FUNCTION_PROPERTY(Stop),
    W_SCRIPT_FUNCTION_PROPERTY(FadeOut, In, "Delay"),
    W_SCRIPT_FUNCTION_PROPERTY(StartOneShot),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Sound/MiniAudio"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum
{
  NoGlobalPitch = 0,
};

WMiniAudioSoundComponent::WMiniAudioSoundComponent() = default;
WMiniAudioSoundComponent::~WMiniAudioSoundComponent() = default;

void WMiniAudioSoundComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSound;
  s << m_bPaused;
  s << m_fPitch;
  s << m_fComponentVolume;

  WOnComponentFinishedAction2::StorageType type = m_OnFinishedAction;
  s << type;
}

void WMiniAudioSoundComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSound;
  s >> m_bPaused;
  s >> m_fPitch;
  s >> m_fComponentVolume;

  WOnComponentFinishedAction2::StorageType type;
  s >> type;
  m_OnFinishedAction = (WOnComponentFinishedAction2::Enum)type;
}

void WMiniAudioSoundComponent::SetPaused(bool b)
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

void WMiniAudioSoundComponent::SetPitch(float f)
{
  if (f == m_fPitch)
    return;

  m_fPitch = f;
}

void WMiniAudioSoundComponent::SetVolume(float f)
{
  if (f == m_fComponentVolume)
    return;

  m_fComponentVolume = f;
}

void WMiniAudioSoundComponent::SetNoGlobalPitch(bool bEnable)
{
  SetUserFlag(NoGlobalPitch, bEnable);
}

bool WMiniAudioSoundComponent::GetNoGlobalPitch() const
{
  return GetUserFlag(NoGlobalPitch);
}

void WMiniAudioSoundComponent::OnSimulationStarted()
{
  if (!m_bPaused)
  {
    Play();
  }
}

void WMiniAudioSoundComponent::OnDeactivated()
{
  if (m_pInstance)
  {
    WMiniAudioSingleton* pMA = WMiniAudioSingleton::GetSingleton();

    // fade it out over a short period
    pMA->DetachAndFadeOutSoundInstance(m_pInstance, WTime::Milliseconds(500));
  }
}

void WMiniAudioSoundComponent::Play()
{
  if (!m_hSound.IsValid())
    return;

  if (m_pInstance == nullptr)
  {
    WResourceLock<WMiniAudioSoundResource> pResource(m_hSound, WResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pResource.GetAcquireResult() != WResourceAcquireResult::Final)
      return;

    WRandom& rng = GetWorld()->GetRandomNumberGenerator();
    m_pInstance = pResource->InstantiateSound(&rng, GetWorld(), GetHandle());

    m_fResourceVolume = pResource->GetVolume(rng);
    m_fResourcePitch = pResource->GetPitch(rng);

    Update();
  }

  W_MA_CHECK(ma_sound_start(&m_pInstance->m_Sound));
  m_bPaused = false;
}

void WMiniAudioSoundComponent::Pause()
{
  if (m_pInstance)
  {
    W_MA_CHECK(ma_sound_stop(&m_pInstance->m_Sound));
  }
}

void WMiniAudioSoundComponent::Stop()
{
  if (m_pInstance == nullptr)
    return;

  // just free the sound, this will stop it right away
  WMiniAudioSingleton* pMA = WMiniAudioSingleton::GetSingleton();
  pMA->FreeSoundInstance(m_pInstance);
}

void WMiniAudioSoundComponent::FadeOut(WTime fadeDuration)
{
  if (m_pInstance == nullptr)
    return;

  WMiniAudioSingleton* pMA = WMiniAudioSingleton::GetSingleton();
  pMA->DetachAndFadeOutSoundInstance(m_pInstance, fadeDuration);
}

void WMiniAudioSoundComponent::StartOneShot()
{
  if (!m_hSound.IsValid())
    return;

  WResourceLock<WMiniAudioSoundResource> pResource(m_hSound, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pResource.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  WRandom& rng = GetWorld()->GetRandomNumberGenerator();
  auto pInstance = pResource->InstantiateSound(&rng, GetWorld(), {});

  const float fResourceVolume = pResource->GetVolume(rng);
  const float fResourcePitch = pResource->GetPitch(rng);

  UpdateParameters(pInstance, m_fComponentVolume * fResourceVolume, m_fPitch * fResourcePitch);

  // the sound will play until it ends and then get cleaned up automatically
  W_MA_CHECK(ma_sound_start(&pInstance->m_Sound));
}

void WMiniAudioSoundComponent::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  WOnComponentFinishedAction2::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void WMiniAudioSoundComponent::Update()
{
  if (m_pInstance)
  {
    UpdateParameters(m_pInstance, m_fComponentVolume * m_fResourceVolume, m_fPitch * m_fResourcePitch);
  }
}

void WMiniAudioSoundComponent::UpdateParameters(WMiniAudioSoundInstance* pInstance, float fVolume, float fPitch) const
{
  const WVec3 pos = GetOwner()->GetGlobalPosition();

  ma_sound_set_position(&pInstance->m_Sound, pos.x, pos.y, pos.z);

  if (GetNoGlobalPitch())
  {
    ma_sound_set_pitch(&pInstance->m_Sound, fPitch);
  }
  else
  {
    ma_sound_set_pitch(&pInstance->m_Sound, fPitch * (float)GetWorld()->GetClock().GetSpeed());
  }

  ma_sound_set_volume(&pInstance->m_Sound, fVolume);

  // no need to set the direction, we currently don't support directional sounds
  // const WVec3 dir = GetOwner()->GetGlobalDirForwards();
  // ma_sound_set_direction(&m_pInstance->m_Sound, dir.x, dir.y, dir.z);
}

void WMiniAudioSoundComponent::SoundFinished()
{
  // reset used sound
  m_pInstance = nullptr;

  // TODO MiniAudio: send event
  // WMsgFmodSoundFinished msg;
  // m_SoundFinishedEventSender.SendEventMessage(msg, this, GetOwner());

  WOnComponentFinishedAction2::HandleFinishedAction(this, m_OnFinishedAction);

  if (m_OnFinishedAction == WOnComponentFinishedAction2::Restart)
  {
    Play();
  }
}
