#pragma once

#include <Core/Messages/EventMessageSender.h>
#include <Core/ResourceManager/Resource.h>
#include <FmodPlugin/Components/FmodComponent.h>

struct WFmodParameterId
{
public:
  W_ALWAYS_INLINE void Invalidate() { m_uiValue = -1; }
  W_ALWAYS_INLINE bool IsInvalidated() const { return m_uiValue == -1; }

private:
  WUInt64 m_uiValue = -1;
};

class WPhysicsWorldModuleInterface;
struct WMsgSetFloatParameter;

class WFmodEventComponentManager : public WComponentManager<class WFmodEventComponent, WBlockStorageType::FreeList>
{
public:
  WFmodEventComponentManager(WWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class WFmodEventComponent;

  struct OcclusionState
  {
    WFmodEventComponent* m_pComponent = nullptr;
    WFmodParameterId m_OcclusionParamId;
    WUInt32 m_uiRaycastHits = 0;
    WUInt8 m_uiNextRayIndex = 0;
    WUInt8 m_uiNumUsedRays = 0;
    float m_fRadius = 0.0f;
    float m_fLastOcclusionValue = -1.0f;

    float GetOcclusionValue(float fThreshold) const { return WMath::Clamp((m_fLastOcclusionValue - fThreshold) / WMath::Max(1.0f - fThreshold, 0.0001f), 0.0f, 1.0f); }
  };

  WUInt32 m_uiFirstComponentIndex = 0;
  WDynamicArray<OcclusionState> m_OcclusionStates;

  WUInt32 AddOcclusionState(WFmodEventComponent* pComponent, WFmodParameterId occlusionParamId, float fRadius);
  void RemoveOcclusionState(WUInt32 uiIndex);
  const OcclusionState& GetOcclusionState(WUInt32 uiIndex) const { return m_OcclusionStates[uiIndex]; }

  void ShootOcclusionRays(
    OcclusionState& state, WVec3 listenerPos, WUInt32 uiNumRays, const WPhysicsWorldModuleInterface* pPhysicsWorldModule, WTime deltaTime);
  void UpdateOcclusion(const WWorldModule::UpdateContext& context);
  void UpdateEvents(const WWorldModule::UpdateContext& context);

  void ResourceEventHandler(const WResourceEvent& e);
};

using WFmodSoundEventResourceHandle = WTypedResourceHandle<class WFmodSoundEventResource>;

struct WResourceEvent;

//////////////////////////////////////////////////////////////////////////

/// Sent when a WFmodEventComponent finishes playing a sound. Not sent for one-shot sound events.
struct W_FMODPLUGIN_DLL WMsgFmodSoundFinished : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgFmodSoundFinished, WMessage);
};


//////////////////////////////////////////////////////////////////////////

/// Represents a sound (called an 'event') in the FMOD sound system.
///
/// Provides functions to start, pause, stop a sound, set parameters, change volume, pitch etc.
class W_FMODPLUGIN_DLL WFmodEventComponent : public WFmodComponent
{
  W_DECLARE_COMPONENT_TYPE(WFmodEventComponent, WFmodComponent, WFmodEventComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WFmodComponent

private:
  virtual void WFmodComponentIsAbstract() override {}
  friend class WComponentManagerSimple<class WFmodEventComponent, WComponentUpdateType::WhenSimulating>;


  //////////////////////////////////////////////////////////////////////////
  // WFmodEventComponent

public:
  WFmodEventComponent();
  ~WFmodEventComponent();

  void SetPaused(bool b);                                                               // [ property ]
  bool GetPaused() const { return m_bPaused; }                                          // [ property ]

  void SetUseOcclusion(bool b);                                                         // [ property ]
  bool GetUseOcclusion() const { return m_bUseOcclusion; }                              // [ property ]

  void SetOcclusionCollisionLayer(WUInt8 uiCollisionLayer);                            // [ property ]
  WUInt8 GetOcclusionCollisionLayer() const { return m_uiOcclusionCollisionLayer; }    // [ property ]

  void SetOcclusionThreshold(float fThreshold);                                         // [ property ]
  float GetOcclusionThreshold() const;                                                  // [ property ]

  void SetPitch(float f);                                                               // [ property ]
  float GetPitch() const { return m_fPitch; }                                           // [ property ]

  void SetVolume(float f);                                                              // [ property ]
  float GetVolume() const { return m_fVolume; }                                         // [ property ]

  void SetSoundEvent(const WFmodSoundEventResourceHandle& hSoundEvent);                // [ property ]
  const WFmodSoundEventResourceHandle& GetSoundEvent() const { return m_hSoundEvent; } // [ property ]

  WEnum<WOnComponentFinishedAction> m_OnFinishedAction;                               // [ property ]

  void SetShowDebugInfo(bool bShow);                                                    // [ property ]
  bool GetShowDebugInfo() const;                                                        // [ property ]

  /// If set, the global game speed does not affect the pitch of this event.
  ///
  /// This is important for global sounds, such as music or UI effects, so that they always play at their regular speed,
  /// even when the game is in slow motion.
  void SetNoGlobalPitch(bool bEnable); // [ property ]
  bool GetNoGlobalPitch() const;       // [ property ]

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
  void FadeOut(); // [ scriptable ]

  /// Plays a completely new sound at the location of this component and with all its current properties.
  ///
  /// Pitch, volume, position, direction and velocity are copied to the new sound instance.
  /// The new sound event then plays to the end and cannot be controlled through this component any further.
  /// If the referenced FMOD sound event is not a "one shot" event, this function is ignored.
  /// The event that is controlled through this component is unaffected by this.
  void StartOneShot(); // [ scriptable ]

  /// Triggers an FMOD sound cue. Whatever that is useful for.
  void SoundCue(); // [ scriptable ]

  /// Tries to find the FMOD event parameter by name. Returns the parameter id or -1, if no such parameter exists.
  WFmodParameterId FindParameter(const char* szName) const;

  /// Sets an FMOD event parameter value. See FindParameter() for the index.
  void SetParameter(WFmodParameterId paramId, float fValue);

  /// Gets an FMOD event parameter value. See FindParameter() for the index. Returns 0, if the index is invalid.
  float GetParameter(WFmodParameterId paramId) const;

  /// Sets an event parameter via name lookup, so this is less efficient than SetParameter()
  void SetEventParameter(const char* szParamName, float fValue); // [ scriptable ]

  /// Allows one to set event parameters through the generic WMsgSetFloatParameter message.
  ///
  /// Requires event parameter lookup via a name, so this is less efficient than SetParameter().
  void OnMsgSetFloatParameter(WMsgSetFloatParameter& ref_msg); // [ msg handler ]

protected:
  void OnMsgDeleteGameObject(WMsgDeleteGameObject& msg);       // [ msg handler ]

  void Update();
  void UpdateParameters(FMOD::Studio::EventInstance* pInstance);
  void UpdateOcclusion();

  /// Called when the event resource has been unloaded (for a reload)
  void InvalidateResource(bool bTryToRestore);


  bool m_bPaused;
  bool m_bUseOcclusion;
  WUInt8 m_uiOcclusionThreshold;
  WUInt8 m_uiOcclusionCollisionLayer;
  float m_fPitch;
  float m_fVolume;
  WInt32 m_iTimelinePosition = -1; // used to restore a sound after reloading the resource
  WUInt32 m_uiOcclusionStateIndex = WInvalidIndex;
  WFmodSoundEventResourceHandle m_hSoundEvent;

  FMOD::Studio::EventDescription* m_pEventDesc;
  FMOD::Studio::EventInstance* m_pEventInstance;

  WEventMessageSender<WMsgFmodSoundFinished> m_SoundFinishedEventSender; // [ event ]
};
