#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WParticleEffectInstance;
class WParticleEventReaction;

/// Base class for all particle event reaction factories
///
/// Event reaction factories create and configure event reactions that respond to particle events.
/// Each factory specifies which event type to respond to and the probability of triggering.
class W_PARTICLEPLUGIN_DLL WParticleEventReactionFactory : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEventReactionFactory, WReflectedClass);

public:
  virtual const WRTTI* GetEventReactionType() const = 0;
  virtual void CopyReactionProperties(WParticleEventReaction* pObject, bool bFirstTime) const = 0;

  /// Creates and initializes a new event reaction instance for the given effect.
  WParticleEventReaction* CreateEventReaction(WParticleEffectInstance* pOwner) const;

  virtual void Save(WStreamWriter& inout_stream) const;
  virtual void Load(WStreamReader& inout_stream);

  WString m_sEventType;         ///< The type of particle event this reaction responds to
  WUInt8 m_uiProbability = 100; ///< Probability (1-100) that this reaction triggers when the event occurs
};

/// Base class for all particle event reactions.
///
/// Event reactions respond to particle events by performing actions like spawning effects or prefabs.
/// Reactions are checked probabilistically - they may not trigger every time an event occurs.
class W_PARTICLEPLUGIN_DLL WParticleEventReaction : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEventReaction, WReflectedClass);

  friend class WParticleEventReactionFactory;
  friend class WParticleEffectInstance;

protected:
  WParticleEventReaction();
  ~WParticleEventReaction();

  /// Initializes the reaction with a reference to the owning effect instance.
  void Reset(WParticleEffectInstance* pOwner);

  /// Called when a matching event occurs and the probability check passes.
  ///
  /// Derived classes implement the actual reaction behavior (e.g., spawning an effect).
  virtual void ProcessEvent(const WParticleEvent& e) = 0;

  WTempHashedString m_sEventName;                    ///< Hashed event type name for fast comparison
  WUInt8 m_uiProbability;                            ///< Probability value (1-100) for triggering this reaction
  WParticleEffectInstance* m_pOwnerEffect = nullptr; ///< The effect instance that owns this reaction
};
