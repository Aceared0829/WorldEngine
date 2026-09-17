#pragma once

#include <Core/Interfaces/SoundInterface.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <FmodPlugin/FmodPluginDLL.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

struct WGameApplicationExecutionEvent;
class WOpenDdlWriter;
class WOpenDdlReaderElement;
using WDataBuffer = WDynamicArray<WUInt8>;

using WFmodSoundBankResourceHandle = WTypedResourceHandle<class WFmodSoundBankResource>;

/// Abstraction of FMOD_SPEAKERMODE
enum class WFmodSpeakerMode : WUInt8
{
  ModeStereo,
  Mode5Point1,
  Mode7Point1,
};

/// The FMOD configuration to be used on a specific platform
struct W_FMODPLUGIN_DLL WFmodConfiguration
{
  WString m_sMasterSoundBank;
  WFmodSpeakerMode m_SpeakerMode = WFmodSpeakerMode::Mode5Point1; ///< This must be set to what is configured in FMOD Studio for the
                                                                    ///< target platform. Using anything else is incorrect.
  WUInt16 m_uiVirtualChannels = 32;                                ///< See FMOD::Studio::System::initialize
  WUInt32 m_uiSamplerRate = 0;                                     ///< See FMOD::System::setSoftwareFormat

  void Save(WOpenDdlWriter& ref_ddl) const;
  void Load(const WOpenDdlReaderElement& ddl);

  bool operator==(const WFmodConfiguration& rhs) const;
  bool operator!=(const WFmodConfiguration& rhs) const { return !operator==(rhs); }
};

/// All available FMOD platform configurations
struct W_FMODPLUGIN_DLL WFmodAssetProfiles
{
  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/FmodConfig.ddl"_wsv;

  WResult Save(WStringView sFile = s_sConfigFile) const;
  WResult Load(WStringView sFile = s_sConfigFile);

  WMap<WString, WFmodConfiguration> m_AssetProfiles;
};

class W_FMODPLUGIN_DLL WFmod : public WSoundInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WFmod, WSoundInterface);

public:
  WFmod();

  void Startup();
  void Shutdown();

  FMOD::Studio::System* GetStudioSystem() const { return m_pStudioSystem; }
  FMOD::System* GetLowLevelSystem() const { return m_pLowLevelSystem; }

  /// Can be called before startup to load the FMOD configs from a different file.
  /// Otherwise will automatically be loaded by FMOD startup with the default path.
  virtual void LoadConfiguration(WStringView sFile) override;

  /// By default the FMOD integration will auto-detect the platform (and thus the config) to use.
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

  /// Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance or 'Speech'.
  /// Note that the FMOD strings banks are never loaded, so the given string must be a GUID (FMOD Studio -> Copy GUID).
  virtual void SetSoundGroupVolume(WStringView sVcaGroupGuid, float fVolume) override;
  virtual float GetSoundGroupVolume(WStringView sVcaGroupGuid) const override;
  void UpdateSoundGroupVolumes();

  /// Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(WUInt8 uiNumListeners) override;
  virtual WUInt8 GetNumListeners() override;

  static void GameApplicationEventHandler(const WGameApplicationExecutionEvent& e);

  /// Configures how many reverb ('EAX') volumes are being blended/mixed for a sound.
  ///
  /// The number is clamped between 0 and 4. 0 Means all environmental effects are disabled for all sound sources.
  /// 1 means only the most important reverb is applied. 2, 3 and 4 allow to add more fidelity, but will cost more CPU resources.
  ///
  /// The default is currently 4.
  void SetNumBlendedReverbVolumes(WUInt8 uiNumBlendedVolumes);

  /// See SetNumBlendedReverbVolumes()
  WUInt8 GetNumBlendedReverbVolumes() const { return m_uiNumBlendedVolumes; }

  /// Sets the global parameter with the given name to a the desired value.
  ///
  /// Global parameters affect any sound event that uses them, no matter on which instance they play.
  /// They are convenient to use for general things like passing the game state to the sound system.
  void SetGlobalParameter(const char* szName, float fValue);

  /// Returns the current value of the given global parameter.
  ///
  /// This may be different from what was set via SetGlobalParameter(), since FMOD may modify the value when playing events.
  float GetGlobalParameter(const char* szName);


  virtual void SetListenerOverrideMode(bool bEnabled) override;
  virtual void SetListener(WInt32 iIndex, const WVec3& vPosition, const WVec3& vForward, const WVec3& vUp, const WVec3& vVelocity) override;
  WVec3 GetListenerPosition() { return m_vListenerPosition; }

  virtual WResult OneShotSound(WWorld* pWorld, WStringView sResourceID, const WTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) override;

private:
  friend class WFmodSoundBankResource;
  void QueueSoundBankDataForDeletion(WDataBuffer* pData);
  void ClearSoundBankDataDeletionQueue();
  mutable WMutex m_DeletionQueueMutex;

private:
  void DetectPlatform();
  WResult LoadMasterSoundBank(const char* szMasterBankResourceID);

  bool m_bInitialized = false;
  bool m_bListenerOverrideMode = false;
  WVec3 m_vListenerPosition;
  WUInt8 m_uiNumBlendedVolumes = 4;

  FMOD::Studio::System* m_pStudioSystem;
  FMOD::System* m_pLowLevelSystem;

  struct Data
  {
    WMap<WString, float> m_VcaVolumes;
    WFmodAssetProfiles m_Configs;
    WString m_sPlatform;
    WFmodSoundBankResourceHandle m_hMasterBank;
    WFmodSoundBankResourceHandle m_hMasterBankStrings;
    WHybridArray<WDataBuffer*, 4> m_SbDeletionQueue;
  };

  WUniquePtr<Data> m_pData;
};
