#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

struct WMiniAudioSoundInstance;
using WMiniAudioSoundResourceHandle = WTypedResourceHandle<class WMiniAudioSoundResource>;

class WMiniAudioSoundComponentManager : public WComponentManager<class WMiniAudioSoundComponent, WBlockStorageType::FreeList>
{
public:
  WMiniAudioSoundComponentManager(WWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class WMiniAudioSoundComponent;

  void UpdateEvents(const WWorldModule::UpdateContext& context);

  WUInt32 m_uiFirstComponentIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

class W_MINIAUDIOPLUGIN_DLL WMiniAudioSoundComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WMiniAudioSoundComponent, WComponent, WMiniAudioSoundComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WMiniAudioComponent

private:
  friend class WComponentManagerSimple<class WMiniAudioSoundComponent, WComponentUpdateType::WhenSimulating>;


  //////////////////////////////////////////////////////////////////////////
  // WMiniAudioSoundComponent

public:
  WMiniAudioSoundComponent();
  ~WMiniAudioSoundComponent();

  void SetPaused(bool b);                                // [ property ]
  bool GetPaused() const { return m_bPaused; }           // [ property ]

  void SetPitch(float f);                                // [ property ]
  float GetPitch() const { return m_fPitch; }            // [ property ]

  void SetVolume(float f);                               // [ property ]
  float GetVolume() const { return m_fComponentVolume; } // [ property ]

  /// If set, the global game speed does not affect the pitch of this event.
  ///
  /// This is important for global sounds, such as music or UI effects, so that they always play at their regular speed,
  /// even when the game is in slow motion.
  void SetNoGlobalPitch(bool bEnable);                     // [ property ]
  bool GetNoGlobalPitch() const;                           // [ property ]

  WEnum<WOnComponentFinishedAction2> m_OnFinishedAction; // [ property ]

  /// Makes the sound play.
  ///
  /// If it was not yet playing, it starts playing a new sound.
  /// If it was already playing, but paused, playback is resumed.
  /// If it was already playing, there is no change.
  void Play(); // [ scriptable ]

  /// If a sound is playing, it pauses at the current play position.
  ///
  /// Call Play() to resume playing.
  void Pause(); // [ scriptable ]

  /// Interrupts the sound playback abruptly.
  void Stop(); // [ scriptable ]

  /// Stops the sound, by fading it out over a short period.
  void FadeOut(WTime fadeDuration); // [ scriptable ]

  /// Plays a completely new sound at the location of this component and with all its current properties.
  ///
  /// Pitch, volume, position and direction are copied to the new sound instance.
  /// The new sound then plays to the end and cannot be controlled through this component any further.
  /// No sound should be playing on this component, when using this function.
  void StartOneShot();                                    // [ scriptable ]

protected:
  void OnMsgDeleteGameObject(WMsgDeleteGameObject& msg); // [ msg handler ]

  void Update();
  void UpdateParameters(WMiniAudioSoundInstance* pInstance, float fVolume, float fPitch) const;

  friend class WMiniAudioSingleton;
  void SoundFinished();

  WMiniAudioSoundResourceHandle m_hSound;
  float m_fComponentVolume = 1.0f;
  float m_fPitch = 1.0f;
  float m_fResourceVolume = 1.0f;
  float m_fResourcePitch = 1.0f;
  bool m_bPaused = false;

  WMiniAudioSoundInstance* m_pInstance = nullptr;
};
