#include <FmodPlugin/FmodPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Platform/PlatformDesc.h>
#include <GameEngine/GameApplication/GameApplication.h>

W_IMPLEMENT_SINGLETON(WFmod);

static WFmod g_FmodSingleton;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
HANDLE g_hLiveUpdateMutex = NULL;
#endif

WCVarFloat cvar_FmodMasterVolume("FMOD.MasterVolume", 1.0f, WCVarFlags::Save, "Overall volume for all FMOD output");
WCVarBool cvar_FmodMute("FMOD.Mute", false, WCVarFlags::Default, "Whether FMOD output is muted");
WCVarBool cvar_FmodPause("FMOD.Pause", false, WCVarFlags::Default, "Whether FMOD output is paused");

WFmod::WFmod()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_vListenerPosition.SetZero();

  m_pStudioSystem = nullptr;
  m_pLowLevelSystem = nullptr;
}

void WFmod::Startup()
{
  if (m_bInitialized)
    return;

  m_pData = W_DEFAULT_NEW(Data);

  DetectPlatform();

  if (m_pData->m_Configs.m_AssetProfiles.IsEmpty())
  {
    LoadConfiguration(WFmodAssetProfiles::s_sConfigFile);

    if (m_pData->m_Configs.m_AssetProfiles.IsEmpty())
    {
      WLog::Warning("No valid FMOD configuration file available in '{0}'. FMOD will be deactivated.", WFmodAssetProfiles::s_sConfigFile);
      return;
    }
  }

  if (!m_pData->m_Configs.m_AssetProfiles.Find(m_pData->m_sPlatform).IsValid())
  {
    WLog::Error("FMOD configuration for platform '{0}' not available. FMOD will be deactivated.", m_pData->m_sPlatform);
    return;
  }

  const auto& config = m_pData->m_Configs.m_AssetProfiles[m_pData->m_sPlatform];

  FMOD_SPEAKERMODE fmodMode = FMOD_SPEAKERMODE_5POINT1;
  {
    WString sMode = "Unknown";
    switch (config.m_SpeakerMode)
    {
      case WFmodSpeakerMode::ModeStereo:
        sMode = "Stereo";
        fmodMode = FMOD_SPEAKERMODE_STEREO;
        break;
      case WFmodSpeakerMode::Mode5Point1:
        sMode = "5.1";
        fmodMode = FMOD_SPEAKERMODE_5POINT1;
        break;
      case WFmodSpeakerMode::Mode7Point1:
        sMode = "7.1";
        fmodMode = FMOD_SPEAKERMODE_7POINT1;
        break;
    }

    W_LOG_BLOCK("FMOD Configuration");
    WLog::Dev("Platform = '{0}', Mode = {1}, Channels = {2}, SamplerRate = {3}", m_pData->m_sPlatform, sMode, config.m_uiVirtualChannels, config.m_uiSamplerRate);
    WLog::Dev("Master Bank = '{0}'", config.m_sMasterSoundBank);
  }

  W_FMOD_ASSERT(FMOD::Studio::System::create(&m_pStudioSystem));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  W_FMOD_ASSERT(m_pStudioSystem->getCoreSystem(&m_pLowLevelSystem));
  W_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(config.m_uiSamplerRate, fmodMode, 0));

  void* extraDriverData = nullptr;
  FMOD_STUDIO_INITFLAGS studioflags = FMOD_STUDIO_INIT_NORMAL;

  // FMOD live update doesn't work with multiple instances and the same default IP
  // bank loading fails, once two processes are running that use this feature with the same IP
  // this could be reconfigured through the advanced settings, but for now we just enable live update for the first process
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  {
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
    // mutex handle will be closed automatically on process termination
    GetLastError(); // clear any pending error codes
    g_hLiveUpdateMutex = CreateMutexW(nullptr, TRUE, L"WFmodLiveUpdate");

    DWORD err = GetLastError();
    if (g_hLiveUpdateMutex != NULL && err != ERROR_ALREADY_EXISTS)
    {
      studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
    }
    else
    {
      WLog::Warning("FMOD Live-Update not available for this process, another process using FMOD is already running.");
      if (g_hLiveUpdateMutex != NULL)
      {
        CloseHandle(g_hLiveUpdateMutex); // we didn't create it, so don't keep it alive
        g_hLiveUpdateMutex = NULL;
      }
    }
#  endif
  }
#endif

  W_FMOD_ASSERT(m_pStudioSystem->initialize(config.m_uiVirtualChannels, studioflags, FMOD_INIT_NORMAL, extraDriverData));

  if ((studioflags & FMOD_STUDIO_INIT_LIVEUPDATE) != 0)
  {
    WLog::Success("FMOD Live-Update is enabled for this process.");
  }

  if (LoadMasterSoundBank(config.m_sMasterSoundBank).Failed())
  {
    WLog::Error("Failed to load FMOD master sound bank '{0}'. Sounds will not play.", config.m_sMasterSoundBank);
    return;
  }

  m_bInitialized = true;

  UpdateSound();
}

void WFmod::Shutdown()
{
  if (m_pData == nullptr)
    return;

  // this also runs when the initialization failed (e.g. no master bank): sound bank resources may exist
  // regardless, and unloading them needs m_pData to still be around

  // delete all FMOD resources, except the master bank
  WResourceManager::FreeAllUnusedResources();

  m_bInitialized = false;
  m_pData->m_hMasterBank.Invalidate();
  m_pData->m_hMasterBankStrings.Invalidate();

  // now also delete the master bank
  WResourceManager::FreeAllUnusedResources();

  // now actually delete the sound bank data
  ClearSoundBankDataDeletionQueue();

  if (m_pStudioSystem != nullptr)
  {
    m_pStudioSystem->release();
    m_pStudioSystem = nullptr;
    m_pLowLevelSystem = nullptr;
  }

  // finally delete all data
  m_pData.Clear();
}

void WFmod::SetNumListeners(WUInt8 uiNumListeners)
{
  W_ASSERT_DEV(uiNumListeners <= FMOD_MAX_LISTENERS, "FMOD supports only up to {0} listeners.", FMOD_MAX_LISTENERS);

  m_pStudioSystem->setNumListeners(uiNumListeners);
}

WUInt8 WFmod::GetNumListeners()
{
  int i = 0;
  m_pStudioSystem->getNumListeners(&i);
  return static_cast<WUInt8>(i);
}

void WFmod::LoadConfiguration(WStringView sFile)
{
  m_pData->m_Configs.Load(sFile).IgnoreResult();
}

void WFmod::SetOverridePlatform(WStringView sPlatform)
{
  m_pData->m_sPlatform = sPlatform;
}

void WFmod::UpdateSound()
{
  if (m_pStudioSystem == nullptr)
    return;

  W_ASSERT_DEV(m_pData != nullptr, "UpdateSound() should not be called at this time.");

  // make sure to reload the sound bank, if it has been unloaded
  if (m_pData->m_hMasterBank.IsValid())
  {
    WResourceLock<WFmodSoundBankResource> pMaster(m_pData->m_hMasterBank, WResourceAcquireMode::BlockTillLoaded);
  }

  // Master Volume
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);
    channel->setVolume(WMath::Clamp<float>(cvar_FmodMasterVolume, 0.0f, 1.0f));
  }

  // Mute
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);

    channel->setMute(cvar_FmodMute);
  }

  // Pause
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);

    channel->setPaused(cvar_FmodPause);
  }

  m_pStudioSystem->update();

  ClearSoundBankDataDeletionQueue();
}

void WFmod::SetMasterChannelVolume(float fVolume)
{
  cvar_FmodMasterVolume = WMath::Clamp<float>(fVolume, 0.0f, 1.0f);
}

float WFmod::GetMasterChannelVolume() const
{
  return cvar_FmodMasterVolume;
}

void WFmod::SetMasterChannelMute(bool bMute)
{
  cvar_FmodMute = bMute;
}

bool WFmod::GetMasterChannelMute() const
{
  return cvar_FmodMute;
}

void WFmod::SetMasterChannelPaused(bool bPaused)
{
  cvar_FmodPause = bPaused;
}

bool WFmod::GetMasterChannelPaused() const
{
  return cvar_FmodPause;
}

void WFmod::SetSoundGroupVolume(WStringView sVcaGroupGuid, float fVolume)
{
  m_pData->m_VcaVolumes[sVcaGroupGuid] = WMath::Clamp(fVolume, 0.0f, 1.0f);

  UpdateSoundGroupVolumes();
}

float WFmod::GetSoundGroupVolume(WStringView sVcaGroupGuid) const
{
  return m_pData->m_VcaVolumes.GetValueOrDefault(sVcaGroupGuid, 1.0f);
}

void WFmod::UpdateSoundGroupVolumes()
{
  for (auto it = m_pData->m_VcaVolumes.GetIterator(); it.IsValid(); ++it)
  {
    FMOD::Studio::VCA* pVca = nullptr;
    m_pStudioSystem->getVCA(it.Key().GetData(), &pVca);

    if (pVca != nullptr)
    {
      pVca->setVolume(it.Value());
    }
  }
}

void WFmod::GameApplicationEventHandler(const WGameApplicationExecutionEvent& e)
{
  if (e.m_Type == WGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    WFmod::GetSingleton()->UpdateSound();
  }
}

void WFmod::SetNumBlendedReverbVolumes(WUInt8 uiNumBlendedVolumes)
{
  m_uiNumBlendedVolumes = WMath::Clamp<WUInt8>(m_uiNumBlendedVolumes, 0, 4);
}

void WFmod::SetGlobalParameter(const char* szName, float fValue)
{
  m_pStudioSystem->setParameterByName(szName, fValue);
}

float WFmod::GetGlobalParameter(const char* szName)
{
  float fValue = 0.0f;
  float fFinalValue = 0.0f;
  m_pStudioSystem->getParameterByName(szName, &fValue, &fFinalValue);
  return fFinalValue;
}

void WFmod::SetListenerOverrideMode(bool bEnabled)
{
  m_bListenerOverrideMode = bEnabled;
}

void WFmod::SetListener(WInt32 iIndex, const WVec3& vPosition, const WVec3& vForward, const WVec3& vUp, const WVec3& vVelocity)
{
  if (m_bListenerOverrideMode)
  {
    if (iIndex != -1)
      return;

    iIndex = 0;
  }

  if (iIndex < 0 || iIndex >= FMOD_MAX_LISTENERS)
    return;

  if (iIndex == 0)
  {
    m_vListenerPosition = vPosition;
  }

  FMOD_3D_ATTRIBUTES attr;
  attr.position.x = vPosition.x;
  attr.position.y = vPosition.y;
  attr.position.z = vPosition.z;
  attr.forward.x = vForward.x;
  attr.forward.y = vForward.y;
  attr.forward.z = vForward.z;
  attr.up.x = vUp.x;
  attr.up.y = vUp.y;
  attr.up.z = vUp.z;
  attr.velocity.x = vVelocity.x;
  attr.velocity.y = vVelocity.y;
  attr.velocity.z = vVelocity.z;

  if (m_pStudioSystem != nullptr)
  {
    m_pStudioSystem->setListenerAttributes(iIndex, &attr);
  }
}

WResult WFmod::OneShotSound(WWorld* pWorld, WStringView sResourceID, const WTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  WFmodSoundEventResourceHandle hSound = WResourceManager::LoadResource<WFmodSoundEventResource>(sResourceID);

  if (!hSound.IsValid())
    return W_FAILURE;

  WResourceLock<WFmodSoundEventResource> pSound(hSound, bBlockIfNotLoaded ? WResourceAcquireMode::BlockTillLoaded_NeverFail : WResourceAcquireMode::AllowLoadingFallback_NeverFail);

  if (pSound.GetAcquireResult() != WResourceAcquireResult::Final)
    return W_FAILURE;

  return pSound->PlayOnce(globalPosition, fPitch, fVolume);
}

void WFmod::DetectPlatform()
{
  if (!m_pData->m_sPlatform.IsEmpty())
    return;

  m_pData->m_sPlatform = WPlatformDesc::GetThisPlatformDesc().GetType();
}

WResult WFmod::LoadMasterSoundBank(const char* szMasterBankResourceID)
{
  if (WStringUtils::IsNullOrEmpty(szMasterBankResourceID))
  {
    WLog::Error("FMOD master bank name has not been configured.");
    return W_FAILURE;
  }

  m_pData->m_hMasterBank = WResourceManager::LoadResource<WFmodSoundBankResource>(szMasterBankResourceID);

  {
    WResourceLock<WFmodSoundBankResource> pResource(m_pData->m_hMasterBank, WResourceAcquireMode::BlockTillLoaded);

    if (pResource.GetAcquireResult() == WResourceAcquireResult::MissingFallback)
      return W_FAILURE;
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WStringBuilder sStringsBankPath = szMasterBankResourceID;
  sStringsBankPath.ChangeFileExtension("strings.bank");

  m_pData->m_hMasterBankStrings = WResourceManager::LoadResource<WFmodSoundBankResource>(sStringsBankPath);

  {
    WResourceLock<WFmodSoundBankResource> pResource(m_pData->m_hMasterBankStrings, WResourceAcquireMode::BlockTillLoaded);

    if (pResource.GetAcquireResult() == WResourceAcquireResult::MissingFallback)
      return W_FAILURE;
  }
#endif

  return W_SUCCESS;
}

void WFmod::QueueSoundBankDataForDeletion(WDataBuffer* pData)
{
  W_LOCK(m_DeletionQueueMutex);

  if (m_pData == nullptr)
  {
    // Sound bank resources can outlive the FMOD shutdown (the missing-fallback resource is only freed
    // when the resource manager itself shuts down). By then the FMOD system is already released,
    // so there is nothing to wait for and the data can just be deleted.
    W_DEFAULT_DELETE(pData);
    return;
  }

  m_pData->m_SbDeletionQueue.PushBack(pData);
}

void WFmod::ClearSoundBankDataDeletionQueue()
{
  if (m_pData == nullptr || m_pData->m_SbDeletionQueue.IsEmpty())
    return;

  W_LOCK(m_DeletionQueueMutex);

  if (m_pStudioSystem != nullptr)
  {
    // make sure the data is not in use anymore
    m_pStudioSystem->flushCommands();
  }

  for (auto pData : m_pData->m_SbDeletionQueue)
  {
    W_DEFAULT_DELETE(pData);
  }

  m_pData->m_SbDeletionQueue.Clear();
}

W_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodSingleton);
