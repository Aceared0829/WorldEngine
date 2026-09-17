#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class W_PARTICLEPLUGIN_DLL WParticleEffectDescriptor final : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEffectDescriptor, WReflectedClass);

public:
  WParticleEffectDescriptor();
  ~WParticleEffectDescriptor();

  void AddParticleSystem(WParticleSystemDescriptor* pSystem) { m_ParticleSystems.PushBack(pSystem); }
  void RemoveParticleSystem(WParticleSystemDescriptor* pSystem) { m_ParticleSystems.RemoveAndCopy(pSystem); }
  const WHybridArray<WParticleSystemDescriptor*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddEventReaction(WParticleEventReactionFactory* pSystem) { m_EventReactions.PushBack(pSystem); }
  void RemoveEventReaction(WParticleEventReactionFactory* pSystem) { m_EventReactions.RemoveAndCopy(pSystem); }
  const WHybridArray<WParticleEventReactionFactory*, 4>& GetEventReactions() const { return m_EventReactions; }


  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);

  void ClearSystems();
  void ClearEventReactions();

  WEnum<WEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  bool m_bSimulateInLocalSpace = false;
  bool m_bAlwaysShared = false;
  float m_fApplyInstanceVelocity = 0.0f;
  WTime m_PreSimulateDuration;
  WVec3U32 m_vNumWindSamples = WVec3U32(1);
  WMap<WString, float> m_FloatParameters;
  WMap<WString, WColor> m_ColorParameters;

private:
  WHybridArray<WParticleSystemDescriptor*, 4> m_ParticleSystems;
  WHybridArray<WParticleEventReactionFactory*, 4> m_EventReactions;
};
