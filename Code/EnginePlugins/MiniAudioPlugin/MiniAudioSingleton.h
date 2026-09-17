#pragma once

#include <Core/Interfaces/SoundInterface.h>
#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <MiniAudio/miniaudio.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

// TODO MiniAudio: Future Work
//
// * in MiniAudioResource Load sounds through the MA resource manager (redirect file hooks to our resource manager)
// * then decode one sound right away and use that as a template to copy from for future sound playback
// * Add preview playback to sound asset
// * Add max sound size, check whether MA adds FMOD-like attenuation models
// * skip sounds that are too far away

struct WGameApplicationExecutionEvent;

struct WMiniAudioSoundInstance
{
  ma_sound m_Sound;
  ma_decoder m_Decoder;
  WWorld* pWorld = nullptr;
  WComponentHandle m_hComponent;
  WUInt16 m_uiOwnIndex;
  bool m_bInUse = false;
};


class W_MINIAUDIOPLUGIN_DLL WMiniAudioSingleton : public WSoundInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WMiniAudioSingleton, WSoundInterface);

public:
  WMiniAudioSingleton();
  ~WMiniAudioSingleton();

  void Startup();
  void Shutdown();

  ma_engine* GetEngine() { return &m_pData->m_Engine; }

  /// Can be called before startup to load the configuration from a different file.
  /// Otherwise will automatically be loaded at startup with the default path.
  virtual void LoadConfiguration(WStringView sFile) override;

  /// By default the integration will auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(WStringView sPlatform) override;

  /// Automatically called by the plugin every time WGameApplicationExecutionEvent::BeforeUpdatePlugins is fired.
  virtual void UpdateSound() override;

  /// Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void SetMasterChannelVolume(float fVolume) override;
  virtual float GetMasterChannelVolume() const override;

  /// Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool bMute) override;
  virtual bool GetMasterChannelMute() const override;

  /// Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds,
  /// instead of mute them.
  virtual void SetMasterChannelPaused(bool bPaused) override;
  virtual bool GetMasterChannelPaused() const override;

  /// Specifies the volume for a sound group.
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance or 'Speech'.
  virtual void SetSoundGroupVolume(WStringView sGroupName, float fVolume) override;
  virtual float GetSoundGroupVolume(WStringView sGroupName) const override;

  struct SoundGroup
  {
    WString m_sName;
    float m_fVolume = 1.0f;
    WUniquePtr<ma_sound_group> m_pGroup;
  };

  SoundGroup& GetSoundGroup(WStringView sGroupName);

  /// Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(WUInt8 uiNumListeners) override;
  virtual WUInt8 GetNumListeners() override;

  static void GameApplicationEventHandler(const WGameApplicationExecutionEvent& e);

  virtual void SetListenerOverrideMode(bool bEnabled) override;
  virtual void SetListener(WInt32 iIndex, const WVec3& vPosition, const WVec3& vForward, const WVec3& vUp, const WVec3& vVelocity) override;
  WVec3 GetListenerPosition() { return m_vListenerPosition; }

  virtual WResult OneShotSound(WWorld* pWorld, WStringView sResourceID, const WTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) override;

  WMiniAudioSoundInstance* AllocateSoundInstance(const WDataBuffer& audioData, WWorld* pWorld, WComponentHandle hComponent, ma_sound_group* pGroup);
  void FreeSoundInstance(WMiniAudioSoundInstance*& ref_pInstance);
  void DetachSoundInstance(WMiniAudioSoundInstance*& ref_pInstance);

  void DetachAndFadeOutSoundInstance(WMiniAudioSoundInstance*& ref_pInstance, WTime fadeDuration);

  void SoundEnded(WMiniAudioSoundInstance* pInstance);

  void StopWorldSounds(WWorld* pWorld);

private:
  bool m_bInitialized = false;
  bool m_bListenerOverrideMode = false;
  WVec3 m_vListenerPosition;

  struct Data
  {
    WMutex m_Mutex;

    ma_engine m_Engine;
    WDeque<WMiniAudioSoundInstance> m_SoundInstancesStorage;
    WDeque<WUInt32> m_SoundInstanceFreeList;

    WDeque<WUInt32> m_FadingInstances;
    WDeque<WUInt32> m_FinishedInstances;

    WHybridArray<SoundGroup, 4> m_SoundGroups;
  };

  WUniquePtr<Data> m_pData;
};
