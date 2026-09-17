#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <MiniAudioPlugin/Components/MiniAudioSoundComponent.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>
#include <MiniAudioPlugin/Resources/MiniAudioSoundResource.h>

WCVarFloat cvar_MiniAudioMasterVolume("MiniAudio.Volume", 1.0f, WCVarFlags::Save, "Overall volume for all MiniAudio output");
WCVarBool cvar_MiniAudioMute("MiniAudio.Mute", false, WCVarFlags::Default, "Whether MiniAudio output is muted");
WCVarBool cvar_MiniAudioPause("MiniAudio.Pause", false, WCVarFlags::Default, "Whether MiniAudio output is paused");

W_IMPLEMENT_SINGLETON(WMiniAudioSingleton);

static WMiniAudioSingleton g_MiniAudioSingleton;

WMiniAudioSingleton::WMiniAudioSingleton()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_vListenerPosition.SetZero();
}

WMiniAudioSingleton::~WMiniAudioSingleton() = default;

void WMiniAudioSingleton::Startup()
{
  if (m_bInitialized)
    return;

  m_pData = W_DEFAULT_NEW(Data);

  W_LOCK(m_pData->m_Mutex);

  ma_engine_config cfg = ma_engine_config_init();
  // cfg.pLog
  cfg.listenerCount = 1;
  cfg.channels = 0; // native channel count
  // cfg.allocationCallbacks
  // pContext

  auto result = ma_engine_init(&cfg, &m_pData->m_Engine);
  if (result != MA_SUCCESS)
    return;

  m_bInitialized = true;
  UpdateSound();
}

void WMiniAudioSingleton::Shutdown()
{
  // the lock must be released before m_pData (and thus m_pData->m_Mutex) is destroyed below,
  // otherwise the lock guard's destructor unlocks an already-freed mutex
  {
    W_LOCK(m_pData->m_Mutex);

    if (m_bInitialized)
    {
      m_bInitialized = false;

      for (WUInt32 i = 0; i < m_pData->m_SoundInstancesStorage.GetCount(); ++i)
      {
        auto* pInst = &m_pData->m_SoundInstancesStorage[i];

        if (pInst->m_bInUse)
        {
          FreeSoundInstance(pInst);
        }
      }

      for (auto& group : m_pData->m_SoundGroups)
      {
        ma_sound_group_uninit(group.m_pGroup.Borrow());
      }

      m_pData->m_SoundGroups.Clear();

      ma_engine_uninit(&m_pData->m_Engine);
    }
  }

  // finally delete all data
  m_pData.Clear();
}

void WMiniAudioSingleton::SetNumListeners(WUInt8 uiNumListeners)
{
}

WUInt8 WMiniAudioSingleton::GetNumListeners()
{
  return 1;
}

void WMiniAudioSingleton::LoadConfiguration(WStringView sFile)
{
}

void WMiniAudioSingleton::SetOverridePlatform(WStringView sPlatform)
{
}

void WMiniAudioSingleton::UpdateSound()
{
  if (!m_bInitialized)
    return;

  W_LOCK(m_pData->m_Mutex);

  if (cvar_MiniAudioPause)
  {
    const ma_result res = ma_engine_stop(&m_pData->m_Engine);

    if (res == MA_UNAVAILABLE)
      return;

    W_MA_CHECK(res);
  }
  else
  {
    const ma_result res = ma_engine_start(&m_pData->m_Engine);

    if (res == MA_UNAVAILABLE)
      return;

    W_MA_CHECK(res);

    if (cvar_MiniAudioMute)
    {
      W_MA_CHECK(ma_engine_set_volume(&m_pData->m_Engine, 0.0f));
    }
    else
    {
      W_MA_CHECK(ma_engine_set_volume(&m_pData->m_Engine, cvar_MiniAudioMasterVolume));
    }
  }

  if (!m_pData->m_FinishedInstances.IsEmpty())
  {
    for (WUInt32 idx : m_pData->m_FinishedInstances)
    {
      WMiniAudioSoundInstance* pInstance = &m_pData->m_SoundInstancesStorage[idx];

      if (!pInstance->m_hComponent.IsInvalidated())
      {
        WMiniAudioSoundComponent* pSoundComp;

        W_LOCK(pInstance->pWorld->GetWriteMarker());

        if (pInstance->pWorld->TryGetComponent(pInstance->m_hComponent, pSoundComp))
        {
          if (pSoundComp->IsActiveAndSimulating())
          {
            pSoundComp->SoundFinished();
          }
        }
      }

      FreeSoundInstance(pInstance);
    }

    m_pData->m_FinishedInstances.Clear();
  }

  if (!m_pData->m_FadingInstances.IsEmpty())
  {
    for (WUInt32 i = 0; i < m_pData->m_FadingInstances.GetCount(); ++i)
    {
      const WUInt32 idx = m_pData->m_FadingInstances[i];
      WMiniAudioSoundInstance* pInstance = &m_pData->m_SoundInstancesStorage[idx];

      if (!ma_sound_is_playing(&pInstance->m_Sound) || ma_sound_get_current_fade_volume(&pInstance->m_Sound) <= 0.0f)
      {
        FreeSoundInstance(pInstance);

        m_pData->m_FadingInstances.RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

void WMiniAudioSingleton::SetMasterChannelVolume(float fVolume)
{
  cvar_MiniAudioMasterVolume = WMath::Clamp<float>(fVolume, 0.0f, 1.0f);
}

float WMiniAudioSingleton::GetMasterChannelVolume() const
{
  return cvar_MiniAudioMasterVolume;
}

void WMiniAudioSingleton::SetMasterChannelMute(bool bMute)
{
  cvar_MiniAudioMute = bMute;
}

bool WMiniAudioSingleton::GetMasterChannelMute() const
{
  return cvar_MiniAudioMute;
}

void WMiniAudioSingleton::SetMasterChannelPaused(bool bPaused)
{
  cvar_MiniAudioPause = bPaused;
}

bool WMiniAudioSingleton::GetMasterChannelPaused() const
{
  return cvar_MiniAudioPause;
}

void WMiniAudioSingleton::SetSoundGroupVolume(WStringView sGroupName, float fVolume)
{
  auto& group = GetSoundGroup(sGroupName);
  group.m_fVolume = fVolume;

  ma_sound_group_set_volume(group.m_pGroup.Borrow(), fVolume);
}

float WMiniAudioSingleton::GetSoundGroupVolume(WStringView sGroupName) const
{
  for (WUInt32 i = 0; i < m_pData->m_SoundGroups.GetCount(); ++i)
  {
    if (m_pData->m_SoundGroups[i].m_sName == sGroupName)
      return m_pData->m_SoundGroups[i].m_fVolume;
  }

  return 1.0f;
}

WMiniAudioSingleton::SoundGroup& WMiniAudioSingleton::GetSoundGroup(WStringView sGroupName)
{
  for (WUInt32 i = 0; i < m_pData->m_SoundGroups.GetCount(); ++i)
  {
    if (m_pData->m_SoundGroups[i].m_sName == sGroupName)
      return m_pData->m_SoundGroups[i];
  }

  auto& group = m_pData->m_SoundGroups.ExpandAndGetRef();
  group.m_sName = sGroupName;
  group.m_pGroup = W_DEFAULT_NEW(ma_sound_group);

  ma_sound_group_init(GetEngine(), 0, nullptr, group.m_pGroup.Borrow());

  return group;
}

void WMiniAudioSingleton::GameApplicationEventHandler(const WGameApplicationExecutionEvent& e)
{
  if (e.m_Type == WGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    WMiniAudioSingleton::GetSingleton()->UpdateSound();
  }
}

void WMiniAudioSingleton::SetListenerOverrideMode(bool bEnabled)
{
  m_bListenerOverrideMode = bEnabled;
}

void WMiniAudioSingleton::SetListener(WInt32 iIndex, const WVec3& vPosition, const WVec3& vForward, const WVec3& vUp, const WVec3& vVelocity)
{
  if (m_bListenerOverrideMode)
  {
    if (iIndex != -1)
      return;

    iIndex = 0;
  }

  if (iIndex < 0 || iIndex >= /*FMOD_MAX_LISTENERS*/ 1)
    return;

  if (iIndex == 0)
  {
    m_vListenerPosition = vPosition;
  }

  const WVec3 vMaUp = -vUp; // W and MiniAudio use different coordinate systems

  ma_engine_listener_set_position(&m_pData->m_Engine, iIndex, vPosition.x, vPosition.y, vPosition.z);
  ma_engine_listener_set_direction(&m_pData->m_Engine, iIndex, vForward.x, vForward.y, vForward.z);
  ma_engine_listener_set_world_up(&m_pData->m_Engine, iIndex, vMaUp.x, vMaUp.y, vMaUp.z);
  ma_engine_listener_set_velocity(&m_pData->m_Engine, iIndex, vVelocity.x, vVelocity.y, vVelocity.z);
}

WResult WMiniAudioSingleton::OneShotSound(WWorld* pWorld, WStringView sResourceID, const WTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  WMiniAudioSoundResourceHandle hSound = WResourceManager::LoadResource<WMiniAudioSoundResource>(sResourceID);

  if (!hSound.IsValid())
    return W_FAILURE;

  WResourceLock<WMiniAudioSoundResource> pResource(hSound, bBlockIfNotLoaded ? WResourceAcquireMode::BlockTillLoaded_NeverFail : WResourceAcquireMode::AllowLoadingFallback_NeverFail);

  if (pResource.GetAcquireResult() != WResourceAcquireResult::Final)
    return W_FAILURE;

  if (pResource->GetLoop())
    return W_FAILURE; // never play looping sounds

  WRandom* pRng = nullptr;

  if (pWorld)
  {
    pRng = &pWorld->GetRandomNumberGenerator();

    fVolume *= pResource->GetVolume(*pRng);
    fPitch *= pResource->GetPitch(*pRng);
  }

  auto pInstance = pResource->InstantiateSound(pRng, pWorld, {});

  const WVec3 pos = globalPosition.m_vPosition;
  ma_sound_set_position(&pInstance->m_Sound, pos.x, pos.y, pos.z);
  ma_sound_set_pitch(&pInstance->m_Sound, fPitch);
  ma_sound_set_volume(&pInstance->m_Sound, fVolume);

  // the sound will play until it ends and then get cleaned up automatically
  W_MA_CHECK(ma_sound_start(&pInstance->m_Sound));

  return W_SUCCESS;
}

static void SoundEndedCallback(void* pUserData, ma_sound* pSound)
{
  WMiniAudioSingleton::GetSingleton()->SoundEnded((WMiniAudioSoundInstance*)pUserData);
}

WMiniAudioSoundInstance* WMiniAudioSingleton::AllocateSoundInstance(const WDataBuffer& audioData, WWorld* pWorld, WComponentHandle hComponent, ma_sound_group* pGroup)
{
  if (!m_bInitialized)
    return nullptr;

  W_LOCK(m_pData->m_Mutex);

  WMiniAudioSoundInstance* pInstance = nullptr;

  if (!m_pData->m_SoundInstanceFreeList.IsEmpty())
  {
    const WUInt32 idx = m_pData->m_SoundInstanceFreeList.PeekBack();
    m_pData->m_SoundInstanceFreeList.PopBack();
    pInstance = &m_pData->m_SoundInstancesStorage[idx];
  }
  else
  {
    const WUInt32 idx = m_pData->m_SoundInstancesStorage.GetCount();
    pInstance = &m_pData->m_SoundInstancesStorage.ExpandAndGetRef();
    pInstance->m_uiOwnIndex = idx;
  }

  pInstance->m_bInUse = true;
  pInstance->pWorld = pWorld;
  pInstance->m_hComponent = hComponent;

  if (ma_decoder_init_memory(audioData.GetData(), audioData.GetCount(), nullptr, &pInstance->m_Decoder) != MA_SUCCESS)
  {
    FreeSoundInstance(pInstance);
    return nullptr;
  }

  if (ma_sound_init_from_data_source(GetEngine(), &pInstance->m_Decoder, MA_SOUND_FLAG_DECODE, pGroup, &pInstance->m_Sound) != MA_SUCCESS)
  {
    FreeSoundInstance(pInstance);
    return nullptr;
  }

  // make sure to be notified when the sound ends
  W_MA_CHECK(ma_sound_set_end_callback(&pInstance->m_Sound, SoundEndedCallback, pInstance));

  return pInstance;
}

void WMiniAudioSingleton::FreeSoundInstance(WMiniAudioSoundInstance*& ref_pInstance)
{
  const WUInt32 uiIndex = ref_pInstance->m_uiOwnIndex;

  if (!ref_pInstance->m_bInUse) // already freed
  {
    W_ASSERT_DEBUG(m_pData->m_SoundInstanceFreeList.Contains(uiIndex), "Sound is not in the free list");
    return;
  }

  W_LOCK(m_pData->m_Mutex);

  W_ASSERT_DEBUG(!m_pData->m_SoundInstanceFreeList.Contains(uiIndex), "Sound is already freed");

  ref_pInstance->m_bInUse = false;

  auto& inst = m_pData->m_SoundInstancesStorage[uiIndex];
  inst.m_hComponent.Invalidate();
  inst.pWorld = nullptr;

  W_MA_CHECK(ma_sound_stop(&inst.m_Sound));
  W_MA_CHECK(ma_decoder_uninit(&inst.m_Decoder));
  ma_sound_uninit(&inst.m_Sound);

  m_pData->m_SoundInstanceFreeList.PushBack(uiIndex);

  ref_pInstance = nullptr;
}

void WMiniAudioSingleton::DetachSoundInstance(WMiniAudioSoundInstance*& ref_pInstance)
{
  if (ref_pInstance == nullptr)
    return;

  // deactivate looping
  W_MA_CHECK(ma_data_source_set_looping(&ref_pInstance->m_Decoder, false));

  ref_pInstance->m_hComponent.Invalidate(); // owner doesn't want to be notified anymore
  // pInstance->pWorld = nullptr; // but keep the world reference for shutdown behavior

  // owner doesn't point to it any longer, but we'll continue playing it
  ref_pInstance = nullptr;
}


void WMiniAudioSingleton::DetachAndFadeOutSoundInstance(WMiniAudioSoundInstance*& ref_pInstance, WTime fadeDuration)
{
  if (ref_pInstance == nullptr)
    return;

  W_LOCK(m_pData->m_Mutex);
  m_pData->m_FadingInstances.PushBack(ref_pInstance->m_uiOwnIndex);

  ref_pInstance->m_hComponent.Invalidate(); // owner doesn't want to be notified anymore
  // pInstance->pWorld = nullptr; // but keep the world reference for shutdown behavior

  ma_sound_stop_with_fade_in_milliseconds(&ref_pInstance->m_Sound, static_cast<ma_uint64>(fadeDuration.GetMilliseconds()));

  // owner doesn't point to it any longer, but we'll continue playing it
  ref_pInstance = nullptr;
}

void WMiniAudioSingleton::SoundEnded(WMiniAudioSoundInstance* pInstance)
{
  if (pInstance == nullptr)
    return;

  W_LOCK(m_pData->m_Mutex);
  W_ASSERT_DEBUG(pInstance->m_bInUse, "Sound should still be flagged as in-use");
  m_pData->m_FinishedInstances.PushBack(pInstance->m_uiOwnIndex);
}

void WMiniAudioSingleton::StopWorldSounds(WWorld* pWorld)
{
  if (m_pData == nullptr)
    return;

  W_LOCK(m_pData->m_Mutex);

  for (WUInt32 i = 0; i < m_pData->m_FinishedInstances.GetCount();)
  {
    const WUInt32 idx = m_pData->m_FinishedInstances[i];

    if (m_pData->m_SoundInstancesStorage[idx].pWorld == pWorld)
    {
      m_pData->m_FinishedInstances.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (WUInt32 i = 0; i < m_pData->m_FadingInstances.GetCount();)
  {
    const WUInt32 idx = m_pData->m_FadingInstances[i];

    if (m_pData->m_SoundInstancesStorage[idx].pWorld == pWorld)
    {
      m_pData->m_FadingInstances.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (WUInt32 i = 0; i < m_pData->m_SoundInstancesStorage.GetCount(); ++i)
  {
    auto* pInst = &m_pData->m_SoundInstancesStorage[i];

    if (pInst->pWorld == pWorld)
    {
      // since the world pointer is set, that means the sound is not freed
      FreeSoundInstance(pInst);
    }
  }
}
